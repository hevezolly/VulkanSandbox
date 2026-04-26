#pragma once

#include <vulkan_engine.h>
#include "..\draw_context.h"
#include "..\material_database.h"

template<typename Attachments, typename... Bindings>
struct GeometryPass: Pass {

    GeometryPass(): Pass(){}
    GeometryPass(RenderContext* context, MaterialDatabase* materialDatabase): Pass(context, materialDatabase) {}
    
    virtual void Run(const DrawContext& context, std::vector<uint32_t>& models) = 0;
    
};