/**
 * @file rhi_image_item_renderer.h
 * @brief Renderer implementation for RHIImageItem using QQuickRhiItemRenderer.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickRhiItemRenderer>
#include <rhi/qrhi.h>
#include <memory>
#include <QMutex> // For thread-safe access to shared data from RHIImageItem

#include "common/image_region.h" // Assuming ImageRegion is in common

namespace CaptureMoment::UI {

namespace Rendering {
class RHIImageItem; // Forward declaration

/**
 * @class RHIImageItemRenderer
 * @brief Handles the RHI rendering logic for RHIImageItem.
 * 
 * This class inherits from QQuickRhiItemRenderer and is responsible for
 * creating and managing GPU resources (textures, buffers, pipelines) and
 * executing the rendering commands using QRhi. It receives state updates
 * from the RHIImageItem (main thread) via synchronize() and performs
 * the rendering on the render thread.
 */
class RHIImageItemRenderer : public QQuickRhiItemRenderer
{
private:
    /**
     * @brief Pointer to the RHIImageItem that owns this renderer.
     * 
     * Used to access the image data and state from the main thread via synchronize().
     */
    RHIImageItem* m_item{nullptr};

    // RHI Resources (owned by this renderer using RAII)
    /**
     * @brief GPU texture storing the image data.
     */
    std::unique_ptr<QRhiTexture> m_texture;

    /**
     * @brief Sampler defining how the texture is sampled (e.g., filtering, wrapping).
     */
    std::unique_ptr<QRhiSampler> m_sampler;

    /**
     * @brief Vertex buffer containing the quad geometry.
     */
    std::unique_ptr<QRhiBuffer> m_vertex_buffer;

    /**
     * @brief Index buffer for the quad geometry.
     */
    std::unique_ptr<QRhiBuffer> m_index_buffer;

    /**
     * @brief Uniform buffer for transformation matrices or other uniforms.
     */
    std::unique_ptr<QRhiBuffer> m_uniform_buffer;

    /**
     * @brief Shader resource bindings linking textures, samplers, and buffers to the shader.
     */
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;

    /**
     * @brief Graphics pipeline object defining the rendering state (shaders, blend state, etc.).
     */
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    /**
     * @brief Flag indicating if the node's resources have been initialized.
     */
    bool m_initialized{false};

    // Data copied from m_item for thread-safe rendering
    /**
     * @brief Copy of the image width from the main thread.
     */
    int m_render_image_width{0};

    /**
     * @brief Copy of the image height from the main thread.
     */
    int m_render_image_height{0};

    /**
     * @brief Copy of the zoom value from the main thread.
     */
    float m_render_zoom{1.0f};

    /**
     * @brief Copy of the pan value from the main thread.
     */
    QPointF m_render_pan{0, 0};

    /**
     * @brief Mutex protecting access to m_render_* members.
     */
    mutable QMutex m_render_state_mutex;

public:
    /**
     * @brief Constructs a new RHIImageItemRenderer.
     * @param item The RHIImageItem instance that owns this renderer.
     */
    explicit RHIImageItemRenderer(RHIImageItem* item);

    /**
     * @brief Destroys the RHIImageItemRenderer and releases GPU resources.
     */
    ~RHIImageItemRenderer();

    /**
     * @brief Initializes the RHI resources (pipeline, buffers, etc.).
     * 
     * This method is called on the render thread when the renderer is first used.
     * It sets up the graphics pipeline, vertex/index buffers, shaders, and other resources.
     * 
     * @param cb The command buffer available during initialization.
     */
    void initialize(QRhiCommandBuffer *cb) override;

    /**
     * @brief Synchronizes state from the GUI thread to the render thread.
     * 
     * This method is called on the render thread while the GUI thread is blocked.
     * It checks for updates (like texture changes, zoom, pan) from the RHIImageItem and
     * prepares resources for the next render call. It copies the relevant state
     * from m_item to local members (m_render_*).
     */
    void synchronize(QQuickRhiItem *item) override;

    /**
     * @brief Performs the actual rendering commands using QRhi.
     * 
     * This method is called on the render thread. It sets up the pipeline,
     * binds resources, updates uniforms (zoom/pan), and issues draw commands.
     * 
     * @param cb The command buffer to record rendering commands into.
     */
    void render(QRhiCommandBuffer *cb) override;
            
private:
    /**
     * @brief Creates the vertex and index buffers for the geometry (e.g., a quad).
     * 
     * Called once during initialization.
     */
    void createGeometry();

    /**
     * @brief Creates the graphics pipeline (shaders, bindings, render pass descriptor).
     * 
     * Called once during initialization.
     */
    void createPipeline();

    /**
     * @brief Updates the GPU texture if the image data has changed.
     * 
     * Checks the state from m_item (copied during synchronize) and uploads
     * the new data if necessary.
     */
    void updateTexture();

    /**
     * @brief Uploads pixel data from an ImageRegion to the GPU texture.
     * 
     * @param image The ImageRegion containing the pixel data to upload.
     */
    void uploadPixelData(const std::shared_ptr<ImageRegion>& image);
};

} // namespace Rendering

} // namespace CaptureMoment::UI
