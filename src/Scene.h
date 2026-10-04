#pragma once

#include <vulkan_engine.h>
#include <vulkan_engine_ui.h>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "model_loader.h"
#include "material_database.h"
#include "draw_context.h"

#include "passes/depth_pass.h"
#include "passes/main_pass.h"
#include "passes/generate_skybox_pass.h"
#include "passes/draw_skybox_pass.h"
#include "passes/generate_ibl.h"
#include "passes/generate_specular_brdf_lut.h"
#include "passes/post_process_skybox_pass.h"

class Scene {

    struct CameraData {
        glm::mat4x4 view;
        glm::mat4x4 projection;
        glm::mat4x4 invView;
        glm::mat4x4 invProjection;
        glm::vec4 forward;
        glm::vec4 position;
    };

    struct MaterialData {
        glm::vec4 f0Roughness;
        glm::vec4 albedoMetallic;
        uint32_t textureId;
    };

    glm::vec3 m_cameraPosition = {2, 2, 2};
    glm::vec3 m_cameraForward = -m_cameraPosition;
    float m_cameraVelocity = 0.025f;
    float m_cameraRotationVelocity = 0.01f;

    RenderContext& m_renderContext;

    LightsConfig m_lights {
        .dirLightTransform = glm::identity<glm::mat4>(),
        .dirLightDirection = {0, -0.94, 0.33, 0},
        .dirLightColor = {1, 1, 1, 7},
        .depthBias = 0.0005f
    };

    SkyboxGenData m_skyboxGenData {
        .SkyColor = {0.35, 0.82, 1.},
        .GroundColor = {0.5, 0.45, 0.35},
        .GroundTransition = 3.5,
    };

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    
    ResourcesRefs m_resources;

    std::vector<ModelData> m_models;
    std::vector<Material> m_materials;
    std::unordered_map<std::string, std::tuple<uint32_t, uint32_t>> m_loadedModels;
    std::unordered_map<Pass*, std::vector<uint32_t>> m_modelPassMapping;

    DepthPass depthPass;
    MainPass colorPass;
    DepthPass directShadowmapPass;
    GenerateSkyboxPass skyboxPass;
    DrawSkyboxPass drawSkyboxPass;
    GenerateIBLPass generateIblPass;
    GenerateBrdfLut generateBrdf;
    PostProcessSkyboxPass postProcessSkyboxPass;


    glm::vec3 f0 = {0, 0, 0};
    float roughness = 0.71f;
    float metallicness = 0.055f;

    DebugData m_debugData;

    float size = 2.0f;

    bool m_skyboxActual = false;
    bool m_lutGenerated = false;

    void InitializeResources() {
        Resources& resources = m_renderContext.Get<Resources>();
        
        m_resources.vertexBuffer = resources.CreateBuffer(BufferPreset::VERTEX, m_vertices);
        resources.GiveName(m_resources.vertexBuffer, "vertex buffer");

        m_resources.indexBuffer = resources.CreateBuffer(BufferPreset::INDEX, m_indices);
        resources.GiveName(m_resources.indexBuffer, "index buffer");
        
        auto extents = m_renderContext.Get<PresentFeature>().swapChain->extent;
        m_resources.depthBuffer = resources.CreateImage(ImageDescription(materialDatabase.depthFormat, 
            ImageUsage::DepthStencil | ImageUsage::TransferDst | ImageUsage::Sampled, extents));
        resources.GiveName(m_resources.depthBuffer, "depth buffer");

        m_resources.linearSampler = resources.CreateSampler(SamplerFilter::LINEAR, SamplerAddressMode::CLAMP);
        m_resources.images.push_back(resources.LoadImageResource(ImageUsage::Sampled, "textures/viking_room.png", VK_FORMAT_R8G8B8A8_UNORM));
        
        m_resources.directShadowmap = resources.CreateImage(ImageDescription(
            materialDatabase.depthFormat,
            ImageUsage::DepthStencil | ImageUsage::Sampled | ImageUsage::TransferDst, {500, 500}
        ));

        resources.GiveName(m_resources.directShadowmap, "direct shadowmap");

        m_resources.skybox_raw = resources.LoadCubeImage(
            ImageUsage::Sampled | ImageUsage::TransferDst,
            "textures/skybox/px.jpg",
            "textures/skybox/nx.jpg",
            "textures/skybox/py.jpg",
            "textures/skybox/ny.jpg",
            "textures/skybox/pz.jpg",
            "textures/skybox/nz.jpg",
            VK_FORMAT_R8G8B8A8_SRGB
        );

        resources.GiveName(m_resources.skybox_raw, "skybox_raw");

        m_resources.skybox = resources.CreateImage(ImageDescription::Cube(
            VK_FORMAT_B10G11R11_UFLOAT_PACK32, m_resources.skybox_raw->description.extent(), 
            ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDst));

        resources.GiveName(m_resources.skybox, "skybox");

        m_resources.diffuseIbl = resources.CreateImage(ImageDescription::Cube(VK_FORMAT_B10G11R11_UFLOAT_PACK32, {32, 32}, 
            ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDst));

        resources.GiveName(m_resources.diffuseIbl, "diffuseIbl");

        m_resources.specularIbl = resources.CreateImage(
            ImageDescription::Cube(VK_FORMAT_B10G11R11_UFLOAT_PACK32, {512, 512}, 
            ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDst, 5));

        resources.GiveName(m_resources.specularIbl, "specularIbl");

        m_resources.brdfLut = resources.CreateImage(ImageDescription(VK_FORMAT_R16G16_SFLOAT, 
            ImageUsage::Storage | ImageUsage::TransferDst | ImageUsage::Sampled,
            {128, 128}
        ));

        resources.GiveName(m_resources.brdfLut, "brdfLut");
    }

