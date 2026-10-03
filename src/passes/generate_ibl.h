#pragma once

#include "../material_database.h"

#define BLOCK_NAME GenDiffuseIblInputs
#define BLOCK \
IMAGE_SAMPLER(skybox, 0, Stage::Compute) \
IMAGE_STORAGE(output, 1, Stage::Compute, Access::Write)
#include <gen_bindings.h>

struct GenerateIBLPass: Pass {

    using Pass::Pass;

    void Run(
        ResourceRef<Image> skybox,
        ResourceRef<Image> output,
        ResourceRef<Sampler> sampler
    ) {

        auto& node = context->Get<RenderGraph>().AddNode<ComputeNode<GenDiffuseIblInputs>>(
            GetComputePipeline(), QueueType::Compute
        );

        auto b = GenDiffuseIblInputs {
            .skybox = skybox,
            .skybox_sampler = sampler,
            .output = output
        };

        node.SetBindings(b);

        node.SetGroups((output->description.width + 7) / 8, (output->description.height + 7) / 8, 6);
    }

protected:
    Ref<ComputePipeline> CreateComputePipeline(uint32_t) {
        
        ShaderBinary shader = context->Get<ShaderLoader>().Get("shaders/bake_diffuse.comp", Stage::Compute);

        return context->Get<Compute>().NewComputePipeline()
            .AddShaderStage(shader)
            .AddLayout<GenDiffuseIblInputs>()
            .Build();
    }
 
};