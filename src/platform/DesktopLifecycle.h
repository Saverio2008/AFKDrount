#pragma once

#include <QObject>
#include <QPointer>

class QAction;
class QMenu;
class QSystemTrayIcon;
class QWindow;

class DesktopLifecycle final : public QObject
{
    Q_OBJECT
public:
    explicit DesktopLifecycle(QObject *parent = nullptr);
    ~DesktopLifecycle() override;
    void attach(QWindow *window);
    void retranslate() const;

private:
    void showWindow() const;
    QPointer<QWindow> m_window;
    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_show = nullptr;
    QAction *m_quit = nullptr;
};