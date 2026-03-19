/**
 * @file rhi_image_item_renderer.h
 * @brief RHI renderer implementation for hardware-accelerated image display.
 * @author CaptureMoment Team
 * @date 2025
 *
 * @details
 * This file provides the RHIImageItemRenderer class which handles all GPU-side
 * rendering operations for RHIImageItem. It operates on Qt's render thread and
 * manages the complete graphics pipeline for displaying images.
 *
 * @par Threading Model
 * The renderer follows Qt's threaded rendering architecture:
 * - GUI Thread: RHIImageItem lives here, receives setImage() calls
 * - Render Thread: RHIImageItemRenderer lives here, performs GPU operations
 * - Synchronization: synchronize() copies data between threads safely
 *
 * @par Resource Ownership
 * All RHI resources (texture, sampler, buffers, pipeline) are owned via
 * unique_ptr and automatically cleaned up in the destructor.
 *
 * @see RHIImageItem for the QQuickItem frontend
 * @see QRhi documentation for Qt's rendering hardware interface
 */

#pragma once

#include <QQuickRhiItemRenderer>
#include <rhi/qrhi.h>
#include <QMutex>
#include <QPointF>
#include <QSize>
#include <vector>
#include <memory>

namespace CaptureMoment::Core::Common {
struct ImageRegion;
}

namespace CaptureMoment::UI::Rendering {

// Forward declaration
class RHIImageItem;

/**
 * @class RHIImageItemRenderer
 * @brief GPU renderer for RHIImageItem using Qt's Rendering Hardware Interface.
 *
 * @details
 * This class implements the rendering backend for RHIImageItem. It manages
 * all GPU resources and performs the actual drawing operations. The class
 * inherits from QQuickRhiItemRenderer and follows Qt's rendering model.
 *
 * @par Lifecycle
 * 1. Construction: Called when RHIImageItem is first rendered
 * 2. initialize(): Called when RHI context is ready
 * 3. synchronize(): Called each frame before render() on render thread
 * 4. render(): Called each frame to draw
 * 5. Destruction: Called when RHIImageItem is destroyed
 *
 * @par Data Flow
 * @code
 * RHIImageItem (GUI Thread)
 *      │
 *      │ setImage(unique_ptr<ImageRegion>)
 *      ▼
 * m_full_image (protected by mutex)
 *      │
 *      │ synchronize() - locks mutex, copies data
 *      ▼
 * m_pixel_data (RGBA8, render thread local)
 *      │
 *      │ render() - uploads to GPU
 *      ▼
 * m_texture (GPU memory)
 *      │
 *      ▼
 * Screen
 * @endcode
 *
 * @par Format Conversion
 * Input format: RGBA_F32 (4 channels, 32-bit float per channel)
 * GPU format:   RGBA8_UNORM (4 channels, 8-bit unsigned normalized)
 * Conversion happens in updateTextureFromImage() during synchronize().
 *
 * @note All methods except the constructor are called on the render thread
 * @warning Do not call GUI thread methods from render thread
 */
class RHIImageItemRenderer : public QQuickRhiItemRenderer {
public:
    // =========================================================================
    // Constructor & Destructor
    // =========================================================================

    /**
     * @brief Constructs the renderer with a reference to its parent item.
     *
     * @details
     * Initializes all member variables to their default states. The actual
     * RHI resource creation is deferred to initialize() when the RHI context
     * becomes available.
     *
     * @param item Pointer to the parent RHIImageItem. Must not be nullptr.
     *             The pointer remains valid for the lifetime of the renderer
     *             (Qt's scene graph guarantees this).
     *
     * @pre item != nullptr
     * @post m_item is set, all other resources are null
     *
     * @note Called on the render thread
     */
    explicit RHIImageItemRenderer(RHIImageItem* item);

    /**
     * @brief Destructor releasing all GPU resources.
     *
     * @details
     * Releases all RHI resources in the correct order. The unique_ptr
     * destructors handle the actual cleanup. QRhi will ensure resources
     * are properly released on the GPU.
     *
     * @note Called on the render thread
     * @note QRhi resources are released automatically via unique_ptr
     */
    ~RHIImageItemRenderer() override;

