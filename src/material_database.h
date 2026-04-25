#pragma once

#include <vulkan_engine.h>
#include <unordered_map>
#include "model_loader.h"
#include "draw_context.h"

#define BLOCK_NAME DefaultColorAttachments
#define BLOCK \
COLOR(color, LoadOp::Load) \
DEPTH(depth, LoadOp::Load) 
#include <gen_attachments.h>

#define BLOCK_NAME DepthOnlyAttachments
#define BLOCK \
DEPTH(depth, LoadOp::Load)
#include <gen_attachments.h>

#define BLOCK_NAME ColorOnlyAttachments
#define BLOCK \
COLOR(color, LoadOp::Clear)
#include <gen_attachments.h>

enum struct PipelineType {
    DepthOnly,
    DepthShadowmap,
    DefaultColor,
    FullScreenQuad,
};

struct MaterialDatabase;

struct MaterialPass {

    std::vector<uint32_t> models;
    
    void (*setUpDraw)(MaterialDatabase&, const DrawContext&, MaterialPass&);
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

    static void DepthOnlyPass(MaterialDatabase& materialDatabase, const DrawContext& drawContext, MaterialPass& material) {
        GenericDepthPass(materialDatabase, drawContext, material, PipelineType::DepthOnly);
    }

    static void DirectShadowmapPass(MaterialDatabase& materialDatabase, const DrawContext& drawContext, MaterialPass& material) {
        GenericDepthPass(materialDatabase, drawContext, material, PipelineType::DepthShadowmap);
    }


    static void DefaultColorPass(MaterialDatabase& materialDatabase, const DrawContext& drawContext, MaterialPass& material) {

        auto& node = drawContext.context.Get<RenderGraph>().AddNode<GraphicsNode<
            DefaultColorAttachments, 
            Transforms,
            Textures,
            Lights
        >>(materialDatabase.GetPipeline(PipelineType::DefaultColor));

        node.SetName("Default Color Node");
        node.SetAttachments(
            DefaultColorAttachments{
                .color = drawContext.output,
                .depth = drawContext.depth
            }
        );
        
        MaterialDatabase::QueueDrawModels(node, drawContext, material.models, 
            [](const ModelData& model, const DrawContext& context) {

            Transforms transforms {
                .camera = context.mainViewCamera,
                .model = model.transformsRange
            };

            Textures textures {
                .textureIds = model.textureIds,
                .colorTexture = context.colorTextures[0],
                .colorTexture_sampler = context.linearSampler
            };

            Lights lights {
                .lightsConfig = context.lights,
                .directShadowmap = context.directShadowmap,
                .directShadowmap_sampler = context.linearSampler
            };

            return std::tuple {transforms, textures, lights};
        });
    }

    VkFormat depthFormat;

private: 

    static void GenericDepthPass(
        MaterialDatabase& materialDatabase, 
        const DrawContext& drawContext, 
        MaterialPass& material,
        PipelineType pipeline 
    ) {
        auto& node = drawContext.context.Get<RenderGraph>().AddNode<GraphicsNode<
            DepthOnlyAttachments, 
            Transforms
        >>(materialDatabase.GetPipeline(pipeline));

        node.SetName("Depth only node");
        node.SetAttachments(
            DepthOnlyAttachments{
                .depth = drawContext.depth
            }
        );
        
        MaterialDatabase::QueueDrawModels(node, drawContext, material.models, 
            [](const ModelData& model, const DrawContext& context) {

            Transforms transforms {
                .camera = context.mainViewCamera,
                .model = model.transformsRange
            };
            return std::tuple {transforms};
        });
    }

    RenderContext& m_renderContext;

    template<typename Attachments, typename... Bindings, typename Func>
    static void QueueDrawModels(
        GraphicsNode<Attachments, Bindings...>& node, 
        const DrawContext& drawContext,
        std::vector<uint32_t>& models, 
        Func&& getBindings
    ) {
        
        node.SetIndexBuffer(drawContext.indexBuffer, VkIndexType::VK_INDEX_TYPE_UINT32);
        node.AddVertexBuffer(drawContext.vertexBuffer);

        uint32_t i = 0;
        for (uint32_t index: models) {
            ModelData model = drawContext.models[index];

            std::tuple<Bindings...> parameters = getBindings(model, drawContext);

            ShaderDynamicState state = {};
            if (i++ == 0) {
                std::apply([&](auto... args) {node.SetBindings(args...);}, parameters);
            }
            else {
                ShaderDynamicState state = std::apply([&](auto... args) -> ShaderDynamicState {
                    return drawContext.context.Get<Descriptors>().GatherDynamicState(args...);
                }, parameters);
            }


            node.AddDrawParameters(DrawParameters{
                model.indexSize, model.indexOffset, state
            });
        }
    }


    Ref<GraphicsPipeline> CreatePipeline(PipelineType type) {
        switch (type) {
            case PipelineType::DefaultColor:
                return CreateDefaultColor();
            case PipelineType::DepthOnly:
                return CreateDepthPrepass(VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT);
            case PipelineType::DepthShadowmap:
                return CreateDepthPrepass(VkCullModeFlagBits::VK_CULL_MODE_FRONT_BIT);
            case PipelineType::FullScreenQuad:
                return CreateFullScreenQuad();
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
            .AddLayout<Textures>()
            .AddLayout<Lights>()
            .SetAttachments<DefaultColorAttachments>(DefaultColorAttachments::Formats{
                .color = m_renderContext.Get<PresentFeature>().swapChain->format,
                .depth = depthFormat
            })
            .AddShaderStage(vertexBin)
            .AddShaderStage(fragmentBin)
            .SetDepthWriteEnabled(false)
            .SetDepthCompareOp(VkCompareOp::VK_COMPARE_OP_EQUAL)
            .SetCullMode(VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT, VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
            .Build();
    }

    Ref<GraphicsPipeline> CreateDepthPrepass(VkCullModeFlags cullMode) {
        ShaderBinary vertexBin = m_renderContext.Get<ShaderLoader>().Get("shaders/basic.vert", Stage::Vertex);
        
        return m_renderContext
            .Get<GraphicsFeature>().NewGraphicsPipeline()
            .AddVertex<Vertex>()
            .AddLayout<Transforms>()
            .SetAttachments<DepthOnlyAttachments>(DepthOnlyAttachments::Formats{
                .depth = depthFormat
            })
            .AddShaderStage(vertexBin)
            .SetCullMode(cullMode, VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
            .Build();
    }

    Ref<GraphicsPipeline> CreateFullScreenQuad() {
        ShaderBinary vertexBin = m_renderContext.Get<ShaderLoader>().Get("shaders/fullScreenQuad.vert", Stage::Vertex);
        ShaderBinary fragmentBin = m_renderContext.Get<ShaderLoader>().Get("shaders/fullScreenQuad.frag", Stage::Fragment);

        return m_renderContext
            .Get<GraphicsFeature>().NewGraphicsPipeline()
            .AddLayout<FullScreenQuad>()
            .SetAttachments<ColorOnlyAttachments>(ColorOnlyAttachments::Formats{
                .color = m_renderContext.Get<PresentFeature>().swapChain->format
            })
            .AddShaderStage(vertexBin)
            .AddShaderStage(fragmentBin)
            .SetCullMode(VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT, VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
            .Build();
    }
};
