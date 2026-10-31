#pragma once

#include <QFutureWatcher>
#include <QObject>

class CredentialVault final : public QObject
{
    Q_OBJECT

public:
    explicit CredentialVault(QObject *parent = nullptr);
    void loadOrCreateKey();

signals:
    void keyReady(QByteArray key);
    void failed();

private:
    QFutureWatcher<QByteArray> m_watcher;
};