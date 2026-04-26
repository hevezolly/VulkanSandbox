#pragma once

#include "../material_database.h"

#define BLOCK_NAME GenSkyboxInput
#define BLOCK \
IMAGE_STORAGE(output, 1, Stage::Compute, Access::Write) \
DYNAMIC_UNIFORM(lightData, 0, Stage::Compute)
#include <gen_bindings.h>

struct GenerateSkyboxPass: Pass {

    using Pass::Pass;

    void Run(ResourceRef<Image> output, BufferRegion light) {

        auto node = context->Get<RenderGraph>().AddNode<ComputeNode<GenSkyboxInput>>(
            GetComputePipeline(), QueueType::Graphics
        );

        node.SetName("Gen skybox");

        auto b = GenSkyboxInput {
            .output = output,
            .lightData = light
        };

        LOG("reference")
        logMemory(&b, sizeof(BufferRegion) + sizeof(ImageSubresource));

        node.SetBindings(b);

        node.SetGroups((output->description.width + 7) / 8, (output->description.height + 7) / 8, 6);
    }

protected:
    Ref<ComputePipeline> CreateComputePipeline(uint32_t) {
        
        ShaderBinary shader = context->Get<ShaderLoader>().Get("shaders/generateSkybox.comp", Stage::Compute);

        return context->Get<Compute>().NewComputePipeline()
            .AddShaderStage(shader)
            .AddLayout<GenSkyboxInput>()
            .Build();
    }
 
};