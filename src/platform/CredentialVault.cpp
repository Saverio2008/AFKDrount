#include "CredentialVault.h"
#include "AppStorage.h"
#include "CredentialBackend.h"

#include <QDirIterator>
#include <QLockFile>
#include <QRandomGenerator>
#include <QtConcurrentRun>

namespace {
QByteArray loadKey(const QString &directory)
{
    if (!QDir().mkpath(directory))
        return {};
    QLockFile lock(directory + QStringLiteral("/credential-key.lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock())
        return {};
    auto result = CredentialBackend::readKey();
    if (result.status == CredentialBackend::Status::Found)
        return result.key;
    if (result.status != CredentialBackend::Status::Missing)
        return {};
    QDirIterator files(directory + QStringLiteral("/accounts/credentials"),
                       { QStringLiteral("*.enc") }, QDir::Files, QDirIterator::Subdirectories);
    if (files.hasNext())
        return {};
    QByteArray key(32, Qt::Uninitialized);
    for (qsizetype offset = 0; offset < key.size(); offset += 4) {
        const quint32 random = QRandomGenerator::system()->generate();
        for (qsizetype byte = 0; byte < 4; ++byte)
            key[offset + byte] = static_cast<char>((random >> (byte * 8)) & 0xff);
    }
    const bool saved = CredentialBackend::writeKey(key);
    key.fill('\0');
    if (!saved)
        return {};
    result = CredentialBackend::readKey();
    return result.status == CredentialBackend::Status::Found ? result.key : QByteArray();
}
}

CredentialVault::CredentialVault(QObject *parent) : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<QByteArray>::finished, this, [this] {
        QByteArray key = m_watcher.result();
        if (key.size() == 32)
            emit keyReady(key);
        else
            emit failed();
        key.fill('\0');
    });
}

void CredentialVault::loadOrCreateKey()
{
    if (m_watcher.isRunning())
        return;
    const QString directory = AppStorage::writableLocation(QStandardPaths::AppLocalDataLocation);
    m_watcher.setFuture(QtConcurrent::run(loadKey, directory));
}