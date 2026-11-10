#pragma once

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>
#include <functional>

class MinecraftImages : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int revision READ revision NOTIFY changed)
public:
    explicit MinecraftImages(QObject *parent = nullptr);
    [[nodiscard]] int revision() const { return m_revision; }
    Q_INVOKABLE static QUrl itemIcon(const QString &name);
    Q_INVOKABLE [[nodiscard]] QUrl avatar(const QString &uuid) const;
    Q_INVOKABLE void ensureAvatar(const QString &uuid, bool refresh = false);

signals:
    void changed();

private:
    void get(const QUrl &url, std::function<void(QByteArray)> callback);
    void loadSkin(const QString &uuid, const QUrl &url);
    QNetworkAccessManager m_network;
    QHash<QString, QUrl> m_avatars;
    QSet<QString> m_requested;
    QUrl m_defaultAvatar;
    QString m_cacheDirectory;
    int m_revision = 0;
};
