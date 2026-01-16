/**
 * @file operation_pipeline.h
 * @brief Declaration of the OperationPipeline class.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <vector>
#include "image_processing/interfaces/i_working_image_hardware.h"

namespace CaptureMoment::Core {

namespace Operations {

// Forward declarations
class OperationFactory;
struct OperationDescriptor;

/**
 * @brief Static class responsible for applying a sequence of operations to a working image.
 *
 * The OperationPipeline provides a pure, stateless function (`applyOperations`)
 * to execute a list of image processing operations on a given working image.
 * It uses an OperationFactory to create instances of the required operations.
 * This class is designed to be independent of SourceManager or other high-level
 * state managers, focusing solely on the execution logic of the operation pipeline.
 * It is now agnostic to the underlying hardware (CPU/GPU) thanks to IWorkingImageHardware.
 */
class OperationPipeline {
public:
    /**
     * @brief Applies a sequence of operations to a working image.
     *
     * Iterates through the provided list of OperationDescriptors, uses the
     * given OperationFactory to create the corresponding operation instances,
     * and executes them sequentially on the input/output working image.
     * The working image is modified in-place.
     *
     * @param[in,out] working_image The working image to process. This object is modified
     *                              by the operations.
     * @param[in] operations A vector of OperationDescriptor objects defining
     *                       the sequence of operations to apply.
     * @param[in] factory The OperationFactory instance used to create the
     *                    concrete operation objects.
     * @return true if all operations were applied successfully, false otherwise.
     */
    [[nodiscard]] static bool applyOperations(
        ImageProcessing::IWorkingImageHardware& working_image,
        const std::vector<OperationDescriptor>& operations,
        const OperationFactory& factory
        );
};

} // namespace Operations

} // namespace CaptureMoment::Core
