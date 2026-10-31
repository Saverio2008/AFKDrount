#include "BotSession.h"
#include "AppStorage.h"
#include "GameCatalog.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QUuid>
#include <algorithm>
#include <utility>

namespace {
void appendBounded(QVariantList &entries, qsizetype &bytes, const QVariantMap &entry)
{
    constexpr qsizetype budget = 2 * 1024 * 1024;
    const qsizetype size = QJsonDocument::fromVariant(entry).toJson(QJsonDocument::Compact).size();
    if (size > budget)
        return;
    while (!entries.isEmpty() && (entries.size() >= 1000 || bytes + size > budget)) {
        bytes -= QJsonDocument::fromVariant(entries.first()).toJson(QJsonDocument::Compact).size();
        entries.removeFirst();
    }
    bytes += size;
    entries.append(entry);
}
}

BotSession::BotSession(QVariantMap account, QVariantMap instance, GameCatalog *catalog, QObject *parent)
    : QObject(parent), m_id(QUuid::createUuid().toString(QUuid::WithoutBraces)),
      m_account(std::move(account)), m_instance(std::move(instance)), m_catalog(catalog),
      m_startedAt(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs))
{
    connect(m_catalog, &GameCatalog::changed, this, &BotSession::sendTranslations);
    m_titleTimer.setSingleShot(true);
    m_actionBarTimer.setSingleShot(true);
    connect(&m_titleTimer, &QTimer::timeout, this, [this] {
        m_serverDisplay.remove(QStringLiteral("title"));
        m_serverDisplay.remove(QStringLiteral("subtitle"));
        emit changed();
    });
    connect(&m_actionBarTimer, &QTimer::timeout, this, [this] {
        m_serverDisplay.remove(QStringLiteral("actionBar"));
        emit changed();
    });
    m_inputTimer.setSingleShot(true);
    connect(&m_inputTimer, &QTimer::timeout, this, [this] {
        finishInput(QStringLiteral("monitor.inputFailed"));
    });
    auto *counter = new CountingWorker;
    m_counter = counter;
    counter->moveToThread(&m_counterThread);
    connect(this, &BotSession::configureCounter, counter, &CountingWorker::configure);
    connect(this, &BotSession::entityBatch, counter, &CountingWorker::process);
    connect(this, &BotSession::resetCounter, counter, &CountingWorker::resetConnection);
    connect(this, &BotSession::counterOnline, counter, &CountingWorker::setOnline);
    connect(this, &BotSession::finishCounter, counter, &CountingWorker::snapshot);
    connect(&m_counterThread, &QThread::finished, counter, &QObject::deleteLater);
    connect(counter, &CountingWorker::processed, this, [this](qsizetype bytes) { m_pendingBytes -= bytes; });
    connect(counter, &CountingWorker::failed, this, [this] { fail(QStringLiteral("session.error.overload")); });
    connect(counter, &CountingWorker::updated, this, [this](const QVariantMap &statistics) {
        m_statistics = statistics;
        emit changed();
    });
    connect(counter, &CountingWorker::finalized, this, [this](const QVariantMap &statistics) {
        m_statistics = statistics;
        m_counter = nullptr;
        m_counterThread.quit();
        emit changed();
    });
    m_counterThread.start();
    emit configureCounter(m_instance.value(QStringLiteral("configuration")).toMap());
    m_applyTimer.setSingleShot(true);
    m_applyTimer.setInterval(10000);
    connect(&m_applyTimer, &QTimer::timeout, this, [this] { fail(QStringLiteral("session.error.protocol")); });
    m_retryTimer.setSingleShot(true);
    m_connectTimer.setSingleShot(true);
    m_connectTimer.setInterval(120000);
    connect(&m_retryTimer, &QTimer::timeout, this, &BotSession::launch);
    connect(&m_connectTimer, &QTimer::timeout, this, [this] {
        m_endKind = QStringLiteral("connectionError");
        addEvent(QStringLiteral("timeout"));
        m_process.kill();
    });
    connect(&m_vault, &CredentialVault::keyReady, this, [this](QByteArray key) {
        if (m_active && !m_stopping) {
            m_key = key;
            launch();
        }
        key.fill('\0');
    });
    connect(&m_vault, &CredentialVault::failed, this, [this] { fail(QStringLiteral("account.error.keychain")); });
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &BotSession::readOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this, [this] { m_process.readAllStandardError(); });
    connect(&m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        readOutput();
        processEnded(code);
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            fail(QStringLiteral("session.error.backend"));
    });
}

