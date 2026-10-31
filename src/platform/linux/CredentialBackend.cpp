#include "CredentialBackend.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS) && !defined(Q_OS_ANDROID)
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QEventLoop>
#include <QMap>
#include <QTimer>
#include <QVariantMap>

struct NativeSecret {
    QDBusObjectPath session;
    QByteArray parameters;
    QByteArray value;
    QString contentType;
};
Q_DECLARE_METATYPE(NativeSecret)

QDBusArgument &operator<<(QDBusArgument &argument, const NativeSecret &secret)
{
    argument.beginStructure();
    argument << secret.session << secret.parameters << secret.value << secret.contentType;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, NativeSecret &secret)
{
    argument.beginStructure();
    argument >> secret.session >> secret.parameters >> secret.value >> secret.contentType;
    argument.endStructure();
    return argument;
}

class SecretPrompt final : public QObject
{
    Q_OBJECT

public:
    bool done = false;
    bool accepted = false;

public slots:
    void completed(bool dismissed, const QDBusVariant &result)
    {
        Q_UNUSED(result)
        done = true;
        accepted = !dismissed;
        emit finished();
    }

signals:
    void finished();
};

namespace {
const auto serviceName = QStringLiteral("org.freedesktop.secrets");
const auto rootPath = QStringLiteral("/org/freedesktop/secrets");
const auto serviceInterface = QStringLiteral("org.freedesktop.Secret.Service");
const QMap<QString, QString> attributes = {
    { QStringLiteral("service"), QStringLiteral("Mineflayer AFK Helper") },
    { QStringLiteral("account"), QStringLiteral(AFK_CREDENTIAL_ACCOUNT) }
};

bool validReply(const QDBusMessage &reply, qsizetype minimumArguments)
{
    return reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() >= minimumArguments;
}

bool completePrompt(QDBusConnection &bus, const QDBusObjectPath &path)
{
    if (path.path().isEmpty())
        return false;
    if (path.path() == QStringLiteral("/"))
        return true;
    const auto interfaceName = QStringLiteral("org.freedesktop.Secret.Prompt");
    SecretPrompt receiver;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&receiver, &SecretPrompt::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    if (!bus.connect(serviceName, path.path(), interfaceName, QStringLiteral("Completed"),
                     &receiver, SLOT(completed(bool,QDBusVariant))))
        return false;
    QDBusInterface prompt(serviceName, path.path(), interfaceName, bus);
    const QDBusMessage started = prompt.call(QStringLiteral("Prompt"), QString());
    if (validReply(started, 0)) {
        timeout.start(120000);
        if (!receiver.done)
            loop.exec();
    }
    bus.disconnect(serviceName, path.path(), interfaceName, QStringLiteral("Completed"),
                   &receiver, SLOT(completed(bool,QDBusVariant)));
    if (!receiver.done)
        prompt.call(QDBus::NoBlock, QStringLiteral("Dismiss"));
    return receiver.done && receiver.accepted;
}

class SecretSession final
{
public:
    SecretSession() : bus(QDBusConnection::sessionBus()),
        service(serviceName, rootPath, serviceInterface, bus)
    {
        qDBusRegisterMetaType<NativeSecret>();
        qDBusRegisterMetaType<QMap<QString, QString>>();
        service.setTimeout(15000);
        const auto reply = service.call(QStringLiteral("OpenSession"), QStringLiteral("plain"),
                                        QVariant::fromValue(QDBusVariant(QString())));
        if (validReply(reply, 2))
            path = qdbus_cast<QDBusObjectPath>(reply.arguments()[1]);
    }

    ~SecretSession()
    {
        if (valid()) {
            QDBusInterface session(serviceName, path.path(), QStringLiteral("org.freedesktop.Secret.Session"), bus);
            session.call(QDBus::NoBlock, QStringLiteral("Close"));
        }
    }

    [[nodiscard]] bool valid() const { return !path.path().isEmpty() && path.path() != QStringLiteral("/"); }

    bool unlock(const QDBusObjectPath &object)
    {
        const auto reply = service.call(QStringLiteral("Unlock"),
                                        QVariant::fromValue(QList<QDBusObjectPath>{ object }));
        return validReply(reply, 2) && completePrompt(bus, qdbus_cast<QDBusObjectPath>(reply.arguments()[1]));
    }

    QDBusConnection bus;
    QDBusInterface service;
    QDBusObjectPath path;
};
}

CredentialBackend::Result CredentialBackend::readKey()
{
    SecretSession session;
    if (!session.valid())
        return {};
    const auto reply = session.service.call(QStringLiteral("SearchItems"), QVariant::fromValue(attributes));
    if (!validReply(reply, 2))
        return {};
    const auto unlocked = qdbus_cast<QList<QDBusObjectPath>>(reply.arguments()[0]);
    const auto locked = qdbus_cast<QList<QDBusObjectPath>>(reply.arguments()[1]);
    if (unlocked.isEmpty() && locked.isEmpty())
        return { .status = Status::Missing, .key = {} };
    const auto itemPath = !unlocked.isEmpty() ? unlocked.first() : locked.first();
    if (unlocked.isEmpty() && !session.unlock(itemPath))
        return {};
    QDBusInterface item(serviceName, itemPath.path(), QStringLiteral("org.freedesktop.Secret.Item"), session.bus);
    const auto secretReply = item.call(QStringLiteral("GetSecret"), QVariant::fromValue(session.path));
    if (!validReply(secretReply, 1))
        return {};
    const auto secret = qdbus_cast<NativeSecret>(secretReply.arguments()[0]);
    return secret.value.size() == 32 ? Result{ .status = Status::Found, .key = secret.value } : Result{};
}

bool CredentialBackend::writeKey(const QByteArray &key)
{
    SecretSession session;
    if (!session.valid())
        return false;
    const auto alias = session.service.call(QStringLiteral("ReadAlias"), QStringLiteral("default"));
    if (!validReply(alias, 1))
        return false;
    const auto collectionPath = qdbus_cast<QDBusObjectPath>(alias.arguments()[0]);
    if (collectionPath.path() == QStringLiteral("/") || !session.unlock(collectionPath))
        return false;
    QDBusInterface collection(serviceName, collectionPath.path(),
                              QStringLiteral("org.freedesktop.Secret.Collection"), session.bus);
    const QVariantMap properties = {
        { QStringLiteral("org.freedesktop.Secret.Item.Label"), QStringLiteral("AFK Drount") },
        { QStringLiteral("org.freedesktop.Secret.Item.Attributes"), QVariant::fromValue(attributes) }
    };
    const NativeSecret secret{ session.path, {}, key, QStringLiteral("application/octet-stream") };
    const auto reply = collection.call(QStringLiteral("CreateItem"), properties, QVariant::fromValue(secret), false);
    if (!validReply(reply, 2))
        return false;
    return completePrompt(session.bus, qdbus_cast<QDBusObjectPath>(reply.arguments()[1]));
}

#include "CredentialBackend.moc"
#endif
