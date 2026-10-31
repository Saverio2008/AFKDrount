#include "CredentialBackend.h"

#ifdef Q_OS_WIN
#include <QString>
#include <windows.h>
#include <wincred.h>

namespace {
const auto target = QStringLiteral("Mineflayer AFK Helper/" AFK_CREDENTIAL_ACCOUNT);
}

CredentialBackend::Result CredentialBackend::readKey()
{
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(reinterpret_cast<LPCWSTR>(target.utf16()), CRED_TYPE_GENERIC, 0, &credential))
        return GetLastError() == ERROR_NOT_FOUND ? Result{ .status = Status::Missing, .key = {} } : Result{};
    const QByteArray key(reinterpret_cast<const char *>(credential->CredentialBlob), credential->CredentialBlobSize);
    CredFree(credential);
    return key.size() == 32 ? Result{ .status = Status::Found, .key = key } : Result{};
}

bool CredentialBackend::writeKey(const QByteArray &key)
{
    const auto current = readKey();
    if (current.status == Status::Found)
        return true;
    if (current.status != Status::Missing)
        return false;
    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<LPWSTR>(reinterpret_cast<LPCWSTR>(target.utf16()));
    credential.CredentialBlobSize = static_cast<DWORD>(key.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char *>(key.constData()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    return CredWriteW(&credential, 0) != FALSE;
}
#endif
