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

enum struct ComputePipelineType {
    GenerateSkybox,
};

struct PipelineKey {
    TypeId typeId;
    uint32_t index;

    bool operator==(const PipelineKey& other) const {
        return typeId == other.typeId && index == other.index;
    }

    bool operator!=(const PipelineKey& other) const {
        return !(*this == other);
    }
};

namespace std {
    template <>
    struct hash<PipelineKey> {
        size_t operator()(const PipelineKey& _Keyval) const noexcept {
            size_t seed = 0;
            hash_combine(seed, _Keyval.typeId);
            hash_combine(seed, _Keyval.index);
            return seed;
        }
    };
}

struct MaterialDatabase;

struct Pass {

    Pass(): materialDatabase(nullptr) {}
    Pass(RenderContext* context, MaterialDatabase* materialDatabase): context(context), materialDatabase(materialDatabase) {}

    virtual ~Pass() {}

protected:

    virtual Ref<GraphicsPipeline> CreateGraphicsPipeline(uint32_t index) {ASSERT_MSG(false, "not implemented"); return Ref<GraphicsPipeline>::Null();}
    virtual Ref<ComputePipeline> CreateComputePipeline(uint32_t index) {ASSERT_MSG(false, "not implemented"); return Ref<ComputePipeline>::Null();}

    Ref<GraphicsPipeline> GetGraphicsPipeline(uint32_t index=0);

    Ref<ComputePipeline> GetComputePipeline(uint32_t index=0);

    MaterialDatabase* materialDatabase;
    RenderContext* context;
};

struct MaterialDatabase
{
    std::unordered_map<PipelineKey, Ref<GraphicsPipeline>> _graphicsPipelines;
    std::unordered_map<PipelineKey, Ref<ComputePipeline>> _compuePipelines;
    VkFormat depthFormat;
    RenderContext& m_renderContext;

    MaterialDatabase(RenderContext& renderContext): m_renderContext(renderContext) {
        depthFormat = renderContext.Get<Device>().SelectSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    void RegisterPipeline(Ref<GraphicsPipeline> pipeline, PipelineKey key) {
        _graphicsPipelines[key] = pipeline;
    }

    void RegisterPipeline(Ref<ComputePipeline> pipeline, PipelineKey key) {
        _compuePipelines[key] = pipeline;
    }

    void InvalidatePipelines() {
        _graphicsPipelines.clear();
        _compuePipelines.clear();
    }

    Ref<GraphicsPipeline> GetGraphicsPipeline(PipelineKey key) {
        return _graphicsPipelines[key];
    }

    Ref<ComputePipeline> GetComputePipeline(PipelineKey key) {
        return _compuePipelines[key];
    }

    template<typename PassType, typename... Args>
    PassType Create(Args&&... args) {
        return PassType(&m_renderContext, this, std::forward<Args>(args)...);
    }

    template<typename Attachments, typename... Bindings, typename Func>
    static void QueueDrawModels(
        GraphicsNode<Attachments, Bindings...>& node, 
        const DrawContext& drawContext,
        std::vector<uint32_t>& models, 
        Func&& getBindings
    ) {
        
        node.SetIndexBuffer(drawContext.resources.indexBuffer, VkIndexType::VK_INDEX_TYPE_UINT32);
        node.AddVertexBuffer(drawContext.resources.vertexBuffer);

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
};

Ref<ComputePipeline> Pass::GetComputePipeline(uint32_t index) {
    PipelineKey key {
        .typeId = TypeId(typeid(*this)),
        .index = index
    };
    Ref<ComputePipeline> pipeline = materialDatabase->GetComputePipeline(key);
    
    if (!pipeline.isNull())
        return pipeline;

    pipeline = CreateComputePipeline(index);
    materialDatabase->RegisterPipeline(pipeline, key);
    return pipeline;
}     

Ref<GraphicsPipeline> Pass::GetGraphicsPipeline(uint32_t index) {
    PipelineKey key {
        .typeId = TypeId(typeid(*this)),
        .index = index
    };
    Ref<GraphicsPipeline> pipeline = materialDatabase->GetGraphicsPipeline(key);
    
    if (!pipeline.isNull())
        return pipeline;

    pipeline = CreateGraphicsPipeline(index);
    materialDatabase->RegisterPipeline(pipeline, key);
    return pipeline;
}     