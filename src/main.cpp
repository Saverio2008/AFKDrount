#include "LanguageManager.h"
#include "InstanceStore.h"
#include "AccountStore.h"
#include "AppStorage.h"
#include "SessionStore.h"
#include "GameCatalog.h"
#include "DesktopLifecycle.h"

#include <QWindow>

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QQuickStyle>
#include <QSettings>

Q_IMPORT_QML_PLUGIN(AppAppearancePlugin)
Q_IMPORT_QML_PLUGIN(RoundedPlugin)

int main(int argc, char *argv[])
{
#ifdef AFK_DEBUG_DATA_DIR
    qputenv("QML_DISK_CACHE_PATH", AppStorage::writableLocation(QStandardPaths::CacheLocation).toUtf8() + "/qml");
    qputenv("QSG_RHI_DISABLE_DISK_CACHE", "1");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       AppStorage::writableLocation(QStandardPaths::AppConfigLocation));
#endif
    QApplication app(argc, argv);
    // Keep the storage identifiers so existing local settings and data are still found.
    QGuiApplication::setOrganizationName(QStringLiteral("MineflayerAFKHelper"));
#ifdef AFK_RELEASE_PROFILE
    QGuiApplication::setApplicationName(QStringLiteral("MineflayerAFKHelperRelease"));
#else
    QGuiApplication::setApplicationName(QStringLiteral("MineflayerAFKHelper"));
#endif
    QGuiApplication::setApplicationDisplayName(QStringLiteral("AFK Drount"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/app/AFKDrount.png")));
#ifdef Q_OS_LINUX
    QGuiApplication::setDesktopFileName(QStringLiteral("AFKDrount"));
#endif

    QQuickStyle::setStyle(QStringLiteral("Rounded"));
    QQuickStyle::setFallbackStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;
    auto *languageManager = engine.singletonInstance<LanguageManager *>("AFKDrount", "LanguageManager");
    auto *instanceStore = engine.singletonInstance<InstanceStore *>("AFKDrount", "InstanceStore");
    QObject::connect(languageManager, &LanguageManager::languageChanged, instanceStore, &InstanceStore::errorChanged);
    auto *accountStore = engine.singletonInstance<AccountStore *>("AFKDrount", "AccountStore");
    auto *sessionStore = engine.singletonInstance<SessionStore *>("AFKDrount", "SessionStore");
    auto *gameCatalog = engine.singletonInstance<GameCatalog *>("AFKDrount", "GameCatalog");
    sessionStore->configureStores(accountStore, instanceStore, gameCatalog);
    sessionStore->setLocale(languageManager->minecraftLocale());
    QObject::connect(languageManager, &LanguageManager::languageChanged, sessionStore, [languageManager, sessionStore] {
        sessionStore->setLocale(languageManager->minecraftLocale());
    });
    QObject::connect(&app, &QCoreApplication::aboutToQuit, sessionStore, &SessionStore::shutdown);
    QObject::connect(&app, &QGuiApplication::applicationStateChanged, &engine, [&engine](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive && !engine.rootObjects().isEmpty()) {
            if (auto *window = qobject_cast<QWindow *>(engine.rootObjects().first()); window && !window->isVisible()) {
                window->show();
                window->raise();
                window->requestActivate();
            }
        }
    });
    engine.loadFromModule("AFKDrount", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;
    DesktopLifecycle lifecycle;
    lifecycle.attach(qobject_cast<QWindow *>(engine.rootObjects().first()));
    QObject::connect(languageManager, &LanguageManager::languageChanged, &lifecycle, &DesktopLifecycle::retranslate);

    return QApplication::exec();
}