BotSession::~BotSession()
{
    disconnect(&m_process, nullptr, this, nullptr);
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(2000);
    }
    m_key.fill('\0');
    m_counterThread.quit();
    m_counterThread.wait();
}

void BotSession::start()
{
    if (m_account.value(QStringLiteral("type")).toString() == QLatin1String("microsoft"))
        m_vault.loadOrCreateKey();
    else
        launch();
}

void BotSession::send(const QJsonObject &request)
{
    if (m_process.state() == QProcess::Running)
        m_process.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n');
}

int BotSession::submitInput(const QString &mode, const QString &text)
{
    if (!m_online || !m_active || m_stopping || m_pendingInput || m_process.state() != QProcess::Running
        || (mode != QLatin1String("chat") && mode != QLatin1String("command"))
        || (mode == QLatin1String("command") && !m_commandReady)
        || text.trimmed().isEmpty() || text.size() > 256)
        return 0;
    m_pendingInput = ++m_inputSerial;
    m_inputMode = mode;
    m_inputTimer.start(15000);
    send({{QStringLiteral("command"), QStringLiteral("manualInput")}, {QStringLiteral("id"), m_pendingInput},
          {QStringLiteral("mode"), mode}, {QStringLiteral("text"), text}});
    return m_pendingInput;
}

void BotSession::finishInput(const QString &code)
{
    if (!m_pendingInput)
        return;
    m_inputTimer.stop();
    const int request = std::exchange(m_pendingInput, 0);
    addEvent(code.isEmpty() ? QStringLiteral("manualSent") : QStringLiteral("manualFailed"), {},
             {{QStringLiteral("mode"), m_inputMode}, {QStringLiteral("code"), code}});
    emit inputFinished(request, code);
}

void BotSession::setMonitorPreferences(const QVariantMap &preferences)
{
    m_monitorPreferences = preferences;
    m_regionsRedacted |= preferences.value(QStringLiteral("hidePosition"), true).toBool();
    m_accountRedacted = preferences.value(QStringLiteral("hideAccount")).toBool() || (!m_active && m_accountRedacted);
    m_serverRedacted = preferences.value(QStringLiteral("hideServer")).toBool() || (!m_active && m_serverRedacted);
    if (preferences.value(QStringLiteral("hidePosition"), true).toBool())
        m_player.remove(QStringLiteral("position"));
    emit changed();
}

void BotSession::clearServerDisplay()
{
    m_serverDisplay.clear();
    m_titleTimer.stop();
    m_actionBarTimer.stop();
}

