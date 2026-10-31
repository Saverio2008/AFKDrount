#include "AccountStore.h"
#include "AppStorage.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QUrl>

AccountStore::AccountStore(QObject *parent) : QObject(parent)
{
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(16 * 60 * 1000);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        fail(QStringLiteral("account.error.timeout"));
        m_process.kill();
    });
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &AccountStore::readOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this, [this] {
        m_process.readAllStandardError();
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_timeout.stop();
            m_busy = false;
            fail(QStringLiteral("account.error.backend"));
        }
    });
    connect(&m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        readOutput();
        m_timeout.stop();
        m_busy = false;
        m_deviceCode.clear();
        m_verificationUrl.clear();
        if (!m_receivedResult && m_errorCode.isEmpty())
            m_errorCode = QStringLiteral("account.error.backend");
        emit stateChanged();
        if (m_receivedResult && exitCode == 0 && status == QProcess::NormalExit && !m_selectedId.isEmpty())
            emit accountSaved(m_selectedId);
        if (m_receivedResult && exitCode == 0 && status == QProcess::NormalExit && !m_removingId.isEmpty())
            emit accountRemoved(m_removingId);
    });
    connect(&m_vault, &CredentialVault::keyReady, this, [this](QByteArray key) {
        m_awaitingKey = false;
        m_busy = false;
        if (m_cancelKey) {
            m_cancelKey = false;
            key.fill('\0');
            emit stateChanged();
            return;
        }
        start({ { QStringLiteral("command"), QStringLiteral("loginMicrosoft") },
                { QStringLiteral("accountId"), m_loginAccountId },
                { QStringLiteral("key"), QString::fromLatin1(key.toBase64()) } });
        key.fill('\0');
    });
    connect(&m_vault, &CredentialVault::failed, this, [this] {
        m_awaitingKey = false;
        m_busy = false;
        if (m_cancelKey) {
            m_cancelKey = false;
            emit stateChanged();
        } else {
            fail(QStringLiteral("account.error.keychain"));
        }
    });
    QTimer::singleShot(0, this, &AccountStore::reload);
}

AccountStore::~AccountStore()
{
    disconnect(&m_process, nullptr, this, nullptr);
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(2000);
    }
}

void AccountStore::fail(const QString &code)
{
    m_errorCode = code;
    emit stateChanged();
}

void AccountStore::start(const QJsonObject &request)
{
    if (m_busy)
        return;
    const QDir appDirectory(QCoreApplication::applicationDirPath());
    const QDir runtime(appDirectory.filePath(QStringLiteral(AFK_BACKEND_SUBDIR)));
    const QString node = appDirectory.filePath(QStringLiteral(AFK_NODE_SUBPATH));
    m_output.clear();
    m_errorCode.clear();
    m_selectedId.clear();
    m_removingId = request.value(QStringLiteral("command")).toString() == QStringLiteral("removeAccount")
        ? request.value(QStringLiteral("accountId")).toString() : QString();
    m_deviceCode.clear();
    m_verificationUrl.clear();
    m_receivedResult = false;
    m_busy = true;
    emit stateChanged();
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.remove(QStringLiteral("NODE_OPTIONS"));
    environment.remove(QStringLiteral("NODE_PATH"));
    environment.remove(QStringLiteral("DEBUG"));
    m_process.setProcessEnvironment(environment);
    m_process.setWorkingDirectory(runtime.absolutePath());
    m_process.start(node, { runtime.filePath(QStringLiteral("main.mjs")),
        AppStorage::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/accounts") });
    m_process.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n');
    m_process.closeWriteChannel();
    m_timeout.start();
}

void AccountStore::readOutput()
{
    m_output += m_process.readAllStandardOutput();
    if (m_output.size() > 1024 * 1024) {
        fail(QStringLiteral("account.error.protocol"));
        m_process.kill();
        return;
    }
    while (m_output.contains('\n')) {
        const qsizetype end = m_output.indexOf('\n');
        const auto document = QJsonDocument::fromJson(m_output.first(end));
        m_output.remove(0, end + 1);
        const auto event = document.object();
        if (const QString kind = event.value(QStringLiteral("event")).toString();
            kind == QStringLiteral("result") && event.value(QStringLiteral("accounts")).isArray()) {
            m_accounts = event.value(QStringLiteral("accounts")).toArray().toVariantList();
            m_selectedId = event.value(QStringLiteral("selectedId")).toString();
            m_receivedResult = true;
            emit accountsChanged();
        } else if (kind == QStringLiteral("deviceCode")) {
            const QUrl url(event.value(QStringLiteral("url")).toString());
            if (url.scheme() != QStringLiteral("https") ||
                !QStringList{ QStringLiteral("microsoft.com"), QStringLiteral("www.microsoft.com"), QStringLiteral("login.live.com") }.contains(url.host())) {
                fail(QStringLiteral("account.error.protocol"));
                m_process.kill();
                return;
            }
            m_deviceCode = event.value(QStringLiteral("code")).toString();
            m_verificationUrl = url.toString();
            emit stateChanged();
        } else if (kind == QStringLiteral("error")) {
            const QString code = event.value(QStringLiteral("code")).toString();
            fail(code.startsWith(QStringLiteral("account.error.")) ? code : QStringLiteral("account.error.backend"));
        } else {
            fail(QStringLiteral("account.error.protocol"));
            m_process.kill();
            return;
        }
    }
}

void AccountStore::reload()
{
    start({ { QStringLiteral("command"), QStringLiteral("list") } });
}

void AccountStore::addOffline(const QString &name)
{
    start({ { QStringLiteral("command"), QStringLiteral("addOffline") }, { QStringLiteral("name"), name } });
}

void AccountStore::loginMicrosoft(const QString &accountId)
{
    if (m_busy)
        return;
    m_busy = true;
    m_awaitingKey = true;
    m_cancelKey = false;
    m_loginAccountId = accountId;
    m_errorCode.clear();
    m_deviceCode.clear();
    m_verificationUrl.clear();
    emit stateChanged();
    m_vault.loadOrCreateKey();
}

void AccountStore::cancel()
{
    if (!m_busy)
        return;
    m_errorCode = QStringLiteral("account.error.cancelled");
    if (m_awaitingKey) {
        m_cancelKey = true;
        emit stateChanged();
    } else {
        m_process.kill();
    }
}

void AccountStore::removeAccount(const QString &accountId)
{
    start({ { QStringLiteral("command"), QStringLiteral("removeAccount") },
            { QStringLiteral("accountId"), accountId } });
}

void AccountStore::moveAccount(const QString &accountId, const QString &targetId, bool after)
{
    start({ { QStringLiteral("command"), QStringLiteral("moveAccount") },
            { QStringLiteral("accountId"), accountId },
            { QStringLiteral("targetId"), targetId },
            { QStringLiteral("after"), after } });
}