    // Deleted copy and move operations (GPU resources are non-copyable)
    RHIImageItemRenderer(const RHIImageItemRenderer&) = delete;
    RHIImageItemRenderer& operator=(const RHIImageItemRenderer&) = delete;
    RHIImageItemRenderer(RHIImageItemRenderer&&) = delete;
    RHIImageItemRenderer& operator=(RHIImageItemRenderer&&) = delete;

    // =========================================================================
    // QQuickRhiItemRenderer Interface
    // =========================================================================

    /**
     * @brief Initializes all GPU resources when the RHI context is ready.
     *
     * @details
     * Called by Qt exactly once when the RHI context becomes available.
     * Creates all necessary GPU resources:
     *
     * - Vertex buffer: 4 vertices for fullscreen quad
     * - Index buffer: 4 indices for triangle strip
     * - Uniform buffer: MVP matrix + zoom/pan
     * - Texture: Image storage (created on demand)
     * - Sampler: Linear filtering, clamp to edge
     * - Pipeline: Vertex/fragment shaders, blend state
     * - Shader Resource Bindings: Links resources to shaders
     *
     * @param cb Command buffer for initialization commands.
     *           Used for initial vertex/index buffer uploads.
     *
     * @pre rhi() returns valid QRhi pointer
     * @pre renderTarget() returns valid render target
     * @post All static resources are created and ready
     * @post m_initialized is set to true
     *
     * @note Called on the render thread
     * @note Called only once per renderer lifetime
     */
    void initialize(QRhiCommandBuffer* cb) override;

    /**
     * @brief Synchronizes data from the GUI thread to the render thread.
     *
     * @details
     * Called by Qt before render() on each frame. This is the ONLY safe
     * place to access data from RHIImageItem (which lives on GUI thread).
     *
     * Operations performed:
     * 1. Locks the image mutex from RHIImageItem
     * 2. Reads current zoom, pan, and image dimensions
     * 3. If image changed, converts RGBA_F32 to RGBA8
     * 4. Sets m_texture_needs_update flag if needed
     *
     * @par Thread Safety
     * This method is called on the render thread but safely accesses
     * GUI thread data via mutex protection. The mutex is held only
     * for the minimum necessary time.
     *
     * @param item Pointer to the RHIImageItem being synchronized.
     *             Same as m_item but provided by Qt for consistency.
     *
     * @pre item != nullptr
     * @post Render thread has current copy of display state
     * @post m_pixel_data contains current image data if changed
     *
     * @note Called on the render thread, but can safely access GUI thread data
     * @note Called every frame, so must be efficient
     */
    void synchronize(QQuickRhiItem* item) override;

    /**
     * @brief Performs the actual GPU rendering.
     *
     * @details
     * Called by Qt to render the item to the screen. This method:
     *
     * 1. Checks if texture needs upload, uploads if necessary
     * 2. Updates the uniform buffer with current zoom/pan
     * 3. Sets up the graphics pipeline
     * 4. Binds vertex/index buffers and shader resources
     * 5. Issues the draw call (indexed, 4 vertices = triangle strip)
     *
     * @par Render State
     * - Viewport: Set to item's bounding rectangle
     * - Scissor: Disabled
     * - Blend: Enabled (source-over alpha blending)
     * - Topology: Triangle strip
     *
     * @param cb Command buffer for recording draw commands.
     *           Provided by Qt's scene graph.
     *
     * @pre initialize() has been called successfully
     * @pre synchronize() has been called for this frame
     * @post The image is rendered to the current render target
     *
     * @note Called on the render thread
     * @note Called every frame when item is visible
     */
    void render(QRhiCommandBuffer* cb) override;

private:
    // =========================================================================
    // Private Helper Methods
    // =========================================================================

