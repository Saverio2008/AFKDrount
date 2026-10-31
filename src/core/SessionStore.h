#pragma once

#include "BotSession.h"

#include <QObject>
#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>

class AccountStore;
class GameCatalog;
class InstanceStore;

class SessionStore final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList sessions READ sessions NOTIFY sessionsChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY sessionsChanged)
    Q_PROPERTY(int monitorRevision READ monitorRevision NOTIFY sessionsChanged)
    Q_PROPERTY(QString errorCode READ errorCode NOTIFY errorChanged)
    Q_PROPERTY(bool confirmClose READ confirmClose WRITE setConfirmClose NOTIFY settingsChanged)
    Q_PROPERTY(bool stopOnClose READ stopOnClose WRITE setStopOnClose NOTIFY settingsChanged)
public:
    explicit SessionStore(QObject *parent = nullptr);
    ~SessionStore() override;
    void configureStores(AccountStore *accounts, InstanceStore *instances, GameCatalog *catalog);
    void setLocale(const QString &locale);
    void shutdown();
    [[nodiscard]] bool confirmClose() const { return m_confirmClose; }
    [[nodiscard]] bool stopOnClose() const { return m_stopOnClose; }
    void setConfirmClose(bool value);
    void setStopOnClose(bool value);
    [[nodiscard]] QVariantList sessions() const;
    [[nodiscard]] int activeCount() const;
    [[nodiscard]] int monitorRevision() const { return m_monitorRevision; }
    [[nodiscard]] QString errorCode() const { return m_error; }
    Q_INVOKABLE QString startSession(const QString &accountId, const QString &instanceId);
    Q_INVOKABLE void stopSession(const QString &id);
    Q_INVOKABLE bool removeRecord(const QString &id);
    Q_INVOKABLE void applyConfiguration(const QString &id);
    Q_INVOKABLE [[nodiscard]] QVariantMap monitorRecord(const QString &id) const;
    Q_INVOKABLE int submitInput(const QString &id, const QString &mode, const QString &text);
    Q_INVOKABLE bool setMonitorPreferences(const QString &id, const QVariantMap &preferences);
    Q_INVOKABLE void stopAll();

signals:
    void sessionsChanged();
    void errorChanged();
    void settingsChanged();
    void inputFinished(const QString &id, int request, const QString &code);

private:
    void save();
    AccountStore *m_accounts = nullptr;
    InstanceStore *m_instances = nullptr;
    GameCatalog *m_catalog = nullptr;
    QList<BotSession *> m_live;
    QVariantList m_history;
    QString m_locale = QStringLiteral("en_us");
    QString m_error;
    QTimer m_notifications;
    QTimer m_persistence;
    QTimer m_accountRefresh;
    int m_monitorRevision = 0;
    bool m_dirty = false;
    bool m_confirmClose = true;
    bool m_stopOnClose = true;
};
