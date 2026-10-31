#pragma once

#include <QElapsedTimer>
#include <QJsonArray>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QVariantMap>
#include <array>

class CountingWorker final : public QObject
{
    Q_OBJECT
public:
    explicit CountingWorker(QObject *parent = nullptr);
    [[nodiscard]] QVariantMap statistics() const;

public slots:
    void configure(const QVariantMap &configuration);
    void process(const QJsonArray &events, qsizetype bytes);
    void resetConnection();
    void setOnline(bool online);
    void snapshot();

signals:
    void updated(const QVariantMap &statistics);
    void processed(qsizetype bytes);
    void failed();
    void finalized(const QVariantMap &statistics);

private:
    using Point = std::array<double, 3>;
    struct Track {
        Point previous{};
        bool hasPrevious = false;
        bool qualified = false;
    };
    struct Rule {
        QVariantMap definition;
        QHash<QString, Track> tracks;
        QSet<QString> counted;
        QMap<QString, quint64> totals;
        QMap<QString, int> stackSizes;
        QVariantList matchers;
        int segment = 1;
    };
    static bool contains(const QVariantMap &region, const Point &point);
    static bool crosses(const QVariantMap &region, const Point &from, const Point &to);
    [[nodiscard]] qint64 elapsed() const;
    void publish();
    QVariantList m_archived;
    QHash<QString, Rule> m_rules;
    QStringList m_order;
    QTimer *m_timer = nullptr;
    QElapsedTimer m_onlineClock;
    qint64 m_onlineMilliseconds = 0;
    bool m_online = false;
};