    void InitPasses() {
        depthPass = materialDatabase.Create<DepthPass>(VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT);
        directShadowmapPass = materialDatabase.Create<DepthPass>(VkCullModeFlagBits::VK_CULL_MODE_FRONT_BIT);
        colorPass = materialDatabase.Create<MainPass>();
        skyboxPass = materialDatabase.Create<GenerateSkyboxPass>();
        drawSkyboxPass = materialDatabase.Create<DrawSkyboxPass>();
        generateIblPass = materialDatabase.Create<GenerateIBLPass>();
        generateBrdf = materialDatabase.Create<GenerateBrdfLut>();
        postProcessSkyboxPass = materialDatabase.Create<PostProcessSkyboxPass>();
    }

    void PopulateScene() {
        InitPasses();

        const int xSize = 5;
        const int ySize = 5;

        // for (int roughness = 0; roughness < xSize; roughness++) {
        //     for (int metallness = 0; metallness < ySize; metallness++) {
        //         uint32_t material = AddMaterial(Material{
        //             .f0 = glm::vec3(1.000, 0.766, 0.336),
        //             .roughness = static_cast<float>(roughness) / (xSize - 1),
        //             .metallness = static_cast<float>(metallness) / (ySize - 1),
        //             .albedo = glm::vec3(1.000, 0.766, 0.336)
        //         });

        //         uint32_t sphere = AddObject("models/sphere.obj",
        //             {&depthPass, &colorPass, &directShadowmapPass}
        //         );
        //         m_models[sphere].material = material;
        //         m_models[sphere].modelTransform = glm::translate(
        //             m_models[sphere].modelTransform, 
        //             glm::vec3(static_cast<float>(roughness) * 2.5, 0, static_cast<float>(metallness) * 2.5)
        //         );
        //     }
        // }

        uint32_t material = AddMaterial(Material{
            .f0 = f0,
            .roughness = roughness,
            .metallness = metallicness,
            .albedo = glm::vec3(1, 1, 1),
            .imageId = 0
        });
        
        uint32_t viking_room = AddObject("models/viking_room.obj", 
            {&depthPass, &colorPass, &directShadowmapPass});
        m_models[viking_room].material = material;
    }

    glm::mat4 viewMatrix(glm::vec3 pos, glm::vec3 forward, glm::vec3 right, glm::vec3 up) {
        // Remap axes:
        // GLM X (right)    = your Y (right)
        // GLM Y (up)       = your Z (up)
        // GLM Z (backward) = your X (forward)  -- negated because GLM looks down -Z
        glm::mat4 m(1.0f);
        m[0][0] = right.x;    m[1][0] = right.y;    m[2][0] = right.z;    m[3][0] = -glm::dot(right,   pos);
        m[0][1] = up.x;       m[1][1] = up.y;       m[2][1] = up.z;       m[3][1] = -glm::dot(up,      pos);
        m[0][2] = -forward.x; m[1][2] = -forward.y; m[2][2] = -forward.z; m[3][2] = -glm::dot(-forward, pos);
        m[0][3] = 0.0f;       m[1][3] = 0.0f;       m[2][3] = 0.0f;       m[3][3] = 1.0f;
        return m;
    }

