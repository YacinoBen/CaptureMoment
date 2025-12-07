/**
 * @file examples/basic_brightness.cpp
 * @brief Simple example: Load image → Apply brightness → Process
 * @author CaptureMoment Team
 * @date 2025
 *
 * This example demonstrates:
 * 1. Loading an image with SourceManager
 * 2. Setting up OperationFactory with operations
 * 3. Creating a PipelineEngine
 * 4. Processing a tile region with brightness adjustment
 * 5. Retrieving the result
 *
 * Compile with:
 *   g++ -std=c++17 examples/basic_brightness.cpp \
 *       src/*.cpp src/operations/*.cpp \
 *       -o basic_brightness \
 *       -I./include \
 *       -loiio -lhalide -lspdlog
 *
 * Run:
 *   ./basic_brightness <image_file>
 *
 * Example:
 *   ./basic_brightness input.exr
 */

#include "types.h"
#include "managers/source_manager.h"
#include "operations/operation_factory.h"
#include "engine/pipeline_engine.h"
#include "operations/operation_brightness.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <string_view>

using namespace CaptureMoment;

/**
 * @brief Initialize logger with colored output
 */
void initializeLogging() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("CaptureMoment", console_sink);
    spdlog::register_logger(logger);
    logger->set_level(spdlog::level::debug);
    spdlog::set_default_logger(logger);
}

/**
 * @brief Setup and register all available operations
 * @return Configured OperationFactory
 */
OperationFactory setupFactory() {
    OperationFactory factory;
    
    // Register available operations
    factory.registerOperation<BrightnessOperation>(OperationType::Brightness);
    // factory.registerOperation<ContrastOperation>(OperationType::Contrast);
    // factory.registerOperation<SaturationOperation>(OperationType::Saturation);
    
    spdlog::info("✓ OperationFactory initialized with {} operation type(s)", 1);
    return factory;
}

/**
 * @brief Create a brightness operation descriptor
 * @param value Brightness adjustment value (-1.0 to 1.0)
 * @return OperationDescriptor configured for brightness
 */
OperationDescriptor createBrightnessOp(float value) {
    OperationDescriptor brightness;
    brightness.type = OperationType::Brightness;
    brightness.name = "Brightness(+" + std::to_string(value) + ")";
    brightness.enabled = true;
    brightness.setParam("value", value);
    return brightness;
}

/**
 * @brief Main entry point
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @return Exit code (0 = success, 1 = failure)
 */
int main(int argc, char* argv[]) {
    // ========================================================================
    // 1. Validate arguments
    // ========================================================================
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <image_file>\n";
        std::cerr << "Example: " << argv[0] << " sample.exr\n";
        return 1;
    }

    std::string_view imagePath = argv[1];

    // ========================================================================
    // 2. Initialize logging
    // ========================================================================
    initializeLogging();
    spdlog::info("========== CaptureMoment Example: Basic Brightness ==========");
    spdlog::info("Image file: {}", imagePath);

    try {
        // ====================================================================
        // 3. Setup OperationFactory
        // ====================================================================
        OperationFactory factory = setupFactory();

        // ====================================================================
        // 4. Load image via SourceManager
        // ====================================================================
        SourceManager source;
        
        if (!source.loadFile(imagePath)) {
            spdlog::error("✗ Failed to load image: {}", imagePath);
            return 1;
        }
        
        spdlog::info("✓ Image loaded: {}x{} ({} channels)",
                     source.width(), source.height(), source.channels());

        // ====================================================================
        // 5. Create PipelineEngine
        // ====================================================================
        PipelineEngine engine(source, factory);
        spdlog::info("✓ PipelineEngine initialized");

        // ====================================================================
        // 6. Define tile region to process
        // ====================================================================
        int tile_x = 0;
        int tile_y = 0;
        int tile_width = std::min(512, source.width());
        int tile_height = std::min(512, source.height());

        spdlog::info("Processing tile at ({}, {}) size {}x{}",
                     tile_x, tile_y, tile_width, tile_height);

        // ====================================================================
        // 7. Create operation sequence
        // ====================================================================
        std::vector<OperationDescriptor> operations;
        operations.push_back(createBrightnessOp(0.3f));  // +30% brightness

        spdlog::info("✓ Operation sequence created: {} operation(s)", operations.size());

        // ====================================================================
        // 8. Process tile through engine
        // ====================================================================
        // Flow:
        // 1. SourceManager::getTile() extracts pixels (clamped, RGBA F32)
        // 2. For each operation:
        //    - Factory creates BrightnessOperation instance
        //    - Engine calls operation->execute(tile, descriptor)
        //    - Halide modifies pixels in-place
        // 3. writeTileBack() (optional)
        
        spdlog::debug("Starting tile processing...");
        
        if (!engine.processRegion(tile_x, tile_y, tile_width, tile_height, operations)) {
            spdlog::error("✗ Pipeline processing failed");
            return 1;
        }

        spdlog::info("✓ Tile processing completed successfully");

        // ====================================================================
        // 9. Retrieve result tile (optional)
        // ====================================================================
        auto result = source.getTile(tile_x, tile_y, tile_width, tile_height);
        
        if (!result) {
            spdlog::error("✗ Failed to retrieve result tile");
            return 1;
        }

        spdlog::info("✓ Result tile: {}x{} RGBA F32",
                     result->m_width, result->m_height);
        spdlog::info("  Total pixels: {}",
                     result->m_width * result->m_height);
        spdlog::info("  Channels: {}", result->m_channels);
        spdlog::info("  Data size: {:.2f} MB",
                     (result->m_data.size() * sizeof(float)) / (1024.0f * 1024.0f));

        // ====================================================================
        // 10. Display sample pixel values
        // ====================================================================
        if (result->m_data.size() >= 4) {
            spdlog::info("Sample pixel (first) - R:{:.3f} G:{:.3f} B:{:.3f} A:{:.3f}",
                         result->m_data[0],
                         result->m_data[1],
                         result->m_data[2],
                         result->m_data[3]);
        }

        // ====================================================================
        // Success
        // ====================================================================
        spdlog::info("========== Example completed successfully ==========");
        return 0;

    } catch (const std::exception& e) {
        spdlog::critical("✗ Unhandled exception: {}", e.what());
        return 1;
    }
}