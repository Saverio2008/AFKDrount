#pragma once

#include <QVariantMap>

namespace InstanceConfiguration {
QVariantMap defaults();
QVariantMap newRule();
QVariantMap newTask();
QString normalizeTask(const QVariantMap &input, QVariantMap &output, const QString &version, bool loading = false);
QString normalizeRule(const QVariantMap &input, QVariantMap &output);
QString normalize(const QVariantMap &input, QVariantMap &output, bool loading = false);
}