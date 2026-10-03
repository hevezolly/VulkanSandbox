#pragma once

#include "geometry_pass.h"

struct MainPass: GeometryPass<DefaultColorAttachments, Transforms, Textures, Lights> {

    using GeometryPass<DefaultColorAttachments, Transforms, Textures, Lights>::GeometryPass;

    void Run(const DrawContext& drawContext, std::vector<uint32_t>& models) override {

        auto& node = context->Get<RenderGraph>().AddNode<GraphicsNode<
            DefaultColorAttachments, 
            Transforms,
            Textures,
            Lights
        >>(GetGraphicsPipeline());

        node.SetName("Default Color Node");
        node.SetAttachments(
            DefaultColorAttachments{
                .color = drawContext.output,
                .depth = drawContext.depth
            }
        );
        
        MaterialDatabase::QueueDrawModels(node, drawContext, models, 
            [](const ModelData& model, const DrawContext& context) {

            Transforms transforms {
                .camera = context.mainViewCamera,
                .model = model.transformsRange
            };

            Textures textures {
                .textureIds = model.textureIds,
                .colorTexture = context.resources.images[0],
                .colorTexture_sampler = context.resources.linearSampler
            };

            Lights lights {
                .lightsConfig = context.lights,
                .directShadowmap = context.resources.directShadowmap,
                .directShadowmap_sampler = context.resources.linearSampler,
                .diffuseIBL = context.resources.diffuseIbl,
                .diffuseIBL_sampler = context.resources.linearSampler
            };

            return std::tuple {transforms, textures, lights};
        });

    }

protected:

    Ref<GraphicsPipeline> CreateGraphicsPipeline(uint32_t id) override {
        ShaderBinary vertexBin = context->Get<ShaderLoader>().Get("shaders/basic.vert", Stage::Vertex);
        ShaderBinary fragmentBin = context->Get<ShaderLoader>().Get("shaders/basic.frag", Stage::Fragment);
        
        return context->Get<GraphicsFeature>()
            .NewGraphicsPipeline()
            .AddVertex<Vertex>()
            .AddLayout<Transforms>()
            .AddLayout<Textures>()
            .AddLayout<Lights>()
            .SetAttachments<DefaultColorAttachments>(DefaultColorAttachments::Formats{
                .color = context->Get<PresentFeature>().swapChain->format,
                .depth = materialDatabase->depthFormat
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
};