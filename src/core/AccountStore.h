#pragma once

#include "CredentialVault.h"

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

class AccountStore : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList accounts READ accounts NOTIFY accountsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString errorCode READ errorCode NOTIFY stateChanged)
    Q_PROPERTY(QString deviceCode READ deviceCode NOTIFY stateChanged)
    Q_PROPERTY(QString verificationUrl READ verificationUrl NOTIFY stateChanged)

public:
    explicit AccountStore(QObject *parent = nullptr);
    ~AccountStore() override;

    [[nodiscard]] QVariantList accounts() const { return m_accounts; }
    [[nodiscard]] bool busy() const { return m_busy; }
    [[nodiscard]] QString errorCode() const { return m_errorCode; }
    [[nodiscard]] QString deviceCode() const { return m_deviceCode; }
    [[nodiscard]] QString verificationUrl() const { return m_verificationUrl; }
    Q_INVOKABLE void reload();
    Q_INVOKABLE void addOffline(const QString &name);
    Q_INVOKABLE void loginMicrosoft(const QString &accountId = {});
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void removeAccount(const QString &accountId);
    Q_INVOKABLE void moveAccount(const QString &accountId, const QString &targetId, bool after);

signals:
    void accountsChanged();
    void stateChanged();
    void accountRemoved(const QString &accountId);
    void accountSaved(const QString &accountId);

private:
    void start(const QJsonObject &request);
    void readOutput();
    void fail(const QString &code);

    CredentialVault m_vault;
    QProcess m_process;
    QTimer m_timeout;
    QVariantList m_accounts;
    QByteArray m_output;
    QString m_errorCode;
    QString m_deviceCode;
    QString m_verificationUrl;
    QString m_selectedId;
    QString m_removingId;
    QString m_loginAccountId;
    bool m_awaitingKey = false;
    bool m_cancelKey = false;
    bool m_busy = false;
    bool m_receivedResult = false;
};
