#include "LanguageManager.h"

#include <QFile>
#include <QGuiApplication>
#include <QJsonObject>
#include <QLocale>
#include <QQmlEngine>
#include <algorithm>

namespace {
struct Language {
    QLatin1StringView code;
    const char *name;
    QLocale::Language language;
    QLocale::Script script = QLocale::AnyScript;
};

constexpr Language languages[] = {
    {.code = QLatin1StringView("en"), .name = "English", .language = QLocale::English},
    {.code = QLatin1StringView("zh_CN"), .name = "简体中文", .language = QLocale::Chinese, .script = QLocale::SimplifiedHanScript},
    {.code = QLatin1StringView("zh_TW"), .name = "繁體中文", .language = QLocale::Chinese, .script = QLocale::TraditionalHanScript},
    {.code = QLatin1StringView("es_ES"), .name = "Español", .language = QLocale::Spanish},
};

class JsonTranslator final : public QTranslator
{
public:
    bool loadLanguage(const QString &language)
    {
        if (!readCatalog(QStringLiteral("en")))
            return false;
        return language == QStringLiteral("en") || readCatalog(language);
    }

    [[nodiscard]] bool isEmpty() const override { return m_messages.isEmpty(); }

    [[nodiscard]] QString translate(const char *context, const char *sourceText,
                                   const char *disambiguation, int n) const override
    {
        Q_UNUSED(disambiguation)
        Q_UNUSED(n)
        if (context && *context)
            return {};
        return m_messages.value(QString::fromUtf8(sourceText)).toString();
    }

private:
    bool readCatalog(const QString &language)
    {
        QFile file(QStringLiteral(":/lang/%1.json").arg(language));
        if (!file.open(QIODevice::ReadOnly))
            return false;
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject())
            return false;
        const QJsonObject messages = document.object();
        if (messages.isEmpty())
            return false;
        for (auto it = messages.begin(); it != messages.end(); ++it) {
            if (!it.value().isString() || it.value().toString().isEmpty())
                return false;
            m_messages.insert(it.key(), it.value());
        }
        return true;
    }

    QJsonObject m_messages;
};

QString resolvedLanguage(const QString &language)
{
    if (language != QStringLiteral("auto"))
        return language;
    for (const QString &tag : QLocale::system().uiLanguages()) {
        const QLocale locale(tag);
        for (const auto &candidate : languages) {
            if (locale.language() == candidate.language
                && (candidate.script == QLocale::AnyScript || locale.script() == candidate.script))
                return {candidate.code};
        }
    }
    return QStringLiteral("en");
}

bool supportedLanguage(const QString &language)
{
    return language == QLatin1String("auto") || std::ranges::any_of(languages, [&language](const Language &candidate) {
        return language == candidate.code;
    });
}
}

LanguageManager *LanguageManager::create(QQmlEngine *engine, QJSEngine *)
{
    return new LanguageManager(engine);
}

LanguageManager::LanguageManager(QQmlEngine *engine) : QObject(engine), m_engine(engine)
{
    const QString stored = m_settings.value(QStringLiteral("general/language"), QStringLiteral("auto")).toString();
    m_language = supportedLanguage(stored) ? stored : QStringLiteral("auto");
    applyLanguage(m_language);
    QGuiApplication::instance()->installEventFilter(this);
}

LanguageManager::~LanguageManager()
{
    if (m_translator)
        QGuiApplication::removeTranslator(m_translator.get());
}

QString LanguageManager::minecraftLocale() const
{
    return m_effectiveLanguage.isEmpty() || m_effectiveLanguage == QLatin1String("en")
        ? QStringLiteral("en_us") : m_effectiveLanguage.toLower();
}

QVariantList LanguageManager::availableLanguages()
{
    QVariantList result;
    for (const auto &language : languages)
        result.append(QVariantMap{{QStringLiteral("code"), QString(language.code)},
                                  {QStringLiteral("name"), QString::fromUtf8(language.name)}});
    return result;
}

bool LanguageManager::applyLanguage(const QString &language)
{
    const QString effective = resolvedLanguage(language);
    auto translator = std::make_unique<JsonTranslator>();
    if (!translator->loadLanguage(effective)) {
        qWarning("Could not load JSON language catalog: %s", qPrintable(effective));
        return false;
    }
    if (m_translator)
        QGuiApplication::removeTranslator(m_translator.get());
    m_translator = std::move(translator);
    QGuiApplication::installTranslator(m_translator.get());
    m_effectiveLanguage = effective;
    QLocale::setDefault(QLocale(effective));
    QGuiApplication::setLayoutDirection(QLocale(effective).textDirection());
    m_engine->retranslate();
    return true;
}

void LanguageManager::setLanguage(const QString &language)
{
    if (!supportedLanguage(language) || m_language == language || !applyLanguage(language))
        return;
    m_language = language;
    m_settings.setValue(QStringLiteral("general/language"), language);
    emit languageChanged();
}

bool LanguageManager::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == QGuiApplication::instance() && event->type() == QEvent::LocaleChange
        && m_language == QStringLiteral("auto") && resolvedLanguage(m_language) != m_effectiveLanguage) {
        if (applyLanguage(m_language))
            emit languageChanged();
    }
    return QObject::eventFilter(watched, event);
}
