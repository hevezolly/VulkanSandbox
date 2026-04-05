#pragma once

#include <vulkan_engine.h>
#include <vulkan_engine_ui.h>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include "model_loader.h"
#include "material_database.h"

class Scene {

    struct CameraData {
        glm::mat4x4 view;
        glm::mat4x4 projection;
    };

    struct ModelData {
        uint32_t indexOffset;
        uint32_t indexSize;
        uint32_t materialHandle;
    
        glm::mat4x4 modelTransform; 
        
        BufferRegion uniformsRange;
    };

    struct DrawContext {
        BufferRegion mainViewCamera;
        ResourceRef<Image> output;
        ResourceRef<Image> depth;
        ResourceRef<Buffer> vertexBuffer;
        ResourceRef<Buffer> indexBuffer;
        std::vector<ModelData>& models;
        RenderContext& context;
    };

    struct MaterialPass {

        std::vector<uint32_t> models;
        
        void (*setUpDraw)(Scene&, const DrawContext&, MaterialPass&);
    };

    RenderContext& m_renderContext;

    uint32_t m_nextObjectPosition;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    
    ResourceRef<Buffer> m_vertexBuffer;
    ResourceRef<Buffer> m_indexBuffer;
    ResourceRef<Image> m_depthBuffer;

    std::vector<ModelData> m_models;
    std::vector<MaterialPass> m_passes;

    template<typename Attachments, typename... Bindings>
    static void QueueDrawModels(
        GraphicsNode<Attachments, Transforms, Bindings...>& node, 
        const DrawContext& drawContext,
        std::vector<uint32_t>& models, 
        const Bindings&... values
    ) {
        
        node.SetIndexBuffer(drawContext.indexBuffer, VkIndexType::VK_INDEX_TYPE_UINT32);
        node.AddVertexBuffer(drawContext.vertexBuffer);

        uint32_t i = 0;
        for (uint32_t index: models) {
            ModelData model = drawContext.models[index];

            Transforms t = Transforms {
                .camera = drawContext.mainViewCamera,
                .model = model.uniformsRange
            };

            if (i++ == 0) {
                node.SetBindings(t, values...);
            }

            ShaderDynamicState state = drawContext.context.Get<Descriptors>().GatherDynamicState(t, values...);

            node.AddDrawParameters(DrawParameters{
                model.indexSize, model.indexOffset, state
            });
        }
    }

    void InitializeResources() {
        Resources& resources = m_renderContext.Get<Resources>();
        
        m_vertexBuffer = resources.CreateBuffer(BufferPreset::VERTEX, m_vertices);
        resources.GiveName(m_vertexBuffer, "vertex buffer");

        m_indexBuffer = resources.CreateBuffer(BufferPreset::INDEX, m_indices);
        resources.GiveName(m_indexBuffer, "index buffer");
        
        auto extents = m_renderContext.Get<PresentFeature>().swapChain->extent;
        m_depthBuffer = resources.CreateImage(ImageDescription(materialDatabase.depthFormat, 
            ImageUsage::DepthStencil | ImageUsage::TransferDst, extents));
        resources.GiveName(m_depthBuffer, "depth buffer");
    }

    void PopulateScene() {

        uint32_t defaultColorMaterial = AddMaterial(&DefaultColorMaterial);
        
        uint32_t viking_room = AddObject("models/viking_room.obj", defaultColorMaterial);
    }

    CameraData UpdateCameraPosition() {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        SwapChain* swapChain = m_renderContext.Get<PresentFeature>().swapChain;
        CameraData d;
        d.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        d.projection = glm::perspective(glm::radians(45.0f), swapChain->extent.width / (float) swapChain->extent.height, 0.1f, 10.0f);
        d.projection[1][1] *= -1;

        return d;
    } 

public:

    MaterialDatabase materialDatabase;


    static void DefaultColorMaterial(Scene& scene, const DrawContext& drawContext, MaterialPass& material) {

        auto& node = drawContext.context.Get<RenderGraph>().AddNode<GraphicsNode<DefaultColorAttachments, Transforms>>(
            scene.materialDatabase.GetPipeline(PipelineType::DefaultColor));

        node.SetName("Default Color Node");
        node.SetAttachments(
            DefaultColorAttachments{
                .color = drawContext.output,
                .depth = drawContext.depth
            }
        );
        
        Scene::QueueDrawModels(node, drawContext, material.models);
    }
    
    Scene(RenderContext& context): 
        m_renderContext(context), materialDatabase(context),
        m_nextObjectPosition(0), m_indices(), m_vertices(), m_models(), m_passes()
    {
        PopulateScene();
        InitializeResources();
    }
    
    uint32_t AddMaterial(void (*setUpDraw)(Scene&, const DrawContext&, MaterialPass&)) {

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
        
        uint32_t indexSize = m_indices.size() - indexStart;
        
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

        
        for (ModelData& model : m_models) {
            model.uniformsRange = m_renderContext.Get<DynamicUniforms>().Allocate(model.modelTransform);
        }
    }

    void OnPrepareDraw(ResourceRef<Image> output) {

        if (m_depthBuffer->description.width != output->description.width ||
            m_depthBuffer->description.height != output->description.height) {

            m_depthBuffer = m_renderContext.Get<Resources>().Resize(m_depthBuffer, {output->description.width, output->description.height});
        }

        DrawContext drawContext {
            .mainViewCamera = m_renderContext.Get<DynamicUniforms>().Allocate(UpdateCameraPosition()),
            .output = output,
            .depth = m_depthBuffer,
            .vertexBuffer = m_vertexBuffer,
            .indexBuffer = m_indexBuffer,
            .models = m_models,
            .context = m_renderContext
        };

        m_renderContext.Get<RenderGraph>().AddNode<ClearImageNode>(m_depthBuffer);
        m_renderContext.Get<RenderGraph>().AddNode<ClearImageNode>(output);

        for (MaterialPass& pass: m_passes) {
            pass.setUpDraw(*this, drawContext, pass);
        }
    }

    void OnEngFrame() {

    }

};