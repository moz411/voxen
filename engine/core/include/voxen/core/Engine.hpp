#pragma once

namespace voxen::core {

class Engine final {
public:
    void initialize();
    void shutdown() noexcept;

    [[nodiscard]] bool initialized() const noexcept { return initialized_; }

private:
    bool initialized_ = false;
};

} // namespace voxen::core
