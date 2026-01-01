/**
 * @file rhi_image_item_renderer.cpp
 * @brief Renderer implementation for RHIImageItem using QQuickRhiItemRenderer
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/rhi_image_item_renderer.h"
#include "rendering/rhi_image_item.h"
#include <spdlog/spdlog.h>
#include <QMutexLocker>
#include <QQuickWindow>
#include <QFile>
#include <QMatrix4x4>
#include <algorithm>
#include <cstring>

namespace CaptureMoment::UI::Rendering {

// Constructor: Initializes the renderer.
RHIImageItemRenderer::RHIImageItemRenderer(RHIImageItem* item)
        : m_item(item) {
        spdlog::debug("RHIImageItemRenderer: Created");
}

// Destructor: Cleans up resources.
RHIImageItemRenderer::~RHIImageItemRenderer() {
        spdlog::debug("RHIImageItemRenderer: Destroyed (resources cleaned up by unique_ptr)");
}

// Initializes the RHI resources (pipeline, buffers, etc.).
void RHIImageItemRenderer::initialize(QRhiCommandBuffer *cb)
{
    spdlog::info("RHIImageItemRenderer::initialize: Start");

    if (!rhi()) {
        spdlog::error("RHIImageItemRenderer::initialize: QRhi is null, cannot initialize resources.");
        return;
    }

    createGeometry();
    if (!m_vertex_buffer || !m_index_buffer || !m_uniform_buffer) {
        spdlog::error("RHIImageItemRenderer::initialize: createGeometry() failed");
        return;
    }
    spdlog::info("RHIImageItemRenderer::initialize: Geometry created successfully");

    createPipeline();
    if (!m_pipeline || !m_srb) {
        spdlog::error("RHIImageItemRenderer::initialize: createPipeline() failed");
        return;
    }
    spdlog::info("RHIImageItemRenderer::initialize: Pipeline created successfully");

    // Only mark as initialized if EVERYTHING succeeded
    m_initialized = true;
    spdlog::info("RHIImageItemRenderer::initialize: Complete - Ready to render");
}

// Synchronizes state from the GUI thread to the render thread.
void RHIImageItemRenderer::synchronize(QQuickRhiItem *item)
{
    // Cast the item to our specific type
    RHIImageItem* rhiItem = static_cast<RHIImageItem*>(item);
    if (!rhiItem) {
        spdlog::error("RHIImageItemRenderer::synchronize: Invalid item cast");
        return;
    }

    spdlog::info("RHIImageItemRenderer::synchronize: Start");

    // Lock to access the item's data safely
    {
        QMutexLocker lock(rhiItem->getImageMutex()); // Use the mutex from the item

        spdlog::info("RHIImageItemRenderer::synchronize: enter lock rhiItem->textureNeedsUpdate(): {}", rhiItem->textureNeedsUpdate());
        // Check if texture needs update
        if (rhiItem->textureNeedsUpdate() && rhiItem->getFullImage())
        {
            spdlog::info("RHIImageItemRenderer::synchronize: Texture update required");
            updateTexture(); // This function should handle the conversion and upload
            rhiItem->setTextureNeedsUpdate(false);
            spdlog::info("RHIImageItemRenderer::synchronize: Texture update flag reset");
        }
        // If m_full_image is not valid, m_image_width/height might be 0 initially
        m_render_image_width = rhiItem->imageWidth();
        m_render_image_height = rhiItem->imageHeight();
    }

    // Lock to update our local render state (zoom, pan)
    {
        QMutexLocker lock(&m_render_state_mutex);
        // Copy zoom and pan from the item's state to our local render state
        // These are safe to read here because BaseImageItem provides thread-safe getters
        // (or they are accessed only on the main thread, so we assume they are stable during sync)
        m_render_zoom = rhiItem->zoom(); // Use getter from BaseImageItem
        m_render_pan = rhiItem->pan();   // Use getter from BaseImageItem
    }

    spdlog::info("RHIImageItemRenderer::synchronize: End");
}

// Performs the actual rendering commands using QRhi.
void RHIImageItemRenderer::render(QRhiCommandBuffer *cb)
{
    spdlog::info("RHIImageItemRenderer::render: Start");

    if (!m_initialized || !rhi() || !m_pipeline) {
        spdlog::warn("RHIImageItemRenderer::render: Not initialized");
        return;
    }

    // Retrieve render state safely
    float current_zoom;
    QPointF current_pan;
    int current_width, current_height;
    {
        QMutexLocker lock(&m_render_state_mutex);
        current_zoom = m_render_zoom;
        current_pan = m_render_pan;
        current_width = m_render_image_width;
        current_height = m_render_image_height;
    }

    // Prepare transformation matrix
    QMatrix4x4 matrix;
    // Use ortho to map from item coordinates (0,0) to (width, height) to clip space (-1, 1)
    // The item's size might be different from the image size.
    QSize rtSize = renderTarget()->pixelSize(); // Use render target size for ortho
    matrix.ortho(0, rtSize.width(), rtSize.height(), 0, -1, 1);
    // Apply pan and zoom transformations
    matrix.translate(current_pan.x(), current_pan.y());
    matrix.scale(current_zoom);

    spdlog::info("RHIImageItemRenderer::render: Setting uniform buffer with matrix (pan: {}, {}, zoom: {})",
                 current_pan.x(), current_pan.y(), current_zoom);

    // Update uniform buffer on the render thread
    if (m_uniform_buffer) {
        QRhiResourceUpdateBatch* batch = rhi()->nextResourceUpdateBatch();
        if (batch)
        {
            batch->updateDynamicBuffer(m_uniform_buffer.get(), 0, 64, matrix.constData());
            cb->resourceUpdate(batch); // Submit the update batch
            spdlog::info("RHIImageItemRenderer::render: Uniform buffer updated and submitted");
        } else {
            spdlog::error("RHIImageItemRenderer::render: Failed to get resource update batch for uniform buffer");
        }
    } else {
        spdlog::error("RHIImageItemRenderer::render: Uniform buffer is null, cannot update");
    }

    // Begin render pass
    const QColor clearColor(0, 0, 0, 1); // Black background
    cb->beginPass(renderTarget(), clearColor, {1.0f, 0});
    spdlog::info("RHIImageItemRenderer::render: Render pass begun");

    // Set pipeline state
    cb->setGraphicsPipeline(m_pipeline.get());
    spdlog::info("RHIImageItemRenderer::render: Pipeline set");

    // Set viewport to match the render target size
    cb->setViewport(QRhiViewport(0, 0, rtSize.width(), rtSize.height()));
    spdlog::info("RHIImageItemRenderer::render: Viewport set to {}x{}", rtSize.width(), rtSize.height());

    // Set shader resources (SRB)
    cb->setShaderResources(m_srb.get());
    spdlog::info("RHIImageItemRenderer::render: Shader resources set");

    // Bind vertex input
    QRhiCommandBuffer::VertexInput vertexInput = { m_vertex_buffer.get(), 0 };
    cb->setVertexInput(0, 1, &vertexInput, m_index_buffer.get(), 0, QRhiCommandBuffer::IndexUInt16);
    spdlog::info("RHIImageItemRenderer::render: Vertex input set");

    // Draw
    cb->drawIndexed(6);  // 2 triangles = 6 indices
    spdlog::info("RHIImageItemRenderer::render: Draw call executed (6 indices)");

    // End render pass
    cb->endPass();
    spdlog::info("RHIImageItemRenderer::render: Render pass ended");

    spdlog::trace("RHIImageItemRenderer::render: Frame rendered");
}

// Creates the vertex and index buffers for the geometry (e.g., a quad).
void RHIImageItemRenderer::createGeometry() {
    spdlog::info("RHIImageItemRenderer::createGeometry: Start");

    struct Vertex {
        float x, y; // Position
        float u, v; // Texture coordinate
    };
    // Define vertices for a quad covering the image size (initially 1x1, will be scaled by uniform matrix)
    // Texture coordinates are set for top-left origin (0,0) to bottom-right (1,1)
    Vertex vertices[] =
    {
        { 0.0f, 0.0f, 0.0f, 0.0f }, // Top-left
        { 1.0f, 0.0f, 1.0f, 0.0f }, // Top-right
        { 1.0f, 1.0f, 1.0f, 1.0f }, // Bottom-right
        { 0.0f, 1.0f, 0.0f, 1.0f }  // Bottom-left
    };
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 }; // Two triangles

    // Create vertex buffer using QRhi factory method
    m_vertex_buffer.reset(rhi()->newBuffer(
        QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices)
    ));
    if (!m_vertex_buffer || !m_vertex_buffer->create()) {
        spdlog::error("RHIImageItemRenderer::createGeometry: Failed to create vertex buffer");
        return;
    }
    spdlog::info("RHIImageItemRenderer::createGeometry (m_vertex_buffer): created");

    // Create index buffer using QRhi factory method
    m_index_buffer.reset(rhi()->newBuffer(
            QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indices)
    ));
    if (!m_index_buffer || !m_index_buffer->create()) {
        spdlog::error("RHIImageItemRenderer::createGeometry: Failed to create index buffer");
        return;
    }
    spdlog::info("RHIImageItemRenderer::createGeometry (m_index_buffer): created");

    // Create uniform buffer using QRhi factory method
    m_uniform_buffer.reset(rhi()->newBuffer(
        QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 // 16 floats * 4 bytes = 64 bytes for a 4x4 matrix
    ));
    if (!m_uniform_buffer || !m_uniform_buffer->create()) {
        spdlog::error("RHIImageItemRenderer::createGeometry: Failed to create uniform buffer");
        return;
    }
    spdlog::info("RHIImageItemRenderer::createGeometry (m_uniform_buffer): created");

    // Upload static buffers using QRhiResourceUpdateBatch
    QRhiResourceUpdateBatch* batch = rhi()->nextResourceUpdateBatch();
    if (!batch) {
        spdlog::error("RHIImageItemRenderer::createGeometry(batch): No command batch");
        return;
    }
    spdlog::info("RHIImageItemRenderer::createGeometry (batch): batch found");
    batch->uploadStaticBuffer(m_vertex_buffer.get(), vertices);
    batch->uploadStaticBuffer(m_index_buffer.get(), indices);
    // Submitting the batch is typically handled by the render loop/QSG internals.

    spdlog::info("RHIImageItemRenderer::createGeometry: Geometry created and buffers uploaded");
}

// Creates the graphics pipeline (shaders, bindings, render pass descriptor).
void RHIImageItemRenderer::createPipeline()
{
    spdlog::info("RHIImageItemRenderer::createPipeline: Start");

    // --- Load Shaders using QShader::fromSerialized (Qt 6.9 compatible) ---
    // 1. Load Vertex Shader
    QFile vsFile(":/shaders/glsl/image_display.vert.qsb"); // Adjust path to your compiled vertex shader resource
    if (!vsFile.open(QIODevice::ReadOnly)) {
        spdlog::error("RHIImageItemRenderer::createPipeline: Failed to open vertex shader resource file: {}", vsFile.fileName().toStdString());
        return; // Exit if file cannot be opened
    }
    spdlog::info("RHIImageItemRenderer::createPipeline: Vertex shader loaded successfully from '{}'", vsFile.fileName().toStdString());
    QByteArray vsData = vsFile.readAll(); // Read the binary .qsb data
    // Deserialize the binary data into a QShader object for the vertex stage.
    QShader vs = QShader::fromSerialized(vsData); // Assuming m_vs is a member variable of type QShader
    if (!vs.isValid()) {
        spdlog::error("RHIImageItemRenderer::createPipeline: Failed to deserialize vertex shader from '{}'", vsFile.fileName().toStdString());
        return; // Exit if deserialization fails
    }
    spdlog::info("RHIImageItemRenderer::createPipeline: Vertex shader loaded successfully from '{}'", vsFile.fileName().toStdString());
    // 2. Load Fragment Shader
    QFile fsFile(":/shaders/glsl/image_display.frag.qsb"); // Adjust path to your compiled fragment shader resource
    if (!fsFile.open(QIODevice::ReadOnly)) {
        spdlog::error("RHIImageItemRenderer::createPipeline: Failed to open fragment shader resource file: {}", fsFile.fileName().toStdString());
        return; // Exit if file cannot be opened
    }
    QByteArray fsData = fsFile.readAll(); // Read the binary .qsb data
    // Deserialize the binary data into a QShader object for the fragment stage.
    QShader fs = QShader::fromSerialized(fsData); // Assuming m_fs is a member variable of type QShader
    if (!fs.isValid()) {
        spdlog::error("RHIImageItemRenderer::createPipeline: Failed to deserialize fragment shader from '{}'", fsFile.fileName().toStdString());
        return; // Exit if deserialization fails
    }
    spdlog::debug("RHIImageItemRenderer::createPipeline: Fragment shader loaded successfully from '{}'", fsFile.fileName().toStdString());

    // --- Create Sampler using QRhi factory method ---
    m_sampler.reset(rhi()->newSampler(
        QRhiSampler::Linear, QRhiSampler::Linear, // Minification and magnification filters
        QRhiSampler::None,                        // Mipmap mode (None for non-mipmapped textures)
        QRhiSampler::ClampToEdge,                 // Wrap mode for U coordinate
        QRhiSampler::ClampToEdge                  // Wrap mode for V coordinate
    ));
    if (!m_sampler || !m_sampler->create()) {
        spdlog::error("RHIImageItemRenderer::createPipeline: Failed to create sampler.");
        return;
    }
    spdlog::info("RHIImageItemRenderer::createPipeline: Sampler created.");

    // --- Create Placeholder Texture (initial size) ---
    // This is just an initial texture. updateTexture/setImage will eventually provide the correct size/data.
    // Use a small default size initially.
    int initial_width = 256;  // Fallback size
    int initial_height = 256; // Fallback size

    m_texture.reset(rhi()->newTexture(
        QRhiTexture::RGBA8, // Common format
        QSize(initial_width, initial_height)
    ));
    if (!m_texture || !m_texture->create()) {
        spdlog::error("RHIImageItemRenderer::createPipeline: Failed to create initial placeholder texture ({}x{}).", initial_width, initial_height);
        return;
    }
    spdlog::info("RHIImageItemRenderer::createPipeline: Initial placeholder texture created ({}x{}).", initial_width, initial_height);

    // --- Create Shader Resource Bindings (SRB) using QRhi factory method ---
    m_srb.reset(rhi()->newShaderResourceBindings());
    // Define the bindings: uniform buffer (MVP matrix) and the texture+sampler pair.
    QRhiShaderResourceBinding bindings[] = {
        // Binding 0: Uniform buffer (e.g., containing the Model-View-Projection matrix)
        QRhiShaderResourceBinding::uniformBuffer(
            0, // Binding point number in the shader (layout(binding = 0) ... )
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, // Stages that use this buffer
            m_uniform_buffer.get() // Pointer to the QRhiBuffer object
            ),
    // Binding 1: Combined texture and sampler
        QRhiShaderResourceBinding::sampledTexture(
                1, // Binding point number in the shader (layout(binding = 1) ... )
                QRhiShaderResourceBinding::FragmentStage, // Stage that uses this texture/sampler
                m_texture.get(), // Pointer to the QRhiTexture object
                m_sampler.get()  // Pointer to the QRhiSampler object
            )
    };

    m_srb->setBindings(bindings, bindings + 2); // Set the array of bindings (size 2)
    if (!m_srb->create()) {
        spdlog::error("RHIImageItemRenderer::createPipeline: Failed to create Shader Resource Bindings.");
        return;
    }
    spdlog::debug("RHIImageItemRenderer::createPipeline: Shader Resource Bindings created.");

    // --- Create Graphics Pipeline Object using QRhi factory method ---
    m_pipeline.reset(rhi()->newGraphicsPipeline());
    // Set the loaded shaders
    m_pipeline->setShaderStages({
        QRhiShaderStage{ QRhiShaderStage::Vertex, vs },   // Use the deserialized vertex shader object
        QRhiShaderStage{ QRhiShaderStage::Fragment, fs }  // Use the deserialized fragment shader object
    });

    // Define the vertex input layout (how data flows from vertex buffer to shader)
    QRhiVertexInputLayout inputLayout;
    // Assuming a simple layout: position (2 floats) + texture coordinate (2 floats) = 4 floats total per vertex
    // Binding 0: Stride is 4 * sizeof(float)
    inputLayout.setBindings({
        QRhiVertexInputBinding(4 * sizeof(float)) // One stream of data, vertex size is 4 floats
    });
    // Attributes:
    // Attribute 0 (layout location = 0 in shader): 2 floats (x, y) starting at offset 0
    // Attribute 1 (layout location = 1 in shader): 2 floats (u, v) starting at offset 2*sizeof(float)
    inputLayout.setAttributes({
        QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0),                    // Position (vec2)
        QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float))    // Tex Coord (vec2)
    });
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_srb.get());

    m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor()); // Use the render target's RP descriptor

    // Optional: Configure blend state for transparency
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

    m_pipeline->setTargetBlends({ blend });

    // --- Create the pipeline object on the GPU ---
    if (!m_pipeline->create()) {
        spdlog::error("RHIImageItemRenderer::createPipeline: Failed to create graphics pipeline on GPU.");
        return;
    }

    spdlog::info("RHIImageItemRenderer::createPipeline: Graphics pipeline created successfully.");
}

// Updates the GPU texture if the image data has changed.
void RHIImageItemRenderer::updateTexture()
{
    // This function is called from synchronize, which already holds the item's lock.
    // We don't need to lock again here if synchronize does it.
    // QMutexLocker lock(&m_item->m_image_mutex); // <--- Commenté car synchronisé en dehors

    if (!m_item->getFullImage()) {
        spdlog::warn("RHIImageItemRenderer::updateTexture: No image data in m_item->m_full_image");
        return;
    }

    spdlog::info("RHIImageItemRenderer::updateTexture: Image data found, width: {}, height: {}, channels: {}",
                 m_item->getFullImage()->m_width, m_item->getFullImage()->m_height, m_item->getFullImage()->m_channels);

    uploadPixelData(m_item->getFullImage());
    spdlog::info("RHIImageItemRenderer::updateTexture: End");
}

// Uploads pixel data from an ImageRegion to the GPU texture.
void RHIImageItemRenderer::uploadPixelData(const std::shared_ptr<Core::Common::ImageRegion>& image)
{
    spdlog::info("RHIImageItemRenderer::uploadPixelData: Start, image size: {}x{}", image->m_width, image->m_height);

    // --- Validate input data and QRhi instance ---
    if (!image || !rhi()) {
        spdlog::warn("RHIImageItemRenderer::uploadPixelData: Invalid image ({}) or RHI ({})",
                     image ? "valid" : "null", rhi() ? "valid" : "null");
        return;
    }

    // --- Convert pixel data format (float32 → uint8) ---
    // Prepare a buffer to hold the converted pixel data.
    std::vector<uint8_t> pixelData;
    // Resize the buffer to fit the entire image (width * height * 4 channels).
    pixelData.resize(image->m_width * image->m_height * 4);
    spdlog::info("RHIImageItemRenderer::uploadPixelData: Resized pixel data buffer to {}", pixelData.size());

    // Iterate through the source float data and convert/clamp each value to uint8.
    for (int y = 0; y < image->m_height; ++y)
    {
        for (int x = 0; x < image->m_width; ++x)
        {
            size_t baseIdx = (y * image->m_width + x) * image->m_channels;
            if (baseIdx + image->m_channels - 1 < image->m_data.size())
            {
                float r = std::clamp(image->m_data[baseIdx + 0], 0.0f, 1.0f);
                float g = std::clamp(image->m_data[baseIdx + 1], 0.0f, 1.0f);
                float b = std::clamp(image->m_data[baseIdx + 2], 0.0f, 1.0f);
                float a = (image->m_channels == 4) ? std::clamp(image->m_data[baseIdx + 3], 0.0f, 1.0f) : 1.0f;

                size_t dstIdx = (y * image->m_width + x) * 4;
                pixelData[dstIdx + 0] = static_cast<uint8_t>(r * 255.0f);
                pixelData[dstIdx + 1] = static_cast<uint8_t>(g * 255.0f);
                pixelData[dstIdx + 2] = static_cast<uint8_t>(b * 255.0f);
                pixelData[dstIdx + 3] = static_cast<uint8_t>(a * 255.0f);
            }
        }
    }
    spdlog::info("RHIImageItemRenderer::uploadPixelData: Conversion from float32 to uint8 completed");

    bool textureRecreated = false;

    // --- Check if texture needs recreation (size changed) ---
    QSize currentTextureSize = m_texture ? m_texture->pixelSize() : QSize(0,0);

    if (!m_texture || currentTextureSize != QSize(image->m_width, image->m_height))
    {
        spdlog::info("RHIImageItemRenderer::uploadPixelData: Recreating texture, old size: {}x{}, new size: {}x{}",
                     currentTextureSize.width(), currentTextureSize.height(), image->m_width, image->m_height);

        // Créer la nouvelle texture
        m_texture.reset(rhi()->newTexture(
            QRhiTexture::RGBA8,
            QSize(image->m_width, image->m_height)
        ));

        if (!m_texture || !m_texture->create()) {
            spdlog::error("RHIImageItemRenderer::uploadPixelData: Failed to create texture ({}x{})", image->m_width, image->m_height);
            return;
        }

        textureRecreated = true;
        spdlog::info("RHIImageItemRenderer::uploadPixelData: Texture ({}x{}) recreated successfully", image->m_width, image->m_height);

        // IMPORTANT: Recreate the SRB because it references the old texture
        QRhiShaderResourceBinding bindings[] = {
            QRhiShaderResourceBinding::uniformBuffer(
                0, // Binding point number in the shader (layout(binding = 0) ... )
                QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, // Stages that use this buffer
                m_uniform_buffer.get() // Pointer to the QRhiBuffer object
            ),
            QRhiShaderResourceBinding::sampledTexture(
                1, // Binding point number in the shader (layout(binding = 1) ... )
                QRhiShaderResourceBinding::FragmentStage, // Stage that uses this texture/sampler
                m_texture.get(), // Pointer to the NEW QRhiTexture object
                m_sampler.get()  // Pointer to the QRhiSampler object
            )
        };

        m_srb->setBindings(bindings, bindings + 2); // Set the array of bindings (size 2)
        if (!m_srb->create()) {
            spdlog::error("RHIImageItemRenderer::uploadPixelData: Failed to recreate Shader Resource Bindings after texture update");
            return; // Exit if SRB recreation fails
        } else {
            spdlog::debug("RHIImageItemRenderer::uploadPixelData: SRB updated with new texture.");
        }

    } else {
        spdlog::info("RHIImageItemRenderer::uploadPixelData: Texture size matches, reusing existing texture");
    }

    // --- Prepare GPU Texture Upload Commands (using QRhiResourceUpdateBatch) ---
    // Obtain a batch object to queue the upload command.
    QRhiResourceUpdateBatch* batch = rhi()->nextResourceUpdateBatch();

    if (!batch) {
        spdlog::error("RHIImageItemRenderer::uploadPixelData: Failed to get resource update batch for texture upload");
        return;
    }

    // --- Configure the texture upload description for Qt 6.9 API ---
    // 1. Describe the *specific part* (subresource) of the texture to update.
    QRhiTextureSubresourceUploadDescription subDesc;
    // Set the pixel data buffer (converted from ImageRegion) to be uploaded.
    subDesc.setData(QByteArray(
        reinterpret_cast<const char*>(pixelData.data()), // Source data pointer (cast to char*)
        pixelData.size()                                 // Size of the data buffer in bytes
    ));
    // Set the top-left corner in the destination texture where the data should be placed (offset 0, 0).
    subDesc.setDestinationTopLeft(QPoint(0, 0));

    // 2. Describe the *entry* for the upload batch, specifying the mip level and the subresource description.
    QRhiTextureUploadEntry entry;
    // Set the mip level to update (0 for the base level).
    entry.setLevel(0);
    // Associate the subresource description (containing data and destination offset) with this entry.
    entry.setDescription(subDesc);

    // 3. Create the *overall* upload description containing the entry.
    QRhiTextureUploadDescription uploadDesc(entry); // Constructor with a single entry

    // --- Queue the upload command ---
    // Add the upload command (for the specified texture and upload description) to the batch.
    batch->uploadTexture(m_texture.get(), uploadDesc);

    // Submit the batch directly here, as it's during synchronize
    QRhiCommandBuffer* cb = nullptr;
    if (rhi()->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess)
    {
        cb->resourceUpdate(batch);
        rhi()->endOffscreenFrame();
    }

    spdlog::debug("RHIImageItemRenderer::uploadPixelData: Uploaded {}x{} to GPU",
                     image->m_width, image->m_height);
}

} // namespace CaptureMoment::UI::Rendering
