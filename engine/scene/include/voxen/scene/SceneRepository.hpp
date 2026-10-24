#pragma once
#include <voxen/scene/RoomGallery.hpp>
#include <voxen/scene/SceneIds.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace voxen::gallery {

// Versioned, app-private metadata store. Scene payloads are opaque binary blobs;
// serialization of the full live Voxen runtime is a separate integration task.
class SceneRepository {
public:
    explicit SceneRepository(std::filesystem::path root) : root_(std::move(root)) {}

    // Allocates a stable ID once; the caller owns the returned record.
    static SceneRecord newScene(std::string name) {
        SceneRecord r;
        r.id = newUuid();
        r.displayName = std::move(name);
        r.payloadAsset = "payload-" + newUuid() + ".bin";
        return r;
    }

    bool saveScene(const SceneRecord& r, const std::string& payload) const {
        if (!safe(r.id) || !safe(r.payloadAsset) || r.schemaVersion != 1 ||
            r.displayName.size() > 4096 || r.preview.proxyAsset.size() > 4096 ||
            !radius(r.preview.boundingRadiusMeters) || payload.size() > 64u*1024u*1024u) return false;
        std::ostringstream meta;
        meta << "VOXEN_SCENE 1\n" << std::quoted(r.id) << '\n'
             << std::quoted(r.displayName) << '\n' << std::quoted(r.payloadAsset) << '\n'
             << std::quoted(r.preview.proxyAsset) << '\n'
             << std::setprecision(std::numeric_limits<float>::max_digits10)
             << r.preview.boundingRadiusMeters << ' ' << r.preview.materialPreset << '\n';
        auto dir = root_ / "scenes" / r.id;
        // Use immutable payload filenames per revision so metadata is the commit point.
        return write(dir / r.payloadAsset, payload) && write(dir / "metadata.vxs", meta.str());
    }

    std::optional<SceneRecord> loadSceneMetadata(const SceneId& id) const {
        if (!safe(id)) return std::nullopt;
        auto m = read(root_ / "scenes" / id / "metadata.vxs", 16384);
        if (!m) return std::nullopt;
        std::istringstream in(*m);
        std::string magic; unsigned version = 0;
        SceneRecord r;
        if (!(in >> magic >> version) || magic != "VOXEN_SCENE" || version != 1 ||
            !(in >> std::quoted(r.id) >> std::quoted(r.displayName)
                 >> std::quoted(r.payloadAsset) >> std::quoted(r.preview.proxyAsset)
                 >> r.preview.boundingRadiusMeters >> r.preview.materialPreset) ||
            r.id != id || !safe(r.payloadAsset) || r.displayName.size() > 4096 ||
            r.preview.proxyAsset.size() > 4096 || !radius(r.preview.boundingRadiusMeters))
            return std::nullopt;
        return r;
    }

    std::optional<std::pair<SceneRecord, std::string>> loadScene(const SceneId& id) const {
        auto r=loadSceneMetadata(id);
        if (!r) return std::nullopt;
        auto payload=read(root_ / "scenes" / id / r->payloadAsset, 64u*1024u*1024u);
        if (!payload) return std::nullopt;
        return std::make_pair(std::move(*r),std::move(*payload));
    }

    // Remove a saved scene and its portal placements from every persisted room.
    // Refuse deletion if a gallery cannot be updated, to avoid dangling portals.
    bool deleteScene(const SceneId& id) const {
        if(!safe(id) || !loadSceneMetadata(id)) return false;
        std::error_code ec;
        const auto rooms=root_/"rooms";
        for(std::filesystem::directory_iterator it(rooms,ec),end;
            !ec && it!=end;it.increment(ec)) {
            if(!it->is_regular_file(ec) || it->path().extension()!=".vxg")continue;
            const auto roomId=it->path().stem().string();
            auto gallery=loadGallery(roomId);
            if(!gallery)return false;
            const auto previous=gallery->placements.size();
            gallery->placements.erase(
                std::remove_if(gallery->placements.begin(),gallery->placements.end(),
                    [&](const Placement& p){return p.sceneId==id;}),
                gallery->placements.end());
            if(gallery->placements.size()!=previous && !saveGallery(*gallery))
                return false;
        }
        if(ec)return false;
        std::filesystem::remove_all(root_/"scenes"/id,ec);
        return !ec;
    }