    CameraData UpdateCameraPosition() {
    
        SwapChain* swapChain = m_renderContext.Get<PresentFeature>().swapChain;
        CameraData d;
        glm::vec3 right = glm::cross(m_cameraForward, {0, 0, 1});
        glm::vec3 up = glm::cross(right, m_cameraForward);
        d.view = glm::lookAt(m_cameraPosition, m_cameraPosition + m_cameraForward, up);
        d.projection = glm::perspective(glm::radians(45.0f), swapChain->extent.width / (float) swapChain->extent.height, 0.1f, 100.0f);
        d.projection[1][1] *= -1;
        d.forward = glm::vec4(glm::normalize(m_cameraForward), 0);
        d.invProjection = glm::inverse(d.projection);
        d.invView = glm::inverse(d.view);
        d.position = glm::vec4(m_cameraPosition, 0);
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
        result.forward = -m_lights.dirLightDirection;
        result.invProjection = glm::inverse(result.projection);
        result.invView = glm::inverse(result.view);
        return result;
    }

    void DispalyUI() {
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);

        m_cameraForward = glm::normalize(m_cameraForward);
        glm::vec3 right = glm::cross(m_cameraForward, {0, 0, 1});
        glm::vec3 up = glm::cross(right, m_cameraForward);

        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            m_cameraPosition += m_cameraForward * m_cameraVelocity;
        }

        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            m_cameraPosition -= m_cameraForward * m_cameraVelocity;
        }

        if (ImGui::IsKeyDown(ImGuiKey_D)) {
            m_cameraPosition += right * m_cameraVelocity;
        }

        if (ImGui::IsKeyDown(ImGuiKey_A)) {
            m_cameraPosition -= right * m_cameraVelocity;
        }

        if (ImGui::IsKeyDown(ImGuiKey_Space)) {
            m_cameraPosition += up * m_cameraVelocity;
        }

        if (ImGui::IsKeyDown(ImGuiKey_C)) {
            m_cameraPosition -= up * m_cameraVelocity;
        }

        glm::quat rotation = glm::quat(1, 0, 0, 0);

        if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
            rotation *= glm::angleAxis(-m_cameraRotationVelocity, right);
        }

        if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
            rotation *= glm::angleAxis(m_cameraRotationVelocity, right);
        }

        if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
            rotation *= glm::angleAxis(m_cameraRotationVelocity, up);
        }

        if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
            rotation *= glm::angleAxis(-m_cameraRotationVelocity, up);
        }

        m_cameraForward = rotation * m_cameraForward;
        
        

        if (ImGui::Begin("Debug")) {

            ImVec2 available = ImGui::GetContentRegionAvail();
            // subtract height for any buttons/widgets below the child
            float childHeight = available.y - ImGui::GetFrameHeightWithSpacing();

            bool skyboxChanged = false;


            if (ImGui::Button("Reload Shaders")) {
                materialDatabase.InvalidatePipelines();
                skyboxChanged = true;                
            }

            ImGui::BeginChild("scrollable", ImVec2(available.x, childHeight), true);

            ImGui::SliderFloat3("camera pos", &m_cameraPosition[0], -2, 2);

            if (ImGui::CollapsingHeader("light")) 
            {

                if (ImGui::SliderFloat3("light direction", &m_lights.dirLightDirection[0], -1, 1)) {
                    m_lights.dirLightDirection = glm::vec4(
                    glm::normalize(glm::vec3(m_lights.dirLightDirection)), 0);
                    skyboxChanged |= true;
                }

                skyboxChanged |= ImGui::ColorPicker3("light color", &m_lights.dirLightColor[0]);
                skyboxChanged |= ImGui::DragFloat("light intensity", &m_lights.dirLightColor.w);
            
            }

            if (ImGui::CollapsingHeader("shadows")) {
                ImGui::InputFloat("depth bias", &m_lights.depthBias, 0.0001f, 0.001f, "%.5f");
                ImGui::DragFloat("size", &size);
            }

            if (ImGui::CollapsingHeader("surface")) {

                ImGui::ColorPicker3("f0", &m_materials[0].f0[0]);
                ImGui::SliderFloat("roughness", &m_materials[0].roughness, 0.0001f, 1.0f);
                ImGui::SliderFloat("metallicness", &m_materials[0].metallness, 0.0f, 1.0f);
                ImGui::ColorPicker3("albedo", &m_materials[0].albedo[0]);
            }

            if (ImGui::CollapsingHeader("skybox")) {
                skyboxChanged |= ImGui::ColorPicker3("SkyColor", &m_skyboxGenData.SkyColor[0]);
                skyboxChanged |= ImGui::ColorPicker3("GroundColor", &m_skyboxGenData.GroundColor[0]);
                skyboxChanged |= ImGui::SliderFloat("GroundTransition", &m_skyboxGenData.GroundTransition, 0, 180);
            }
            ImGui::EndChild();

            if (skyboxChanged) {
                m_skyboxActual = false;
            }
        }
        ImGui::End();        

    }

