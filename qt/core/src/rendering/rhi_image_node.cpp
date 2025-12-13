/**
 * @file rhi_image_node.cpp
 * @brief Implementation of RHIImageNode for direct QRhi rendering
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/rhi_image_node.h"
#include "rendering/rhi_image_item.h"
#include <spdlog/spdlog.h>
#include <QQuickWindow>
#include <QMatrix4x4>
#include <QMutexLocker>
#include <algorithm>
#include <cstring>
#include <QFile>

namespace CaptureMoment::UI::Rendering {

    RHIImageNode::RHIImageNode(RHIImageItem* item)
        : m_item(item) {
        spdlog::debug("RHIImageNode: Created");
    }

    RHIImageNode::~RHIImageNode() {
        spdlog::debug("RHIImageNode: Destroyed (resources cleaned up by unique_ptr)");
    }

    void RHIImageNode::synchronize() {
        if (!m_initialized) {
            m_rhi = m_item->window()->rhi();
            
            if (!m_rhi) {
                spdlog::error("RHIImageNode::synchronize: No QRhi available");
                return;
            }
            
            spdlog::info("RHIImageNode::synchronize: Using {} backend",
                        static_cast<int>(m_rhi->backend()));
            
            initialize();
        }
        
        {
            QMutexLocker lock(&m_item->m_image_mutex);
            if (m_item->m_texture_needs_update && m_item->m_full_image) {
                updateTexture();
                m_item->m_texture_needs_update = false;
            }
        }
    }

    void RHIImageNode::render(const RenderState* state) {
        if (!m_initialized || !m_rhi || !m_pipeline) {
            spdlog::warn("RHIImageNode::render: Not initialized");
            return;
        }
        
        QRhiCommandBuffer* cb = commandBuffer();
        if (!cb) {
            spdlog::error("RHIImageNode::render: No command buffer");
            return;
        }
        
        const QSize fbSize(m_item->width(), m_item->height());
        
        // Prepare transformation matrix
        QMatrix4x4 matrix = *state->projectionMatrix();
        matrix.ortho(0, fbSize.width(), fbSize.height(), 0, -1, 1);
        matrix.translate(m_item->m_pan.x(), m_item->m_pan.y());
        matrix.scale(m_item->m_zoom);
        
        // Update uniform buffer
        QRhiResourceUpdateBatch* batch = m_rhi->nextResourceUpdateBatch();
        batch->updateDynamicBuffer(m_uniform_buffer.get(), 0, 64, matrix.constData());
        cb->resourceUpdate(batch);
        
        // Begin render pass
        const QColor clearColor(0, 0, 0, 1);
        cb->beginPass(renderTarget(), clearColor, {1.0f, 0});
        
        // Set pipeline state
        cb->setGraphicsPipeline(m_pipeline.get());
        cb->setViewport(QRhiViewport(0, 0, fbSize.width(), fbSize.height()));
        cb->setShaderResources(m_srb.get());
        
        // Bind vertex input - Qt 6.9 API
        QRhiCommandBuffer::VertexInput vertexInput = { m_vertex_buffer.get(), 0 };
        cb->setVertexInput(0, 1, &vertexInput, m_index_buffer.get(), 0, QRhiCommandBuffer::IndexUInt16);
        
        // Draw
        cb->drawIndexed(6);  // 2 triangles = 6 indices
        
        // End render pass
        cb->endPass();
        
        spdlog::trace("RHIImageNode::render: Frame rendered");
    }

    void RHIImageNode::initialize() {
        createGeometry();
        createPipeline();
        
        m_initialized = true;
        spdlog::info("RHIImageNode::initialize: Complete");
    }

    void RHIImageNode::createGeometry() {
        struct Vertex {
            float x, y; // Position
            float u, v; // Texture coordinate
        };
        
        Vertex vertices[] = {
            { 0, 0, 0, 0 },
            { (float)m_item->imageWidth(), 0, 1, 0 },
            { (float)m_item->imageWidth(), (float)m_item->imageHeight(), 1, 1 },
            { 0, (float)m_item->imageHeight(), 0, 1 }
        };
        
        uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
        
        // Create vertex buffer using QRhi factory method
        m_vertex_buffer.reset(m_rhi->newBuffer(
            QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices)
        ));
        if (!m_vertex_buffer || !m_vertex_buffer->create()) {
            spdlog::error("RHIImageNode::createGeometry: Failed to create vertex buffer");
            return;
        }
        
        // Create index buffer using QRhi factory method
        m_index_buffer.reset(m_rhi->newBuffer(
            QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indices)
        ));
        if (!m_index_buffer || !m_index_buffer->create()) {
            spdlog::error("RHIImageNode::createGeometry: Failed to create index buffer");
            return;
        }
        
        // Create uniform buffer using QRhi factory method
        m_uniform_buffer.reset(m_rhi->newBuffer(
            QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64
        ));
        if (!m_uniform_buffer || !m_uniform_buffer->create()) {
            spdlog::error("RHIImageNode::createGeometry: Failed to create uniform buffer");
            return;
        }
        
        // Upload static buffers using QRhiResourceUpdateBatch
        QRhiResourceUpdateBatch* batch = m_rhi->nextResourceUpdateBatch();
        batch->uploadStaticBuffer(m_vertex_buffer.get(), vertices);
        batch->uploadStaticBuffer(m_index_buffer.get(), indices);
        // Submitting the batch is typically handled by the render loop/QSG internals.
        // m_rhi->resourceUpdateBatchApply(batch); // Only if needed for immediate submission outside render loop (rare in QSGRenderNode).

        spdlog::debug("RHIImageNode::createGeometry: Geometry created");
    }

    void RHIImageNode::createPipeline() {
        // --- Load Shaders using QShader::fromSerialized (Qt 6.9 compatible) ---
        // 1. Load Vertex Shader
        QFile vsFile(":/shaders/image_display_vs.qsb"); // Adjust path to your compiled vertex shader resource
        if (!vsFile.open(QIODevice::ReadOnly)) {
            spdlog::error("RHIImageNode::createPipeline: Failed to open vertex shader resource file: {}", vsFile.fileName().toStdString());
            return; // Exit if file cannot be opened
        }
        QByteArray vsData = vsFile.readAll(); // Read the binary .qsb data
        // Deserialize the binary data into a QShader object for the vertex stage.
        QShader vs = QShader::fromSerialized(vsData); // Assuming m_vs is a member variable of type QShader
        if (!vs.isValid()) {
            spdlog::error("RHIImageNode::createPipeline: Failed to deserialize vertex shader from '{}'", vsFile.fileName().toStdString());
            return; // Exit if deserialization fails
        }
        spdlog::debug("RHIImageNode::createPipeline: Vertex shader loaded successfully from '{}'", vsFile.fileName().toStdString());

        // 2. Load Fragment Shader
        QFile fsFile(":/shaders/image_display_fs.qsb"); // Adjust path to your compiled fragment shader resource
        if (!fsFile.open(QIODevice::ReadOnly)) {
            spdlog::error("RHIImageNode::createPipeline: Failed to open fragment shader resource file: {}", fsFile.fileName().toStdString());
            return; // Exit if file cannot be opened
        }
        QByteArray fsData = fsFile.readAll(); // Read the binary .qsb data
        // Deserialize the binary data into a QShader object for the fragment stage.
        QShader fs = QShader::fromSerialized(fsData); // Assuming m_fs is a member variable of type QShader
        if (!fs.isValid()) {
            spdlog::error("RHIImageNode::createPipeline: Failed to deserialize fragment shader from '{}'", fsFile.fileName().toStdString());
            return; // Exit if deserialization fails
        }
        spdlog::debug("RHIImageNode::createPipeline: Fragment shader loaded successfully from '{}'", fsFile.fileName().toStdString());


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
        spdlog::debug("RHIImageNode::createPipeline: Sampler created.");


        // --- Create Placeholder Texture (if not already created by updateTexture or setImage) ---
        // This is just an initial texture. updateTexture/setImage will eventually provide the correct size/data.
        // Or, you might want to defer texture creation until setImage is called.
        if (!m_texture) {
            // Use a default size or size from m_item if available *after* setImage
            int width = m_item ? m_item->imageWidth() : 256;  // Fallback size
            int height = m_item ? m_item->imageHeight() : 256; // Fallback size
            if (width <= 0 || height <= 0) {
                width = 256;
                height = 256;
            }

            // Create texture using QRhi factory method
            m_texture.reset(m_rhi->newTexture(
                QRhiTexture::RGBA8, // Common format
                QSize(width, height)
            ));
            if (!m_texture || !m_texture->create()) {
                spdlog::error("RHIImageNode::createPipeline: Failed to create initial placeholder texture ({}x{}).", width, height);
                return;
            }
            spdlog::debug("RHIImageNode::createPipeline: Placeholder texture created ({}x{}).", width, height);
        }


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
        spdlog::debug("RHIImageNode::createPipeline: Shader Resource Bindings created.");

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

        // Set the shader resource bindings for this pipeline
        m_pipeline->setShaderResourceBindings(m_srb.get());

        // IMPORTANT: Get the render pass descriptor from the current render target
        // This ensures the pipeline is compatible with the target (e.g., window swapchain, offscreen texture).
        // The 'state' parameter in render() or the render target obtained via commandBuffer()->renderTarget()
        // in older Qt versions usually provides this. Here, we assume 'renderTarget()' is available on the node.
        // This might need adjustment based on your exact render call setup.
        // Example (if renderTarget() is a method of QSGRenderNode or accessible context):
        // m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
        // OR if you have access to the window's render target descriptor during pipeline setup:
        // m_pipeline->setRenderPassDescriptor(m_windowRenderTargetDescriptor); // You'd need to pass/store this
        // For QSGRenderNode, it's usually implicitly linked via the render state in the render() call.
        // Let's assume the pipeline is set up within the correct context where the render target is known.
        // m_pipeline->setRenderPassDescriptor(/* obtained from render state */); // <-- You need this part

        // --- Create the pipeline object on the GPU ---
        if (!m_pipeline->create()) {
            spdlog::error("RHIImageNode::createPipeline: Failed to create graphics pipeline on GPU.");
            return;
        }

        spdlog::info("RHIImageNode::createPipeline: Graphics pipeline created successfully.");
    }

    void RHIImageNode::updateTexture() {
        QMutexLocker lock(&m_item->m_image_mutex);
        
        if (!m_item->m_full_image) {
            spdlog::warn("RHIImageNode::updateTexture: No image data");
            return;
        }
        
        uploadPixelData(m_item->m_full_image);
    }

    void RHIImageNode::uploadPixelData(const std::shared_ptr<ImageRegion>& image) {
        // --- Validate input data and QRhi instance ---
        if (!image || !m_rhi) return;

        // --- Convert pixel data format (float32 → uint8) ---
        // Prepare a buffer to hold the converted pixel data.
        std::vector<uint8_t> pixelData;
        // Resize the buffer to fit the entire image (width * height * 4 channels).
        pixelData.resize(image->m_width * image->m_height * 4);

        // Iterate through the source float data and convert/clamp each value to uint8.
        for (size_t i = 0; i < image->m_data.size(); ++i) {
            // Clamp the float value to the [0.0, 1.0] range.
            float clampedVal = std::clamp(image->m_data[i], 0.0f, 1.0f);
            // Scale the clamped value to the [0, 255] range and cast to uint8_t.
            pixelData[i] = static_cast<uint8_t>(clampedVal * 255.0f);
        }

        // --- Check if texture needs recreation (size changed) ---
        if (!m_texture || m_texture->pixelSize() != QSize(image->m_width, image->m_height)) {
            // --- Create a new GPU texture with the correct size using QRhi factory ---
            m_texture.reset(m_rhi->newTexture(
                QRhiTexture::RGBA8, // Specify the pixel format (8 bits per channel, Red-Green-Blue-Alpha).
                QSize(image->m_width, image->m_height) // Specify the texture dimensions.
            ));

            // Attempt to create the texture resource on the GPU.
            if (!m_texture || !m_texture->create()) {
                spdlog::error("RHIImageNode::uploadPixelData: Failed to create texture");
                return; // Exit if texture creation fails.
            }

            // --- Update Shader Resource Bindings (SRB) if needed ---
            // If the texture pointer changed, the SRB (which binds the texture to the shader) needs to be updated.
            if (m_srb) {
                // Define the new shader resource bindings, linking the updated texture and sampler.
                QRhiShaderResourceBinding bindings[] = {
                    // Bind the uniform buffer (e.g., for transformation matrices) to binding point 0 in the vertex stage.
                    QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage,
                                                             m_uniform_buffer.get()),
                    // Bind the new texture and its sampler to binding point 1 in the fragment stage.
                    QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                              m_texture.get(), m_sampler.get())
                };
                // Set the new bindings on the SRB object.
                m_srb->setBindings(bindings, bindings + 2); // +2 because there are 2 bindings defined.
                // Recreate the SRB object on the GPU to apply the changes.
                if (!m_srb->create()) {
                     spdlog::error("RHIImageNode::uploadPixelData: Failed to recreate Shader Resource Bindings after texture update");
                     // Depending on your error handling strategy, you might return or continue.
                     // The texture itself is updated, but the pipeline might not see the new one immediately.
                } else {
                    spdlog::debug("RHIImageNode::uploadPixelData: SRB updated with new texture.");
                }
            }
        }

        // --- Prepare GPU Texture Upload Commands (using QRhiResourceUpdateBatch) ---
        // Obtain a batch object to queue the upload command.
        QRhiResourceUpdateBatch* batch = m_rhi->nextResourceUpdateBatch();

        // --- Configure the texture upload description for Qt 6.9 API ---
        // 1. Describe the *specific part* (subresource) of the texture to update.
        QRhiTextureSubresourceUploadDescription subDesc;
        // Set the pixel data buffer (converted from ImageRegion) to be uploaded.
        subDesc.setData(QByteArray(
            reinterpret_cast<const char*>(pixelData.data()), // Source data pointer (cast to char*)
            pixelData.size()                                 // Size of the data buffer in bytes
        ));
        // Set the top-left corner in the destination texture where the data should be placed (offset 0, 0).
        // NOTE: Use setDestinationTopLeft (not setDestinationTopLeftInPixels which is Qt 6.10+).
        subDesc.setDestinationTopLeft(QPoint(0, 0));

        // 2. Describe the *entry* for the upload batch, specifying the mip level and the subresource description.
        QRhiTextureUploadEntry entry;
        // Set the mip level to update (0 for the base level).
        entry.setLevel(0);
        // Associate the subresource description (containing data and destination offset) with this entry.
        entry.setDescription(subDesc);

        // 3. Create the *overall* upload description containing the entry.
        // NOTE: Use the constructor with the entry or setEntries() (not addEntry which is Qt 6.10+).
        QRhiTextureUploadDescription uploadDesc(entry); // Constructor with a single entry
        // Alternatively: QRhiTextureUploadDescription uploadDesc; uploadDesc.setEntries({entry});

        // --- Queue the upload command ---
        // Add the upload command (for the specified texture and upload description) to the batch.
        batch->uploadTexture(m_texture.get(), uploadDesc);
        // Note: The batch is typically submitted later by the render loop or Qt's scene graph.
        // For static initial uploads, you might use m_rhi->resourceUpdateBatchApply(batch) if necessary,
        // but this is often handled automatically by the system.

        // --- Log successful upload ---
        spdlog::debug("RHIImageNode::uploadPixelData: Uploaded {}x{} to GPU",
                     image->m_width, image->m_height);
    }

} // namespace CaptureMoment::Qt::Rendering