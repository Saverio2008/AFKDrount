#include "CountingWorker.h"

#include <QJsonObject>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
QVariantList matcherIdentity(const QVariantMap &definition)
{
    QVariantList matchers;
    for (const QVariant &entry : definition.value(QStringLiteral("customItems")).toList()) {
        QVariantMap matcher = entry.toMap();
        matcher.remove(QStringLiteral("stackSize"));
        matchers.append(matcher);
    }
    return matchers;
}

QVariantMap statisticsRow(const QVariantMap &definition, int segment,
                          const QMap<QString, quint64> &totals, QMap<QString, int> stackSizes)
{
    for (const QVariant &entry : definition.value(QStringLiteral("customItems")).toList()) {
        const QVariantMap matcher = entry.toMap();
        const QString key = QStringLiteral("custom:") + matcher.value(QStringLiteral("id")).toString();
        const int size = matcher.value(QStringLiteral("stackSize")).toInt();
        stackSizes.insert(key, stackSizes.value(key, size) == size ? size : 0);
    }
    QVariantMap items;
    QVariantMap units;
    for (auto it = totals.cbegin(); it != totals.cend(); ++it) {
        items.insert(it.key(), QString::number(it.value()));
        if (const int size = stackSizes.value(it.key()); size > 0) {
            const auto stack = static_cast<quint64>(size);
            units.insert(it.key(), QVariantMap{{QStringLiteral("boxes"), QString::number(it.value() / stack / 27)},
                                                {QStringLiteral("stacks"), QString::number(it.value() / stack % 27)},
                                                {QStringLiteral("items"), QString::number(it.value() % stack)}});
        }
    }
    return {{QStringLiteral("ruleId"), definition.value(QStringLiteral("id"))},
            {QStringLiteral("name"), definition.value(QStringLiteral("name"))},
            {QStringLiteral("segment"), segment}, {QStringLiteral("items"), items}, {QStringLiteral("units"), units},
            {QStringLiteral("definition"), definition}};
}
}

CountingWorker::CountingWorker(QObject *parent) : QObject(parent) {}

void CountingWorker::configure(const QVariantMap &configuration)
{
    if (!m_timer) {
        m_timer = new QTimer(this);
        m_timer->setInterval(500);
        connect(m_timer, &QTimer::timeout, this, &CountingWorker::publish);
        m_timer->start();
    }
    QHash<QString, Rule> next;
    QStringList order;
    for (const QVariantList definitions = configuration.value(QStringLiteral("rules")).toList();
         const QVariant &value : definitions) {
        const QVariantMap definition = value.toMap();
        const QString id = definition.value(QStringLiteral("id")).toString();
        Rule rule = m_rules.take(id);
        const QVariantList matchers = matcherIdentity(definition);
        if (!rule.definition.isEmpty()) {
            QVariantMap previous = rule.definition;
            QVariantMap current = definition;
            previous.remove(QStringLiteral("name"));
            current.remove(QStringLiteral("name"));
            previous.remove(QStringLiteral("customItems"));
            current.remove(QStringLiteral("customItems"));
            if (previous != current || rule.matchers != matchers) {
                m_archived.append(statisticsRow(rule.definition, rule.segment, rule.totals, rule.stackSizes));
                rule.tracks.clear();
                rule.counted.clear();
                rule.totals.clear();
                rule.stackSizes.clear();
                ++rule.segment;
            }
        }
        rule.definition = definition;
        rule.matchers = matchers;
        order.append(id);
        next.insert(id, rule);
    }
    for (const Rule &removed : std::as_const(m_rules))
        m_archived.append(statisticsRow(removed.definition, removed.segment, removed.totals, removed.stackSizes));
    m_rules = next;
    m_order = order;
    publish();
}

bool CountingWorker::contains(const QVariantMap &region, const Point &point)
{
    const QVariantList from = region.value(QStringLiteral("from")).toList();
    const QVariantList to = region.value(QStringLiteral("to")).toList();
    for (int axis = 0; axis < 3; ++axis) {
        if (axis == 1 && region.value(QStringLiteral("unlimitedY")).toBool())
            continue;
        if (point[axis] < std::min(from[axis].toDouble(), to[axis].toDouble())
            || point[axis] > std::max(from[axis].toDouble(), to[axis].toDouble()))
            return false;
    }
    return true;
}

bool CountingWorker::crosses(const QVariantMap &region, const Point &from, const Point &to)
{
    if (contains(region, from))
        return false;
    const QVariantList a = region.value(QStringLiteral("from")).toList();
    const QVariantList b = region.value(QStringLiteral("to")).toList();
    double near = 0;
    double far = 1;
    for (int axis = 0; axis < 3; ++axis) {
        const double minimum = std::min(a[axis].toDouble(), b[axis].toDouble());
        const double maximum = std::max(a[axis].toDouble(), b[axis].toDouble());
        if (const double delta = to[axis] - from[axis]; std::abs(delta) < 1e-12) {
            if (from[axis] < minimum || from[axis] > maximum)
                return false;
        } else {
            const double first = (minimum - from[axis]) / delta;
            const double second = (maximum - from[axis]) / delta;
            near = std::max(near, std::min(first, second));
            far = std::min(far, std::max(first, second));
            if (near > far)
                return false;
        }
    }
    return far >= 0 && near <= 1;
}

