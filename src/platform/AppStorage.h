#pragma once

#include <QStandardPaths>

namespace AppStorage {
inline QString writableLocation(QStandardPaths::StandardLocation location)
{
#ifdef AFK_DEBUG_DATA_DIR
    switch (location) {
    case QStandardPaths::AppConfigLocation:
        return QStringLiteral(AFK_DEBUG_DATA_DIR "/config");
    case QStandardPaths::AppLocalDataLocation:
        return QStringLiteral(AFK_DEBUG_DATA_DIR "/data");
    case QStandardPaths::CacheLocation:
        return QStringLiteral(AFK_DEBUG_DATA_DIR "/cache");
    default:
        break;
    }
#endif
    return QStandardPaths::writableLocation(location);
}
}
