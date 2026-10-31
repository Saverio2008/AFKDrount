#pragma once

#include <QByteArray>

#ifdef AFK_RELEASE_PROFILE
#define AFK_CREDENTIAL_ACCOUNT "release-credential-key-v1"
#else
#define AFK_CREDENTIAL_ACCOUNT "credential-key-v1"
#endif

namespace CredentialBackend {
enum class Status { Found, Missing, Failed };
struct Result {
    Status status = Status::Failed;
    QByteArray key;
};

Result readKey();
bool writeKey(const QByteArray &key);
}
