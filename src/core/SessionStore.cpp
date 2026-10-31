#include "SessionStore.h"
#include "AppStorage.h"
#include "AccountStore.h"
#include "InstanceStore.h"
#include "InstanceConfiguration.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QUuid>
#include <algorithm>
#include <ranges>

namespace {
QString runDirectory()
{
    return AppStorage::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/runs");
}

QString monitorTemplate(const QString &account, const QString &instance)
{
    return QStringLiteral("monitor/layouts/") + account + u'/' + instance;
}

QVariant scrub(const QVariant &value, const QSet<QString> &keys)
{
    if (value.metaType().id() == QMetaType::QVariantMap) {
        QVariantMap map = value.toMap();
        for (const QString &key : keys)
            map.remove(key);
        for (auto it = map.begin(); it != map.end(); ++it)
            if (it.key() != QLatin1String("items") && it.key() != QLatin1String("units"))
                it.value() = scrub(it.value(), keys);
        return map;
    }
    if (value.metaType().id() == QMetaType::QVariantList) {
        QVariantList list = value.toList();
        for (QVariant &entry : list)
            entry = scrub(entry, keys);
        return list;
    }
    return value;
}

QVariantMap privateRecord(QVariantMap record)
{
    const QVariantMap preferences = record.value(QStringLiteral("monitor")).toMap();
    QSet<QString> keys;
    const bool hidePosition = preferences.value(QStringLiteral("hidePosition"), true).toBool();
    if (hidePosition)
        keys << QStringLiteral("position");
    if (hidePosition || record.value(QStringLiteral("regionsRedacted")).toBool())
        keys << QStringLiteral("trackingRegion") << QStringLiteral("countingRegion");
    if (preferences.value(QStringLiteral("hideServer")).toBool())
        keys << QStringLiteral("server") << QStringLiteral("port") << QStringLiteral("host");
    if (preferences.value(QStringLiteral("hideAccount")).toBool())
        keys << QStringLiteral("accountName") << QStringLiteral("username");
    if (keys.isEmpty())
        return record;
    for (const QString &key : keys)
        record.remove(key);
    for (const QString &key : {QStringLiteral("instance"), QStringLiteral("connection"),
                              QStringLiteral("player"), QStringLiteral("statistics")})
        if (auto it = record.find(key); it != record.end())
            it.value() = scrub(it.value(), keys);
    QVariantList events = record.value(QStringLiteral("events")).toList();
    for (QVariant &entry : events) {
        if (QVariantMap event = entry.toMap(); event.value(QStringLiteral("kind")) == QLatin1String("summary")) {
            event.insert(QStringLiteral("data"), scrub(event.value(QStringLiteral("data")), keys));
            entry = event;
        }
    }
    record.insert(QStringLiteral("events"), events);
    return record;
}

bool writeRecord(const QVariantMap &record)
{
    if (!QDir().mkpath(runDirectory()))
        return false;
    QSaveFile file(runDirectory() + u'/' + record.value(QStringLiteral("id")).toString() + QStringLiteral(".json"));
    const QByteArray bytes = QJsonDocument::fromVariant(record).toJson(QJsonDocument::Compact);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
}
}

SessionStore::SessionStore(QObject *parent) : QObject(parent)
{
    const QSettings settings;
    m_confirmClose = settings.value(QStringLiteral("sessions/confirmClose"), true).toBool();
    m_stopOnClose = settings.value(QStringLiteral("sessions/stopOnClose"), true).toBool();
    m_notifications.setSingleShot(true);
    m_notifications.setInterval(200);
    connect(&m_notifications, &QTimer::timeout, this, [this] {
        ++m_monitorRevision;
        emit sessionsChanged();
    });
    m_persistence.setInterval(5000);
    connect(&m_persistence, &QTimer::timeout, this, &SessionStore::save);
    m_persistence.start();
    m_accountRefresh.setSingleShot(true);
    m_accountRefresh.setInterval(100);
    connect(&m_accountRefresh, &QTimer::timeout, this, [this] {
        if (!m_accounts)
            return;
        if (m_accounts->busy())
            m_accountRefresh.start();
        else
            m_accounts->reload();
    });
    const QDir directory(runDirectory());
    for (const QStringList files = directory.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Time);
         const QString &name : files) {
        if (m_history.size() >= 100)
            break;
        QFile file(directory.filePath(name));
        if (!file.open(QIODevice::ReadOnly) || file.size() > 16 * 1024 * 1024)
            continue;
        QVariantMap record = QJsonDocument::fromJson(file.readAll()).object().toVariantMap();
        if (QUuid(record.value(QStringLiteral("id")).toString()).isNull()
            || name != record.value(QStringLiteral("id")).toString() + QStringLiteral(".json")
            || record.value(QStringLiteral("instanceName")).metaType().id() != QMetaType::QString
            || record.value(QStringLiteral("events")).metaType().id() != QMetaType::QVariantList
            || record.value(QStringLiteral("statistics")).metaType().id() != QMetaType::QVariantMap
            || record.value(QStringLiteral("errorCode")).metaType().id() != QMetaType::QString)
            continue;
        file.close();
        if (const QVariantMap sanitized = privateRecord(record); sanitized != record) {
            if (!writeRecord(sanitized))
                m_error = QStringLiteral("session.error.save");
            record = sanitized;
        }
        if (record.value(QStringLiteral("active")).toBool()) {
            record.insert(QStringLiteral("state"), QStringLiteral("interrupted"));
            record.insert(QStringLiteral("active"), false);
        }
        record.remove(QStringLiteral("inputReady"));
        record.remove(QStringLiteral("commandReady"));
        record.remove(QStringLiteral("serverDisplay"));
        if (!record.contains(QStringLiteral("chat"))) {
            QVariantList chat;
            for (const QVariant &entry : record.value(QStringLiteral("events")).toList())
                if (entry.toMap().value(QStringLiteral("kind")) == QLatin1String("chat"))
                    chat.append(entry);
            record.insert(QStringLiteral("chat"), chat);
        }
        m_history.append(record);
    }
}

