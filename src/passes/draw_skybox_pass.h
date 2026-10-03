#pragma once

#include "../material_database.h"

#define BLOCK_NAME DrawSkyboxInput
#define BLOCK \
IMAGE_SAMPLER(skybox, 1, Stage::Compute) \
IMAGE_STORAGE(output, 2, Stage::Compute, Access::Write) \
DYNAMIC_UNIFORM(cameraData, 0, Stage::Compute)
#include <gen_bindings.h>

struct DrawSkyboxPass: Pass {

    using Pass::Pass;

    void Run(ResourceRef<Image> output, ResourceRef<Image> skybox, ResourceRef<Sampler> sampler, BufferRegion cameraData) {

        auto& node = context->Get<RenderGraph>().AddNode<ComputeNode<DrawSkyboxInput>>(
            GetComputePipeline(), QueueType::Compute
        );

        auto b = DrawSkyboxInput {
            .skybox = skybox,
            .skybox_sampler = sampler,
            .output = output,
            .cameraData = cameraData
        };

        node.SetBindings(b);

        node.SetGroups((output->description.width + 7) / 8, (output->description.height + 7) / 8, 1);
    }

protected:
    Ref<ComputePipeline> CreateComputePipeline(uint32_t) {
        
        ShaderBinary shader = context->Get<ShaderLoader>().Get("shaders/drawSkybox.comp", Stage::Compute);

        return context->Get<Compute>().NewComputePipeline()
            .AddShaderStage(shader)
            .AddLayout<DrawSkyboxInput>()
            .Build();
    }
 
};