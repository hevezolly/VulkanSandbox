#pragma once

#include "geometry_pass.h"

struct DepthPass: GeometryPass<DepthOnlyAttachments, Transforms>{

    DepthPass(): GeometryPass<DepthOnlyAttachments, Transforms>() {}

    DepthPass(RenderContext* context, MaterialDatabase* materialDatabase, VkCullModeFlags cullMode): 
        GeometryPass<DepthOnlyAttachments, Transforms>(context, materialDatabase), cullMode(cullMode) {}

    void Run(const DrawContext& drawContext, std::vector<uint32_t>& models) {
        auto& node = context->Get<RenderGraph>().AddNode<GraphicsNode<
            DepthOnlyAttachments, 
            Transforms
        >>(GetGraphicsPipeline(cullMode));

        node.SetName("Depth only node");
        node.SetAttachments(
            DepthOnlyAttachments{
                .depth = drawContext.depth
            }
        );
        
        MaterialDatabase::QueueDrawModels(node, drawContext, models, 
            [](const ModelData& model, const DrawContext& context) {

            Transforms transforms {
                .camera = context.mainViewCamera,
                .model = model.transformsRange
            };
            return std::tuple {transforms};
        });
    }

protected:
    Ref<GraphicsPipeline> CreateGraphicsPipeline(uint32_t index) override {
        VkCullModeFlags mode = index;
        ShaderBinary vertexBin = context->Get<ShaderLoader>().Get("shaders/basic.vert", Stage::Vertex);

        return context->Get<GraphicsFeature>()
            .NewGraphicsPipeline()
            .AddVertex<Vertex>()
            .AddLayout<Transforms>()
            .SetAttachments<DepthOnlyAttachments>(DepthOnlyAttachments::Formats{
                .depth = materialDatabase->depthFormat
            })
            .AddShaderStage(vertexBin)
            .SetCullMode(mode, VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
            .Build();
    }

private:
    VkCullModeFlags cullMode;
};