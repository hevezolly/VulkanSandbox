#pragma once

#include "../material_database.h"

#define BLOCK_NAME GenLut
#define BLOCK \
IMAGE_STORAGE(output, 0, Stage::Compute, Access::Write)
#include <gen_bindings.h>

struct GenerateBrdfLut: Pass {

    using Pass::Pass;

    void Run(
        ResourceRef<Image> output
    ) {
        auto& node = context->Get<RenderGraph>().AddNode<ComputeNode<GenLut>>(
            GetComputePipeline(), QueueType::Compute
        );

        auto b = GenLut {
            .output = output,
        };

        node.SetBindings(b);

        node.SetGroups((output->description.width + 7) / 8, (output->description.height + 7) / 8, 1);
    }

protected:
    Ref<ComputePipeline> CreateComputePipeline(uint32_t) {
        
        ShaderBinary shader = context->Get<ShaderLoader>().Get("shaders/beckmanBRDFLUT.comp", Stage::Compute);

        return context->Get<Compute>().NewComputePipeline()
            .AddShaderStage(shader)
            .AddLayout<GenLut>()
            .Build();
    }
 
};