/**
 * @file rhi_image_node.h
 * @brief Declaration of RHIImageNode for direct QRhi rendering.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QSGRenderNode>
#include <rhi/qrhi.h>
#include <memory>

#include "common/image_region.h"
#include "rendering/rhi_image_item.h"

namespace CaptureMoment::UI {

/**
* @brief Namespace containing Qt-specific UI components for CaptureMoment.
* 
* This namespace includes classes responsible for rendering and UI integration
* using Qt Quick and the Qt Rendering Hardware Interface (QRhi).
*/
namespace Rendering {

class RHIImageItem; 
/**
* @class RHIImageNode
* @brief QSGRenderNode for direct RHI rendering.
* 
* This class is responsible for managing the GPU resources (textures, buffers, pipeline)
* and performing the actual rendering commands using QRhi on the render thread.
* It receives state updates from the main GUI thread via synchronize().
* 
* It manages:
* - GPU texture
* - Graphics pipeline
* - Rendering
*/
class RHIImageNode : public QSGRenderNode {

private:
    /**
    * @brief Pointer to the RHIImageItem that owns this node.
    * 
    * Used to access the image data and state from the main thread.
    */
    RHIImageItem* m_item{nullptr};

    /**
    * @brief Pointer to the QRhi instance for the window.
    */
    QRhi* m_rhi{nullptr};
            
    // RHI Resources (owned by this node using RAII)
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
            
public:

    /**
    * @brief Constructs a new RHIImageNode.
    * @param item The RHIImageItem instance that owns this node.
    */
    explicit RHIImageNode(RHIImageItem* item);

    /**
    * @brief Destroys the RHIImageNode and releases GPU resources.
    */
    ~RHIImageNode();

    /**
    * @brief Synchronizes state from the GUI thread to the render thread.
    * 
    * This method is called on the render thread while the GUI thread is blocked.
    * It checks for updates (like texture changes) from the RHIImageItem and
    * prepares resources for the next render call.
    */
    void synchronize();

    /**
    * @brief Performs the actual rendering commands using QRhi.
    * 
    * This method is called on the render thread. It sets up the pipeline,
    * binds resources, and issues draw commands.
    * 
    * @param state The current render state provided by the scene graph.
    */
    void render(const RenderState* state) override;
            
    /**
    * @brief Returns flags indicating the rendering behavior of this node.
    * 
    * Specifies that this node renders within a bounded rectangle and potentially uses depth.
    * 
    * @return RenderingFlags for this node.
    */
    RenderingFlags flags() const override {
        return BoundedRectRendering | DepthAwareRendering;
    }
            
private:
    /**
    * @brief Initializes the RHI resources (pipeline, buffers, etc.).
    * 
    * Called once during the first synchronize call if resources are not initialized.
    */
    void initialize();

    /**
    * @brief Creates the vertex and index buffers for the geometry (e.g., a quad).
    */
   void createGeometry();

    /**
    * @brief Creates the graphics pipeline (shaders, bindings, render pass descriptor).
    */
    void createPipeline();

    /**
    * @brief Updates the GPU texture if the image data has changed.
    * 
    * Checks the state from m_item and uploads the new data if necessary.
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

} // namespace CaptureMoment::Qt
