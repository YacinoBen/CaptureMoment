/**
 * @file rhi_image_node.cpp
 * @brief Implementation of RHIImageNode for direct QRhi rendering with extensive logging
 * @author CaptureMoment Team
 * @date 2025
 */

#include <QQuickWindow>
#include <QMatrix4x4>
#include <QMutexLocker>
#include <QFile>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cstring>
#include "rendering/rhi_image_node.h"
#include "rendering/rhi_image_item.h"

namespace CaptureMoment::UI::Rendering {
/*
RHIImageNode::RHIImageNode(RHIImageItem* item)
        : m_item(item) {
        spdlog::debug("RHIImageNode: Created");
    }

RHIImageNode::~RHIImageNode() {
        spdlog::debug("RHIImageNode: Destroyed (resources cleaned up by unique_ptr)");
    }

void RHIImageNode::synchronize()
{
    spdlog::info("RHIImageNode::synchronize: Start");

    if (!m_item) {
        spdlog::error("RHIImageNode::synchronize: m_item is null");
        return;
    }
    spdlog::debug("RHIImageNode::synchronize: m_item is valid ");

    if (!m_initialized)
    {
        spdlog::info("RHIImageNode::synchronize: Initialization required");
        if (!m_item->window()) {
            spdlog::error("RHIImageNode::synchronize: m_item->window() is null during initialization");
            return;
        }
        m_rhi = m_item->window()->rhi();
        if (!m_rhi) {
            spdlog::info("RHIImageNode::synchronize (m_rhi) : No QRhi available during initialization");
            return;
        }
        spdlog::info("RHIImageNode::synchronize (m_rhi): Using {} backend", static_cast<int>(m_rhi->backend()));
        initialize();
        if (!m_initialized) {
            spdlog::error("RHIImageNode::synchronize: Initialization failed, cannot proceed.");
            return;
        }
    } else {
        spdlog::info("RHIImageNode::synchronize: Already initialized, skipping init.");
    }

    // Check if texture update is needed
    bool needs_update = false;
    {
        QMutexLocker lock(&m_item->m_image_mutex);
        spdlog::debug("RHIImageNode::synchronize (QMutexLocker) : Lock acquired");
        spdlog::debug("RHIImageNode::synchronize (QMutexLocker) : m_item->m_full_image is {}", m_item->m_full_image ? "valid" : "null");
        spdlog::debug("RHIImageNode::synchronize (QMutexLocker) : m_texture_needs_update is {}", m_item->m_texture_needs_update);

        if (m_item->m_texture_needs_update && m_item->m_full_image)
        {
            needs_update = true;
            spdlog::info("RHIImageNode::synchronize (QMutexLocker) : Texture update flag is set, starting update");
            // Copy image data locally to avoid holding mutex during GPU upload
            updateTexture(); // This function holds the lock internally
            m_item->m_texture_needs_update = false;
            spdlog::info("RHIImageNode::synchronize (QMutexLocker) : Texture update flag reset");
        }
    }

    if (needs_update) {
        spdlog::info("RHIImageNode::synchronize: Texture was updated, SRB needs rebuild.");
        m_srb_needs_rebuild = true; // Mark SRB for rebuild in render
    } else {
        spdlog::info("RHIImageNode::synchronize: No texture update needed.");
    }

    // --- Uniform buffer update moved to render() ---
    // The uniform buffer (for transformations) should be updated in the render() function
    // on the render thread, ideally within the same frame as the draw call.

    spdlog::info("RHIImageNode::synchronize: End");
}

void RHIImageNode::render(const RenderState* state)
{
    spdlog::info("RHIImageNode::render: Start");

    if (!m_initialized || !m_rhi || !m_pipeline) {
        spdlog::warn("RHIImageNode::render: Not initialized or missing resources (m_rhi: {}, m_pipeline: {})",
                     m_rhi ? "valid" : "null", m_pipeline ? "valid" : "null");
        return;
    }
    spdlog::info("RHIImageNode::render: Initialized and resources are valid");

    QRhiCommandBuffer* cb = commandBuffer();
    if (!cb) {
        spdlog::error("RHIImageNode::render (cb): No command buffer available");
        return;
    }
    spdlog::info("RHIImageNode::render (cb): Command buffer acquired");

    // --- Rebuild SRB if needed (NEW) ---
    if (m_srb_needs_rebuild && m_texture && m_sampler && m_uniform_buffer) {
        spdlog::info("RHIImageNode::render: Rebuilding SRB with new texture");

        // Redéfinir les bindings avec la nouvelle texture
        QRhiShaderResourceBinding bindings[] = {
            QRhiShaderResourceBinding::uniformBuffer(
                0,
                QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                m_uniform_buffer.get()
            ),
            QRhiShaderResourceBinding::sampledTexture(
                1,
                QRhiShaderResourceBinding::FragmentStage,
                m_texture.get(),
                m_sampler.get()
            )
        };

        m_srb->setBindings(bindings, bindings + 2);

        // Recréer le SRB AVANT beginPass (OK pour D3D11)
        if (!m_srb->create()) {
            spdlog::error("RHIImageNode::render: Failed to rebuild SRB");
            return;
        }

        spdlog::info("RHIImageNode::render: SRB rebuilt successfully");
        m_srb_needs_rebuild = false;
    } else {
        spdlog::info("RHIImageNode::render: SRB does not need rebuild or resources are missing (srb: {}, tex: {}, samp: {}, ubo: {})",
                     m_srb ? "valid" : "null", m_texture ? "valid" : "null", m_sampler ? "valid" : "null", m_uniform_buffer ? "valid" : "null");
    }

    // Prepare transformation matrix
    QMatrix4x4 matrix;
    // Use ortho to map from item coordinates (0,0) to (width, height) to clip space (-1, 1)
    // The item's size might be different from the image size.
    QSize winSize = m_item->window()->size(); // Use window size for ortho if drawing full screen
    // If drawing within the item's bounds:
    // matrix.ortho(0, m_item->width(), m_item->height(), 0, -1, 1);
    // If drawing within the window bounds (more common for full item coverage):
    matrix.ortho(0, winSize.width(), winSize.height(), 0, -1, 1);
    // Apply pan and zoom transformations
    matrix.translate(m_item->m_pan.x(), m_item->m_pan.y());
    matrix.scale(m_item->m_zoom);

    spdlog::info("RHIImageNode::render: Setting uniform buffer with matrix (pan: {}, {}, zoom: {})",
                 m_item->m_pan.x(), m_item->m_pan.y(), m_item->m_zoom);

    // Update uniform buffer on the render thread
    if (m_uniform_buffer) {
        QRhiResourceUpdateBatch* batch = m_rhi->nextResourceUpdateBatch();
        if (batch) {
            batch->updateDynamicBuffer(m_uniform_buffer.get(), 0, 64, matrix.constData());
            cb->resourceUpdate(batch); // Submit the update batch
            spdlog::info("RHIImageNode::render: Uniform buffer updated and submitted");
        } else {
            spdlog::error("RHIImageNode::render: Failed to get resource update batch for uniform buffer");
        }
    } else {
        spdlog::error("RHIImageNode::render: Uniform buffer is null, cannot update");
    }

    // Begin render pass
    const QColor clearColor(0, 0, 0, 1); // Black background
    cb->beginPass(renderTarget(), clearColor, {1.0f, 0});
    spdlog::info("RHIImageNode::render: Render pass begun");

    // Submit any pending texture upload batch if it exists
    if (m_pending_upload_batch) {
        spdlog::info("RHIImageNode::render: Submitting pending texture upload batch");
        cb->resourceUpdate(m_pending_upload_batch);
        m_pending_upload_batch = nullptr; // Clear the pending batch after submission
    }

    // Set pipeline state
    cb->setGraphicsPipeline(m_pipeline.get());
    spdlog::info("RHIImageNode::render: Pipeline set");

    // Set viewport to match the render target size (often the window or item size)
    QSize rtSize = renderTarget()->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, rtSize.width(), rtSize.height()));
    spdlog::info("RHIImageNode::render: Viewport set to {}x{}", rtSize.width(), rtSize.height());

    // Set shader resources (SRB)
    cb->setShaderResources(m_srb.get());
    spdlog::info("RHIImageNode::render: Shader resources set");

    // Bind vertex input - Qt 6.9 API
    QRhiCommandBuffer::VertexInput vertexInput = { m_vertex_buffer.get(), 0 };
    cb->setVertexInput(0, 1, &vertexInput, m_index_buffer.get(), 0, QRhiCommandBuffer::IndexUInt16);
    spdlog::info("RHIImageNode::render: Vertex input set");

    // Draw
    cb->drawIndexed(6);  // 2 triangles = 6 indices
    spdlog::info("RHIImageNode::render: Draw call executed (6 indices)");

    // End render pass
    cb->endPass();
    spdlog::info("RHIImageNode::render: Render pass ended");

    spdlog::trace("RHIImageNode::render: Frame rendered");
}

void RHIImageNode::initialize()
{
    spdlog::info("RHIImageNode::initialize: Start");

    if (!m_rhi) {
        spdlog::error("RHIImageNode::initialize: QRhi is null, cannot initialize resources.");
        return;
    }

    createGeometry();
    if (!m_vertex_buffer || !m_index_buffer || !m_uniform_buffer) {
        spdlog::error("RHIImageNode::initialize: createGeometry() failed, buffers are null.");
        return;
    }
    spdlog::info("RHIImageNode::initialize: Geometry created successfully");

    createPipeline();
    if (!m_pipeline || !m_srb) {
        spdlog::error("RHIImageNode::initialize: createPipeline() failed, pipeline or SRB are null.");
        return;
    }
    spdlog::info("RHIImageNode::initialize: Pipeline created successfully");

    // Only mark as initialized if EVERYTHING succeeded
    m_initialized = true;
    spdlog::info("RHIImageNode::initialize: Complete - Ready to render");
}

void RHIImageNode::createGeometry() {
    spdlog::info("RHIImageNode::createGeometry: Start");

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
    m_vertex_buffer.reset(m_rhi->newBuffer(
        QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices)
    ));
    if (!m_vertex_buffer || !m_vertex_buffer->create()) {
        spdlog::error("RHIImageNode::createGeometry: Failed to create vertex buffer");
        return;
    }
    spdlog::info("RHIImageNode::createGeometry (m_vertex_buffer): created");

    // Create index buffer using QRhi factory method
    m_index_buffer.reset(m_rhi->newBuffer(
            QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indices)
    ));
    if (!m_index_buffer || !m_index_buffer->create()) {
        spdlog::error("RHIImageNode::createGeometry: Failed to create index buffer");
        return;
    }
    spdlog::info("RHIImageNode::createGeometry (m_index_buffer): created");

    // Create uniform buffer using QRhi factory method
    m_uniform_buffer.reset(m_rhi->newBuffer(
        QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 // 16 floats * 4 bytes = 64 bytes for a 4x4 matrix
    ));
    if (!m_uniform_buffer || !m_uniform_buffer->create()) {
        spdlog::error("RHIImageNode::createGeometry: Failed to create uniform buffer");
        return;
    }
    spdlog::info("RHIImageNode::createGeometry (m_uniform_buffer): created");

    // Upload static buffers using QRhiResourceUpdateBatch
    QRhiResourceUpdateBatch* batch = m_rhi->nextResourceUpdateBatch();
    if (!batch) {
        spdlog::error("RHIImageNode::geometry(batch): No command batch");
        return;
    }
    spdlog::info("RHIImageNode::geometry (batch): batch found");
    batch->uploadStaticBuffer(m_vertex_buffer.get(), vertices);
    batch->uploadStaticBuffer(m_index_buffer.get(), indices);
    // Submitting the batch is typically handled by the render loop/QSG internals.

    spdlog::info("RHIImageNode::createGeometry: Geometry created and buffers uploaded");
}

void RHIImageNode::createPipeline()
{
    spdlog::info("RHIImageNode::createPipeline: Start");

    // --- Load Shaders using QShader::fromSerialized (Qt 6.9 compatible) ---
    // 1. Load Vertex Shader
    QFile vsFile(":/shaders/glsl/image_display.vert.qsb"); // Adjust path to your compiled vertex shader resource
    if (!vsFile.open(QIODevice::ReadOnly)) {
        spdlog::error("RHIImageNode::createPipeline: Failed to open vertex shader resource file: {}", vsFile.fileName().toStdString());
        return; // Exit if file cannot be opened
    }
    spdlog::info("RHIImageNode::createPipeline: Vertex shader loaded successfully from '{}'", vsFile.fileName().toStdString());
    QByteArray vsData = vsFile.readAll(); // Read the binary .qsb data
    // Deserialize the binary data into a QShader object for the vertex stage.
    QShader vs = QShader::fromSerialized(vsData);
    if (!vs.isValid()) {
        spdlog::error("RHIImageNode::createPipeline: Failed to deserialize vertex shader from '{}'", vsFile.fileName().toStdString());
        return; // Exit if deserialization fails
    }
    spdlog::info("RHIImageNode::createPipeline: Vertex shader loaded successfully from '{}'", vsFile.fileName().toStdString());

    // 2. Load Fragment Shader
    QFile fsFile(":/shaders/glsl/image_display.frag.qsb"); // Adjust path to your compiled fragment shader resource
    if (!fsFile.open(QIODevice::ReadOnly)) {
        spdlog::error("RHIImageNode::createPipeline: Failed to open fragment shader resource file: {}", fsFile.fileName().toStdString());
        return; // Exit if file cannot be opened
    }
    QByteArray fsData = fsFile.readAll(); // Read the binary .qsb data
    // Deserialize the binary data into a QShader object for the fragment stage.
    QShader fs = QShader::fromSerialized(fsData);
    if (!fs.isValid()) {
        spdlog::error("RHIImageNode::createPipeline: Failed to deserialize fragment shader from '{}'", fsFile.fileName().toStdString());
        return; // Exit if deserialization fails
    }
    spdlog::info("RHIImageNode::createPipeline: Fragment shader loaded successfully from '{}'", fsFile.fileName().toStdString());

    // --- Create Sampler using QRhi factory method ---
    m_sampler.reset(m_rhi->newSampler(
        QRhiSampler::Linear, QRhiSampler::Linear, // Minification and magnification filters
        QRhiSampler::None,                        // Mipmap mode (None for non-mipmapped textures)
        QRhiSampler::ClampToEdge,                 // Wrap mode for U coordinate
        QRhiSampler::ClampToEdge                  // Wrap mode for V coordinate
    ));
    if (!m_sampler || !m_sampler->create()) {
        spdlog::error("RHIImageNode::createPipeline: Failed to create sampler.");
        return;
    }
    spdlog::info("RHIImageNode::createPipeline: Sampler created.");

    // --- Create Placeholder Texture (initial size) ---
    // This is just an initial texture. updateTexture/setImage will eventually provide the correct size/data.
    // Use a small default size initially.
    int initial_width = 256;  // Fallback size
    int initial_height = 256; // Fallback size

    m_texture.reset(m_rhi->newTexture(
        QRhiTexture::RGBA8, // Common format
        QSize(initial_width, initial_height)
    ));
    if (!m_texture || !m_texture->create()) {
        spdlog::error("RHIImageNode::createPipeline: Failed to create initial placeholder texture ({}x{}).", initial_width, initial_height);
        return;
    }
    spdlog::info("RHIImageNode::createPipeline: Initial placeholder texture created ({}x{}).", initial_width, initial_height);

    // --- Create Shader Resource Bindings (SRB) using QRhi factory method ---
    m_srb.reset(m_rhi->newShaderResourceBindings());
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
        spdlog::error("RHIImageNode::createPipeline: Failed to create Shader Resource Bindings.");
        return;
    }
    spdlog::info("RHIImageNode::createPipeline: Shader Resource Bindings created.");

    // --- Create Graphics Pipeline Object using QRhi factory method ---
    m_pipeline.reset(m_rhi->newGraphicsPipeline());
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

    if (!m_item || !m_item->window()) {
        spdlog::error("RHIImageNode::createPipeline: No window available to get render pass descriptor");
        return;
    }

    QRhiSwapChain* sc = m_item->window()->swapChain();
    if (!sc) {
        spdlog::error("RHIImageNode::createPipeline: No swapchain available to get render pass descriptor");
        return;
    }

    QRhiRenderPassDescriptor* rpDesc = sc->renderPassDescriptor();
    if (!rpDesc) {
        spdlog::error("RHIImageNode::createPipeline: No render pass descriptor available");
        return;
    }

    m_pipeline->setRenderPassDescriptor(rpDesc);

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
        spdlog::error("RHIImageNode::createPipeline: Failed to create graphics pipeline on GPU.");
        return;
    }

    spdlog::info("RHIImageNode::createPipeline: Graphics pipeline created successfully.");
}

void RHIImageNode::updateTexture()
{
    spdlog::info("RHIImageNode::updateTexture: Start");

    // This function is called from synchronize, which already holds the lock.
    // We don't need to lock again here.
    // QMutexLocker lock(&m_item->m_image_mutex); // <--- Commenté car synchronisé en dehors

    if (!m_item->m_full_image) {
        spdlog::warn("RHIImageNode::updateTexture: No image data in m_full_image");
        return;
    }

    spdlog::info("RHIImageNode::updateTexture: Image data found, width: {}, height: {}, channels: {}",
                 m_item->m_full_image->m_width, m_item->m_full_image->m_height, m_item->m_full_image->m_channels);

    uploadPixelData(m_item->m_full_image);
    spdlog::info("RHIImageNode::updateTexture: End");
}

void RHIImageNode::uploadPixelData(const std::shared_ptr<Core::Common::ImageRegion>& image)
{
    spdlog::info("RHIImageNode::uploadPixelData: Start, image size: {}x{}", image->m_width, image->m_height);

    // --- Validate input data and QRhi instance ---
    if (!image || !m_rhi) {
        spdlog::warn("RHIImageNode::uploadPixelData: Invalid image ({}) or RHI ({})",
                     image ? "valid" : "null", m_rhi ? "valid" : "null");
        return;
    }

    // --- Convert pixel data format (float32 → uint8) ---
    // Prepare a buffer to hold the converted pixel data.
    std::vector<uint8_t> pixelData;
    // Resize the buffer to fit the entire image (width * height * 4 channels).
    pixelData.resize(image->m_width * image->m_height * 4);
    spdlog::info("RHIImageNode::uploadPixelData: Resized pixel data buffer to {}", pixelData.size());

    // Iterate through the source float data and convert/clamp each value to uint8.
    for (int y = 0; y < image->m_height; ++y) {
        for (int x = 0; x < image->m_width; ++x) {
            size_t baseIdx = (y * image->m_width + x) * image->m_channels;
            if (baseIdx + image->m_channels - 1 < image->m_data.size()) {
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
    spdlog::info("RHIImageNode::uploadPixelData: Conversion from float32 to uint8 completed");

    bool textureRecreated = false;

    // --- Check if texture needs recreation (size changed) ---
    if (!m_texture || m_texture->pixelSize() != QSize(image->m_width, image->m_height)) {
        spdlog::info("RHIImageNode::uploadPixelData: Recreating texture new size: {}x{}", image->m_width, image->m_height);

        // Créer la nouvelle texture
        m_texture.reset(m_rhi->newTexture(
            QRhiTexture::RGBA8,
            QSize(image->m_width, image->m_height)
        ));

        if (!m_texture || !m_texture->create()) {
            spdlog::error("RHIImageNode::uploadPixelData: Failed to create texture ({}x{})", image->m_width, image->m_height);
            return;
        }

        textureRecreated = true;
        spdlog::info("RHIImageNode::uploadPixelData: Texture ({}x{}) recreated successfully", image->m_width, image->m_height);

        // IMPORTANT: Mark SRB for rebuild in render() function
        m_srb_needs_rebuild = true;
        spdlog::info("RHIImageNode::uploadPixelData: SRB rebuild scheduled for next render call");
    } else {
        spdlog::info("RHIImageNode::uploadPixelData: Texture size matches, reusing existing texture");
    }

    // --- Prepare GPU Texture Upload Commands (using QRhiResourceUpdateBatch) ---
    // Obtain a batch object to queue the upload command.
    m_pending_upload_batch = m_rhi->nextResourceUpdateBatch();

    if (!m_pending_upload_batch) {
        spdlog::error("RHIImageNode::uploadPixelData: Failed to get resource update batch for texture upload");
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
    m_pending_upload_batch->uploadTexture(m_texture.get(), uploadDesc);
    spdlog::info("RHIImageNode::uploadPixelData: Upload command queued for texture ({}x{})", image->m_width, image->m_height);

    spdlog::info("RHIImageNode::uploadPixelData: End");
}*/

} // namespace CaptureMoment::UI::Rendering
