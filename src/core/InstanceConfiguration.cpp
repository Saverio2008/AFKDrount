#include "InstanceConfiguration.h"
#include "GameCatalog.h"

#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <algorithm>
#include <cmath>

namespace {
bool isNumber(const QVariant &value)
{
    return value.metaType().id() == QMetaType::Double || value.metaType().id() == QMetaType::Int
        || value.metaType().id() == QMetaType::LongLong;
}

bool integerInRange(const QVariant &value, int minimum, int maximum)
{
    if (!isNumber(value))
        return false;
    const double number = value.toDouble();
    return std::isfinite(number) && std::floor(number) == number
        && number >= minimum && number <= maximum;
}

bool normalizeRegion(const QVariant &input, bool tracking, QVariantMap &output)
{
    if (input.metaType().id() != QMetaType::QVariantMap)
        return false;
    const QVariantMap region = input.toMap();
    if (tracking && region.value(QStringLiteral("unlimitedY")).metaType().id() != QMetaType::Bool)
        return false;
    const bool unlimited = tracking && region.value(QStringLiteral("unlimitedY")).toBool();
    for (const QString &corner : {QStringLiteral("from"), QStringLiteral("to")}) {
        const QVariant raw = region.value(corner);
        if (raw.metaType().id() != QMetaType::QVariantList)
            return false;
        QVariantList coordinates = raw.toList();
        if (coordinates.size() != 3)
            return false;
        for (int axis = 0; axis < 3; ++axis) {
            if (axis == 1 && unlimited) {
                coordinates[axis] = QVariant();
                continue;
            }
            if (!isNumber(coordinates[axis]) || !std::isfinite(coordinates[axis].toDouble()))
                return false;
            coordinates[axis] = coordinates[axis].toDouble();
        }
        output.insert(corner, coordinates);
    }
    if (tracking)
        output.insert(QStringLiteral("unlimitedY"), unlimited);
    const QVariantList from = output.value(QStringLiteral("from")).toList();
    const QVariantList to = output.value(QStringLiteral("to")).toList();
    for (int axis = 0; axis < 3; ++axis) {
        if (!(axis == 1 && unlimited) && from[axis].toDouble() == to[axis].toDouble())
            return false;
    }
    return true;
}
}

QVariantMap InstanceConfiguration::defaults()
{
    return {{QStringLiteral("gameVersion"), QString()},
            {QStringLiteral("rules"), QVariantList{}},
            {QStringLiteral("tasks"), QVariantList{}},
            {QStringLiteral("failurePolicies"), QVariantMap{
                {QStringLiteral("disconnect"), QStringLiteral("stop")},
                {QStringLiteral("kicked"), QStringLiteral("stop")},
                {QStringLiteral("connectionError"), QStringLiteral("stop")},
                {QStringLiteral("hurt"), QStringLiteral("stop")},
                {QStringLiteral("unexpectedMovement"), QStringLiteral("stop")}}},
            {QStringLiteral("reconnect"), QVariantMap{
                {QStringLiteral("delaySeconds"), 10}, {QStringLiteral("maxAttempts"), 5},
                {QStringLiteral("unlimited"), false}}}};
}

