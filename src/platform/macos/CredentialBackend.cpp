#include "CredentialBackend.h"

#ifdef Q_OS_MACOS
#include <Security/Security.h>

CredentialBackend::Result CredentialBackend::readKey()
{
    const void *keys[] = { kSecClass, kSecAttrService, kSecAttrAccount, kSecReturnData };
    const void *values[] = { kSecClassGenericPassword, CFSTR("Mineflayer AFK Helper"),
                            CFSTR(AFK_CREDENTIAL_ACCOUNT), kCFBooleanTrue };
    const auto query = CFDictionaryCreate(nullptr, keys, values, 4,
                                         &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFTypeRef result = nullptr;
    const OSStatus status = SecItemCopyMatching(query, &result);
    CFRelease(query);
    if (status == errSecItemNotFound)
        return { .status = Status::Missing, .key = {} };
    if (status != errSecSuccess)
        return {};
    QByteArray key;
    if (CFGetTypeID(result) == CFDataGetTypeID()) {
        const auto data = static_cast<CFDataRef>(result);
        key = QByteArray(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
    }
    CFRelease(result);
    return key.size() == 32 ? Result{ .status = Status::Found, .key = key } : Result{};
}

bool CredentialBackend::writeKey(const QByteArray &key)
{
    const auto data = CFDataCreate(nullptr, reinterpret_cast<const UInt8 *>(key.constData()), key.size());
    const void *keys[] = { kSecClass, kSecAttrService, kSecAttrAccount, kSecValueData };
    const void *values[] = { kSecClassGenericPassword, CFSTR("Mineflayer AFK Helper"),
                            CFSTR(AFK_CREDENTIAL_ACCOUNT), data };
    const auto attributes = CFDictionaryCreate(nullptr, keys, values, 4,
                                              &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    const OSStatus status = SecItemAdd(attributes, nullptr);
    CFRelease(attributes);
    CFRelease(data);
    return status == errSecSuccess || status == errSecDuplicateItem;
}
#endif