SessionStore::~SessionStore()
{
    shutdown();
}

void SessionStore::configureStores(AccountStore *accounts, InstanceStore *instances, GameCatalog *catalog)
{
    m_accounts = accounts;
    m_instances = instances;
    m_catalog = catalog;
    connect(accounts, &AccountStore::accountsChanged, this, [this] {
        const auto records = m_accounts->accounts();
        for (BotSession *session : std::as_const(m_live)) {
            if (const QString id = session->record().value(QStringLiteral("accountId")).toString();
                session->active() && !std::ranges::any_of(records, [&id](const QVariant &record) {
                return record.toMap().value(QStringLiteral("id")).toString() == id;
            }))
                session->stop();
        }
    });
}

void SessionStore::setLocale(const QString &locale)
{
    m_locale = locale;
    for (BotSession *session : std::as_const(m_live))
        if (session->active())
            session->setLocale(m_locale);
}

QVariantList SessionStore::sessions() const
{
    QVariantList result;
    auto append = [&result](const QVariantMap &record) {
        QVariantMap summary;
        for (const QString &key : {QStringLiteral("id"), QStringLiteral("accountId"), QStringLiteral("accountName"),
             QStringLiteral("instanceId"), QStringLiteral("instanceName"), QStringLiteral("state"),
             QStringLiteral("active"), QStringLiteral("applying"), QStringLiteral("errorCode"),
             QStringLiteral("startedAt"), QStringLiteral("finishedAt"), QStringLiteral("retries")})
            summary.insert(key, record.value(key));
        if (record.value(QStringLiteral("monitor")).toMap().value(QStringLiteral("hideAccount")).toBool())
            summary.remove(QStringLiteral("accountName"));
        result.append(summary);
    };
    for (const BotSession *session : m_live | std::views::reverse)
        append(session->record());
    for (const QVariant &entry : m_history)
        append(entry.toMap());
    return result;
}

QVariantMap SessionStore::monitorRecord(const QString &id) const
{
    for (const BotSession *session : m_live)
        if (session->id() == id)
            return privateRecord(session->record());
    for (const QVariant &entry : m_history)
        if (const QVariantMap record = entry.toMap(); record.value(QStringLiteral("id")) == id)
            return privateRecord(record);
    return {};
}

int SessionStore::submitInput(const QString &id, const QString &mode, const QString &text)
{
    for (BotSession *session : m_live)
        if (session->id() == id)
            return session->submitInput(mode, text);
    return 0;
}

bool SessionStore::setMonitorPreferences(const QString &id, const QVariantMap &preferences)
{
    const auto live = std::ranges::find_if(m_live, [&id](const BotSession *session) { return session->id() == id; });
    const auto history = std::ranges::find_if(m_history, [&id](const QVariant &entry) {
        return entry.toMap().value(QStringLiteral("id")) == id;
    });
    if (live == m_live.end() && history == m_history.end())
        return false;
    QVariantMap record = live != m_live.end() ? (*live)->record() : history->toMap();
    record.insert(QStringLiteral("monitor"), preferences);
    const QVariantMap sanitized = privateRecord(record);
    if (!writeRecord(sanitized)) {
        m_error = QStringLiteral("session.error.save");
        emit errorChanged();
        return false;
    }
    if (live != m_live.end())
        (*live)->setMonitorPreferences(preferences);
    else
        *history = sanitized;
    QSettings().setValue(monitorTemplate(record.value(QStringLiteral("accountId")).toString(),
                                         record.value(QStringLiteral("instanceId")).toString()), preferences);
    ++m_monitorRevision;
    emit sessionsChanged();
    return true;
}

int SessionStore::activeCount() const
{
    return static_cast<int>(std::ranges::count_if(m_live, [](const BotSession *session) { return session->active(); }));
}

