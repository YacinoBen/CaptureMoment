/**
 * @file pch.h
 * @brief Precompiled Header for the CaptureMoment Core Library.
 *        Includes standard STL, third-party, and internal core headers to accelerate compilation.
 * @author CaptureMoment Team
 * @date 2025
 */
#pragma once

// ============================================================
// 1. C++ Standard Library
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

// ============================================================
// 2. External Libraries
// ============================================================
#include <Halide.h>
#include <spdlog/spdlog.h>


// ============================================================
// 3. Internal Core Modules (Umbrella Headers Only)
// ============================================================

// Core Image Processing (This is where image_processing.h goes)
#include "image_processing/image_processing.h"


// Core Common & Error Handling
#include "common/error_handling/core_error.h"

// ============================================================
// 4. Internal Core - Common & Config
// ============================================================
#include "common/types/memory_type.h"
#include "config/app_config.h"
#include "common/image_region.h"

// ============================================================
// 5. Internal Core - Interfaces
// ============================================================
#include "operations/operation_type.h"
#include "operations/interfaces/i_operation.h"
#include "operations/interfaces/i_operation_fusion_logic.h"

// ============================================================
// 5. Internal Core - Implementation & Logic
// ============================================================
#include "operations/operation_descriptor.h"
#include "operations/operation_factory.h"
#include "operations/operation_ranges.h"