QVariantMap InstanceConfiguration::newRule()
{
    const QVariantMap region{{QStringLiteral("from"), QVariantList{0.0, 0.0, 0.0}},
                             {QStringLiteral("to"), QVariantList{1.0, 1.0, 1.0}}};
    QVariantMap tracking = region;
    tracking.insert(QStringLiteral("unlimitedY"), false);
    return {{QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
            {QStringLiteral("name"), QString()}, {QStringLiteral("enabled"), true},
            {QStringLiteral("items"), QVariantList{}},
            {QStringLiteral("customItems"), QVariantList{}},
            {QStringLiteral("trackingRegion"), tracking},
            {QStringLiteral("countingRegion"), region}};
}

QString InstanceConfiguration::normalizeRule(const QVariantMap &input, QVariantMap &output)
{
    const QString id = input.value(QStringLiteral("id")).toString();
    const QString name = input.value(QStringLiteral("name")).toString().trimmed();
    if (QUuid(id).isNull() || name.isEmpty() || name.size() > 80
        || input.value(QStringLiteral("enabled")).metaType().id() != QMetaType::Bool)
        return QStringLiteral("config.error.ruleName");
    const QVariant rawItems = input.value(QStringLiteral("items"));
    if (rawItems.metaType().id() != QMetaType::QVariantList)
        return QStringLiteral("config.error.items");
    static const QRegularExpression itemPattern(QStringLiteral("^(?:minecraft:)?[a-z0-9_]+$"));
    QVariantList items;
    QSet<QString> seen;
    for (const QVariantList values = rawItems.toList(); const QVariant &value : values) {
        if (value.metaType().id() != QMetaType::QString)
            return QStringLiteral("config.error.items");
        QString item = value.toString().trimmed();
        if (!itemPattern.match(item).hasMatch())
            return QStringLiteral("config.error.items");
        item.remove(0, item.startsWith(QLatin1String("minecraft:")) ? 10 : 0);
        if (!seen.contains(item)) {
            seen.insert(item);
            items.append(item);
        }
    }
    QVariantList customItems;
    const QVariant rawCustom = input.value(QStringLiteral("customItems"), QVariantList{});
    if (rawCustom.metaType().id() != QMetaType::QVariantList || rawCustom.toList().size() > 128)
        return QStringLiteral("config.error.customItem");
    static const QRegularExpression pathPattern(QStringLiteral("^(?:/(?:[^~/]|~[01])*)+$"));
    for (const QVariantList entries = rawCustom.toList(); const QVariant &entry : entries) {
        const QVariantMap matcher = entry.toMap();
        const QString customId = matcher.value(QStringLiteral("id")).toString();
        const QString source = matcher.value(QStringLiteral("source")).toString();
        const QString path = matcher.value(QStringLiteral("path")).toString();
        if (entry.metaType().id() != QMetaType::QVariantMap
            || matcher.value(QStringLiteral("id")).metaType().id() != QMetaType::QString
            || customId.isEmpty() || customId.size() > 256
            || (source != QLatin1String("nbt") && source != QLatin1String("component"))
            || path.size() > 1024 || !pathPattern.match(path).hasMatch())
            return QStringLiteral("config.error.customItem");
        const QVariant stackSize = matcher.value(QStringLiteral("stackSize"), 0);
        if (!integerInRange(stackSize, 0, 2147483647))
            return QStringLiteral("config.error.customItem");
        QVariantMap normalized{{QStringLiteral("id"), customId}, {QStringLiteral("source"), source},
                               {QStringLiteral("path"), path}};
        if (stackSize.toInt() > 0)
            normalized.insert(QStringLiteral("stackSize"), stackSize);
        if (!customItems.contains(normalized))
            customItems.append(normalized);
    }
    if (items.isEmpty() && customItems.isEmpty())
        return QStringLiteral("config.error.items");
    QVariantMap tracking;
    QVariantMap counting;
    if (!normalizeRegion(input.value(QStringLiteral("trackingRegion")), true, tracking)
        || !normalizeRegion(input.value(QStringLiteral("countingRegion")), false, counting))
        return QStringLiteral("config.error.region");
    const QVariantList trackFrom = tracking.value(QStringLiteral("from")).toList();
    const QVariantList trackTo = tracking.value(QStringLiteral("to")).toList();
    const QVariantList countFrom = counting.value(QStringLiteral("from")).toList();
    const QVariantList countTo = counting.value(QStringLiteral("to")).toList();
    for (int axis = 0; axis < 3; ++axis) {
        if (axis == 1 && tracking.value(QStringLiteral("unlimitedY")).toBool())
            continue;
        if (std::min(countFrom[axis].toDouble(), countTo[axis].toDouble())
                < std::min(trackFrom[axis].toDouble(), trackTo[axis].toDouble())
            || std::max(countFrom[axis].toDouble(), countTo[axis].toDouble())
                > std::max(trackFrom[axis].toDouble(), trackTo[axis].toDouble()))
            return QStringLiteral("config.error.containment");
    }
    output = {{QStringLiteral("id"), id}, {QStringLiteral("name"), name},
              {QStringLiteral("enabled"), input.value(QStringLiteral("enabled"))},
              {QStringLiteral("items"), items}, {QStringLiteral("customItems"), customItems},
              {QStringLiteral("trackingRegion"), tracking},
              {QStringLiteral("countingRegion"), counting}};
    return {};
}

QVariantMap InstanceConfiguration::newTask()
{
    return {{QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
            {QStringLiteral("name"), QString()}, {QStringLiteral("enabled"), true},
            {QStringLiteral("trigger"), QStringLiteral("connection")},
            {QStringLiteral("steps"), QVariantList{QVariantMap{
                {QStringLiteral("mode"), QStringLiteral("chat")}, {QStringLiteral("text"), QString()},
                {QStringLiteral("delaySeconds"), 1.0}}}}};
}

QString InstanceConfiguration::normalizeTask(const QVariantMap &input, QVariantMap &output,
                                             const QString &version, bool loading)
{
    const QString id = input.value(QStringLiteral("id")).toString();
    const QString name = input.value(QStringLiteral("name")).toString().trimmed();
    const QString trigger = input.value(QStringLiteral("trigger")).toString();
    const QVariant enabled = input.value(QStringLiteral("enabled"));
    const QVariant rawSteps = input.value(QStringLiteral("steps"));
    if (QUuid(id).isNull() || name.isEmpty() || name.size() > 80 || enabled.metaType().id() != QMetaType::Bool
        || !QStringList{QStringLiteral("startup"), QStringLiteral("connection"), QStringLiteral("world")}.contains(trigger)
        || rawSteps.metaType().id() != QMetaType::QVariantList || rawSteps.toList().isEmpty() || rawSteps.toList().size() > 64)
        return QStringLiteral("task.error.format");
    QVariantList steps;
    for (const QVariantList entries = rawSteps.toList(); const QVariant &entry : entries) {
        const QVariantMap step = entry.toMap();
        const QString mode = step.value(QStringLiteral("mode")).toString();
        const QString text = step.value(QStringLiteral("text")).toString();
        const QVariant delay = step.value(QStringLiteral("delaySeconds"));
        static const QRegularExpression controls(QStringLiteral("[\\x{0000}-\\x{001f}\\x{007f}-\\x{009f}\\x{2028}\\x{2029}]"));
        if (entry.metaType().id() != QMetaType::QVariantMap || step.value(QStringLiteral("text")).metaType().id() != QMetaType::QString
            || (mode != QLatin1String("chat") && mode != QLatin1String("command"))
            || text.trimmed().isEmpty() || text.size() > 256 || controls.match(text).hasMatch()
            || !isNumber(delay) || !std::isfinite(delay.toDouble()) || delay.toDouble() < 0 || delay.toDouble() > 86400)
            return QStringLiteral("task.error.step");
        if (mode == QLatin1String("chat") && text.trimmed().startsWith(u'/'))
            return QStringLiteral("task.error.chatSlash");
        if (mode == QLatin1String("chat") && !loading && enabled.toBool() && text.size() > GameCatalog::chatLengthLimit(version))
            return QStringLiteral("task.error.chatLength");
        if (mode == QLatin1String("command")) {
            if (const QString command = text.startsWith(u'/') ? text.mid(1) : text;
                command.trimmed().isEmpty() || text.front().isSpace() || command.front().isSpace() || command.startsWith(u'/'))
                return QStringLiteral("task.error.step");
            if (!loading && enabled.toBool() && !GameCatalog::supportsCommands(version))
                return QStringLiteral("task.error.commandVersion");
        }
        steps.append(QVariantMap{{QStringLiteral("mode"), mode}, {QStringLiteral("text"), text},
                                 {QStringLiteral("delaySeconds"), delay.toDouble()}});
    }
    output = {{QStringLiteral("id"), id}, {QStringLiteral("name"), name}, {QStringLiteral("enabled"), enabled},
              {QStringLiteral("trigger"), trigger}, {QStringLiteral("steps"), steps}};
    return {};
}

QString InstanceConfiguration::normalize(const QVariantMap &input, QVariantMap &output, bool loading)
{
    const QString version = input.value(QStringLiteral("gameVersion")).toString().trimmed();
    if ((!loading && !GameCatalog::supports(version)) || version.size() > 64)
        return QStringLiteral("config.error.version");
    if (input.value(QStringLiteral("failurePolicies")).metaType().id() != QMetaType::QVariantMap
        || input.value(QStringLiteral("reconnect")).metaType().id() != QMetaType::QVariantMap
        || input.value(QStringLiteral("rules")).metaType().id() != QMetaType::QVariantList)
        return QStringLiteral("config.error.format");
    const QVariantMap policies = input.value(QStringLiteral("failurePolicies")).toMap();
    QVariantMap normalizedPolicies;
    for (const QString &key : {QStringLiteral("disconnect"), QStringLiteral("kicked"),
                              QStringLiteral("connectionError"), QStringLiteral("hurt"),
                              QStringLiteral("unexpectedMovement")}) {
        const QString policy = policies.value(key).toString();
        if (const bool movement = key == QLatin1String("hurt") || key == QLatin1String("unexpectedMovement");
            policy != QLatin1String("stop")
            && policy != (movement ? QLatin1String("ignore") : QLatin1String("reconnect")))
            return QStringLiteral("config.error.policy");
        normalizedPolicies.insert(key, policy);
    }
    const QVariantMap reconnect = input.value(QStringLiteral("reconnect")).toMap();
    if (!integerInRange(reconnect.value(QStringLiteral("delaySeconds")), 1, 86400)
        || !integerInRange(reconnect.value(QStringLiteral("maxAttempts")), 1, 1000000)
        || reconnect.value(QStringLiteral("unlimited")).metaType().id() != QMetaType::Bool)
        return QStringLiteral("config.error.reconnect");
    QVariantList rules;
    QSet<QString> ids;
    for (const QVariantList entries = input.value(QStringLiteral("rules")).toList(); const QVariant &entry : entries) {
        QVariantMap rule;
        if (entry.metaType().id() != QMetaType::QVariantMap)
            return QStringLiteral("config.error.format");
        if (QString error = normalizeRule(entry.toMap(), rule); !error.isEmpty())
            return error;
        if (!loading) {
            for (const QVariantList items = rule.value(QStringLiteral("items")).toList(); const QVariant &item : items)
                if (!GameCatalog::containsItem(version, item.toString()))
                    return QStringLiteral("config.error.itemsVersion");
        }
        const QString id = rule.value(QStringLiteral("id")).toString();
        if (ids.contains(id))
            return QStringLiteral("config.error.duplicateRule");
        ids.insert(id);
        rules.append(rule);
    }
    const QVariant rawTasks = input.value(QStringLiteral("tasks"), QVariantList{});
    if (rawTasks.metaType().id() != QMetaType::QVariantList || rawTasks.toList().size() > 64)
        return QStringLiteral("task.error.format");
    QVariantList tasks;
    QSet<QString> taskIds;
    qsizetype stepCount = 0;
    for (const QVariantList entries = rawTasks.toList(); const QVariant &entry : entries) {
        QVariantMap task;
        if (entry.metaType().id() != QMetaType::QVariantMap)
            return QStringLiteral("task.error.format");
        if (const QString error = normalizeTask(entry.toMap(), task, version, loading); !error.isEmpty())
            return error;
        const QString id = task.value(QStringLiteral("id")).toString();
        if (taskIds.contains(id))
            return QStringLiteral("task.error.format");
        taskIds.insert(id);
        stepCount += task.value(QStringLiteral("steps")).toList().size();
        if (stepCount > 256)
            return QStringLiteral("task.error.format");
        tasks.append(task);
    }
    output = {{QStringLiteral("gameVersion"), version},
              {QStringLiteral("tasks"), tasks},
              {QStringLiteral("rules"), rules}, {QStringLiteral("failurePolicies"), normalizedPolicies},
              {QStringLiteral("reconnect"), QVariantMap{
                  {QStringLiteral("delaySeconds"), reconnect.value(QStringLiteral("delaySeconds")).toInt()},
                  {QStringLiteral("maxAttempts"), reconnect.value(QStringLiteral("maxAttempts")).toInt()},
                  {QStringLiteral("unlimited"), reconnect.value(QStringLiteral("unlimited"))}}}};
    return {};
}
