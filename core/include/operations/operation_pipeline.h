/**
 * @file operation_pipeline.h
 * @brief Declaration of the OperationPipeline class.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <vector> // Required for std::vector

namespace CaptureMoment::Core {

namespace Common {
struct ImageRegion;
}

namespace Operations {

// Forward declarations
class OperationFactory;
struct OperationDescriptor;

/**
 * @brief Static class responsible for applying a sequence of operations to an image region.
 *
 * The PipelineEngine provides a pure, stateless function (`applyOperations`)
 * to execute a list of image processing operations on a given tile of image data.
 * It uses an OperationFactory to create instances of the required operations.
 * This class is designed to be independent of SourceManager or other high-level
 * state managers, focusing solely on the execution logic of the operation pipeline.
 */
class OperationPipeline {
public:
    /**
     * @brief Applies a sequence of operations to an image region.
     *
     * Iterates through the provided list of OperationDescriptors, uses the
     * given OperationFactory to create the corresponding operation instances,
     * and executes them sequentially on the input/output ImageRegion tile.
     * The tile is modified in-place.
     *
     * @param[in,out] tile The ImageRegion to process. This object is modified
     *                     by the operations.
     * @param[in] operations A vector of OperationDescriptor objects defining
     *                       the sequence of operations to apply.
     * @param[in] factory The OperationFactory instance used to create the
     *                    concrete operation objects.
     * @return true if all operations were applied successfully, false otherwise.
     */
    [[nodiscard]] static bool applyOperations(
        Common::ImageRegion& tile,
        const std::vector<OperationDescriptor>& operations,
        const OperationFactory& factory
        );
};

} // namespace Operations

} // namespace CaptureMoment::Core
