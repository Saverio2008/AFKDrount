#pragma once

#include <QClipboard>
#include <QGuiApplication>
#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

class Clipboard final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Clipboard(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE static void copyText(const QString &text)
    {
        QGuiApplication::clipboard()->setText(text);
    }
};