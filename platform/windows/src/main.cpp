#include <chrono>
#include <cstdio>
#include <exception>
#include <thread>

#include <voxen/core/Engine.hpp>
#include <voxen/render/VulkanContext.hpp>
#include <voxen/render/XrSwapchainRenderer.hpp>
#include <voxen/xr/XrContext.hpp>
#include <voxen/xr/XrSession.hpp>

int main() {
    voxen::core::Engine engine;
    voxen::xr::XrContext xr;
    voxen::render::VulkanContext vulkan;
    voxen::xr::XrSession session;
    voxen::render::XrSwapchainRenderer renderer;

    try {
        engine.initialize();
        xr.initialize();
        vulkan.initialize(xr.instance(), xr.systemId());
        session.initialize(
            xr.instance(),
            xr.systemId(),
            vulkan.instance(),
            vulkan.physicalDevice(),
            vulkan.device(),
            vulkan.graphicsQueueFamily(),
            xr.passthroughEnabled());
        renderer.initialize(
            xr.instance(),
            xr.systemId(),
            session.handle(),
            vulkan.device(),
            vulkan.graphicsQueue(),
            vulkan.graphicsQueueFamily());

        std::printf(
            "[Voxen] OpenXR session created: instance=%p system=%llu\n",
            reinterpret_cast<void*>(xr.instance()),
            static_cast<unsigned long long>(xr.systemId()));

        bool running = true;
        while (running) {
            running = session.pollEvents();
            if (session.running()) {
                session.frame(renderer);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "[Voxen] Fatal: %s\n", error.what());
    } catch (...) {
        std::fprintf(stderr, "[Voxen] Fatal: unknown error\n");
    }

    renderer.shutdown();
    session.shutdown();
    vulkan.shutdown();
    xr.shutdown();
    engine.shutdown();
    return 0;
}
