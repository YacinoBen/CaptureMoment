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

// ============================================================
// 2. External Libraries
// ============================================================
#include <Halide.h>
#include <spdlog/spdlog.h>

// ============================================================
// 3. Internal Core - Common & Config
// ============================================================
#include "common/types/memory_type.h"
#include "config/app_config.h"
#include "common/image_region.h"

// ============================================================
// 4. Internal Core - Interfaces
// ============================================================
#include "operations/operation_type.h"
#include "operations/interfaces/i_operation.h"
#include "operations/interfaces/i_operation_fusion_logic.h"
#include "image_processing/interfaces/i_working_image_hardware.h"

// ============================================================
// 5. Internal Core - Implementation & Logic
// ============================================================
#include "operations/operation_descriptor.h"
#include "operations/operation_factory.h"
#include "operations/operation_ranges.h"

// Concrete implementations (included for maximum pre-compilation coverage)
#include "image_processing/cpu/working_image_cpu_halide.h"
#include "image_processing/gpu/working_image_gpu_halide.h"

