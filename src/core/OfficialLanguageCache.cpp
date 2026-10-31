#include "OfficialLanguageCache.h"
#include "AppStorage.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSharedPointer>

namespace {
bool officialUrl(const QUrl &url)
{
    return url.scheme() == QLatin1String("https") && url.userInfo().isEmpty()
        && QStringList{QStringLiteral("piston-meta.mojang.com"), QStringLiteral("launchermeta.mojang.com"),
                       QStringLiteral("resources.download.minecraft.net")}.contains(url.host());
}

QString sha1(const QByteArray &bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha1).toHex());
}
}

OfficialLanguageCache::OfficialLanguageCache(QObject *parent, const QString &directory)
    : QObject(parent), m_directory(directory.isEmpty()
        ? AppStorage::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/minecraft-language")
        : directory) {}

QString OfficialLanguageCache::cachePath(const Request &request) const
{
    // Older caches used the version alone for Simplified Chinese.
    const QString key = request.locale == QLatin1String("zh_cn") ? request.version : request.key();
    const QByteArray name = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256).toHex();
    return m_directory + u'/' + QString::fromLatin1(name) + QStringLiteral(".json");
}

QJsonObject OfficialLanguageCache::parse(const QByteArray &bytes, bool legacy)
{
    if (!legacy)
        return QJsonDocument::fromJson(bytes).object();
    QJsonObject strings;
    for (const QStringList lines = QString::fromUtf8(bytes).split(u'\n'); const QString &line : lines) {
        if (line.startsWith(u'#'))
            continue;
        if (const qsizetype separator = line.indexOf(u'='); separator > 0) {
            QString value = line.mid(separator + 1);
            if (value.endsWith(u'\r'))
                value.chop(1);
            strings.insert(line.first(separator), value);
        }
    }
    return strings;
}

bool OfficialLanguageCache::busy(const QString &version, const QString &locale) const
{
    return m_busy.contains(Request{.version = version, .locale = locale}.key());
}

QString OfficialLanguageCache::error(const QString &version, const QString &locale) const
{
    return m_errors.value(Request{.version = version, .locale = locale}.key());
}

QJsonObject OfficialLanguageCache::translations(const QString &version, const QString &locale) const
{
    return m_strings.value(Request{.version = version, .locale = locale}.key());
}

void OfficialLanguageCache::fail(const Request &request)
{
    m_busy.remove(request.key());
    m_errors.insert(request.key(), QStringLiteral("catalog.error.language"));
    emit changed();
}

