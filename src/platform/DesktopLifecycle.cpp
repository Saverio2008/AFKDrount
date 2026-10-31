#include "DesktopLifecycle.h"

#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QSystemTrayIcon>
#include <QWindow>

DesktopLifecycle::DesktopLifecycle(QObject *parent) : QObject(parent)
{
    QPixmap image(64, 64);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(QStringLiteral("#4b70bf")));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(4, 4, 56, 56, 14, 14);
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(38);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(image.rect(), Qt::AlignCenter, QStringLiteral("M"));
    painter.end();
    m_tray = new QSystemTrayIcon(QIcon(image), this);
    m_tray->setToolTip(QStringLiteral("AFK Drount"));
    m_menu = new QMenu;
    m_show = m_menu->addAction(QString());
    m_quit = m_menu->addAction(QString());
    connect(m_show, &QAction::triggered, this, &DesktopLifecycle::showWindow);
    connect(m_quit, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            showWindow();
    });
    m_tray->setContextMenu(m_menu);
    retranslate();
    m_tray->show();
}

DesktopLifecycle::~DesktopLifecycle()
{
    m_tray->hide();
    m_tray->setContextMenu(nullptr);
    delete m_menu;
}

void DesktopLifecycle::attach(QWindow *window)
{
    m_window = window;
}

void DesktopLifecycle::showWindow() const
{
    if (m_window) {
        m_window->show();
        m_window->raise();
        m_window->requestActivate();
    }
}

void DesktopLifecycle::retranslate() const
{
    m_show->setText(qtTrId("session.tray.show"));
    m_quit->setText(qtTrId("session.tray.quit"));
}