#pragma once

#include <vulkan_engine.h>
#include <unordered_map>
#include <model_loader.h>

#define BLOCK_NAME Transforms
#define BLOCK \
UNIFORM_BUFFER(camera, 0, Stage::Vertex) \
DYNAMIC_UNIFORM(model, 1, Stage::Vertex)
#include <gen_bindings.h>

#define BLOCK_NAME DefaultColorAttachments
#define BLOCK \
COLOR(color, LoadOp::Load) \
DEPTH(depth, LoadOp::Load) 
#include <gen_attachments.h>

enum struct PipelineType {
    DefaultColor
};

struct MaterialDatabase
{
    std::unordered_map<PipelineType, Ref<GraphicsPipeline>> _pipelines;

    MaterialDatabase(RenderContext& renderContext): m_renderContext(renderContext) {
        depthFormat = renderContext.Get<Device>().SelectSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    Ref<GraphicsPipeline> GetPipeline(PipelineType pipeline) {
        Ref<GraphicsPipeline> result = _pipelines[pipeline];
        
        if (result.isNull()) {
            result = CreatePipeline(pipeline);
            _pipelines[pipeline] = result;
        }

        return result;
    }

    VkFormat depthFormat;

private: 

    RenderContext& m_renderContext;

    Ref<GraphicsPipeline> CreatePipeline(PipelineType type) {
        switch (type) {
            case PipelineType::DefaultColor:
                return CreateDefaultColor();
            default:
                ASSERT_MSG(false, "unknown pipeline type");
        }
    }

    Ref<GraphicsPipeline> CreateDefaultColor() {
        ShaderBinary vertexBin = m_renderContext.Get<ShaderLoader>().Get("shaders/basic.vert", Stage::Vertex);
        ShaderBinary fragmentBin = m_renderContext.Get<ShaderLoader>().Get("shaders/basic.frag", Stage::Fragment);
        
        return m_renderContext
            .Get<GraphicsFeature>().NewGraphicsPipeline()
            .AddVertex<Vertex>()
            .AddLayout<Transforms>()
            .SetAttachments<DefaultColorAttachments>(DefaultColorAttachments::Formats{
                .color = m_renderContext.Get<PresentFeature>().swapChain->format,
                .depth = depthFormat
            })
            .AddShaderStage(vertexBin)
            .AddShaderStage(fragmentBin)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
            .Build();
    }
};
