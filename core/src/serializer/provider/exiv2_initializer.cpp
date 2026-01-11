/**
 * @file exiv2_initializer.cpp
 * @brief Implementation of Exiv2Initializer
 * @author CaptureMoment Team
 * @date 2025
 */

#include "serializer/provider/exiv2_initializer.h"
#include <exiv2/exiv2.hpp>
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Serializer {

// Static member definition
std::once_flag Exiv2Initializer::m_init_flag;

void Exiv2Initializer::initialize()
{
    std::call_once(m_init_flag, [](){
        spdlog::debug("Exiv2Initializer: Initializing Exiv2 XMP Parser...");
        Exiv2::XmpParser::initialize();
        ::atexit(Exiv2Initializer::terminate); // Register termination function
        spdlog::info("Exiv2Initializer: Exiv2 XMP Parser initialized.");
    });
}

void Exiv2Initializer::terminate()
{
    spdlog::debug("Exiv2Initializer: Terminating Exiv2 XMP Parser...");
    Exiv2::XmpParser::terminate();
    spdlog::info("Exiv2Initializer: Exiv2 XMP Parser terminated.");
}

} // namespace CaptureMoment::Core::Serializer
