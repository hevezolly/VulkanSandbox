#pragma once

#include <vulkan_engine.h>
#include <vulkan_engine_ui.h>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include "model_loader.h"
#include "material_database.h"
#include "draw_context.h"

class Scene {

    struct CameraData {
        glm::mat4x4 view;
        glm::mat4x4 projection;
    };

    RenderContext& m_renderContext;

    LightsConfig m_lights {
        .dirLightTransform = glm::identity<glm::mat4>(),
        .dirLightDirection = {0.813, 0.366, 0.453, 0},
        .dirLightColor = {1, 1, 1, 1},
        .depthBias = 0.005
    };

    uint32_t m_nextObjectPosition;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    
    ResourceRef<Buffer> m_vertexBuffer;
    ResourceRef<Buffer> m_indexBuffer;
    ResourceRef<Image> m_depthBuffer;
    ResourceRefs<Image> m_images;
    ResourceRef<Sampler> m_linearSampler;
    ResourceRef<Image> m_directShadowmap;

    std::vector<ModelData> m_models;
    std::vector<MaterialPass> m_passes;

    uint32_t depthPass;
    uint32_t colorPass;
    uint32_t directShadowmapPass;

    DebugData m_debugData;

    float size = 2.0f;

    void InitializeResources() {
        Resources& resources = m_renderContext.Get<Resources>();
        
        m_vertexBuffer = resources.CreateBuffer(BufferPreset::VERTEX, m_vertices);
        resources.GiveName(m_vertexBuffer, "vertex buffer");

        m_indexBuffer = resources.CreateBuffer(BufferPreset::INDEX, m_indices);
        resources.GiveName(m_indexBuffer, "index buffer");
        
        auto extents = m_renderContext.Get<PresentFeature>().swapChain->extent;
        m_depthBuffer = resources.CreateImage(ImageDescription(materialDatabase.depthFormat, 
            ImageUsage::DepthStencil | ImageUsage::TransferDst | ImageUsage::Sampled, extents));
        resources.GiveName(m_depthBuffer, "depth buffer");

        m_linearSampler = resources.CreateSampler(SamplerFilter::LINEAR, SamplerAddressMode::CLAMP);
        m_images.push_back(resources.LoadImageResource(ImageUsage::Sampled, "textures/viking_room.png", VK_FORMAT_R8G8B8A8_UNORM));
        m_directShadowmap = resources.CreateImage(ImageDescription(
            materialDatabase.depthFormat,
            ImageUsage::DepthStencil | ImageUsage::Sampled | ImageUsage::TransferDst, {500, 500}
        ));
        resources.GiveName(m_directShadowmap, "direct shadowmap");
    }

