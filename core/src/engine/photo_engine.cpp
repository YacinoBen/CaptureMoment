/**
 * @file photo_engine.cpp
 * @brief Implementation of PhotoEngine
 * @author CaptureMoment Team
 * @date 2025
 */

#include "engine/photo_engine.h"
#include "engine/photo_task.h"
#include <string_view>
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Engine {

// Constructor: Initializes the engine with required managers/factories.
PhotoEngine::PhotoEngine(std::shared_ptr<Managers::SourceManager> source_manager,
                         std::shared_ptr<Operations::OperationFactory> operation_factory)
    : m_source_manager{source_manager}, m_operation_factory{operation_factory}, m_working_image{nullptr} {}

// Loads a photo file using the SourceManager.
bool PhotoEngine::loadImage(std::string_view path)
{
    // Check if the source manager is valid before attempting to load.
    if (!m_source_manager) {
        // Error handling: SourceManager is required.
        spdlog::error("PhotoEngine::LoadImage : error to load m_source_manager");
        return false;
    }

    // 1. Load original data into SourceManager (The Truth)
    if (!m_source_manager->loadFile(path)) {
        spdlog::error("PhotoEngine::LoadImage : error to loadFile(path) from m_source_manager");
        return false;
    }

    // 2. Initialize Working Image as a copy of the Source
    // This allows displaying the image immediately before any processing.
    resetWorkingImage();

    if (!m_working_image) {
        spdlog::error("PhotoEngine::LoadImage : error resetWorkingImage()");
        return false;
    }

    return m_source_manager->loadFile(path);
}

// Creates a processing task (Implementation of IProcessingBackend interface).
std::shared_ptr<Domain::IProcessingTask> PhotoEngine::createTask(
    const std::vector<Operations::OperationDescriptor>& ops, // List of operations to apply.
    int x, int y, int width, int height          // Region of interest (ROI) for the task.
    )
{
    // Check if required managers are valid.
    if (!m_working_image || !m_operation_factory) {
        // Error handling: SourceManager and OperationFactory are required.
        spdlog::error("PhotoEngine::createTask : (!m_working_image| !m_operation_factory ) error check");
        return nullptr;
    }

    // Check if the requested ROI is within the bounds of the working image.
    if (x < 0 || y < 0 || x + width > m_working_image->m_width || y + height > m_working_image->m_height) {
        spdlog::error("PhotoEngine::createTask: ROI ({},{},{},{}) is out of bounds for working image ({}x{}).", x, y, width, height, m_working_image->m_width, m_working_image->m_height);
        return nullptr;
    }

    auto tile_unique_ptr = std::make_unique<Common::ImageRegion>();
    tile_unique_ptr->m_x = x; // Use the requested coordinates for the tile
    tile_unique_ptr->m_y = y;
    tile_unique_ptr->m_width = width;
    tile_unique_ptr->m_height = height;
    tile_unique_ptr->m_channels = m_working_image->m_channels;
    tile_unique_ptr->m_format = m_working_image->m_format;
    // Calculate size for the region
    const size_t region_data_size = static_cast<size_t>(width) * height * m_working_image->m_channels;
    tile_unique_ptr->m_data.resize(region_data_size);

    // Copy data from the working image to the region
    for (int py = 0; py < height; ++py) {
        for (int px = 0; px < width; ++px) {
            for (int c = 0; c < m_working_image->m_channels; ++c) {
                // Calculate source index in m_working_image
                size_t src_idx = ((y + py) * m_working_image->m_width + (x + px)) * m_working_image->m_channels + c;
                // Calculate destination index in tile_unique_ptr
                size_t dst_idx = (py * width + px) * m_working_image->m_channels + c;
                tile_unique_ptr->m_data[dst_idx] = m_working_image->m_data[src_idx];
            }
        }
    }

    if (!tile_unique_ptr) {
        spdlog::warn("PhotoEngine::createTask: Failed to create a valid tile from working image for ROI ({},{},{},{}).", x, y, width, height);
        return nullptr;
    }

    // 2. Converts the unique_ptr to shared_ptr BEFORE calling it at make_shared
    // We use the shared_ptr constructor which takes a unique_ptr.
    std::shared_ptr<Common::ImageRegion> tile_shared_ptr = std::move(tile_unique_ptr);

    // 3. Creates and returns a new PhotoTask instance with the shared_ptr
    // The order of arguments in the PhotoTask constructor is:
    // (input_tile, ops, operation_factory)
    return std::make_shared<PhotoTask>(tile_shared_ptr, ops, m_operation_factory);
}

// Submits a task for execution (Implementation of IProcessingBackend interface).
bool PhotoEngine::submit(std::shared_ptr<Domain::IProcessingTask> task)
{
    // Check if the task pointer is valid.
    if (!task) {
        // Error handling: The task pointer is null.
        spdlog::error("PhotoEngine::submit Failed task");
        return false;
    }

    // Execute the task synchronously (Version 1 implementation).
    task->execute();

    // Assume success if execution completes without throwing an exception
    // and the task itself doesn't signal an internal error.
    return true;
}

// Commits the result of a completed task back to the source image.
bool PhotoEngine::commitResult(const std::shared_ptr<Domain::IProcessingTask>& task)
{
    // Check if the task pointer is valid.
    if (!task) {
        // Error handling: The task pointer is null.
        spdlog::error("PhotoEngine::commitResult Failed task");
        return false;
    }

    auto result = task->result();
    if (!result) {
        spdlog::error("PhotoEngine::commitResult Failed task result");
        return false; // Task failed or has no result
    }

    // Ensure Working Image exists and matches dimensions
    if (!m_working_image ||
        m_working_image->m_width != width() ||
        m_working_image->m_height != height())
    {
        spdlog::warn("PhotoEngine::commitResult Working image invalid or resized. Resetting.");
        resetWorkingImage();
        if (!m_working_image) {
            spdlog::error("PhotoEngine::commitResult Failed Working Image exists and matches dimensions");
            return false;
        }
    }

    // Update the Working Image buffer with the processed tile data
    // We copy pixel by pixel (or memcpy row by row) from Result -> WorkingImage
    // based on the result's coordinates (x, y).

    // Note: ImageRegion should support this copy logic. Assuming direct access here.
    auto& resRef = *result;

    // Safety check bounds
    if (resRef.m_x + resRef.m_width > m_working_image->m_width ||
        resRef.m_y + resRef.m_height > m_working_image->m_height) {
        spdlog::error("PhotoEngine: Result tile is out of bounds of Working Image.");
        return false;
    }

    // Merge logic (Copy result into working buffer)
    for (int y = 0; y < resRef.m_height; ++y) {
        for (int x = 0; x < resRef.m_width; ++x) {
            for (int c = 0; c < resRef.m_channels; ++c) {
                // Determine destination index in Working Image
                int destY = resRef.m_y + y;
                int destX = resRef.m_x + x;

                // Use operator() or direct access if available
                (*m_working_image)(destY, destX, c) = resRef(y, x, c);
            }
        }
    }

    spdlog::info("PhotoEngine::commitResult Result merged into Working Image.");
    return true;
}

bool PhotoEngine::commitWorkingImageToSource()
{
    if (!m_working_image || !m_source_manager) {
        spdlog::error("PhotoEngine::commitWorkingImageToSource: (!m_source_manager & !m_working_image) error check");
        return false;
    }

    // Write current Working Image back to Source Manager (Destructive Save)
    return m_source_manager->setTile(*m_working_image);
}

void PhotoEngine::resetWorkingImage()
{
    if (!m_source_manager) {
        spdlog::warn("PhotoEngine::resetWorkingImage: SourceManager is null, cannot reset.");
        return;
    }

    // Fetch the full original image
    int w = m_source_manager->width();
    int h = m_source_manager->height();

    if (w > 0 && h > 0) {
        auto full_original = m_source_manager->getTile(0, 0, w, h);
        if (full_original) {
            m_working_image = std::shared_ptr<Common::ImageRegion>(std::move(full_original));
            spdlog::info("PhotoEngine::resetWorkingImage: Working Image reset to Original ({}x{}).", w, h);
        } else {
            spdlog::error("PhotoEngine::resetWorkingImage: Failed to get full original tile from SourceManager.");
            m_working_image = nullptr;
        }
    } else {
        spdlog::warn("PhotoEngine::resetWorkingImage: Source image has invalid dimensions ({}x{}).", w, h);
        m_working_image = nullptr;
    }
}

// Returns the width of the currently loaded image.
int PhotoEngine::width() const noexcept
{
    // Check if the source manager is valid to prevent crashes.
    if (m_source_manager) {
        return m_source_manager->width();
    }
    // Return 0 if no image is loaded or the manager is null.
    return 0;
}

// Returns the height of the currently loaded image.
int PhotoEngine::height() const noexcept
{
    if (m_source_manager) {
        return m_source_manager->height();
    }
    return 0;
}

// Returns the number of channels of the currently loaded image.
int PhotoEngine::channels() const noexcept
{
    if (m_source_manager) {
        return m_source_manager->channels();
    }
    return 0;
}

} // namespace CaptureMoment::Core::Engine
