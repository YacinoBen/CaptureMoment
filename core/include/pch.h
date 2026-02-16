/**
 * @file pch.h
 * @brief Precompiled Header for the CaptureMoment Core Library.
 *        Includes standard STL, third-party, and internal core headers to accelerate compilation.
 * @author CaptureMoment Team
 * @date 2025
 */
#pragma once

// ============================================================
// 1. C++ Standard Library (Foundational types and utilities)
// ============================================================
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <utility>
#include <functional>
#include <unordered_map>

// ============================================================
// 2. Third-Party Libraries (Can sometimes define global macros)
//    Place Halide early if other headers might rely on its types/macros.
//    spdlog is generally well-behaved regarding macros.
// ============================================================
#include "Halide.h"     // Halide library headers
#include <spdlog/spdlog.h> // spdlog library headers

// ============================================================
// 3. Internal Core Modules (Umbrella Headers Only)
//    Order might matter if one umbrella header includes another,
//    but ideally, umbrella headers are self-contained.
//    Common types/config often used by many others.
// ============================================================

// 3a. Common & Configuration (Often foundational)
#include "common/types/memory_type.h" // Basic enums/types
#include "config/app_config.h"        // Global config singleton
#include "common/image_region.h"      // Core POD data structure
#include "common/error_handling/core_error.h" // Error handling types

// 3b. Core Modules (Order might be less critical with umbrella headers)
#include "common/common.h"            // Umbrella for common types/utils (if exists)
#include "operations/operations.h"    // Operations and factories
#include "image_processing/image_processing.h" // Hardware abstraction, factories, etc.
#include "pipeline/pipeline.h"        // Pipeline executors, builders, etc.
#include "strategies/strategies.h"    // Pipeline management strategies
#include "managers/managers.h"        // Source, State managers
#include "domain/domain.h"            // Task interfaces, etc.
#include "engine/engine.h"           // Orchestrators like PhotoEngine
#include "serializer/serializer.h"   // Serialization interfaces, providers, etc.


// ============================================================
// 4. Utility Headers (Could be included by internal modules)
//    Placed here if not part of an umbrella header.
//    Ensure utilities don't depend on other core modules.
// ============================================================
#include "utils/to_string_utils.h"   // Generic string conversion utilities
#include "utils/image_conversion.h"  // Image format conversion utilities