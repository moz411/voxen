#include <voxen/core/Engine.hpp>

#include <stdexcept>

namespace voxen::core {

void Engine::initialize() {
    if (initialized_) {
        throw std::logic_error("Voxen engine is already initialized");
    }

    initialized_ = true;
}

void Engine::shutdown() noexcept {
    initialized_ = false;
}

} // namespace voxen::core
