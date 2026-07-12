/**
 * @file working_image_gpu_halide.cpp
 * @brief Implementation of WorkingImageGPU_Halide.
 * @details Implements real GPU transfers via Halide runtime.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/gpu/working_image_gpu_halide.h"
#include "config/app_config.h"
#include <spdlog/spdlog.h>

#include "HalideRuntime.h"

namespace CaptureMoment::Core::ImageProcessing {

bool WorkingImageGPU_Halide::bindView(const ImageView& view)
{

    if (!IWorkingImageHardware::bindView(view)) {
        return false;
    }

    // Initialize the Halide buffer to reference the GPU data (zero-copy)
    initDataForHalide();

    if (!isHalideBufferValid()) {
        spdlog::error("[WorkingImageGPU_Halide::bindView]: Failed to init working buffer.");
        return false;
    }
    m_original_halide_buffer = Halide::Buffer<float>::make_interleaved(
        const_cast<float*>(m_view_data_image.original_data.data()),
        m_view_data_image.width,
        m_view_data_image.height,
        m_view_data_image.channels
        );
    m_original_halide_buffer.set_name("original_cpu_buffer");

    m_original_halide_buffer.set_host_dirty();
    m_halide_buffer.set_host_dirty();

    auto vram_result { transferToVRAM() };
    if (!vram_result.has_value()) {
        spdlog::error("[WorkingImageGPU_Halide::bindView]: Failed to transfer to VRAM.");
        return false;
    }
    spdlog::debug("[WorkingImageGPU_Halide::bindView]: Bound and initialized GPU Halide buffer ({}x{}).",
                  m_view_data_image.width, m_view_data_image.height);
    return true;
}

std::expected<void, ErrorHandling::CoreError>
WorkingImageGPU_Halide::transferToVRAM()
{
    if (m_pipelines_initialized) return {};

    try {
        Halide::Target target{Config::AppConfig::getHalideTarget()};
        Halide::Var x, y, c;

        // Pipeline Reset
        Halide::Func reset_func("gpu_reset");

        reset_func(x, y, c) = m_original_halide_buffer(x, y, c);
        reset_func.output_buffer().dim(0).set_stride(4);
        reset_func.output_buffer().dim(2).set_stride(1);
        reset_func.compile_jit(target);
        m_reset_pipeline = Halide::Pipeline(reset_func);

        if (!m_downsample_built) {
            buildDownsamplePipeline();
        }

        m_pipelines_initialized = true;
        spdlog::info("[WorkingImageGPU_Halide::transferToVRAM]: Pipelines compiled successfully.");
        return {};
    } catch (const std::exception& e) {
        spdlog::critical("[WorkingImageGPU_Halide::transferToVRAM]: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

bool WorkingImageGPU_Halide::downloadDeviceToHost()
{
    if (!m_pipelines_initialized) return false;

    int ret { m_halide_buffer.copy_to_host() };
    if (ret != 0) return false;

    m_halide_buffer.device_sync();
    return true;
}

void WorkingImageGPU_Halide::initDataForHalide()
{
    initializeHalide(m_view_data_image.working_data,
                         m_view_data_image.width,
                         m_view_data_image.height,
                         m_view_data_image.channels);
}

bool WorkingImageGPU_Halide::isValid() const {
    return WorkingImageGPU::isValid() && isHalideBufferValid();
}

void WorkingImageGPU_Halide::resetExecutionBuffer()
{
    if (!m_pipelines_initialized) return;

    Halide::Target target{Config::AppConfig::getHalideTarget()};
    m_reset_pipeline.realize(m_halide_buffer, target);
}
Halide::Buffer<float>& WorkingImageGPU_Halide::getExecutionBuffer()
{
    return m_halide_buffer;
}

// ==============================================================================
// DOWNSAMPLE PIPELINE
// ==============================================================================

static Halide::Expr kernel_cubic(Halide::Expr x){
    Halide::Expr xx { Halide::abs(x) };
    Halide::Expr xx2 { xx * xx };
    Halide::Expr xx3 { xx2 * xx };
    Halide::Expr a { -0.5f };

    return Halide::select(xx < 1.0f,
                          (a + 2.0f) * xx3 - (a + 3.0f) * xx2 + 1.0f,
                          Halide::select(xx < 2.0f,
                                         a * xx3 - 5.0f * a * xx2 + 8.0f * a * xx - 4.0f * a,
                                         0.0f));
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageGPU_Halide::downsample(Common::ImageDim target_width, Common::ImageDim target_height)
{
    if (!isValid()) {
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    if (target_width == 0 || target_height == 0) {
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    try {
        if (m_view_data_image.width == target_width && m_view_data_image.height == target_height) {
            return getFullResImage();
        }

        if (!m_downsample_built) {
            buildDownsamplePipeline();
        }

        spdlog::debug("[WorkingImageGPU_Halide::downsample]: Starting downsample from {}x{} to {}x{}.",
                      m_view_data_image.width, m_view_data_image.height, target_width, target_height);

        m_downsample_input.set(m_halide_buffer);

        std::vector<float> result_data(target_width * target_height * m_view_data_image.channels);
        Halide::Buffer<float> dst_buffer { Halide::Buffer<float>::make_interleaved(
            result_data.data(),
            static_cast<int>(target_width),
            static_cast<int>(target_height),
            static_cast<int>(m_view_data_image.channels)
            )};

        m_downsample_scale_x.set(static_cast<float>(target_width) / m_view_data_image.width);
        m_downsample_scale_y.set(static_cast<float>(target_height) / m_view_data_image.height);

        Halide::Target target{Config::AppConfig::getHalideTarget()};

        m_downsample_pipeline.realize(dst_buffer, target);
        dst_buffer.copy_to_host();

        spdlog::debug("[WorkingImageGPU_Halide::downsample]: Successful downsample.");

        return std::make_unique<Common::ImageRegion>(
            std::move(result_data),
            static_cast<int>(target_width),
            static_cast<int>(target_height),
            static_cast<int>(m_view_data_image.channels)
            );

    } catch (const std::bad_alloc&) {
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    } catch (const std::exception& e) {
        spdlog::critical("[WorkingImageGPU_Halide::downsample]: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

void WorkingImageGPU_Halide::buildDownsamplePipeline()
{
    if (m_downsample_built) return;

    m_downsample_input.dim(0).set_stride(4);
    m_downsample_input.dim(2).set_stride(1);

    Halide::Var x, y, c, k;
    Halide::Func clamped{Halide::BoundaryConditions::repeat_edge(m_downsample_input)};

    Halide::Expr inv_scale_x{Halide::strict_float(1.0f / m_downsample_scale_x)};
    Halide::Expr inv_scale_y{Halide::strict_float(1.0f / m_downsample_scale_y)};

    Halide::Expr sourcex{(Halide::cast<float>(x) + 0.5f) * inv_scale_x - 0.5f};
    Halide::Expr sourcey{(Halide::cast<float>(y) + 0.5f) * inv_scale_y - 0.5f};

    Halide::Expr fx{Halide::floor(sourcex)};
    Halide::Expr fy{Halide::floor(sourcey)};

    Halide::RDom r_x(0, 4, "r_x");
    Halide::RDom r_y(0, 4, "r_y");

    Halide::Func kx{"kx"};
    Halide::Func ky{"ky"};
    kx(x, k) = kernel_cubic(k + fx - sourcex);
    ky(y, k) = kernel_cubic(k + fy - sourcey);

    Halide::Func resized_y{"resized_y"};
    Halide::Func resized_x{"resized_x"};

    resized_y(x, y, c) = Halide::sum(ky(y, r_y) * clamped(x, Halide::cast<int>(fy) + r_y, c));
    resized_x(x, y, c) = Halide::sum(kx(x, r_x) * resized_y(Halide::cast<int>(fx) + r_x, y, c));

    Halide::Func final_output{"final_output"};
    final_output(x, y, c) = Halide::clamp(resized_x(x, y, c), 0.0f, 1.0f);

    final_output.output_buffer().dim(0).set_stride(4);
    final_output.output_buffer().dim(2).set_stride(1);

    Halide::Target target{Config::AppConfig::getHalideTarget()};

    if (target.has_gpu_feature())
    {
        Halide::Var tx{"tx"}, ty{"ty"};
        kx.compute_at(resized_x, tx);
        ky.compute_at(resized_y, tx);
        resized_y.compute_root().gpu_tile(x, y, tx, ty, 16, 16);
        resized_x.compute_root().gpu_tile(x, y, tx, ty, 16, 16);
        final_output.compute_root().gpu_tile(x, y, tx, ty, 16, 16);
    }

    m_downsample_pipeline = Halide::Pipeline(final_output);
    final_output.compile_jit(target);
    m_downsample_built = true;
}

} // namespace CaptureMoment::Core::ImageProcessing
