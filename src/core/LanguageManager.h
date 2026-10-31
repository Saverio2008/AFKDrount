#pragma once

#include <QObject>
#include <QSettings>
#include <QTranslator>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>
#include <memory>

class QQmlEngine;
class QJSEngine;

class LanguageManager final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString effectiveLanguage READ effectiveLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString minecraftLocale READ minecraftLocale NOTIFY languageChanged)
    Q_PROPERTY(QVariantList availableLanguages READ availableLanguages CONSTANT)

public:
    static LanguageManager *create(QQmlEngine *engine, QJSEngine *scriptEngine);
    ~LanguageManager() override;

    [[nodiscard]] QString language() const { return m_language; }
    [[nodiscard]] QString effectiveLanguage() const { return m_effectiveLanguage; }
    [[nodiscard]] QString minecraftLocale() const;
    [[nodiscard]] static QVariantList availableLanguages();
    void setLanguage(const QString &language);
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void languageChanged();

private:
    explicit LanguageManager(QQmlEngine *engine);
    bool applyLanguage(const QString &language);

    QQmlEngine *m_engine;
    QSettings m_settings;
    std::unique_ptr<QTranslator> m_translator;
    QString m_language;
    QString m_effectiveLanguage;
};
