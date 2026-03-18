/**
 * @file rhi_image_item_renderer.cpp
 * @brief RHI renderer implementation.
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/rhi_image_item_renderer.h"
#include "rendering/rhi_image_item.h"
#include "common/image_region.h"

#include <spdlog/spdlog.h>
#include <QMutexLocker>
#include <QFile>
#include <QMatrix4x4>
#include <algorithm>

namespace CaptureMoment::UI::Rendering {

// =============================================================================
// Constructor & Destructor
// =============================================================================

RHIImageItemRenderer::RHIImageItemRenderer(RHIImageItem* item)
    : m_item(item)
{
    spdlog::debug("[RHIImageItemRenderer::RHIImageItemRenderer]: Created");
}

RHIImageItemRenderer::~RHIImageItemRenderer() = default;

// =============================================================================
// QQuickRhiItemRenderer Interface
// =============================================================================

void RHIImageItemRenderer::initialize(QRhiCommandBuffer* cb)
{
    if (m_initialized) {
        return;
    }

    if (!rhi()) {
        spdlog::error("[RHIImageItemRenderer::initialize]: No RHI context available");
        return;
    }

    spdlog::debug("[RHIImageItemRenderer::initialize]: Starting initialization...");

    // Create all GPU resources
    createGeometry(cb);
    createPipeline();

    // Verify all resources were created successfully
    if (m_vertex_buffer && m_index_buffer && m_uniform_buffer && m_pipeline && m_sampler && m_srb) {
        m_initialized = true;
        spdlog::info("[RHIImageItemRenderer::initialize]: Initialized successfully");
    } else {
        spdlog::error("[RHIImageItemRenderer::initialize]: Initialization failed - missing resources");
    }
}

void RHIImageItemRenderer::synchronize(QQuickRhiItem* item)
{
    spdlog::debug("[RHIImageItemRenderer::synchronize]: Synchronizing...");

    auto* rhi_item = static_cast<RHIImageItem*>(item);
    if (!rhi_item) {
        spdlog::warn("[RHIImageItemRenderer::synchronize]: Null item");
        return;
    }

    // Lock mutex and copy state from GUI thread
    QMutexLocker lock(rhi_item->getImageMutex());

    // Copy display parameters
    m_zoom = rhi_item->zoom();
    m_pan = rhi_item->pan();

    spdlog::debug("[RHIImageItemRenderer::synchronize]: zoom={}, pan=({}, {})",
                  m_zoom, m_pan.x(), m_pan.y());

    // Get the current image
    const auto* image = rhi_item->getFullImage();

    // Check if texture needs update:
    // 1. Flag was set by setImage() or updateTile()
    // 2. Or we have an image but texture dimensions don't match
    const bool needs_update = rhi_item->m_texture_needs_update;
    const bool size_mismatch = image && image->isValid() &&
                               (!m_texture || m_texture->pixelSize() != QSize(image->width(), image->height()));

    if (needs_update || size_mismatch) {
        if (image && image->isValid()) {
            m_image_width = static_cast<int>(image->width());
            m_image_height = static_cast<int>(image->height());

            spdlog::debug("[RHIImageItemRenderer::synchronize]: Converting image {}x{}",
                          m_image_width, m_image_height);

            updateTextureFromImage(*image);
            m_texture_needs_update = true;
        }
        // Clear the item's flag
        rhi_item->m_texture_needs_update = false;
    }
}

void RHIImageItemRenderer::render(QRhiCommandBuffer* cb)
{
    spdlog::error("[RHIImageItemRenderer::render]: RENDER CALLED");

    if (!m_initialized || !m_pipeline || !cb) {
        spdlog::warn("[RHIImageItemRenderer::render]: Not ready (initialized={}, pipeline={})",
                     m_initialized, static_cast<bool>(m_pipeline));
        return;
    }

    if (!renderTarget()) {
        spdlog::warn("[RHIImageItemRenderer::render]: No render target");
        return;
    }

    // Upload texture data if needed
    if (m_texture_needs_update && m_texture && !m_pixel_data.empty()) {
        QRhiResourceUpdateBatch* resourceUpdates = rhi()->nextResourceUpdateBatch();
        if (resourceUpdates) {
            QRhiTextureSubresourceUploadDescription subresDesc(
                m_pixel_data.data(),
                static_cast<quint32>(m_pixel_data.size())
                );

            QRhiTextureUploadDescription desc(QRhiTextureUploadEntry(0, 0, subresDesc));
            resourceUpdates->uploadTexture(m_texture.get(), desc);
            cb->resourceUpdate(resourceUpdates);
            m_texture_needs_update = false;

            spdlog::debug("[RHIImageItemRenderer::render]: Texture uploaded {}x{}",
                          m_pixel_data_size.width(), m_pixel_data_size.height());
        }
    }

    // Build transformation matrix
    // The quad vertices are in [0,1] range, so we scale to image dimensions
    // then apply zoom and pan transformations
    QMatrix4x4 matrix;
    QSize rt_size = renderTarget()->pixelSize();

    // Orthographic projection: (0,0) at top-left, matches Qt Quick coordinate system
    matrix.ortho(0.0f, static_cast<float>(rt_size.width()),
                 static_cast<float>(rt_size.height()), 0.0f, -1.0f, 1.0f);

    // Apply transformations in correct order (right to left in matrix multiplication):
    // 1. Scale quad from [0,1] to image dimensions [0, width] x [0, height]
    // 2. Apply zoom around origin (0, 0)
    // 3. Apply pan translation
    matrix.translate(static_cast<float>(m_pan.x()), static_cast<float>(m_pan.y()));
    matrix.scale(m_zoom, m_zoom);
    matrix.scale(static_cast<float>(m_image_width), static_cast<float>(m_image_height));


    spdlog::debug("[RHIImageItemRenderer::render]: image={}x{}, zoom={}, pan=({}, {}), viewport={}x{}",
                  m_image_width, m_image_height, m_zoom, m_pan.x(), m_pan.y(),
                  rt_size.width(), rt_size.height());

    // Update uniform buffer
    if (m_uniform_buffer) {
        QRhiResourceUpdateBatch* batch = rhi()->nextResourceUpdateBatch();
        if (batch) {
            batch->updateDynamicBuffer(m_uniform_buffer.get(), 0, 64, matrix.constData());
            cb->resourceUpdate(batch);
        }
    }

    // Begin render pass - use dark gray background to distinguish from rendering issues
    cb->beginPass(renderTarget(), QColor(30, 30, 30, 255), {1.0f, 0});

    // Set graphics pipeline
    cb->setGraphicsPipeline(m_pipeline.get());

    // Set viewport to render target size
    cb->setViewport(QRhiViewport(0, 0, rt_size.width(), rt_size.height()));

    // Bind shader resources (uniform buffer + texture sampler)
    cb->setShaderResources(m_srb.get());

    // Bind vertex and index buffers
    QRhiCommandBuffer::VertexInput vertex_input{m_vertex_buffer.get(), 0};
    cb->setVertexInput(0, 1, &vertex_input, m_index_buffer.get(), 0, QRhiCommandBuffer::IndexUInt16);

    // Draw quad (6 indices for 2 triangles)
    cb->drawIndexed(6);

    cb->endPass();
}

void RHIImageItemRenderer::createGeometry(QRhiCommandBuffer* cb)
{
    // Vertex structure: position (x, y) + texcoord (u, v)
    struct Vertex {
        float x, y;  // Position in [0,1] range
        float u, v;  // Texture coordinates
    };

    // Quad vertices in [0,1] position range
    // Texture coordinates: V=0 at top, V=1 at bottom (matches Qt Quick Y-down coordinate system)
    //
    // (0,0)----(1,0)
    //    |   /   |
    //    |  /    |
    //    | /     |
    // (0,1)----(1,1)
    //
    // Vertex layout for two triangles:
    // Triangle 1: v0, v1, v2 (top-left, top-right, bottom-right)
    // Triangle 2: v0, v2, v3 (top-left, bottom-right, bottom-left)
    Vertex vertices[] = {
        {0.0f, 0.0f, 0.0f, 0.0f},  // v0: top-left
        {1.0f, 0.0f, 1.0f, 0.0f},  // v1: top-right
        {1.0f, 1.0f, 1.0f, 1.0f},  // v2: bottom-right
        {0.0f, 1.0f, 0.0f, 1.0f}   // v3: bottom-left
    };

    // Two triangles with explicit indices (more reliable than triangle strip)
    // Triangle 1: v0 -> v1 -> v2
    // Triangle 2: v0 -> v2 -> v3
    uint16_t indices[] = {
        0, 1, 2,  // First triangle (top-left, top-right, bottom-right)
        0, 2, 3   // Second triangle (top-left, bottom-right, bottom-left)
    };

    // Create buffers
    m_vertex_buffer.reset(rhi()->newBuffer(
        QRhiBuffer::Immutable,
        QRhiBuffer::VertexBuffer,
        sizeof(vertices)
        ));

    m_index_buffer.reset(rhi()->newBuffer(
        QRhiBuffer::Immutable,
        QRhiBuffer::IndexBuffer,
        sizeof(indices)
        ));

    // Uniform buffer: 256 bytes (aligned for GPU requirements)
    m_uniform_buffer.reset(rhi()->newBuffer(
        QRhiBuffer::Dynamic,
        QRhiBuffer::UniformBuffer,
        256
        ));

    if (!m_vertex_buffer->create()) {
        spdlog::error("[RHIImageItemRenderer::createGeometry]: Failed to create vertex buffer");
        return;
    }

    if (!m_index_buffer->create()) {
        spdlog::error("[RHIImageItemRenderer::createGeometry]: Failed to create index buffer");
        return;
    }

    if (!m_uniform_buffer->create()) {
        spdlog::error("[RHIImageItemRenderer::createGeometry]: Failed to create uniform buffer");
        return;
    }

    // CRITICAL: Upload vertex/index data via command buffer
    if (cb) {
        QRhiResourceUpdateBatch* batch = rhi()->nextResourceUpdateBatch();
        if (batch) {
            batch->uploadStaticBuffer(m_vertex_buffer.get(), vertices);
            batch->uploadStaticBuffer(m_index_buffer.get(), indices);
            cb->resourceUpdate(batch);
            spdlog::debug("[RHIImageItemRenderer::createGeometry]: Geometry buffers uploaded");
        } else {
            spdlog::error("[RHIImageItemRenderer::createGeometry]: Failed to get resource update batch");
        }
    } else {
        spdlog::error("[RHIImageItemRenderer::createGeometry]: No command buffer provided");
    }
}

void RHIImageItemRenderer::createPipeline()
{
    // Load compiled shaders (.qsb files)
    QFile vs_file(":/shaders/glsl/image_display.vert.qsb");
    QFile fs_file(":/shaders/glsl/image_display.frag.qsb");

    if (!vs_file.open(QIODevice::ReadOnly)) {
        spdlog::error("[RHIImageItemRenderer::createPipeline]: Failed to open vertex shader: {}",
                      vs_file.fileName().toStdString());
        return;
    }

    if (!fs_file.open(QIODevice::ReadOnly)) {
        spdlog::error("[RHIImageItemRenderer::createPipeline]: Failed to open fragment shader: {}",
                      fs_file.fileName().toStdString());
        return;
    }

    QShader vs = QShader::fromSerialized(vs_file.readAll());
    QShader fs = QShader::fromSerialized(fs_file.readAll());

    if (!vs.isValid()) {
        spdlog::error("[RHIImageItemRenderer::createPipeline]: Invalid vertex shader");
        return;
    }

    if (!fs.isValid()) {
        spdlog::error("[RHIImageItemRenderer::createPipeline]: Invalid fragment shader");
        return;
    }

    spdlog::debug("[RHIImageItemRenderer::createPipeline]: Shaders loaded successfully");

    // Create texture sampler
    m_sampler.reset(rhi()->newSampler(
        QRhiSampler::Linear,      // mag filter
        QRhiSampler::Linear,      // min filter
        QRhiSampler::None,        // mipmap mode
        QRhiSampler::ClampToEdge, // address U
        QRhiSampler::ClampToEdge  // address V
        ));

    if (!m_sampler->create()) {
        spdlog::error("[RHIImageItemRenderer::createPipeline]: Failed to create sampler");
        return;
    }

    // Create placeholder texture (1x1, will be replaced when image is loaded)
    m_texture.reset(rhi()->newTexture(QRhiTexture::RGBA8, QSize(1, 1)));

    if (!m_texture->create()) {
        spdlog::error("[RHIImageItemRenderer::createPipeline]: Failed to create placeholder texture");
        return;
    }

    // Create shader resource bindings
    m_srb.reset(rhi()->newShaderResourceBindings());

    QRhiShaderResourceBinding bindings[] = {
        // Binding 0: Uniform buffer (vertex + fragment stages)
        QRhiShaderResourceBinding::uniformBuffer(
            0,
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_uniform_buffer.get()
            ),
        // Binding 1: Sampled texture (fragment stage only)
        QRhiShaderResourceBinding::sampledTexture(
            1,
            QRhiShaderResourceBinding::FragmentStage,
            m_texture.get(),
            m_sampler.get()
            )
    };

    m_srb->setBindings(bindings, bindings + 2);

    if (!m_srb->create()) {
        spdlog::error("[RHIImageItemRenderer::createPipeline]: Failed to create shader resource bindings");
        return;
    }

    // Create graphics pipeline
    m_pipeline.reset(rhi()->newGraphicsPipeline());

    // Set shader stages
    m_pipeline->setShaderStages({
        QRhiShaderStage{QRhiShaderStage::Vertex, vs},
        QRhiShaderStage{QRhiShaderStage::Fragment, fs}
    });

    // Vertex input layout
    QRhiVertexInputLayout input_layout;
    input_layout.setBindings({
        QRhiVertexInputBinding(4 * sizeof(float))  // 4 floats per vertex: x, y, u, v
    });
    input_layout.setAttributes({
        QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0),           // position
        QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float))  // texcoord
    });

    m_pipeline->setVertexInputLayout(input_layout);
    m_pipeline->setShaderResourceBindings(m_srb.get());

    // Set render pass descriptor
    if (!renderTarget()) {
        spdlog::error("[RHIImageItemRenderer::createPipeline]: No render target available");
        return;
    }

    m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());

    // Enable alpha blending for proper image display
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_pipeline->setTargetBlends({blend});

    // Use Triangles topology (explicit two triangles, more reliable than triangle strip)
    m_pipeline->setTopology(QRhiGraphicsPipeline::Triangles);

    // Create the pipeline
    if (!m_pipeline->create()) {
        spdlog::error("[RHIImageItemRenderer::createPipeline]: Failed to create graphics pipeline");
        return;
    }

    spdlog::debug("[RHIImageItemRenderer::createPipeline]: Pipeline created successfully");
}

void RHIImageItemRenderer::updateTextureFromImage(const Core::Common::ImageRegion& image)
{
    const int w = static_cast<int>(image.width());
    const int h = static_cast<int>(image.height());

    spdlog::debug("[RHIImageItemRenderer::updateTextureFromImage]: Converting {}x{} (channels: {})",
                  w, h, image.channels());

    // Resize pixel data buffer (RGBA8 = 4 bytes per pixel)
    m_pixel_data.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4);
    m_pixel_data_size = QSize(w, h);

    // Convert from RGBA_F32 (float) to RGBA8 (uint8)
    const float* src = image.getBuffer().data();
    uint8_t* dst = m_pixel_data.data();
    const size_t total_pixels = static_cast<size_t>(w) * static_cast<size_t>(h);

    for (size_t i = 0; i < total_pixels; ++i) {
        const size_t idx = i * 4;

        // Clamp to [0,1] range and convert to [0,255]
        dst[idx + 0] = static_cast<uint8_t>(std::clamp(src[idx + 0], 0.0f, 1.0f) * 255.0f);  // R
        dst[idx + 1] = static_cast<uint8_t>(std::clamp(src[idx + 1], 0.0f, 1.0f) * 255.0f);  // G
        dst[idx + 2] = static_cast<uint8_t>(std::clamp(src[idx + 2], 0.0f, 1.0f) * 255.0f);  // B
        dst[idx + 3] = static_cast<uint8_t>(std::clamp(src[idx + 3], 0.0f, 1.0f) * 255.0f);  // A
    }

    // Recreate texture if size changed
    if (!m_texture || m_texture->pixelSize() != m_pixel_data_size) {
        m_texture.reset(rhi()->newTexture(QRhiTexture::RGBA8, m_pixel_data_size));

        if (!m_texture->create()) {
            spdlog::error("[RHIImageItemRenderer::updateTextureFromImage]: Failed to create texture {}x{}",
                          w, h);
            return;
        }

        spdlog::debug("[RHIImageItemRenderer::updateTextureFromImage]: Texture created {}x{}", w, h);

        // Recreate shader resource bindings with new texture
        m_srb.reset(rhi()->newShaderResourceBindings());

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

        if (!m_srb->create()) {
            spdlog::error("[RHIImageItemRenderer::updateTextureFromImage]: Failed to recreate SRB");
        }
    }
}

} // namespace CaptureMoment::UI::Rendering
