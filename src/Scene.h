#pragma once

#include <vulkan_engine.h>
#include <vulkan_engine_ui.h>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include "model_loader.h"
#include "material_database.h"

class Scene {

    struct ModelData {
        uint32_t indexOffset;
        uint32_t indexSize;
        uint32_t materialHandle;
    
        glm::mat4x4 modelTransform; 
        
        BufferRegion uniformsRange;
    };

    struct MaterialPass {

        std::vector<uint32_t> models;
        
        void (*setUpDraw)(RenderContext&, Scene&, MaterialPass&);
    };

    RenderContext& m_renderContext;

    uint32_t m_nextObjectPosition;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    
    ResourceRef<Buffer> m_vertexBuffer;
    ResourceRef<Buffer> m_indexBuffer;
    ResourceRef<Buffer> m_cameraData;

    std::vector<ModelData> m_models;
    std::vector<MaterialPass> m_passes;

    template<typename Attachments, typename... Bindings>
    void QueueDrawModels(GraphicsNode<Attachments, Transforms, Bindings...>& node, std::vector<uint32_t>& models, const Bindings&... values) {
        
        node.SetIndexBuffer(m_indexBuffer, VkIndexType::VK_INDEX_TYPE_UINT32);
        node.AddVertexBuffer(m_vertexBuffer);

        uint32_t i = 0;
        for (uint32_t index: models) {
            ModelData model = m_models[index];

            Transforms t = Transforms {
                .model = model.uniformsRange,
                .camera = m_cameraData
            };

            if (i++ == 0) {
                node.SetBindings(t, values...);
            }

            ShaderDynamicState state = m_renderContext.Get<Descriptors>().GatherDynamicState(t, values...);

            node.AddDrawParameters(DrawParameters{
                model.indexSize, model.indexOffset, state
            });
        }
    }

    void PopulateScene() {

        uint32_t defaultColorMaterial = AddMaterial(&DefaultColorMaterial);
        
        uint32_t viking_room = AddObject("models/viking_room.obj", defaultColorMaterial);
    }

public:

    MaterialDatabase materialDatabase;


    static void DefaultColorMaterial(RenderContext& context, Scene& scene, MaterialPass& material) {

        auto& node = context.Get<RenderGraph>().AddNode<GraphicsNode<DefaultColorAttachments, Transforms>>(
            scene.materialDatabase.GetPipeline(PipelineType::DefaultColor));

        node.SetName("Default Color Node");
        
        scene.QueueDrawModels(node, material.models);
    }
    
    Scene(RenderContext& context): m_renderContext(context), materialDatabase(context) {
        PopulateScene();
    }
    
    uint32_t AddMaterial(void (*setUpDraw)(RenderContext&, Scene&, MaterialPass&)) {

        uint32_t materialId = m_passes.size();

        m_passes.push_back(MaterialPass{
            .models = std::vector<uint32_t>(),
            .setUpDraw = setUpDraw
        });

        return materialId;
    }

    uint32_t AddObject(const char* path, uint32_t materialHandle) {
        
        uint32_t indexStart = m_indices.size();
        loadModel(m_renderContext, path, m_vertices, m_indices);
        
        uint32_t indexSize = indexStart - m_indices.size();
        
        uint32_t modelIndex = m_models.size();

        m_models.push_back(ModelData {
            .indexOffset = m_nextObjectPosition,
            .indexSize = indexSize,
            .materialHandle = materialHandle,
            .modelTransform = glm::identity<glm::mat4x4>()
        });

        m_passes[materialHandle].models.push_back(modelIndex);

        m_nextObjectPosition += indexSize;

        return modelIndex;
    }

    void OnBeginFrame() {

    }

    void OnPrepareDraw(ResourceRef<Image> output) {

        

        for (MaterialPass& pass: m_passes) {
            pass.setUpDraw(m_renderContext, *this, pass);
        }
    }

    void OnEngFrame() {

    }

};