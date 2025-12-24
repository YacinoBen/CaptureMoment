/**
 * @file operation_registry.h
 * @brief Central registry for all image operations
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "operations/operation_factory.h"

namespace CaptureMoment::Core {

namespace Operations {
class OperationFactory;

/**
 * @class OperationRegistry
 * @brief Manages registration of all available operations
 * 
 * Provides a centralized point to register operations.
 * Extensible for adding new operation categories (color management, etc.)
 * 
 * Usage:
 * @code
 * auto factory = std::make_shared<OperationFactory>();
 * OperationRegistry::registerAll(factory);
 * @endcode
 */
class OperationRegistry {
public:
    /**
     * @brief Register all available operations
     * @param factory OperationFactory to register operations with
     */
    static void registerAll(OperationFactory& factory);
    
private:
    /**
     * @brief Register tone adjustment operations (brightness, contrast, exposure)
     * @param factory OperationFactory
     */
    static void registerToneAdjustments(OperationFactory& factory);
    
    /**
     * @brief Register color operations (saturation, hue, vibrance)
     * @param factory OperationFactory
     */
    static void registerColorOperations(OperationFactory& factory);
    
    /**
     * @brief Register color profile operations (color space conversions)
     * @param factory OperationFactory
     */
    static void registerColorProfiles(OperationFactory& factory);
    
    /**
     * @brief Register detail operations (sharpness, clarity, texture)
     * @param factory OperationFactory
     */
    static void registerDetailOperations(OperationFactory& factory);
    
    /**
     * @brief Register effects operations (blur, vignette, grain)
     * @param factory OperationFactory
     */
    static void registerEffects(OperationFactory& factory);
};

} // namespace Operations

} // namespace CaptureMoment::Core