    void PopulateScene() {

        depthPass = AddMaterial(&MaterialDatabase::DepthOnlyPass);
        colorPass = AddMaterial(&MaterialDatabase::DefaultColorPass);
        directShadowmapPass = AddMaterial(&MaterialDatabase::DirectShadowmapPass);
        
        uint32_t viking_room = AddObject("models/viking_room.obj", 
            {depthPass, colorPass, directShadowmapPass});
        m_models[viking_room].imageId = 0;
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

    glm::mat4 VulkanOrtho(float left, float right, float bottom, float top, float n, float f) {
        glm::mat4 m(1.0f);
        m[0][0] =  2.0f / (right - left);
        m[1][1] =  2.0f / (bottom - top); // bottom/top swapped = Y flip for Vulkan
        m[2][2] =  -1.0f / (f - n);   // [0,1] depth range
        m[3][0] = -(right + left)   / (right - left);
        m[3][1] = -(bottom + top)   / (bottom - top);
        m[3][2] = n / (f - n);
        m[2][3] *= -1;
        return m;
    };

    CameraData GetShadowmapViewProjection() {
        CameraData result;
        SwapChain* swapChain = m_renderContext.Get<PresentFeature>().swapChain;
        
        result.view = glm::lookAt(glm::vec3(m_lights.dirLightDirection) * glm::vec3(2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        result.projection = VulkanOrtho(-size, size, -size, size, 0.1f, 10.0f);
        return result;
    }

    void DispalyUI() {
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowContentSize({200, 300});

        if (ImGui::Begin("Debug")) {
            if (ImGui::SliderFloat3("light direction", &m_lights.dirLightDirection[0], -1, 1)) {
                m_lights.dirLightDirection = glm::vec4(
                glm::normalize(glm::vec3(m_lights.dirLightDirection)), 0);
            }

            ImGui::ColorPicker3("light color", &m_lights.dirLightColor[0]);
            ImGui::SliderFloat("depth bias", &m_lights.depthBias, 0.0f, 0.01f, "%.5");

            ImGui::DragFloat("size", &size);
        }
        ImGui::End();        
    }

public:

    MaterialDatabase materialDatabase;
    
    Scene(RenderContext& context): 
        m_renderContext(context), materialDatabase(context),
        m_nextObjectPosition(0), m_indices(), m_vertices(), m_models(), m_passes()
    {
        PopulateScene();
        InitializeResources();
    }
    
    uint32_t AddMaterial(void (*setUpDraw)(MaterialDatabase&, const DrawContext&, MaterialPass&)) {

        uint32_t materialId = m_passes.size();

        m_passes.push_back(MaterialPass{
            .models = std::vector<uint32_t>(),
            .setUpDraw = setUpDraw
        });

        return materialId;
    }

    uint32_t AddObject(const char* path, std::initializer_list<uint32_t> materialPasses) {
        
        uint32_t indexStart = m_indices.size();
        loadModel(m_renderContext, path, m_vertices, m_indices);
        
        uint32_t indexSize = m_indices.size() - indexStart;
        
        uint32_t modelIndex = m_models.size();

        m_models.push_back(ModelData {
            .indexOffset = m_nextObjectPosition,
            .indexSize = indexSize,
            .modelTransform = glm::identity<glm::mat4x4>()
        });

        for (uint32_t material : materialPasses) {
            m_passes[material].models.push_back(modelIndex);
        }

        m_nextObjectPosition += indexSize;

        return modelIndex;
    }

    void DrawPass(DrawContext& drawContext, uint32_t passId) {
        m_passes[passId].setUpDraw(materialDatabase, drawContext, m_passes[passId]);
    }

    void OnBeginFrame() {

        DispalyUI();
        
        for (ModelData& model : m_models) {
            ModelTransforms transforms {
                model.modelTransform,
                glm::inverse(model.modelTransform)
            };
            model.transformsRange = m_renderContext.Get<DynamicUniforms>().Allocate(transforms);
            model.textureIds = m_renderContext.Get<DynamicUniforms>().Allocate(model.imageId);
        }
    }

    void OnPrepareDraw(ResourceRef<Image> output) {

        if (m_depthBuffer->description.width != output->description.width ||
            m_depthBuffer->description.height != output->description.height) {

            m_depthBuffer = m_renderContext.Get<Resources>().Resize(m_depthBuffer, {output->description.width, output->description.height});
        }

        CameraData shadowmapCamera = GetShadowmapViewProjection();
        BufferRegion directShadowmapRange = m_renderContext.Get<DynamicUniforms>()
            .Allocate(shadowmapCamera);
        BufferRegion mainViewCameraRange = m_renderContext.Get<DynamicUniforms>()
            .Allocate(UpdateCameraPosition());
        
        m_lights.dirLightTransform = shadowmapCamera.projection * shadowmapCamera.view;

        DrawContext drawContext {
            .mainViewCamera = mainViewCameraRange,
            .lights = m_renderContext.Get<DynamicUniforms>().Allocate(m_lights),
            .output = output,
            .depth = m_depthBuffer,
            .directShadowmap = m_directShadowmap,
            .colorTextures = m_images,
            .vertexBuffer = m_vertexBuffer,
            .indexBuffer = m_indexBuffer,
            .linearSampler = m_linearSampler,
            .models = m_models,
            .context = m_renderContext
        };

        m_renderContext.Get<RenderGraph>().AddNode<ClearImageNode>(m_depthBuffer);
        DrawPass(drawContext, depthPass);

        drawContext.mainViewCamera = directShadowmapRange;
        drawContext.depth = m_directShadowmap;
        m_renderContext.Get<RenderGraph>().AddNode<ClearImageNode>(m_directShadowmap);
        DrawPass(drawContext, directShadowmapPass);

        // auto& node = m_renderContext.Get<RenderGraph>().AddNode<
        //     GraphicsNode<ColorOnlyAttachments, FullScreenQuad>>(
        //         materialDatabase.GetPipeline(PipelineType::FullScreenQuad));
        
        // node.SetAttachments(ColorOnlyAttachments {
        //     .color = output
        // });
        // node.SetBindings(
        //     FullScreenQuad {
        //         .image = m_directShadowmap,
        //         .image_sampler = m_linearSampler,
        //         .data = m_renderContext.Get<DynamicUniforms>().Allocate(m_debugData)
        //     }
        // );
        // node.AddDrawParameters(DrawParameters(6, 0));

        drawContext.depth = m_depthBuffer;
        drawContext.mainViewCamera = mainViewCameraRange;
        
        m_renderContext.Get<RenderGraph>().AddNode<ClearImageNode>(output);
        DrawPass(drawContext, colorPass);
    }

    void OnEngFrame() {

    }

};