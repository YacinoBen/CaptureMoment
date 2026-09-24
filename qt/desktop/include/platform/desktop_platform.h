/**
 * @file desktop_platform.h
 * @brief Platform-specific application settings (Linux / macOS / Windows)
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

namespace CaptureMoment::UI {

namespace Platform {

/**
 * @brief Applies Qt settings specific to the host platform.
 *
 * Must be called right after QApplication creation, before any
 * window is shown. The concrete implementation is selected at
 * build time (one .cpp per platform, see CMakeLists.txt).
 */
void applyApplicationSettings();

} // namespace Platform

} // namespace CaptureMoment::UI