    std::vector<SceneId> listScenes() const {
        std::vector<SceneId> ids;
        std::error_code ec;
        for (std::filesystem::directory_iterator it(root_/"scenes", ec), end;
             !ec && it != end; it.increment(ec)) {
            if (!it->is_directory(ec)) continue;
            auto id = it->path().filename().string();
            if (safe(id) && loadSceneMetadata(id)) ids.push_back(std::move(id));
        }
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    bool saveGallery(const RoomGallery& g) const {
        if (!safe(g.roomId) || g.persistentAnchorId.empty() ||
            g.persistentAnchorId.size() > 4096 || g.placements.size() > 2048) return false;
        std::ostringstream out;
        out << "VOXEN_GALLERY 1\n" << std::quoted(g.roomId) << '\n'
            << std::quoted(g.persistentAnchorId) << '\n' << g.placements.size() << '\n'
            << std::setprecision(std::numeric_limits<float>::max_digits10);
        for (const auto& p : g.placements) {
            if (!safe(p.id) || !safe(p.sceneId) || !scale(p.scale) || !loadSceneMetadata(p.sceneId)) return false;
            float norm = 0;
            for (float x : p.roomLocalPose.positionMeters) if (!std::isfinite(x)) return false;
            for (float x : p.roomLocalPose.orientationXyzw) {
                if (!std::isfinite(x)) return false;
                norm += x*x;
            }
            if (norm < 0.5f || norm > 1.5f) return false;
            out << std::quoted(p.id) << ' ' << std::quoted(p.sceneId) << ' ' << p.scale;
            for (float x : p.roomLocalPose.positionMeters) out << ' ' << x;
            for (float x : p.roomLocalPose.orientationXyzw) out << ' ' << x;
            out << '\n';
        }
        return write(root_/"rooms"/(g.roomId+".vxg"), out.str());
    }

    std::optional<RoomGallery> loadGallery(const RoomId& id) const {
        if (!safe(id)) return std::nullopt;
        auto data = read(root_/"rooms"/(id+".vxg"), 1024*1024);
        if (!data) return std::nullopt;
        std::istringstream in(*data);
        std::string magic; unsigned version=0; size_t count=0;
        RoomGallery g;
        if (!(in >> magic >> version) || magic!="VOXEN_GALLERY" || version!=1 ||
            !(in >> std::quoted(g.roomId) >> std::quoted(g.persistentAnchorId) >> count) ||
            g.roomId!=id || g.persistentAnchorId.empty() ||
            g.persistentAnchorId.size()>4096 || count>2048) return std::nullopt;
        for (size_t i=0;i<count;++i) {
            Placement p;
            if (!(in >> std::quoted(p.id) >> std::quoted(p.sceneId) >> p.scale) ||
                !safe(p.id) || !safe(p.sceneId) || !scale(p.scale)) return std::nullopt;
            for (float& x : p.roomLocalPose.positionMeters)
                if (!(in >> x) || !std::isfinite(x)) return std::nullopt;
            float norm=0;
            for (float& x : p.roomLocalPose.orientationXyzw) {
                if (!(in >> x) || !std::isfinite(x)) return std::nullopt;
                norm+=x*x;
            }
            if (norm<0.5f || norm>1.5f) return std::nullopt;
            g.placements.push_back(std::move(p));
        }
        return g;
    }

private:
    std::filesystem::path root_;
    static bool safe(const std::string& s) {
        if (s.empty() || s.size()>128 || s=="." || s=="..") return false;
        for (unsigned char c : s)
            if (!((c>='a'&&c<='z') || (c>='A'&&c<='Z') ||
                  (c>='0'&&c<='9') || c=='-' || c=='_' || c=='.')) return false;
        return true;
    }
    static bool radius(float x) { return std::isfinite(x) && x>0 && x<100; }
    static bool scale(float x) { return std::isfinite(x) && x>0 && x<100; }
    static std::optional<std::string> read(const std::filesystem::path& p, size_t max) {
        std::ifstream f(p, std::ios::binary|std::ios::ate);
        if (!f) return std::nullopt;
        auto n=f.tellg();
        if (n<0 || static_cast<std::uintmax_t>(n)>max) return std::nullopt;
        std::string s(static_cast<size_t>(n), '\0');
        f.seekg(0);
        if (!f.read(s.data(), n)) return std::nullopt;
        return s;
    }
    static bool write(const std::filesystem::path& p, const std::string& s) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
        if (ec) return false;
        // Unique temporary path avoids clobbering another pending writer.
        auto tmp=p; tmp += "." + newUuid() + ".tmp";
        {
            std::ofstream f(tmp, std::ios::binary|std::ios::trunc);
            if (!f || !f.write(s.data(), static_cast<std::streamsize>(s.size())) || !f.flush())
                return false;
        }
        std::filesystem::rename(tmp,p,ec);
        if (ec) { std::filesystem::remove(tmp); return false; }
        return true;
    }
};
} // namespace voxen::gallery