    /**
     * @brief Creates the vertex and index buffers for the fullscreen quad.
     *
     * @details
     * Creates geometry for a quad that fills the entire viewport:
     *
     * @par Vertex Layout
     * Position (vec2) + TexCoord (vec2) = 4 floats per vertex
     *
     * @par Quad Vertices (NDC coordinates)
     * @code
     * (-1, 1)----(1, 1)
     *    |  \      |
     *    |    \    |
     *    |      \  |
     * (-1,-1)----(1,-1)
     * @endcode
     *
     * @par Index Buffer
     * Triangle strip: [0, 1, 2, 3] (4 indices)
     *
     * @param cb Command buffer for uploading geometry data to GPU.
     *
     * @pre rhi() returns valid QRhi pointer
     * @post m_vertex_buffer and m_index_buffer are created and uploaded
     */
    void createGeometry(QRhiCommandBuffer* cb);

    /**
     * @brief Creates the graphics pipeline for rendering.
     *
     * @details
     * Creates the QRhiGraphicsPipeline with:
     *
     * @par Shader Stage
     * - Vertex: image_display.vert.qsb
     * - Fragment: image_display.frag.qsb
     *
     * @par Vertex Input
     * - Binding 0: Position (vec2, offset 0)
     * - Binding 1: TexCoord (vec2, offset 8)
     *
     * @par Blend State
     * - Enabled for alpha blending
     * - Source: SRC_ALPHA
     * - Destination: ONE_MINUS_SRC_ALPHA
     *
     * @par Topology
     * - TriangleStrip (4 vertices)
     *
     * @pre m_vertex_buffer is created
     * @pre m_uniform_buffer is created
     * @post m_pipeline is created and ready for use
     * @post m_sampler and m_texture are created
     * @post m_srb (shader resource bindings) is created
     */
    void createPipeline();

    /**
     * @brief Converts ImageRegion data to GPU-ready format.
     *
     * @details
     * Converts the float32 RGBA image data to uint8 RGBA8 format
     * suitable for GPU texture upload. The conversion includes:
     *
     * - Value clamping: 0.0f to 1.0f range
     * - Scaling: Multiply by 255.0f
     * - Type conversion: float to uint8_t
     *
     * @par Input Format
     * - Source: RGBA_F32 (4 x float32 per pixel)
     * - Size: width * height * 4 * sizeof(float)
     *
     * @par Output Format
     * - Destination: RGBA8 (4 x uint8 per pixel)
     * - Size: width * height * 4 bytes
     *
     * @param image The source ImageRegion to convert. Must have 4 channels.
     *
     * @pre image.isValid() == true
     * @pre image.channels() == 4
     * @post m_pixel_data contains converted RGBA8 data
     * @post m_pixel_data_size matches image dimensions
     * @post m_texture is recreated if dimensions changed
     */
    void updateTextureFromImage(const Core::Common::ImageRegion& image);

    // =========================================================================
    // Member Variables
    // =========================================================================

    /**
     * @brief Pointer to the parent RHIImageItem.
     *
     * @details
     * This pointer is set during construction and remains valid for the
     * renderer's entire lifetime. Qt's scene graph ensures the item outlives
     * the renderer.
     *
     * @warning Only access during synchronize(), never during render()
     * @warning The item lives on the GUI thread, renderer on render thread
     */
    RHIImageItem* m_item{nullptr};

    // -------------------------------------------------------------------------
    // RHI Resources (GPU)
    // -------------------------------------------------------------------------

    /**
     * @brief GPU texture storing the image data.
     *
     * @details
     * Format: QRhiTexture::RGBA8 (4 channels, 8 bits per channel)
     * Size: Matches current image dimensions
     * Usage: Sampled in fragment shader for display
     *
     * @note Recreated when image dimensions change
     */
    std::unique_ptr<QRhiTexture> m_texture;

    /**
     * @brief Texture sampler for filtering mode.
     *
     * @details
     * Configuration:
     * - Minification: Linear
     * - Magnification: Linear
     * - Mipmap: None
     * - Address U/V: Clamp to Edge
     */
    std::unique_ptr<QRhiSampler> m_sampler;

    /**
     * @brief Vertex buffer for the fullscreen quad.
     *
     * @details
     * Contains 4 vertices, each with:
     * - Position (vec2): NDC coordinates (-1 to 1)
     * - TexCoord (vec2): UV coordinates (0 to 1)
     *
     * Total size: 4 vertices * 4 floats * 4 bytes = 64 bytes
     */
    std::unique_ptr<QRhiBuffer> m_vertex_buffer;

