#include "GameCatalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>

GameCatalog::GameCatalog(QObject *parent, const QString &languageDirectory)
    : QObject(parent), m_versions(data().value(QStringLiteral("versions")).toArray().toVariantList()),
      m_languages(nullptr, languageDirectory)
{
    connect(&m_languages, &OfficialLanguageCache::changed, this, [this] {
        ++m_revision;
        emit changed();
    });
}

const QJsonObject &GameCatalog::data()
{
    static const QJsonObject catalog = [] {
        QFile file(QStringLiteral(":/game-data/catalog.json"));
        if (!file.open(QIODevice::ReadOnly))
            return QJsonObject{};
        return QJsonDocument::fromJson(file.readAll()).object();
    }();
    return catalog;
}

QJsonObject GameCatalog::versionInfo(const QString &version)
{
    for (const QJsonArray entries = data().value(QStringLiteral("versions")).toArray(); const auto &entry : entries)
        if (entry.toObject().value(QStringLiteral("version")).toString() == version)
            return entry.toObject();
    return {};
}

bool GameCatalog::supports(const QString &version)
{
    return !versionInfo(version).isEmpty();
}

bool GameCatalog::legacy(const QString &version)
{
    return versionInfo(version).value(QStringLiteral("legacy")).toBool();
}

bool GameCatalog::supportsCommands(const QString &version)
{
    return versionInfo(version).value(QStringLiteral("commands")).toBool();
}

int GameCatalog::chatLengthLimit(const QString &version)
{
    return versionInfo(version).value(QStringLiteral("chatLength")).toInt(256);
}

QJsonArray GameCatalog::items(const QString &version)
{
    return data().value(QStringLiteral("sets")).toObject()
        .value(versionInfo(version).value(QStringLiteral("catalog")).toString()).toArray();
}

bool GameCatalog::containsItem(const QString &version, const QString &name)
{
    return std::ranges::any_of(items(version), [&name](const auto &entry) {
        return entry.toObject().value(QStringLiteral("name")).toString() == name;
    });
}

void GameCatalog::ensureLanguage(const QString &version, const QString &locale)
{
    if (supports(version))
        m_languages.ensure(version, locale);
}

bool GameCatalog::languageBusy(const QString &version, const QString &locale) const
{
    return m_languages.busy(version, locale);
}

QString GameCatalog::languageError(const QString &version, const QString &locale) const
{
    return m_languages.error(version, locale);
}

QString GameCatalog::translatedName(const QJsonObject &dictionary, const QJsonObject &item)
{
    for (const QJsonArray keys = item.value(QStringLiteral("keys")).toArray(); const auto &key : keys) {
        if (const QString text = dictionary.value(key.toString()).toString(); !text.isEmpty())
            return text;
    }
    return {};
}

QVariantList GameCatalog::searchItems(const QString &version, const QString &query, const QString &locale) const
{
    QVariantList results;
    const QString needle = query.trimmed().toCaseFolded();
    const QJsonObject dictionary = translations(version, locale);
    for (const QJsonArray entries = items(version); const auto &entry : entries) {
        const QJsonObject item = entry.toObject();
        const QString name = item.value(QStringLiteral("name")).toString();
        const QString english = item.value(QStringLiteral("displayName")).toString();
        const QString localized = translatedName(dictionary, item);
        QStringList searchable{name, QStringLiteral("minecraft:") + name, english, localized};
        for (const QJsonArray aliases = item.value(QStringLiteral("aliases")).toArray(); const auto &alias : aliases)
            searchable.append(dictionary.value(alias.toString()).toString());
        if (!needle.isEmpty() && !searchable.join(u'\n').toCaseFolded().contains(needle))
            continue;
        results.append(QVariantMap{{QStringLiteral("name"), name},
                                   {QStringLiteral("displayName"), localized.isEmpty() ? english : localized}});
    }
    return results;
}

QString GameCatalog::itemLabel(const QString &version, const QString &name, const QString &locale) const
{
    for (const QJsonArray entries = items(version); const auto &entry : entries) {
        const QJsonObject item = entry.toObject();
        if (item.value(QStringLiteral("name")).toString() != name)
            continue;
        const QString localized = translatedName(translations(version, locale), item);
        return localized.isEmpty() ? item.value(QStringLiteral("displayName")).toString() : localized;
    }
    return name;
}

QVariantMap GameCatalog::filterRules(const QString &version, const QVariantList &rules)
{
    QVariantList updated;
    int removed = 0;
    for (const QVariant &entry : rules) {
        QVariantMap rule = entry.toMap();
        QVariantList kept;
        for (const QVariantList names = rule.value(QStringLiteral("items")).toList(); const QVariant &name : names) {
            if (containsItem(version, name.toString()))
                kept.append(name);
            else
                ++removed;
        }
        rule.insert(QStringLiteral("items"), kept);
        updated.append(rule);
    }
    return {{QStringLiteral("rules"), updated}, {QStringLiteral("removedCount"), removed}};
}
