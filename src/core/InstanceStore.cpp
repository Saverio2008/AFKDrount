#include "InstanceStore.h"
#include "AppStorage.h"
#include "InstanceConfiguration.h"
#include "GameCatalog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QUuid>
#include <algorithm>

InstanceStore::InstanceStore(QObject *parent) : QObject(parent),
    m_filePath(AppStorage::writableLocation(QStandardPaths::AppConfigLocation)
               + QStringLiteral("/instances.json"))
{
    load();
}

QString InstanceStore::error() const
{
    if (!m_errorSource)
        return {};
    const QString message = qtTrId(m_errorSource);
    return m_errorDetail.isEmpty() ? message : message.arg(m_errorDetail);
}

void InstanceStore::setError(const char *source, const QString &detail)
{
    if (m_errorSource == source && m_errorDetail == detail)
        return;
    m_errorSource = source;
    m_errorDetail = detail;
    emit errorChanged();
}

bool InstanceStore::validate(const QString &name, const QString &server, int port)
{
    if (name.trimmed().isEmpty()) {
        setError(QT_TRID_NOOP("instance.error.nameRequired"));
        return false;
    }
    if (const QString host = server.trimmed();
        host.isEmpty() || host.contains(u'/') || host.contains(u'\\')
        || std::ranges::any_of(host, [](QChar c) { return c.isSpace(); })) {
        setError(QT_TRID_NOOP("instance.error.invalidServer"));
        return false;
    }
    if (port < 1 || port > 65535) {
        setError(QT_TRID_NOOP("instance.error.invalidPort"));
        return false;
    }
    return true;
}

