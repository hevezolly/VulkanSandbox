#pragma once

#include "../material_database.h"

#define BLOCK_NAME PostProcessSkyboxInput
#define BLOCK \
IMAGE_STORAGE(output, 1, Stage::Compute, Access::Write) \
IMAGE_SAMPLER(input, 0, Stage::Compute)
#include <gen_bindings.h>

struct PostProcessSkyboxPass: Pass {

    using Pass::Pass;

    void Run(
        ResourceRef<Image> input,
        ResourceRef<Sampler> sampler,
        ResourceRef<Image> output 
    ) {

        auto& node = context->Get<RenderGraph>().AddNode<ComputeNode<PostProcessSkyboxInput>>(
            GetComputePipeline(), QueueType::Compute
        );

        auto b = PostProcessSkyboxInput {
            .output = output,
            .input = input,
            .input_sampler = sampler
        };

        node.SetBindings(b);

        node.SetGroups((output->description.width + 7) / 8, (output->description.height + 7) / 8, 6);
    }

protected:
    Ref<ComputePipeline> CreateComputePipeline(uint32_t) {
        
        ShaderBinary shader = context->Get<ShaderLoader>().Get("shaders/post_process_skybox.comp", Stage::Compute);

        return context->Get<Compute>().NewComputePipeline()
            .AddShaderStage(shader)
            .AddLayout<PostProcessSkyboxInput>()
            .Build();
    }
 
};