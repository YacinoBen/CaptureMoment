/**
 * @file image_pipeline.h
 * @brief Orchestration layer - coordinates SourceManager and Operations
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <vector>
#include <memory>

namespace CaptureMoment {

// Forward declarations
class SourceManager;
class OperationFactory;
class ImageRegion;
class OperationDescriptor;

/**
 * @class ImagePipeline
 * @brief Orchestrates tile extraction and operation execution
 *
 * Responsibilities:
 * - Extract tiles from SourceManager
 * - Apply sequence of operations (Strategy Pattern via factory)
 * - Handle error propagation
 *
 * The pipeline is agnostic to operation types - it delegates to factory.
 */
class ImagePipeline {
public:
    /**
     * @brief Construct pipeline with SourceManager and OperationFactory
     * @param source Reference to SourceManager
     * @param factory Reference to OperationFactory (pre-configured with all operations)
     */
    explicit ImagePipeline(SourceManager& source, const OperationFactory& factory);

    /**
     * @brief Process a rectangular region with a sequence of operations
     *
     * Flow:
     * 1. Get tile from SourceManager (handles clamping, format conversion, HDR/SDR)
     * 2. For each operation:
     *    a. Factory creates concrete operation instance
     *    b. Execute operation on tile (Strategy Pattern)
     * 3. Write tile back (optional)
     *
     * @param x Top-left X coordinate
     * @param y Top-left Y coordinate
     * @param width Tile width
     * @param height Tile height
     * @param operations Vector of operation descriptors
     * @return true if all succeed, false on first failure
     */
    bool processRegion(
        int x, int y, int width, int height,
        const std::vector<OperationDescriptor>& operations
    );

private:
    SourceManager& m_source;
    const OperationFactory& m_factory;

    /**
     * @brief Write processed tile back to source
     */
    bool writeTileBack(int x, int y, const ImageRegion& tile);
};

} // namespace CaptureMoment