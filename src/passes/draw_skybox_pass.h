#pragma once

#include "../material_database.h"

#define BLOCK_NAME DrawSkyboxAttachments
#define MSAA 4
#define BLOCK \
COLOR(color, LoadOp::Load)
#include <gen_attachments.h>

#define BLOCK_NAME DrawSkyboxInput
#define BLOCK \
IMAGE_SAMPLER(skybox, 1, Stage::Fragment) \
DYNAMIC_UNIFORM(cameraData, 0, Stage::Fragment)
#include <gen_bindings.h>

struct DrawSkyboxPass: Pass {

    using Pass::Pass;

    void Run(ResourceRef<Image> output, ResourceRef<Image> skybox, ResourceRef<Sampler> sampler, BufferRegion cameraData) {

        auto& node = context->Get<RenderGraph>().AddNode<GraphicsNode<DrawSkyboxAttachments, DrawSkyboxInput>>(
            GetGraphicsPipeline()
        );

        node.SetBindings(DrawSkyboxInput {
            .skybox = skybox,
            .skybox_sampler = sampler,
            .cameraData = cameraData
        });
        node.SetAttachments(DrawSkyboxAttachments {
            .color = output
        });

        node.AddDrawParameters(DrawParameters{6, 0});
    }

protected:
    Ref<GraphicsPipeline> CreateGraphicsPipeline(uint32_t) {
        
        ShaderBinary vertexBin = context->Get<ShaderLoader>().Get("shaders/fullScreenQuad.vert", Stage::Vertex);
        ShaderBinary fragmentBin = context->Get<ShaderLoader>().Get("shaders/drawSkybox.frag", Stage::Fragment);
        
        return context->Get<GraphicsFeature>()
            .NewGraphicsPipeline()
            .AddLayout<DrawSkyboxInput>()
            .SetAttachments<DrawSkyboxAttachments>(DrawSkyboxAttachments::Formats{
                .color = context->Get<PresentFeature>().swapChainFormat(),
            })
            .AddShaderStage(vertexBin)
            .AddShaderStage(fragmentBin)
            .SetCullMode(VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT, VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT)
            .AddDynamicState(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR)
            .Build();
    }
 
};