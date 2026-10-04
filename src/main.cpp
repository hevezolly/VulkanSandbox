#include <vulkan_engine.h>
#include <vulkan_engine_ui.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include "Scene.h"

int main() {

    RenderContext context;
    WindowInitializer window {
        .width = 800,
        .height = 600,
        .hint = "Sandbox"
    };
    SwapChainInitializer swapChain {
        .desiredFormats = {VK_FORMAT_R8G8B8A8_UNORM},
        .imageUsage = ImageUsage::ColorAttachment | ImageUsage::TransferDst | ImageUsage::Storage,
        .imageCount = 3
    };
    context
        .WithFeature<PresentFeature>(window, swapChain)
        .WithFeature<GraphicsFeature>()
        .WithFeature<Compute>()
        .WithFeature<FrameDispatcher>(3)
        .WithFeature<Registry>("resources")
        .WithFeature<Allocator>(20240)
        .WithFeature<RenderGraph>()
        .WithFeature<DynamicUniforms>()
        .WithFeature<ImguiUI>();

    Initialize(context);

    {
        Scene scene(context);


        while (!glfwWindowShouldClose(context.Get<PresentFeature>().window->pWindow)) {
            glfwPollEvents();

            context.BeginFrame();
            
            ResourceRef<Image> drawImage = context.Get<PresentFeature>().AcquireNextImage();
            scene.OnBeginFrame();
            scene.OnPrepareDraw(drawImage);

            RenderGraph& graph = context.Get<RenderGraph>();
            graph.AddNode<ImguiNode>(drawImage).SetName("ui node");
            graph.AddNode<PresentNode>(drawImage).SetName("present node");
            graph.Run();

            scene.OnEngFrame();
        }
    }

    std::cout << "finish" << std::endl;
    return 0;
}