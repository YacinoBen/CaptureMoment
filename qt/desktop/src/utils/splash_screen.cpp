/**
 * @file splash_screen.cpp
 * @brief Implementation of the SplashScreen class
 * @author CaptureMoment Team
 * @date 2026
 */

#include "utils/splash_screen.h"

#include <spdlog/spdlog.h>

#include <QPainter>
#include <QScreen>
#include <QFontMetrics>

namespace CaptureMoment::UI {

SplashScreen::SplashScreen(const QString& image_path, QScreen* screen)
    : QSplashScreen(screen, QPixmap())
{
    setupPixmap(image_path, screen);
}

void SplashScreen::setupPixmap(const QString& image_path, QScreen* screen)
{
    QPixmap original_logo {image_path};
    if (original_logo.isNull())
    {
        spdlog::error("Splash screen logo NOT FOUND at {}", qPrintable(image_path));
        original_logo = QPixmap {400, 400};
        original_logo.fill(Qt::white);
    }

    const qreal dpr {screen ? screen->devicePixelRatio() : 1.0};

    const int target_width {700};
    const int target_height {380};
    const int bottom_padding {45};

    QPixmap final_pixmap {static_cast<int>(target_width * dpr), static_cast<int>((target_height + bottom_padding) * dpr)};
    final_pixmap.setDevicePixelRatio(dpr);
    final_pixmap.fill(Qt::white);

    QPainter painter {&final_pixmap};
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    painter.drawPixmap(0, 0, target_width, target_height, original_logo);

    painter.end();

    setPixmap(final_pixmap);
}

void SplashScreen::showMessage(const QString& message, int alignment, const QColor& color)
{
    m_current_message = message;
    m_alignment = alignment;
    m_color = color;

    // Call base class to trigger the repaint (which calls drawContents)
    QSplashScreen::showMessage(message, alignment, color);
}

void SplashScreen::drawContents(QPainter* painter)
{
    const int8_t margin {15};
    const int bottom_zone_height {45};

    QRect text_rect {this->rect()};
    text_rect.setTop(text_rect.bottom() - bottom_zone_height);
    text_rect.setBottom(text_rect.bottom() - margin);
    text_rect.setLeft(text_rect.left() + margin);
    text_rect.setRight(text_rect.right() - margin);

    QFont slogan_font {"Segoe UI", 10};
    painter->setFont(slogan_font);
    painter->setPen(Qt::black);
    painter->setOpacity(0.9);

    const QString slogan {tr("Capture Moment - A photographic moment, captured freely.")};
    painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, slogan);

    if (!m_current_message.isEmpty())
    {
        QFont status_font {"Segoe UI", 9};
        status_font.setItalic(true);
        painter->setFont(status_font);
        painter->setPen(Qt::black);
        painter->setOpacity(0.9);

        painter->drawText(text_rect, Qt::AlignRight | Qt::AlignVCenter, m_current_message);
    }

    painter->setOpacity(1.0);
}

} // namespace CaptureMoment::UI