void OfficialLanguageCache::get(const QUrl &url, const QString &hash, std::function<void(QByteArray)> callback)
{
    if (!officialUrl(url)) {
        callback({});
        return;
    }
    QNetworkRequest request(url);
    request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    QNetworkReply *reply = m_network.get(request);
    const auto buffer = QSharedPointer<QByteArray>::create();
    connect(reply, &QNetworkReply::readyRead, this, [reply, buffer] {
        buffer->append(reply->readAll());
        if (buffer->size() > 16 * 1024 * 1024)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [reply, buffer, hash, callback = std::move(callback)] {
        buffer->append(reply->readAll());
        const bool valid = reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200
            && buffer->size() <= 16 * 1024 * 1024 && (hash.isEmpty() || sha1(*buffer) == hash);
        reply->deleteLater();
        callback(valid ? *buffer : QByteArray());
    });
}

void OfficialLanguageCache::ensure(const QString &version, const QString &locale)
{
    static const QRegularExpression localePattern(QStringLiteral("^[a-z]{2}_[a-z]{2}$"));
    if (locale == QLatin1String("en_us") || !localePattern.match(locale).hasMatch())
        return;
    const Request request{.version = version, .locale = locale};
    const QString key = request.key();
    if ((m_strings.contains(key) && !m_errors.contains(key)) || m_busy.contains(key))
        return;
    if (QFile file(cachePath(request)); file.open(QIODevice::ReadOnly) && file.size() <= 24 * 1024 * 1024) {
        const QJsonObject cached = QJsonDocument::fromJson(file.readAll()).object();
        if (const QByteArray bytes = QByteArray::fromBase64(cached.value(QStringLiteral("payload")).toString().toLatin1());
            cached.value(QStringLiteral("version")).toString() == version
            && cached.value(QStringLiteral("locale")).toString(QStringLiteral("zh_cn")) == locale
            && !bytes.isEmpty() && sha1(bytes) == cached.value(QStringLiteral("sha1")).toString()) {
            if (const QJsonObject strings = parse(bytes, cached.value(QStringLiteral("legacy")).toBool()); !strings.isEmpty()) {
                m_strings.insert(key, strings);
                m_errors.remove(key);
                emit changed();
                return;
            }
        }
    }
    m_errors.remove(key);
    m_busy.insert(key);
    emit changed();
    get(QUrl(QStringLiteral("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json")), {},
        [this, request](const QByteArray &bytes) {
        for (const QJsonArray versions = QJsonDocument::fromJson(bytes).object().value(QStringLiteral("versions")).toArray(); const auto &entry : versions) {
            if (entry.toObject().value(QStringLiteral("id")).toString() == request.version) {
                versionMetadata(request, entry.toObject());
                return;
            }
        }
        fail(request);
    });
}

void OfficialLanguageCache::versionMetadata(const Request &request, const QJsonObject &entry)
{
    get(QUrl(entry.value(QStringLiteral("url")).toString()), entry.value(QStringLiteral("sha1")).toString(),
        [this, request](const QByteArray &bytes) {
        if (const QJsonObject index = QJsonDocument::fromJson(bytes).object().value(QStringLiteral("assetIndex")).toObject(); index.isEmpty())
            fail(request);
        else
            assetIndex(request, index);
    });
}

void OfficialLanguageCache::assetIndex(const Request &request, const QJsonObject &index)
{
    get(QUrl(index.value(QStringLiteral("url")).toString()), index.value(QStringLiteral("sha1")).toString(),
        [this, request](const QByteArray &bytes) {
        if (const QJsonObject objects = QJsonDocument::fromJson(bytes).object().value(QStringLiteral("objects")).toObject();
            !objects.isEmpty())
            download(request, objects);
        else
            fail(request);
    });
}

void OfficialLanguageCache::download(const Request &request, const QJsonObject &index)
{
    const QString base = QStringLiteral("minecraft/lang/") + request.locale;
    for (const QString legacyName = base.first(base.size() - 2) + base.last(2).toUpper();
        const QString &key : {base + QStringLiteral(".json"), base + QStringLiteral(".lang"),
                             legacyName + QStringLiteral(".lang")}) {
        const QString hash = index.value(key).toObject().value(QStringLiteral("hash")).toString();
        static const QRegularExpression pattern(QStringLiteral("^[a-f0-9]{40}$"));
        if (!pattern.match(hash).hasMatch())
            continue;
        get(QUrl(QStringLiteral("https://resources.download.minecraft.net/") + hash.first(2) + u'/' + hash), hash,
            [this, request, hash, legacy = key.endsWith(QLatin1String(".lang"))](const QByteArray &bytes) {
            complete(request, bytes, hash, legacy);
        });
        return;
    }
    fail(request);
}

void OfficialLanguageCache::complete(const Request &request, const QByteArray &bytes, const QString &hash, bool legacy)
{
    const QJsonObject strings = parse(bytes, legacy);
    if (strings.isEmpty()) {
        fail(request);
        return;
    }
    m_strings.insert(request.key(), strings);
    m_busy.remove(request.key());
    const QString path = cachePath(request);
    const QJsonObject cached{{QStringLiteral("version"), request.version}, {QStringLiteral("locale"), request.locale},
                             {QStringLiteral("sha1"), hash},
                             {QStringLiteral("legacy"), legacy},
                             {QStringLiteral("payload"), QString::fromLatin1(bytes.toBase64())}};
    const QByteArray encoded = QJsonDocument(cached).toJson(QJsonDocument::Compact);
    if (QSaveFile file(path); !QDir().mkpath(QFileInfo(path).absolutePath()) || !file.open(QIODevice::WriteOnly)
        || file.write(encoded) != encoded.size() || !file.commit())
        m_errors.insert(request.key(), QStringLiteral("catalog.error.cache"));
    emit changed();
}