QString SessionStore::startSession(const QString &accountId, const QString &instanceId)
{
    if (!m_accounts || !m_instances || !m_catalog)
        return {};
    m_error.clear();
    const auto accounts = m_accounts->accounts();
    const auto instances = m_instances->instances();
    const auto account = std::ranges::find_if(accounts, [&accountId](const QVariant &value) {
        return value.toMap().value(QStringLiteral("id")).toString() == accountId;
    });
    const auto instance = std::ranges::find_if(instances, [&instanceId](const QVariant &value) {
        return value.toMap().value(QStringLiteral("id")).toString() == instanceId;
    });
    if (account == accounts.cend() || instance == instances.cend() || m_accounts->busy()) {
        m_error = QStringLiteral("session.error.selection");
        emit errorChanged();
        return {};
    }
    QVariantMap normalized;
    if (const QString error = InstanceConfiguration::normalize(instance->toMap().value(QStringLiteral("configuration")).toMap(), normalized);
        !error.isEmpty()) {
        m_error = error;
        emit errorChanged();
        return {};
    }
    auto *session = new BotSession(account->toMap(), instance->toMap(), m_catalog, this);
    session->setLocale(m_locale);
    session->setMonitorPreferences(QSettings().value(monitorTemplate(accountId, instanceId)).toMap());
    m_live.append(session);
    connect(session, &BotSession::changed, this, [this] {
        m_dirty = true;
        if (!m_notifications.isActive())
            m_notifications.start();
    });
    connect(session, &BotSession::profileChanged, this, [this] { m_accountRefresh.start(); });
    connect(session, &BotSession::inputFinished, this, [this, session](int request, const QString &code) {
        emit inputFinished(session->id(), request, code);
    });
    session->start();
    m_dirty = true;
    emit sessionsChanged();
    emit errorChanged();
    return session->id();
}

void SessionStore::stopSession(const QString &id)
{
    for (BotSession *session : std::as_const(m_live))
        if (session->id() == id)
            session->stop();
}

bool SessionStore::removeRecord(const QString &id)
{
    const auto live = std::ranges::find_if(m_live, [&id](const BotSession *session) {
        return session->id() == id;
    });
    const auto history = std::ranges::find_if(m_history, [&id](const QVariant &entry) {
        return entry.toMap().value(QStringLiteral("id")).toString() == id;
    });
    if (QUuid(id).isNull() || (live == m_live.cend() && history == m_history.cend())) {
        m_error = QStringLiteral("session.error.notFound");
        emit errorChanged();
        return false;
    }
    if (live != m_live.cend() && (*live)->active()) {
        m_error = QStringLiteral("session.error.activeRecord");
        emit errorChanged();
        return false;
    }
    if (QFile file(runDirectory() + u'/' + id + QStringLiteral(".json"));
        file.exists() && !file.remove()) {
        m_error = QStringLiteral("session.error.removeRecord");
        emit errorChanged();
        return false;
    }
    if (live != m_live.cend()) {
        BotSession *session = *live;
        disconnect(session, nullptr, this, nullptr);
        m_live.erase(live);
        session->deleteLater();
    } else {
        m_history.erase(history);
    }
    m_error.clear();
    ++m_monitorRevision;
    emit errorChanged();
    emit sessionsChanged();
    return true;
}

void SessionStore::applyConfiguration(const QString &id)
{
    if (!m_instances)
        return;
    for (BotSession *session : std::as_const(m_live)) {
        if (session->id() != id || !session->active())
            continue;
        const QString instanceId = session->record().value(QStringLiteral("instanceId")).toString();
        for (const QVariantList instances = m_instances->instances(); const QVariant &entry : instances)
            if (entry.toMap().value(QStringLiteral("id")).toString() == instanceId) {
                QVariantMap normalized;
                if (const QString error = InstanceConfiguration::normalize(entry.toMap().value(QStringLiteral("configuration")).toMap(), normalized);
                    !error.isEmpty()) {
                    m_error = error;
                    emit errorChanged();
                    return;
                }
                session->apply(entry.toMap());
            }
    }
}

void SessionStore::stopAll()
{
    for (BotSession *session : std::as_const(m_live))
        session->stop();
}

void SessionStore::save()
{
    if (!m_dirty)
        return;
    for (BotSession *session : std::as_const(m_live)) {
        if (!writeRecord(privateRecord(session->record()))) {
            m_error = QStringLiteral("session.error.save");
            emit errorChanged();
            return;
        }
    }
    m_dirty = false;
}

void SessionStore::shutdown()
{
    for (BotSession *session : std::as_const(m_live))
        session->shutdown();
    save();
}


void SessionStore::setConfirmClose(bool value)
{
    if (m_confirmClose == value)
        return;
    m_confirmClose = value;
    QSettings().setValue(QStringLiteral("sessions/confirmClose"), value);
    emit settingsChanged();
}

void SessionStore::setStopOnClose(bool value)
{
    if (m_stopOnClose == value)
        return;
    m_stopOnClose = value;
    QSettings().setValue(QStringLiteral("sessions/stopOnClose"), value);
    emit settingsChanged();
}