void BotSession::launch()
{
    if (!m_active || m_stopping)
        return;
    if (!m_pendingInstance.isEmpty()) {
        m_instance = m_pendingInstance;
        m_pendingInstance.clear();
        emit configureCounter(m_instance.value(QStringLiteral("configuration")).toMap());
        addEvent(QStringLiteral("configurationApplied"));
    }
    m_state = QStringLiteral("connecting");
    m_connectionVersion = m_instance.value(QStringLiteral("configuration")).toMap().value(QStringLiteral("gameVersion")).toString();
    m_sentTranslations = false;
    requestTranslations();
    m_online = false;
    m_titleDuration = 3500;
    m_resources.clear();
    m_connection = {{QStringLiteral("server"), m_instance.value(QStringLiteral("server"))},
                    {QStringLiteral("port"), m_instance.value(QStringLiteral("port"))}};
    m_reachedWorld = false;
    m_endKind.clear();
    m_output.clear();
    m_error.clear();
    emit resetCounter();
    addEvent(QStringLiteral("connecting"));
    const QDir appDirectory(QCoreApplication::applicationDirPath());
    const QDir runtime(appDirectory.filePath(QStringLiteral(AFK_BACKEND_SUBDIR)));
    auto environment = QProcessEnvironment::systemEnvironment();
    for (const QString &name : {QStringLiteral("NODE_OPTIONS"), QStringLiteral("NODE_PATH"), QStringLiteral("DEBUG")})
        environment.remove(name);
    m_process.setProcessEnvironment(environment);
    m_process.setWorkingDirectory(runtime.absolutePath());
    disconnect(&m_process, &QProcess::started, this, nullptr);
    connect(&m_process, &QProcess::started, this, [this] {
        sendTranslations();
        send({{QStringLiteral("command"), QStringLiteral("start")},
              {QStringLiteral("accountId"), m_account.value(QStringLiteral("id")).toString()},
              {QStringLiteral("instance"), QJsonObject::fromVariantMap(m_instance)},
              {QStringLiteral("locale"), m_locale},
              {QStringLiteral("startupTasks"), !m_startupTasksTriggered},
              {QStringLiteral("key"), QString::fromLatin1(m_key.toBase64())}});
    });
    m_process.start(appDirectory.filePath(QStringLiteral(AFK_NODE_SUBPATH)),
        {runtime.filePath(QStringLiteral("bot-runtime.mjs")),
        AppStorage::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/accounts")});
    m_connectTimer.start();
    emit changed();
}

void BotSession::setLocale(const QString &locale)
{
    if (m_locale == locale)
        return;
    m_locale = locale;
    m_sentTranslations = false;
    if (m_active && !m_stopping) {
        send({{QStringLiteral("command"), QStringLiteral("setLocale")}, {QStringLiteral("locale"), m_locale}});
        requestTranslations();
    }
}

void BotSession::requestTranslations()
{
    if (m_locale == QLatin1String("en_us") || m_sentTranslations || m_connectionVersion.isEmpty())
        return;
    m_catalog->ensureLanguage(m_connectionVersion, m_locale);
    sendTranslations();
}

void BotSession::sendTranslations()
{
    if (!m_active || m_stopping || m_locale == QLatin1String("en_us") || m_sentTranslations
        || m_process.state() != QProcess::Running)
        return;
    const QJsonObject translations = m_catalog->translations(m_connectionVersion, m_locale);
    if (translations.isEmpty())
        return;
    m_sentTranslations = true;
    send({{QStringLiteral("command"), QStringLiteral("setTranslations")},
          {QStringLiteral("locale"), m_locale}, {QStringLiteral("translations"), translations}});
}

void BotSession::addEvent(const QString &kind, const QString &text, const QVariantMap &data)
{
    appendBounded(m_events, m_eventBytes, {{QStringLiteral("time"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                            {QStringLiteral("kind"), kind}, {QStringLiteral("text"), text.left(4096)},
                            {QStringLiteral("data"), data}});
    emit changed();
}

void BotSession::readOutput()
{
    m_output += m_process.readAllStandardOutput();
    if (m_output.size() > 16 * 1024 * 1024) {
        fail(QStringLiteral("session.error.overload"));
        return;
    }
    while (m_output.contains('\n')) {
        const qsizetype end = m_output.indexOf('\n');
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(m_output.first(end), &error);
        m_output.remove(0, end + 1);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            fail(QStringLiteral("session.error.protocol"));
            return;
        }
        if (const QJsonObject event = document.object(); event.value(QStringLiteral("event")).toString() == QLatin1String("entities")) {
            m_pendingBytes += end + 1;
            if (m_pendingBytes > 16 * 1024 * 1024) {
                fail(QStringLiteral("session.error.overload"));
                return;
            }
            emit entityBatch(event.value(QStringLiteral("entries")).toArray(), end + 1);
        } else {
            receive(event);
        }
    }
}

void BotSession::receive(const QJsonObject &event)
{
    if (const QString kind = event.value(QStringLiteral("event")).toString(); kind == QLatin1String("online")) {
        m_connectTimer.stop();
        m_online = true;
        m_reachedWorld = true;
        m_startupTasksTriggered = true;
        m_state = QStringLiteral("running");
        emit counterOnline(true);
        m_version = event.value(QStringLiteral("version")).toString();
        m_commandReady = event.value(QStringLiteral("commandReady")).toBool();
        addEvent(QStringLiteral("online"), m_version);
        if (!m_pendingInstance.isEmpty()
            && m_pendingInstance.value(QStringLiteral("configuration")).toMap().value(QStringLiteral("gameVersion"))
                == m_instance.value(QStringLiteral("configuration")).toMap().value(QStringLiteral("gameVersion")))
            apply(m_pendingInstance);
    } else if (kind == QLatin1String("connected") || kind == QLatin1String("worldTransition")) {
        if (kind == QLatin1String("connected"))
            m_connectTimer.stop();
        m_online = false;
        finishInput(QStringLiteral("task.error.offline"));
        clearServerDisplay();
        m_applyTimer.stop();
        m_applyInFlight = false;
        emit counterOnline(false);
        m_state = QStringLiteral("connected");
        addEvent(kind);
    } else if (kind == QLatin1String("reset")) {
        emit resetCounter();
    } else if (kind == QLatin1String("playerState") && m_online && !m_stopping) {
        m_player = event.value(QStringLiteral("data")).toObject().toVariantMap();
        if (m_monitorPreferences.value(QStringLiteral("hidePosition"), true).toBool())
            m_player.remove(QStringLiteral("position"));
        m_player.insert(QStringLiteral("sampledAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
        emit changed();
    } else if (kind == QLatin1String("resources")) {
        m_resources = {{QStringLiteral("cpu"), event.value(QStringLiteral("cpu")).toVariant()},
                       {QStringLiteral("memory"), event.value(QStringLiteral("memory")).toVariant()}};
        emit changed();
    } else if (kind == QLatin1String("inputResult") && m_pendingInput
               && event.value(QStringLiteral("id")).toInt() == m_pendingInput) {
        finishInput(event.value(QStringLiteral("code")).toString());
    } else if (kind == QLatin1String("serverEvent")) {
        const QString type = event.value(QStringLiteral("kind")).toString();
        const QString text = event.value(QStringLiteral("text")).toString().left(4096);
        if (type == QLatin1String("chat")) {
            appendBounded(m_chat, m_chatBytes, {{QStringLiteral("time"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                                  {QStringLiteral("serial"), ++m_chatSerial},
                                  {QStringLiteral("text"), text},
                                  {QStringLiteral("motd"), event.value(QStringLiteral("motd")).toString().left(4096)}});
        } else if (type == QLatin1String("title_times")) {
            const qint64 ticks = event.value(QStringLiteral("ticks")).toInteger(70);
            m_titleDuration = static_cast<int>(std::clamp<qint64>(ticks, 0, 72000) * 50);
            if (m_serverDisplay.contains(QStringLiteral("title")))
                m_titleTimer.start(m_titleDuration);
        } else if (type == QLatin1String("title_clear")) {
            m_serverDisplay.remove(QStringLiteral("title"));
            m_serverDisplay.remove(QStringLiteral("subtitle"));
            m_titleTimer.stop();
            if (event.value(QStringLiteral("reset")).toBool())
                m_titleDuration = 3500;
        } else if (type == QLatin1String("title") || type == QLatin1String("subtitle")) {
            m_serverDisplay.insert(type, text);
            if (type == QLatin1String("title") || !m_titleTimer.isActive())
                m_titleTimer.start(m_titleDuration);
        } else if (type == QLatin1String("actionBar")) {
            m_serverDisplay.insert(type, text);
            m_actionBarTimer.start(3000);
        }
        addEvent(type, text);
    } else if (kind == QLatin1String("end")) {
        m_endKind = event.value(QStringLiteral("kind")).toString();
        m_online = false;
        finishInput(QStringLiteral("task.error.offline"));
        clearServerDisplay();
        emit counterOnline(false);
        if (m_endKind != QLatin1String("stopped"))
            addEvent(m_endKind, event.value(QStringLiteral("text")).toString());
    } else if (kind == QLatin1String("incident")) {
        const QString incident = event.value(QStringLiteral("kind")).toString();
        addEvent(incident);
        if (const auto policies = m_instance.value(QStringLiteral("configuration")).toMap().value(QStringLiteral("failurePolicies")).toMap();
            incident == QLatin1String("death") || policies.value(incident).toString() == QLatin1String("stop"))
            stop();
    } else if (kind == QLatin1String("error")) {
        const QString code = event.value(QStringLiteral("code")).toString();
        m_error = code.startsWith(QLatin1String("account.error.")) || code.startsWith(QLatin1String("session.error."))
            ? code : QStringLiteral("session.error.backend");
        addEvent(QStringLiteral("error"), m_error);
    } else if (kind == QLatin1String("task")) {
        if (const QString status = event.value(QStringLiteral("kind")).toString();
            QStringList{QStringLiteral("taskStarted"), QStringLiteral("taskStepSent"), QStringLiteral("taskCompleted"),
                        QStringLiteral("taskFailed"), QStringLiteral("tasksCancelled")}.contains(status)) {
            const QString code = event.value(QStringLiteral("code")).toString();
            const QString safeCode = QStringList{QStringLiteral("task.error.send"), QStringLiteral("task.error.step"),
                QStringLiteral("task.error.chatSlash"), QStringLiteral("task.error.chatLength"), QStringLiteral("task.error.commandVersion"), QStringLiteral("task.error.offline")}.contains(code)
                ? code : QStringLiteral("task.error.send");
            addEvent(status, event.value(QStringLiteral("name")).toString().left(80),
                     {{QStringLiteral("step"), event.value(QStringLiteral("step")).toInt()}, {QStringLiteral("code"), safeCode}});
        }
    } else if (kind == QLatin1String("profile")) {
        m_account.insert(QStringLiteral("name"), event.value(QStringLiteral("name")).toString());
        emit profileChanged();
    } else if (kind == QLatin1String("applied") && event.value(QStringLiteral("revision")).toInt() == m_revision) {
        if (!m_pendingInstance.isEmpty()) {
            m_applyTimer.stop();
            m_applyInFlight = false;
            m_instance = m_pendingInstance;
            m_pendingInstance.clear();
            emit configureCounter(m_instance.value(QStringLiteral("configuration")).toMap());
            addEvent(QStringLiteral("configurationApplied"));
        }
    } else if (kind == QLatin1String("applyRejected")) {
        m_applyTimer.stop();
        m_applyInFlight = false;
        m_pendingInstance.clear();
        addEvent(QStringLiteral("applyRejected"), event.value(QStringLiteral("code")).toString());
    }
}

void BotSession::processEnded(int exitCode)
{
    m_connectTimer.stop();
    m_applyTimer.stop();
    m_applyInFlight = false;
    m_online = false;
    finishInput(QStringLiteral("task.error.offline"));
    clearServerDisplay();
    emit counterOnline(false);
    if (!m_active)
        return;
    if (m_stopping) {
        finalize(false);
        return;
    }
    if (!m_pendingInstance.isEmpty()
        && m_pendingInstance.value(QStringLiteral("configuration")).toMap().value(QStringLiteral("gameVersion"))
            == m_instance.value(QStringLiteral("configuration")).toMap().value(QStringLiteral("gameVersion"))) {
        m_instance = m_pendingInstance;
        m_pendingInstance.clear();
        emit configureCounter(m_instance.value(QStringLiteral("configuration")).toMap());
        addEvent(QStringLiteral("configurationApplied"));
    }
    if (exitCode != 0) {
        fail(QStringLiteral("session.error.backend"));
        return;
    }
    const QVariantMap configuration = m_instance.value(QStringLiteral("configuration")).toMap();
    const QVariantMap reconnect = configuration.value(QStringLiteral("reconnect")).toMap();
    const QVariantMap policies = configuration.value(QStringLiteral("failurePolicies")).toMap();
    if (const QString cause = m_endKind.isEmpty() ? (m_reachedWorld ? QStringLiteral("disconnect") : QStringLiteral("connectionError")) : m_endKind;
        policies.value(cause).toString() == QLatin1String("reconnect")
        && (reconnect.value(QStringLiteral("unlimited")).toBool()
            || m_retries < reconnect.value(QStringLiteral("maxAttempts")).toInt())) {
        ++m_retries;
        m_state = QStringLiteral("waiting");
        m_retryTimer.start(reconnect.value(QStringLiteral("delaySeconds")).toInt() * 1000);
        addEvent(QStringLiteral("retry"), QString::number(m_retries));
    } else {
        if (m_error.isEmpty())
            m_error = cause == QLatin1String("authentication") ? QStringLiteral("account.error.authentication") : QStringLiteral("session.error.connection");
        finalize(true);
    }
}

void BotSession::fail(const QString &code)
{
    if (!m_active)
        return;
    m_error = code;
    m_process.kill();
    finalize(true);
}

void BotSession::finalize(bool failed)
{
    if (!m_active)
        return;
    m_active = false;
    m_online = false;
    finishInput(QStringLiteral("task.error.offline"));
    clearServerDisplay();
    m_retryTimer.stop();
    m_connectTimer.stop();
    m_applyTimer.stop();
    m_applyInFlight = false;
    m_state = failed ? QStringLiteral("failed") : QStringLiteral("stopped");
    m_finishedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    m_key.fill('\0');
    m_key.clear();
    emit finishCounter();
    addEvent(m_state);
}

void BotSession::stop()
{
    if (!m_active || m_stopping)
        return;
    m_stopping = true;
    finishInput(QStringLiteral("task.error.offline"));
    m_retryTimer.stop();
    m_connectTimer.stop();
    emit counterOnline(false);
    if (m_process.state() == QProcess::NotRunning) {
        finalize(false);
        return;
    }
    m_state = QStringLiteral("stopping");
    send({{QStringLiteral("command"), QStringLiteral("stop")}});
    QTimer::singleShot(2000, this, [this] {
        if (m_stopping && m_active && m_process.state() != QProcess::NotRunning)
            m_process.kill();
    });
    emit changed();
}

void BotSession::apply(const QVariantMap &instance)
{
    if (!m_active || m_stopping || m_applyInFlight)
        return;
    if (const QString version = instance.value(QStringLiteral("configuration")).toMap().value(QStringLiteral("gameVersion")).toString();
        version != m_instance.value(QStringLiteral("configuration")).toMap().value(QStringLiteral("gameVersion")).toString()
        && (m_online || m_state == QLatin1String("connecting") || m_state == QLatin1String("connected"))) {
        m_pendingInstance = instance;
        addEvent(QStringLiteral("configurationDeferred"), version);
        return;
    }
    if (m_online) {
        m_applyInFlight = true;
        m_applyTimer.start();
        m_pendingInstance = instance;
        send({{QStringLiteral("command"), QStringLiteral("apply")},
              {QStringLiteral("revision"), ++m_revision},
              {QStringLiteral("configuration"), QJsonObject::fromVariantMap(instance.value(QStringLiteral("configuration")).toMap())}});
    } else if (m_state == QLatin1String("connecting") || m_state == QLatin1String("connected")) {
        m_pendingInstance = instance;
        addEvent(QStringLiteral("configurationPending"));
    } else {
        m_instance = instance;
        emit configureCounter(instance.value(QStringLiteral("configuration")).toMap());
        if (m_state == QLatin1String("waiting")) {
            const auto configuration = instance.value(QStringLiteral("configuration")).toMap();
            if (const auto policies = configuration.value(QStringLiteral("failurePolicies")).toMap();
                policies.value(m_endKind).toString() != QLatin1String("reconnect"))
                stop();
            else
                m_retryTimer.start(configuration.value(QStringLiteral("reconnect")).toMap().value(QStringLiteral("delaySeconds")).toInt() * 1000);
        }
        addEvent(QStringLiteral("configurationApplied"));
    }
}

QVariantMap BotSession::record() const
{
    QVariantMap instance = m_instance;
    if (m_serverRedacted) {
        instance.remove(QStringLiteral("server"));
        instance.remove(QStringLiteral("port"));
    }
    QVariantMap configuration = instance.value(QStringLiteral("configuration")).toMap();
    QVariantList tasks = configuration.value(QStringLiteral("tasks")).toList();
    for (QVariant &entry : tasks) {
        QVariantMap task = entry.toMap();
        QVariantList steps = task.value(QStringLiteral("steps")).toList();
        for (QVariant &value : steps) {
            QVariantMap step = value.toMap();
            step.insert(QStringLiteral("text"), QString());
            value = step;
        }
        task.insert(QStringLiteral("steps"), steps);
        entry = task;
    }
    configuration.insert(QStringLiteral("tasks"), tasks);
    instance.insert(QStringLiteral("configuration"), configuration);
    return {{QStringLiteral("id"), m_id}, {QStringLiteral("accountId"), m_account.value(QStringLiteral("id"))},
            {QStringLiteral("accountName"), m_accountRedacted ? QString() : m_account.value(QStringLiteral("name"))},
            {QStringLiteral("instanceId"), m_instance.value(QStringLiteral("id"))},
            {QStringLiteral("instanceName"), m_instance.value(QStringLiteral("name"))},
            {QStringLiteral("instance"), instance}, {QStringLiteral("state"), m_state},
            {QStringLiteral("active"), m_active}, {QStringLiteral("applying"), m_applyInFlight}, {QStringLiteral("errorCode"), m_error},
            {QStringLiteral("startedAt"), m_startedAt}, {QStringLiteral("finishedAt"), m_finishedAt},
            {QStringLiteral("retries"), m_retries}, {QStringLiteral("statistics"), m_statistics},
            {QStringLiteral("events"), m_events}, {QStringLiteral("chat"), m_chat},
            {QStringLiteral("player"), m_player}, {QStringLiteral("resources"), m_resources},
            {QStringLiteral("connection"), m_serverRedacted ? QVariantMap{} : m_connection},
            {QStringLiteral("serverDisplay"), m_serverDisplay}, {QStringLiteral("version"), m_version},
            {QStringLiteral("inputReady"), m_online && !m_stopping && m_active},
            {QStringLiteral("commandReady"), m_online && !m_stopping && m_active && m_commandReady},
            {QStringLiteral("regionsRedacted"), m_regionsRedacted},
            {QStringLiteral("monitor"), m_monitorPreferences}};
}


void BotSession::shutdown()
{
    if (m_active)
        stop();
    if (m_process.state() != QProcess::NotRunning && !m_process.waitForFinished(2000)) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
    if (m_active)
        finalize(false);
    if (m_counter && m_counterThread.isRunning()) {
        QVariantMap statistics;
        QMetaObject::invokeMethod(m_counter, [this, &statistics] {
            m_counter->snapshot();
            statistics = m_counter->statistics();
        }, Qt::BlockingQueuedConnection);
        m_statistics = statistics;
    }
}
