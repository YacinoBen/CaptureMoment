/**
 * @file splash_screen.h
 * @brief Splash screen class for application startup
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <QSplashScreen>
#include <QString>

namespace CaptureMoment::UI {

/**
 * @brief Custom splash screen displaying the application logo and loading status.
 *
 * This class handles high-DPI scaling of the provided logo and renders
 * a subtle loading message at the bottom of the window.
 */
class SplashScreen : public QSplashScreen {
public:
    /**
     * @brief Constructor for the splash screen.
     * @param image_path The file path or Qt resource path (qrc) to the splash screen logo image.
     *                   Defaults to the embedded application logo.
     * @param screen The target screen to display the splash screen on.
     */
    explicit SplashScreen(const QString& image_path = ":/splash/splash-screen.png", QScreen* screen = nullptr);

    /**
     * @brief Updates the loading status message displayed on the splash screen.
     * @param message The text message to display.
     * @param alignment The alignment of the message within the splash screen.
     * @param color The color of the message text.
     */
    void showMessage(const QString& message, int alignment = Qt::AlignBottom | Qt::AlignHCenter, 
                     const QColor& color = Qt::darkGray);

protected:
    /**
     * @brief Draws the custom contents (the status message) over the pixmap.
     * @param painter The painter used to draw the message.
     */
    void drawContents(QPainter* painter) override;

private:
    /**
     * @brief Sets up the initial pixmap, handling scaling and DPI.
     * @param image_path The file path to the logo image.
     * @param screen The target screen used to determine pixel ratio.
     */
    void setupPixmap(const QString& image_path, QScreen* screen);

    QString m_current_message {}; ///< The current loading status message to display.
    int m_alignment {Qt::AlignBottom | Qt::AlignHCenter}; ///< The alignment flag for the message.
    QColor m_color {Qt::darkGray}; ///< The color of the status message.
};

} // namespace CaptureMoment::UI
