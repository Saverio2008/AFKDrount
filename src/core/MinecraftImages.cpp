#include "MinecraftImages.h"
#include "AppStorage.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QPainter>
#include <QRegularExpression>
#include <QSaveFile>

namespace {
QString normalizedUuid(QString uuid)
{
    uuid.remove(u'-');
    static const QRegularExpression pattern(QStringLiteral("^[a-fA-F0-9]{32}$"));
    return pattern.match(uuid).hasMatch() ? uuid.toLower() : QString();
}

QByteArray png(const QImage &image)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

QUrl imageUrl(const QImage &image)
{
    return image.isNull() ? QUrl() : QUrl(QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png(image).toBase64()));
}

QImage skinFace(const QByteArray &bytes)
{
    QBuffer buffer;
    buffer.setData(bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer, "PNG");
    if (reader.size() != QSize(64, 64) && reader.size() != QSize(64, 32))
        return {};
    const QImage skin = reader.read();
    if (skin.isNull())
        return {};
    QImage face = skin.copy(8, 8, 8, 8).convertToFormat(QImage::Format_ARGB32);
    QPainter painter(&face);
    painter.drawImage(0, 0, skin.copy(40, 8, 8, 8));
    return face;
}
}

MinecraftImages::MinecraftImages(QObject *parent) : QObject(parent)
{
    if (QFile skin(QStringLiteral(":/minecraft/26.1/skins/steve.png")); skin.open(QIODevice::ReadOnly))
        m_defaultAvatar = imageUrl(skinFace(skin.readAll()));
    m_cacheDirectory = AppStorage::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/avatars");
}

QUrl MinecraftImages::itemIcon(const QString &name)
{
    static const QRegularExpression pattern(QStringLiteral("^[a-z0-9_]+$"));
    if (!pattern.match(name).hasMatch())
        return {};
    const QString path = QStringLiteral(":/minecraft/26.1/items/") + name + QStringLiteral(".png");
    return QFile::exists(path) ? QUrl(QStringLiteral("qrc") + path) : QUrl();
}

QUrl MinecraftImages::avatar(const QString &uuid) const
{
    return m_avatars.value(normalizedUuid(uuid), m_defaultAvatar);
}

void MinecraftImages::get(const QUrl &url, std::function<void(QByteArray)> callback)
{
    if (url.scheme() != QLatin1String("https") || !url.userInfo().isEmpty()
        || (url.host() != QLatin1String("sessionserver.mojang.com") && url.host() != QLatin1String("textures.minecraft.net")))
        return;
    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::readyRead, reply, [reply] {
        if (reply->bytesAvailable() > 1024 * 1024)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [reply, callback = std::move(callback)] {
        if (reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200
            && reply->bytesAvailable() <= 1024 * 1024)
            callback(reply->readAll());
        reply->deleteLater();
    });
}

void MinecraftImages::ensureAvatar(const QString &uuid, bool refresh)
{
    const QString id = normalizedUuid(uuid);
    if (id.isEmpty() || (!refresh && m_requested.contains(id)))
        return;
    m_requested.insert(id);
    if (const QImage cached(m_cacheDirectory + u'/' + id + QStringLiteral(".png")); cached.size() == QSize(8, 8)) {
        m_avatars.insert(id, imageUrl(cached));
        ++m_revision;
        emit changed();
    }
    get(QUrl(QStringLiteral("https://sessionserver.mojang.com/session/minecraft/profile/") + id),
        [this, id](const QByteArray &bytes) {
        const QJsonObject profile = QJsonDocument::fromJson(bytes).object();
        if (normalizedUuid(profile.value(QStringLiteral("id")).toString()) != id)
            return;
        for (const QJsonArray properties = profile.value(QStringLiteral("properties")).toArray(); const auto &entry : properties) {
            const QJsonObject property = entry.toObject();
            if (property.value(QStringLiteral("name")).toString() != QLatin1String("textures"))
                continue;
            const QJsonObject textures = QJsonDocument::fromJson(QByteArray::fromBase64(
                property.value(QStringLiteral("value")).toString().toLatin1())).object();
            QUrl url(textures.value(QStringLiteral("textures")).toObject().value(QStringLiteral("SKIN")).toObject()
                         .value(QStringLiteral("url")).toString());
            if (url.host() == QLatin1String("textures.minecraft.net") && url.scheme() == QLatin1String("http"))
                url.setScheme(QStringLiteral("https"));
            loadSkin(id, url);
            return;
        }
    });
}

void MinecraftImages::loadSkin(const QString &uuid, const QUrl &url)
{
    if (url.host() != QLatin1String("textures.minecraft.net") || !url.path().startsWith(QLatin1String("/texture/")))
        return;
    get(url, [this, uuid](const QByteArray &bytes) {
        const QImage face = skinFace(bytes);
        if (face.isNull())
            return;
        m_avatars.insert(uuid, imageUrl(face));
        if (QDir().mkpath(m_cacheDirectory)) {
            if (QSaveFile file(m_cacheDirectory + u'/' + uuid + QStringLiteral(".png")); file.open(QIODevice::WriteOnly)) {
                file.write(png(face));
                file.commit();
            }
        }
        ++m_revision;
        emit changed();
    });
}