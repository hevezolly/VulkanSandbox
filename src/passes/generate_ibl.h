#pragma once

#include "../material_database.h"

#define BLOCK_NAME GenDiffuseIblInputs
#define BLOCK \
IMAGE_SAMPLER(skybox, 0, Stage::Compute) \
IMAGE_STORAGE(output, 1, Stage::Compute, Access::Write)
#include <gen_bindings.h>

#define BLOCK_NAME GenSpecularIblInputs
#define BLOCK \
IMAGE_SAMPLER(skybox, 0, Stage::Compute) \
IMAGE_STORAGE(output, 1, Stage::Compute, Access::Write) \
DYNAMIC_UNIFORM(setup, 2, Stage::Compute)
#include <gen_bindings.h>

const uint32_t DIFFUSE_PIPELINE = 0;
const uint32_t SPECULAR_PIPELINE = 1;

struct GenerateIBLPass: Pass {

    using Pass::Pass;

    void Run(
        ResourceRef<Image> skybox,
        ResourceRef<Sampler> sampler,
        ResourceRef<Image> diffuseOutput,
        ResourceRef<Image> specularOutput
    ) {

        auto& diffuse = context->Get<RenderGraph>().AddNode<ComputeNode<GenDiffuseIblInputs>>(
            GetComputePipeline(DIFFUSE_PIPELINE), QueueType::Compute
        );

        auto diffuseBindings = GenDiffuseIblInputs {
            .skybox = skybox,
            .skybox_sampler = sampler,
            .output = diffuseOutput
        };
        diffuse.SetBindings(diffuseBindings);
        diffuse.SetGroups((diffuseOutput->description.width + 7) / 8, (diffuseOutput->description.height + 7) / 8, 6);
        int mipLevels = specularOutput->description.mipLevels;
        for (int i = 0; i < mipLevels; i++) {
            
            auto& specular = context->Get<RenderGraph>().AddNode<ComputeNode<GenSpecularIblInputs>>(
                GetComputePipeline(SPECULAR_PIPELINE), QueueType::Compute
            );

            float roughness = i / (mipLevels - 1);
            
            auto specularBindings = GenSpecularIblInputs {
                .skybox = skybox,
                .skybox_sampler = sampler,
                .output = get(specularOutput, Mip(i)),
                .setup = context->Get<DynamicUniforms>().Allocate(roughness)
            };

            int size = specularOutput->description.width / (1 << i);

            specular.SetBindings(specularBindings);
            specular.SetGroups((size + 7) / 8, (size + 7) / 8, 6);
        }
    }

protected:
    Ref<ComputePipeline> CreateComputePipeline(uint32_t type) {
        switch (type)
        {
            case DIFFUSE_PIPELINE: {
                ShaderBinary shader = context->Get<ShaderLoader>().Get("shaders/bake_diffuse.comp", Stage::Compute);

                return context->Get<Compute>().NewComputePipeline()
                    .AddShaderStage(shader)
                    .AddLayout<GenDiffuseIblInputs>()
                    .Build();
            }
            
            case SPECULAR_PIPELINE: {
                ShaderBinary shader = context->Get<ShaderLoader>().Get("shaders/bake_specular.comp", Stage::Compute);

                return context->Get<Compute>().NewComputePipeline()
                    .AddShaderStage(shader)
                    .AddLayout<GenSpecularIblInputs>()
                    .Build();
            }

            default:
                throw std::runtime_error("unknown bake ibl pipeline");
        }
    }
 
};