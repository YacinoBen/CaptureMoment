/**
 * @file image_view.h
 * @brief Lightweight, non-owning view over image memory and its geometry.
 *
 * @details
 * This structure is used to bind external memory (from a Context) to a 
 * hardware worker (CPU/GPU) without transferring ownership of the underlying data.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "common/types/image_types.h"
#include <span>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @struct ImageView
 * @brief Represents a zero-copy window over contiguous image memory.
 */
struct ImageView {
    std::span<float> working_data;       ///< Mutable span for processing
    std::span<const float> original_data;///< Const span for non-destructive reset
    Common::ImageDim width{0};
    Common::ImageDim height{0};
    Common::ImageChan channels{0};
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
