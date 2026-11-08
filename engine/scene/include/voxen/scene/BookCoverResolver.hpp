#pragma once
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace voxen::gallery {

// Logical cover IDs, not arbitrary filesystem paths. Bundled assets are read
// by Android AssetManager; app-private overrides are resolved on the host FS.
inline bool validBookCoverId(std::string_view id) {
    if (id.empty() || id.size() > 80) return false;
    return std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
               c == '-' || c == '_';
    });
}

struct BookCoverLocation {
    enum class Source { Override, Bundled, Procedural };
    Source source = Source::Procedural;
    std::string path;
};

// `privateFiles` is the Android app-specific files directory. Never resolve
// caller-supplied paths; only a validated logical ID and fixed extensions.
inline BookCoverLocation resolveBookCover(
    const std::filesystem::path& privateFiles, std::string_view id) {
    if (!validBookCoverId(id)) return {};
    const auto stem = std::string(id);
    for (const char* ext : {".jpg", ".png"}) {
        const auto candidate = privateFiles / "Books" / (stem + ext);
        std::error_code ec;
        if (std::filesystem::is_regular_file(candidate, ec) && !ec)
            return {BookCoverLocation::Source::Override, candidate.string()};
    }
    // Bundled existence is checked by Android AssetManager at load time.
    return {BookCoverLocation::Source::Bundled, "books/" + stem + ".jpg"};
}

} // namespace voxen::gallery
