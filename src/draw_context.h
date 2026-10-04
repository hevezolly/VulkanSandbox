#pragma once
#include <vulkan_engine.h>

#define BLOCK_NAME Transforms
#define BLOCK \
DYNAMIC_UNIFORM(camera, 0, Stage::Vertex | Stage::Fragment) \
DYNAMIC_UNIFORM(model, 1, Stage::Vertex)
#include <gen_bindings.h>

struct ModelTransforms {
    glm::mat4x4 transform;
    glm::mat4x4 transformInv;
};

#define BLOCK_NAME FullScreenQuad
#define BLOCK \
IMAGE_SAMPLER(image, 0, Stage::Fragment) \
DYNAMIC_UNIFORM(data, 1, Stage::Fragment)
#include <gen_bindings.h>

struct DebugData {
    float minValue;
    float maxValue;
};

#define BLOCK_NAME Textures
#define BLOCK \
DYNAMIC_UNIFORM(textureIds, 0, Stage::Fragment) \
IMAGE_SAMPLER(colorTexture, 1, Stage::Fragment)
#include <gen_bindings.h>

#define BLOCK_NAME Lights
#define BLOCK \
DYNAMIC_UNIFORM(lightsConfig, 0, Stage::Fragment) \
IMAGE_SAMPLER(directShadowmap, 1, Stage::Fragment) \
IMAGE_SAMPLER(diffuseIBL, 2, Stage::Fragment) \
IMAGE_SAMPLER(specularIBL, 3, Stage::Fragment) \
IMAGE_SAMPLER(brdfLookup, 4, Stage::Fragment)
#include <gen_bindings.h>

struct LightsConfig {
    glm::mat4x4 dirLightTransform;
    glm::vec4 dirLightDirection;
    glm::vec4 dirLightColor;
    float depthBias;
    float specularMipCount;
};

struct ModelData {
    uint32_t indexOffset;
    uint32_t indexSize;

    glm::mat4x4 modelTransform;
    uint32_t imageId;
            
    BufferRegion transformsRange;
    BufferRegion textureIds;
};

struct ResourcesRefs {
    ResourceRef<Sampler> linearSampler;
    ResourceRef<Buffer> vertexBuffer;
    ResourceRef<Buffer> indexBuffer;
    ResourceRef<Image> depthBuffer;
    ResourceRefs<Image> images;
    ResourceRef<Image> directShadowmap;
    ResourceRef<Image> skybox;
    ResourceRef<Image> diffuseIbl;
    ResourceRef<Image> specularIbl;
    ResourceRef<Image> brdfLut;
};

struct DrawContext {
    BufferRegion mainViewCamera;
    BufferRegion lights;
    ResourceRef<Image> output;
    ResourceRef<Image> depth;
    ResourcesRefs& resources;
    std::vector<ModelData>& models;
    RenderContext& context;
};