public:

    MaterialDatabase materialDatabase;
    
    Scene(RenderContext& context): 
        m_renderContext(context), materialDatabase(context), 
        m_indices(), m_vertices(), m_models(), m_modelPassMapping()
    {
        PopulateScene();
        InitializeResources();
    }

    uint32_t AddMaterial(Material material) {
        uint32_t material_index = m_materials.size();
        m_materials.push_back(material);
        return material_index;
    }

    uint32_t AddObject(const char* path, std::initializer_list<Pass*> materialPasses) {
        
        std::string key = path;

        auto entry = m_loadedModels.find(key);
        if (entry == m_loadedModels.end()) {
            uint32_t indexStart = m_indices.size();
            loadModel(m_renderContext, path, m_vertices, m_indices);
            
            uint32_t indexSize = m_indices.size() - indexStart;
            m_loadedModels[key] = std::make_tuple(indexStart, indexSize);
            entry = m_loadedModels.find(key);
        }

        uint32_t indexStart;
        uint32_t indexSize;
        std::tie(indexStart, indexSize) = entry->second;
        
        uint32_t modelIndex = m_models.size();

        m_models.push_back(ModelData {
            .indexOffset = indexStart,
            .indexSize = indexSize,
            .modelTransform = glm::identity<glm::mat4x4>()
        });

        for (Pass* material : materialPasses) {
            m_modelPassMapping[material].push_back(modelIndex);
        }

        return modelIndex;
    }

    template<typename P>
    void DrawPass(DrawContext& drawContext, P& pass) {
        pass.Run(drawContext, m_modelPassMapping[&pass]);
    }

    void OnBeginFrame() {

        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        DispalyUI();

        for (Material& material : m_materials) {
            material.range = m_renderContext.Get<DynamicUniforms>().Allocate(MaterialData {
                glm::vec4(material.f0, material.roughness),
                glm::vec4(material.albedo, material.metallness),
                material.imageId
            });
        }
        
        for (ModelData& model : m_models) {
            auto transform = glm::rotate(
                model.modelTransform, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                
            ModelTransforms transforms {
                transform,
                glm::inverse(transform)
            };
            model.transformsRange = m_renderContext.Get<DynamicUniforms>().Allocate(transforms);
            model.materialRange = m_materials[model.material].range;
        }
    }

    void OnPrepareDraw(ResourceRef<Image> output) {

        if (m_resources.depthBuffer->description.width != output->description.width ||
            m_resources.depthBuffer->description.height != output->description.height) {

            m_resources.depthBuffer = m_renderContext.Get<Resources>().Resize(m_resources.depthBuffer, {output->description.width, output->description.height});
        }

        CameraData shadowmapCamera = GetShadowmapViewProjection();
        BufferRegion directShadowmapRange = m_renderContext.Get<DynamicUniforms>()
            .Allocate(shadowmapCamera);
        BufferRegion mainViewCameraRange = m_renderContext.Get<DynamicUniforms>()
            .Allocate(UpdateCameraPosition());
        
        m_lights.dirLightTransform = shadowmapCamera.projection * shadowmapCamera.view;
        m_lights.specularMipCount = static_cast<float>(m_resources.specularIbl->description.mipLevels);

        DrawContext drawContext {
            .mainViewCamera = mainViewCameraRange,
            .lights = m_renderContext.Get<DynamicUniforms>().Allocate(m_lights),
            .output = output,
            .depth = m_resources.depthBuffer,
            .resources = m_resources,
            .models = m_models,
            .context = m_renderContext
        };

        if (!m_lutGenerated) {
            generateBrdf.Run(m_resources.brdfLut);
            m_lutGenerated = true;
        }
        
        if (!m_skyboxActual) {
            postProcessSkyboxPass.Run(m_resources.skybox_raw, m_resources.linearSampler, m_resources.skybox);
            // skyboxPass.Run(m_resources.skybox, drawContext.lights, m_skyboxGenData);
            generateIblPass.Run(m_resources.skybox, m_resources.linearSampler, m_resources.diffuseIbl, m_resources.specularIbl);
            m_skyboxActual = true;
        }
        drawSkyboxPass.Run(output, m_resources.skybox, m_resources.linearSampler, mainViewCameraRange);
        
        m_renderContext.Get<RenderGraph>().AddNode<ClearImageNode>(m_resources.depthBuffer);
        DrawPass(drawContext, depthPass);

        drawContext.mainViewCamera = directShadowmapRange;
        drawContext.depth = m_resources.directShadowmap;
        m_renderContext.Get<RenderGraph>().AddNode<ClearImageNode>(m_resources.directShadowmap);
        DrawPass(drawContext, directShadowmapPass);

        drawContext.depth = m_resources.depthBuffer;
        drawContext.mainViewCamera = mainViewCameraRange;
        
        DrawPass(drawContext, colorPass);
        
    }

    void OnEngFrame() {

    }

};