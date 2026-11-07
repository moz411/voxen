#pragma once
#include <voxen/scene/RoomGallery.hpp>
#include <algorithm>
#include <cmath>

namespace voxen::gallery {

// Pure per-frame control signal for the existing audio engine. Spatial
// rendering/panning and decoder lifecycle belong to the audio backend.
inline float portalProximityGain(const PortalAudio& audio, float distanceMeters) {
    if(audio.filename.empty() || !std::isfinite(distanceMeters) ||
       !std::isfinite(audio.audibleRadiusMeters) ||
       audio.audibleRadiusMeters<=0.f) return 0.f;
    const float t=std::clamp(1.f-distanceMeters/audio.audibleRadiusMeters,0.f,1.f);
    const float smooth=t*t*(3.f-2.f*t);
    return std::clamp(audio.gain,0.f,1.f)*smooth*smooth;
}

} // namespace voxen::gallery
