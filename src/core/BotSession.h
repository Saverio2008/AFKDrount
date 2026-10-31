#pragma once

#include "CountingWorker.h"
#include "CredentialVault.h"

#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QVariantMap>

class GameCatalog;

class BotSession final : public QObject
{
    Q_OBJECT
public:
    BotSession(QVariantMap account, QVariantMap instance, GameCatalog *catalog, QObject *parent = nullptr);
    ~BotSession() override;
    void start();
    void shutdown();
    void stop();
    void apply(const QVariantMap &instance);
    int submitInput(const QString &mode, const QString &text);
    void setMonitorPreferences(const QVariantMap &preferences);
    void setLocale(const QString &locale);
    [[nodiscard]] QVariantMap record() const;
    [[nodiscard]] QString id() const { return m_id; }
    [[nodiscard]] bool active() const { return m_active; }

signals:
    void changed();
    void profileChanged();
    void inputFinished(int request, const QString &code);
    void configureCounter(const QVariantMap &configuration);
    void entityBatch(const QJsonArray &events, qsizetype bytes);
    void resetCounter();
    void counterOnline(bool online);
    void finishCounter();

private:
    void launch();
    void readOutput();
    void receive(const QJsonObject &event);
    void send(const QJsonObject &request);
    void requestTranslations();
    void sendTranslations();
    void addEvent(const QString &kind, const QString &text = {}, const QVariantMap &data = {});
    void finalize(bool failed);
    void processEnded(int exitCode);
    void fail(const QString &code);
    void finishInput(const QString &code);
    void clearServerDisplay();

    QString m_id;
    QVariantMap m_account;
    QVariantMap m_instance;
    GameCatalog *m_catalog;
    QVariantMap m_pendingInstance;
    QVariantMap m_statistics;
    QVariantList m_events;
    QVariantList m_chat;
    qsizetype m_eventBytes = 0;
    qsizetype m_chatBytes = 0;
    qint64 m_chatSerial = 0;
    QVariantMap m_player;
    QVariantMap m_resources;
    QVariantMap m_connection;
    QVariantMap m_serverDisplay;
    QVariantMap m_monitorPreferences;
    bool m_accountRedacted = false;
    bool m_serverRedacted = false;
    bool m_regionsRedacted = false;
    bool m_commandReady = false;
    QString m_version;
    QTimer m_titleTimer;
    QTimer m_actionBarTimer;
    int m_titleDuration = 3500;
    int m_inputSerial = 0;
    int m_pendingInput = 0;
    QString m_inputMode;
    QTimer m_inputTimer;
    QProcess m_process;
    CredentialVault m_vault;
    QThread m_counterThread;
    CountingWorker *m_counter = nullptr;
    QTimer m_retryTimer;
    QTimer m_connectTimer;
    QTimer m_applyTimer;
    QByteArray m_key;
    QByteArray m_output;
    QString m_state = QStringLiteral("connecting");
    QString m_locale = QStringLiteral("en_us");
    QString m_connectionVersion;
    bool m_sentTranslations = false;
    QString m_error;
    QString m_endKind;
    QString m_startedAt;
    QString m_finishedAt;
    qsizetype m_pendingBytes = 0;
    int m_retries = 0;
    int m_revision = 0;
    bool m_active = true;
    bool m_applyInFlight = false;
    bool m_stopping = false;
    bool m_online = false;
    bool m_reachedWorld = false;
    bool m_startupTasksTriggered = false;
};