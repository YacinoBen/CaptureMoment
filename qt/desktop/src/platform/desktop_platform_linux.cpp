/**
 * @file desktop_platform_linux.cpp
 * @brief Linux implementation of the desktop platform settings
 * @author CaptureMoment Team
 * @date 2026
 */

#include "platform/desktop_platform.h"
#include <QGuiApplication>
#include <QIcon>

namespace CaptureMoment::UI::Platform {

void applyApplicationSettings()
{
    // Linux don't set the window icon automatically, so we need to set it explicitly.
    // This is a common issue on Linux platforms where the application icon may not be displayed correctly in the taskbar or window manager.
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/favicon.png")));
}

} // namespace CaptureMoment::UI::Platform