void InstanceStore::load()
{
    QFile file(m_filePath);
    if (!file.exists())
        return;
    if (!file.open(QIODevice::ReadOnly)) {
        m_storageReady = false;
        setError(QT_TRID_NOOP("instance.error.read"), file.errorString());
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    const QJsonObject root = document.object();
    if (file.error() != QFileDevice::NoError || parseError.error != QJsonParseError::NoError
        || !document.isObject() || (root.value(QStringLiteral("version")).toInt() != 1
            && root.value(QStringLiteral("version")).toInt() != 2)
        || !root.value(QStringLiteral("instances")).isArray()) {
        m_storageReady = false;
        setError(QT_TRID_NOOP("instance.error.parse"));
        return;
    }
    QVariantList loaded;
    QSet<QString> ids;
    for (const QJsonArray entries = root.value(QStringLiteral("instances")).toArray();
         const auto &entry : entries) {
        const QJsonObject object = entry.toObject();
        const QString id = object.value(QStringLiteral("id")).toString();
        const QString name = object.value(QStringLiteral("name")).toString();
        const QString server = object.value(QStringLiteral("server")).toString();
        const int port = object.value(QStringLiteral("port")).toInt(-1);
        if (QUuid(id).isNull() || ids.contains(id) || !validate(name, server, port)) {
            m_storageReady = false;
            setError(QT_TRID_NOOP("instance.error.invalidRecords"));
            return;
        }
        QVariantMap configuration = InstanceConfiguration::defaults();
        if (root.value(QStringLiteral("version")).toInt() == 2
            && (!object.value(QStringLiteral("configuration")).isObject()
                || !InstanceConfiguration::normalize(object.value(QStringLiteral("configuration")).toObject().toVariantMap(), configuration, true).isEmpty())) {
            m_storageReady = false;
            setError(QT_TRID_NOOP("instance.error.invalidRecords"));
            return;
        }
        ids.insert(id);
        loaded.append(QVariantMap{{QStringLiteral("id"), id},
                                  {QStringLiteral("name"), name.trimmed()},
                                  {QStringLiteral("server"), server.trimmed()},
                                  {QStringLiteral("port"), port},
                                  {QStringLiteral("configuration"), configuration}});
    }
    m_instances = loaded;
}

bool InstanceStore::persist(const QVariantList &instances)
{
    if (!m_storageReady)
        return false;
    if (!QDir().mkpath(QFileInfo(m_filePath).absolutePath())) {
        setError(QT_TRID_NOOP("instance.error.createDirectory"));
        return false;
    }
    const QJsonObject root{{QStringLiteral("version"), 2},
                           {QStringLiteral("instances"), QJsonArray::fromVariantList(instances)}};
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (QSaveFile file(m_filePath);
        !file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        setError(QT_TRID_NOOP("instance.error.save"), file.errorString());
        return false;
    }
    m_instances = instances;
    setError(nullptr);
    emit instancesChanged();
    return true;
}

QString InstanceStore::createInstance(const QString &name, const QString &server, int port, const QString &version)
{
    if (!m_storageReady || !validate(name, server, port))
        return {};
    if (!GameCatalog::supports(version)) {
        setError(QT_TRID_NOOP("config.error.version"));
        return {};
    }
    QVariantMap configuration = InstanceConfiguration::defaults();
    configuration.insert(QStringLiteral("gameVersion"), version);
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QVariantList updated = m_instances;
    updated.append(QVariantMap{{QStringLiteral("id"), id},
                               {QStringLiteral("name"), name.trimmed()},
                               {QStringLiteral("server"), server.trimmed()},
                               {QStringLiteral("port"), port},
                               {QStringLiteral("configuration"), configuration}});
    return persist(updated) ? id : QString();
}

bool InstanceStore::updateInstance(const QString &id, const QString &name,
                                  const QString &server, int port, const QVariantMap &configuration)
{
    if (!m_storageReady || !validate(name, server, port))
        return false;
    QVariantMap normalized;
    if (const QString configurationErrorId = InstanceConfiguration::normalize(configuration, normalized);
        !configurationErrorId.isEmpty()) {
        setError(QT_TRID_NOOP("instance.error.configuration"), qtTrId(configurationErrorId.toUtf8().constData()));
        return false;
    }
    for (QVariantList updated = m_instances; QVariant &entry : updated) {
        QVariantMap item = entry.toMap();
        if (item.value(QStringLiteral("id")).toString() != id)
            continue;
        item.insert(QStringLiteral("name"), name.trimmed());
        item.insert(QStringLiteral("server"), server.trimmed());
        item.insert(QStringLiteral("port"), port);
        item.insert(QStringLiteral("configuration"), normalized);
        entry = item;
        return persist(updated);
    }
    setError(QT_TRID_NOOP("instance.error.notFound"));
    return false;
}

bool InstanceStore::removeInstance(const QString &id)
{
    if (!m_storageReady)
        return false;
    QVariantList updated = m_instances;
    for (qsizetype index = 0; index < updated.size(); ++index) {
        if (updated[index].toMap().value(QStringLiteral("id")).toString() == id) {
            updated.removeAt(index);
            return persist(updated);
        }
    }
    setError(QT_TRID_NOOP("instance.error.notFound"));
    return false;
}

bool InstanceStore::moveInstance(const QString &id, const QString &targetId, bool after)
{
    if (id == targetId)
        return false;

    qsizetype source = -1;
    qsizetype target = -1;
    for (qsizetype index = 0; index < m_instances.size(); ++index) {
        if (const auto entryId = m_instances[index].toMap().value(QStringLiteral("id")).toString(); entryId == id)
            source = index;
        else if (entryId == targetId)
            target = index;
    }
    if (source < 0 || target < 0) {
        setError(QT_TRID_NOOP("instance.error.notFound"));
        return false;
    }

    if (const qsizetype destination = target + (after ? 1 : 0) - (source < target ? 1 : 0); destination != source) {
        QVariantList updated = m_instances;
        updated.move(source, destination);
        return persist(updated);
    }
    return false;
}

QString InstanceStore::duplicateInstance(const QString &id)
{
    if (!m_storageReady)
        return {};
    for (const QVariant &entry : m_instances) {
        const QVariantMap source = entry.toMap();
        if (source.value(QStringLiteral("id")).toString() != id)
            continue;
        const QString originalName = source.value(QStringLiteral("name")).toString();
        const QString suffix = qtTrId("instance.copy.suffix");
        const auto makeName = [&originalName, &suffix](const QString &numberSuffix) {
            return originalName.left(80 - suffix.size() - numberSuffix.size()) + suffix + numberSuffix;
        };
        QString name = makeName({});
        int number = 2;
        while (std::ranges::any_of(m_instances, [&name](const QVariant &candidate) {
            return candidate.toMap().value(QStringLiteral("name")).toString() == name;
        }))
            name = makeName(QStringLiteral(" %1").arg(number++));
        QVariantMap copy = source;
        const QString copyId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        copy.insert(QStringLiteral("id"), copyId);
        copy.insert(QStringLiteral("name"), name);
        QVariantList updated = m_instances;
        updated.append(copy);
        return persist(updated) ? copyId : QString();
    }
    setError(QT_TRID_NOOP("instance.error.notFound"));
    return {};
}

QVariantMap InstanceStore::defaultConfiguration()
{
    return InstanceConfiguration::defaults();
}

QVariantMap InstanceStore::newCountingRule()
{
    return InstanceConfiguration::newRule();
}

QVariantMap InstanceStore::newTask()
{
    return InstanceConfiguration::newTask();
}

QString InstanceStore::taskError(const QVariantMap &task, const QString &version)
{
    QVariantMap normalized;
    return InstanceConfiguration::normalizeTask(task, normalized, version);
}

QString InstanceStore::configurationError(const QVariantMap &configuration)
{
    QVariantMap normalized;
    return InstanceConfiguration::normalize(configuration, normalized);
}

QString InstanceStore::countingRuleError(const QVariantMap &rule, const QString &version)
{
    QVariantMap normalized;
    if (QString error = InstanceConfiguration::normalizeRule(rule, normalized); !error.isEmpty())
        return error;
    if (!GameCatalog::supports(version))
        return QStringLiteral("config.error.version");
    for (const QVariantList items = normalized.value(QStringLiteral("items")).toList(); const QVariant &item : items)
        if (!GameCatalog::containsItem(version, item.toString()))
            return QStringLiteral("config.error.itemsVersion");
    return {};
}