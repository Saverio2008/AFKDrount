#pragma once

#include "OfficialLanguageCache.h"

#include <QJsonObject>
#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

class GameCatalog : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList versions READ versions CONSTANT)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
public:
    explicit GameCatalog(QObject *parent = nullptr, const QString &languageDirectory = {});
    [[nodiscard]] QVariantList versions() const { return m_versions; }
    [[nodiscard]] int revision() const { return m_revision; }
    Q_INVOKABLE static bool supports(const QString &version);
    Q_INVOKABLE static bool containsItem(const QString &version, const QString &name);
    Q_INVOKABLE static bool legacy(const QString &version);
    Q_INVOKABLE static bool supportsCommands(const QString &version);
    static int chatLengthLimit(const QString &version);
    Q_INVOKABLE void ensureLanguage(const QString &version, const QString &locale);
    [[nodiscard]] QJsonObject translations(const QString &version, const QString &locale) const { return m_languages.translations(version, locale); }
    Q_INVOKABLE [[nodiscard]] bool languageBusy(const QString &version, const QString &locale) const;
    Q_INVOKABLE [[nodiscard]] QString languageError(const QString &version, const QString &locale) const;
    Q_INVOKABLE [[nodiscard]] QVariantList searchItems(const QString &version, const QString &query, const QString &locale) const;
    Q_INVOKABLE [[nodiscard]] QString itemLabel(const QString &version, const QString &name, const QString &locale) const;
    Q_INVOKABLE static QVariantMap filterRules(const QString &version, const QVariantList &rules);

signals:
    void changed();

private:
    static const QJsonObject &data();
    static QJsonObject versionInfo(const QString &version);
    static QJsonArray items(const QString &version);
    static QString translatedName(const QJsonObject &dictionary, const QJsonObject &item);
    QVariantList m_versions;
    OfficialLanguageCache m_languages;
    int m_revision = 0;
};