    /**
     * @brief Index buffer for the fullscreen quad.
     *
     * @details
     * Contains 4 indices forming a triangle strip.
     * Size: 4 indices * 2 bytes = 8 bytes (uint16)
     */
    std::unique_ptr<QRhiBuffer> m_index_buffer;

    /**
     * @brief Uniform buffer for shader parameters.
     *
     * @details
     * Contains transformation matrix:
     * - MVP matrix (mat4): 64 bytes
     *
     * Total: 256 bytes (aligned for GPU requirements)
     */
    std::unique_ptr<QRhiBuffer> m_uniform_buffer;

    /**
     * @brief Shader resource bindings linking resources to shaders.
     *
     * @details
     * Binding layout:
     * - Binding 0: Uniform buffer (vertex + fragment stages)
     * - Binding 1: Texture + sampler (fragment stage only)
     */
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;

    /**
     * @brief Graphics pipeline for rendering.
     *
     * @details
     * Contains complete pipeline state:
     * - Shader stages (vertex + fragment)
     * - Vertex input layout
     * - Blend state (alpha blending enabled)
     * - Rasterizer state (no culling)
     * - Render pass compatibility
     * - Topology: Triangle strip
     */
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    // -------------------------------------------------------------------------
    // Render State (Thread-Local Copy)
    // -------------------------------------------------------------------------

    /**
     * @brief Current image width in pixels.
     *
     * @details
     * Copied from RHIImageItem during synchronize(). Used to compute
     * the transformation matrix in render().
     */
    int m_image_width{0};

    /**
     * @brief Current image height in pixels.
     *
     * @details
     * Copied from RHIImageItem during synchronize(). Used to compute
     * the transformation matrix in render().
     */
    int m_image_height{0};

    /**
     * @brief Current zoom level.
     *
     * @details
     * Copied from RHIImageItem during synchronize(). Used to compute
     * the transformation matrix in render().
     *
     * @par Range
     * - Minimum: 0.1 (10% zoom)
     * - Maximum: 10.0 (1000% zoom)
     * - Default: 1.0 (100%)
     */
    float m_zoom{1.0f};

    /**
     * @brief Current pan offset in image coordinates.
     *
     * @details
     * Copied from RHIImageItem during synchronize(). Used to compute
     * the transformation matrix in render().
     *
     * @par Coordinate System
     * - Origin (0, 0): Top-left of image
     * - Positive X: Right
     * - Positive Y: Down
     */
    QPointF m_pan{0, 0};

    // -------------------------------------------------------------------------
    // Pixel Data (CPU Staging)
    // -------------------------------------------------------------------------

    /**
     * @brief Converted pixel data in RGBA8 format ready for GPU upload.
     *
     * @details
     * This buffer holds the image data after conversion from float32 to uint8.
     * It is populated in updateTextureFromImage() during synchronize() and
     * uploaded to m_texture during render().
     *
     * @par Memory Layout
     * Row-major order: [R0, G0, B0, A0, R1, G1, B1, A1, ...]
     * Size: width * height * 4 bytes
     *
     * @note Kept between frames to avoid reallocation when image doesn't change
     */
    std::vector<uint8_t> m_pixel_data;

    /**
     * @brief Dimensions of the current pixel data.
     *
     * @details
     * Stores the width and height of the data in m_pixel_data.
     * Used to detect if texture needs recreation when dimensions change.
     */
    QSize m_pixel_data_size;

    /**
     * @brief Flag indicating texture needs GPU upload.
     *
     * @details
     * Set to true when:
     * - New image is set via setImage()
     * - Image content changes
     * - Texture dimensions change
     *
     * Cleared after texture upload in render().
     *
     * @note Optimization to avoid unnecessary GPU uploads
     */
    bool m_texture_needs_update{false};

    /**
     * @brief Flag indicating RHI resources are initialized.
     *
     * @details
     * Set to true after initialize() completes successfully.
     * Used to prevent rendering before resources are ready.
     */
    bool m_initialized{false};
};

} // namespace CaptureMoment::UI::Rendering
