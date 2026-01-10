/**
 * @file exiv2_initializer.h
 * @brief Declaration of Exiv2Initializer class
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <mutex> // for std::once_flag

namespace CaptureMoment::Core {

namespace Serializer {

/**
 * @brief Singleton-like class to handle Exiv2 XMP parser initialization and termination.
 *
 * Ensures Exiv2::XmpParser::initialize() is called exactly once before any XMP operations,
 * and Exiv2::XmpParser::terminate() is called once at program end.
 * Uses std::call_once for thread safety during initialization.
 */
class Exiv2Initializer {
public:
    /**
     * @brief Initializes the Exiv2 XMP parser if not already done.
     * This function is thread-safe.
     */
    static void initialize();

    /**
     * @brief Terminates the Exiv2 XMP parser.
     * This function is registered to run at program termination via std::atexit.
     * Should not be called manually unless required.
     */
    static void terminate(); // Made public so it can be registered with atexit

private:
    /**
     * @brief Ensures initialization happens only once across all threads.
     */
    static std::once_flag m_init_flag;
};

} // namespace Serializer

} // namespace CaptureMoment::Core
