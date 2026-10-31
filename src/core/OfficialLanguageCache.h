#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QSet>
#include <functional>

class OfficialLanguageCache final : public QObject
{
    Q_OBJECT
public:
    explicit OfficialLanguageCache(QObject *parent = nullptr, const QString &directory = {});
    void ensure(const QString &version, const QString &locale);
    [[nodiscard]] bool busy(const QString &version, const QString &locale) const;
    [[nodiscard]] QString error(const QString &version, const QString &locale) const;
    [[nodiscard]] QJsonObject translations(const QString &version, const QString &locale) const;

signals:
    void changed();

private:
    struct Request {
        QString version;
        QString locale;
        [[nodiscard]] QString key() const { return version + u'/' + locale; }
    };

    void get(const QUrl &url, const QString &hash, std::function<void(QByteArray)> callback);
    void versionMetadata(const Request &request, const QJsonObject &entry);
    void assetIndex(const Request &request, const QJsonObject &index);
    void download(const Request &request, const QJsonObject &index);
    void complete(const Request &request, const QByteArray &bytes, const QString &hash, bool legacy);
    void fail(const Request &request);
    [[nodiscard]] QString cachePath(const Request &request) const;
    static QJsonObject parse(const QByteArray &bytes, bool legacy);
    QNetworkAccessManager m_network;
    QString m_directory;
    QHash<QString, QJsonObject> m_strings;
    QHash<QString, QString> m_errors;
    QSet<QString> m_busy;
};
