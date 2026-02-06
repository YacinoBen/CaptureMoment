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

// Managers
#include "managers/manager.h"

// Core Engine Module
#include "engine/engine.h"

// Core Pipeline Module
#include "pipeline/pipeline.h"

// Core Common & Error Handling
#include "common/error_handling/core_error.h"

// Core Operations Module
#include "operations/operations.h"


// Core Domain Module
#include "domain/domain.h"

// Serializer Module
#include "serializer/serializer.h"

// ============================================================
// 4. Internal Core - Common & Config
// ============================================================
#include "common/types/memory_type.h"
#include "config/app_config.h"
#include "common/image_region.h"