void CountingWorker::process(const QJsonArray &events, qsizetype bytes)
{
    for (const auto &value : events) {
        const QJsonObject entity = value.toObject();
        const QString id = entity.value(QStringLiteral("id")).toString();
        if (id.isEmpty())
            continue;
        const bool gone = entity.value(QStringLiteral("gone")).toBool();
        const QJsonArray coordinates = entity.value(QStringLiteral("position")).toArray();
        if (!gone && (coordinates.size() != 3
            || std::ranges::any_of(coordinates, [](const QJsonValue &coordinate) {
                return !coordinate.isDouble() || !std::isfinite(coordinate.toDouble());
            })))
            continue;
        Point point{};
        if (!gone)
            for (int axis = 0; axis < 3; ++axis)
                point[axis] = coordinates[axis].toDouble();
        const QString item = entity.value(QStringLiteral("item")).toString();
        const double quantity = entity.value(QStringLiteral("count")).toDouble();
        for (Rule &rule : m_rules) {
            if (gone) {
                rule.tracks.remove(id);
                continue;
            }
            if (!rule.definition.value(QStringLiteral("enabled")).toBool() || rule.counted.contains(id))
                continue;
            QString matchedItem;
            const QVariantList &custom = rule.matchers;
            for (const QJsonArray matches = entity.value(QStringLiteral("customMatches")).toArray();
                 const auto &match : matches) {
                if (const QVariantMap matcher = match.toObject().toVariantMap(); custom.contains(matcher)) {
                    matchedItem = QStringLiteral("custom:") + matcher.value(QStringLiteral("id")).toString();
                    break;
                }
            }
            if (matchedItem.isEmpty() && rule.definition.value(QStringLiteral("items")).toList().contains(item))
                matchedItem = item;
            if (!item.isEmpty() && matchedItem.isEmpty() && custom.isEmpty()) {
                rule.tracks.remove(id);
                continue;
            }
            const QVariantMap tracking = rule.definition.value(QStringLiteral("trackingRegion")).toMap();
            const bool inside = contains(tracking, point);
            if (!inside && !rule.tracks.contains(id))
                continue;
            auto &[previous, hasPrevious, qualified] = rule.tracks[id];
            if (hasPrevious && crosses(rule.definition.value(QStringLiteral("countingRegion")).toMap(), previous, point))
                qualified = true;
            if (qualified && !matchedItem.isEmpty() && std::isfinite(quantity)
                && quantity > 0 && quantity <= 2147483647.0 && std::floor(quantity) == quantity) {
                const auto count = static_cast<quint64>(quantity);
                if (rule.totals.value(matchedItem) > std::numeric_limits<quint64>::max() - count) {
                    emit failed();
                    emit processed(bytes);
                    return;
                }
                rule.totals[matchedItem] += count;
                if (!matchedItem.startsWith(QLatin1String("custom:"))) {
                    const int size = entity.value(QStringLiteral("stackSize")).toInt();
                    rule.stackSizes.insert(matchedItem, rule.stackSizes.value(matchedItem, size) == size ? size : 0);
                }
                rule.counted.insert(id);
                rule.tracks.remove(id);
            } else if (inside || qualified) {
                previous = point;
                hasPrevious = inside;
            } else {
                rule.tracks.remove(id);
            }
            if (rule.counted.size() + rule.tracks.size() > 1000000) {
                emit failed();
                emit processed(bytes);
                return;
            }
        }
    }
    emit processed(bytes);
}

void CountingWorker::resetConnection()
{
    for (Rule &rule : m_rules) {
        rule.tracks.clear();
        rule.counted.clear();
    }
}

qint64 CountingWorker::elapsed() const
{
    return m_onlineMilliseconds + (m_online ? m_onlineClock.elapsed() : 0);
}

void CountingWorker::setOnline(bool online)
{
    if (m_online == online)
        return;
    if (m_online)
        m_onlineMilliseconds += m_onlineClock.elapsed();
    m_online = online;
    if (online)
        m_onlineClock.start();
    publish();
}

void CountingWorker::snapshot()
{
    setOnline(false);
    if (m_timer)
        m_timer->stop();
    publish();
    emit finalized(statistics());
}

QVariantMap CountingWorker::statistics() const
{
    QVariantList active;
    for (const QString &id : std::as_const(m_order)) {
        const Rule &rule = *m_rules.constFind(id);
        active.append(statisticsRow(rule.definition, rule.segment, rule.totals, rule.stackSizes));
    }
    return {{QStringLiteral("onlineMilliseconds"), elapsed()},
                             {QStringLiteral("rules"), active},
                             {QStringLiteral("archivedSegments"), m_archived}};
}

void CountingWorker::publish()
{
    emit updated(statistics());
}
