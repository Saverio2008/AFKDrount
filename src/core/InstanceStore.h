#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

class InstanceStore : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList instances READ instances NOTIFY instancesChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit InstanceStore(QObject *parent = nullptr);

    [[nodiscard]] QVariantList instances() const { return m_instances; }
    [[nodiscard]] QString error() const;

    Q_INVOKABLE QString createInstance(const QString &name, const QString &server, int port, const QString &version);
    Q_INVOKABLE bool removeInstance(const QString &id);
    Q_INVOKABLE bool moveInstance(const QString &id, const QString &targetId, bool after);
    Q_INVOKABLE QString duplicateInstance(const QString &id);
    Q_INVOKABLE bool updateInstance(const QString &id, const QString &name,
                                    const QString &server, int port, const QVariantMap &configuration);
    Q_INVOKABLE static QVariantMap defaultConfiguration();
    Q_INVOKABLE static QVariantMap newCountingRule();
    Q_INVOKABLE static QVariantMap newTask();
    Q_INVOKABLE static QString taskError(const QVariantMap &task, const QString &version);
    Q_INVOKABLE static QString configurationError(const QVariantMap &configuration);
    Q_INVOKABLE static QString countingRuleError(const QVariantMap &rule, const QString &version);

signals:
    void instancesChanged();
    void errorChanged();

private:
    void load();
    void setError(const char *source, const QString &detail = {});
    bool validate(const QString &name, const QString &server, int port);
    bool persist(const QVariantList &instances);

    QVariantList m_instances;
    const char *m_errorSource = nullptr;
    QString m_errorDetail;
    QString m_filePath;
    bool m_storageReady = true;
};
