#include <stereokit.h>
#include <stereokit_ui.h>
#include <openxr/openxr.h>
#include "voxen_voxel.hlsl.h"
#include "voxel_shapes.h"
#include "shader_registry.h"
#include "artwork.h"
#include "scene_background.h"
#include <cstdio>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace sk;

sk_settings_t app_settings = {"Voxen StereoKit"};
static pose_t palette = {{-0.25f, 0.0f, -0.55f}, quat_identity};
static int menu_page = 0;
static constexpr int page_count = 8;
static constexpr float menu_width = 0.17f;
static constexpr float menu_height = 0.46f;
static float carousel_angle = 0.0f;
static float target_angle = 0.0f;
static pose_t carousel_base = pose_identity;
static bool menu_aiming = false;
static bool menu_trigger_down = false;
static bool menu_click = false;
static bool menu_capture = false;
static int menu_slider_capture = -1;
static vec3 menu_hit{};
static float menu_hit_x = 0.0f;
static float menu_hit_y = 0.0f;
static material_t carousel_mat = nullptr;
static text_style_t panel_text_style = 0;
static bool stick_latched = false;
static bool show_ui_ray = true;
static float voxel_size = 0.04f;
static int selected_shape = 0;
static std::array<mesh_t,voxen_shapes::count> shape_meshes{};
static sound_t click_sound = nullptr;
static sound_t tones[3] = {nullptr,nullptr,nullptr};
static int sound_mode = 0; // 0 silence, 1 soft bell, 2 bright pulse, 3 bass
static int effect_mode = 0; // 0 solid, 1 breathe, 2 color wave, 3 shimmer
static float animation_time = 0.0f;
static int lorenz_playback = 1; // 0 static, 1 once, 2 loop
static int lorenz_speed = 1; // 0 slow, 1 normal, 2 fast
static int generator_playback = 1; // 0 static, 1 once, 2 loop
static int generator_speed = 1;
static int generator_sound = 0;
static int note_stride = 0;
static int algorithm_mode = -1;
static int gpu_preset = 0;
static float lorenz_sigma=10.0f, lorenz_rho=28.0f, lorenz_beta=8.0f/3.0f;
static float fractal_zoom=1.0f, fractal_center_x=0.0f, fractal_center_y=0.0f;
static float julia_real=-0.745f, julia_imag=0.186f;
static int fractal_iterations=64;
static float rossler_a=0.2f, rossler_b=0.2f, rossler_c=5.7f;
static float fibonacci_turns=5.0f, fibonacci_spacing=1.0f;
static shader_t voxel_shader = nullptr;
static material_t gpu_materials[voxen_effects::count] = {};
static std::array<material_t,32> background_materials{};
static voxen_scene::Background scene_background;
static uint32_t brush_step = 0;
static bool volume_active = false;
static bool save_confirmation = false;
static bool restore_confirmation = false;
static const char* storage_status = "NOT SAVED";
static vec3 volume_start{};
static vec3 volume_end{};

static float hue = 0.45f;
static bool painting = true;
static bool edges_enabled = true;
static float camera_opacity = 0.0f;
static float edge_hue = 0.5f;
static float background_hue = 0.64f;
static float background_brightness = 0.33f;
static mesh_t cube = nullptr;
static material_t material = nullptr;
struct PaintedVoxel {
    vec3 position;
    float size;
    color128 tint;
    int shape;
    int effect;
    int gpu_preset;
    float phase;
    float reveal_start; // Seconds since application start
    float reveal_progress; // Normalized sample position, 0..1
    float reveal_duration; // Zero = always visible
    bool reveal_loop;
};
static std::vector<PaintedVoxel> voxels;
static std::vector<voxen::Artwork> artworks;
static std::vector<voxen::ArtworkSnapshot<PaintedVoxel>> redo_artworks;
struct PropertyEdit { uint64_t artwork_id; bool is_shader; int before; int after; };
static std::vector<PropertyEdit> property_undo, property_redo;
static bool applying_property_history=false;
static uint64_t next_artwork_id = 1;
static uint64_t selected_artwork_id = 0;
static bool selection_mode = false;
static bool selection_trigger_latched = false;
static bool stroke_active = false;
static voxen::Artwork pending_artwork{};
// Begin a semantic artwork before generating any voxels.
static void begin_artwork(vec3 origin, float extent = 0.0f) {
    property_undo.clear(); property_redo.clear();
    if (stroke_active || voxels.size() >= 4096) return;
    pending_artwork = {};
    pending_artwork.id = next_artwork_id++;
    pending_artwork.first_voxel = static_cast<uint32_t>(voxels.size());
    pending_artwork.generator = algorithm_mode;
    pending_artwork.shape = selected_shape;
    pending_artwork.animation = effect_mode;
    pending_artwork.shader = gpu_preset;
    pending_artwork.sound = generator_sound;
    pending_artwork.origin = origin;
    pending_artwork.extent = extent;
    stroke_active = true;
}
// Finalize only non-empty artworks. Undo/redo works on semantic units.
static void finish_artwork() {
    if (!stroke_active) return;
    stroke_active = false;
    pending_artwork.voxel_count =
        static_cast<uint32_t>(voxels.size()) - pending_artwork.first_voxel;
    if (pending_artwork.voxel_count) artworks.push_back(pending_artwork);
}
static constexpr size_t max_voxels = 4096;
static bool trigger_down = false;
static bool last_paint_valid = false;
static vec3 last_paint{};
static float brush_distance = 0.025f;
static bool shape_stick_latched = false;

static int hit_page = -1;
static float trigger_threshold = 0.65f;

static float dist(vec3 a, vec3 b) {
    const float dx = a.x-b.x, dy = a.y-b.y, dz = a.z-b.z;
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}
static vec3 interpolate(vec3 a, vec3 b, float t) {
    return {a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t};
}
// OpenXR/StereoKit aim is oriented along local -Z.
static vec3 aim_forward(quat q) {
    return {-2*(q.x*q.z+q.w*q.y),
            -2*(q.y*q.z-q.w*q.x),
            -(1-2*(q.x*q.x+q.y*q.y))};
}

static void add_voxel(vec3 p,float size) {
    if(voxels.size()>=max_voxels) return;
    // Keep the same collision checks as normal painting.
    for(const auto& v:voxels) {
        const float gap=(v.size+size)*0.5f-0.0001f;
        if(std::fabs(v.position.x-p.x)<gap &&
           std::fabs(v.position.y-p.y)<gap &&
           std::fabs(v.position.z-p.z)<gap) return;
    }
    redo_artworks.clear();
    const float phase=p.x*11+p.y*7+p.z*13;
    voxels.push_back({p,size,color_hsv(hue,1,1,1),selected_shape,effect_mode,gpu_preset,phase,0.0f,0.0f,0.0f,false});
    if(sound_mode>0 && tones[sound_mode-1] && (++note_stride%8)==1)
        sound_play(tones[sound_mode-1],p,0.2f);
}
static void paint_at(vec3 p) {
    if(voxels.size()>=max_voxels) return;
    const float size=voxel_size;
    p={std::round(p.x/size)*size,std::round(p.y/size)*size,
       std::round(p.z/size)*size};
    add_voxel(p,size);
}
// First artwork in "Attracteurs étranges": the real Lorenz system.
// RK4 integrates the classic (sigma=10, rho=28, beta=8/3) flow in phase space.
// The resulting trajectory is uniformly fitted inside the trigger-defined cube,
// preserving the attractor's proportions instead of faking its butterfly shape.
struct LorenzPoint { float x,y,z; };
static LorenzPoint lorenz_velocity(const LorenzPoint& p) {
    return {lorenz_sigma*(p.y-p.x),p.x*(lorenz_rho-p.z)-p.y,
            p.x*p.y-lorenz_beta*p.z};
}
static LorenzPoint lorenz_add(const LorenzPoint& a,const LorenzPoint& b,float scale) {
    return {a.x+b.x*scale,a.y+b.y*scale,a.z+b.z*scale};
}
static LorenzPoint lorenz_advance(const LorenzPoint& p,float dt) {
    const auto k1=lorenz_velocity(p);
    const auto k2=lorenz_velocity(lorenz_add(p,k1,dt*0.5f));
    const auto k3=lorenz_velocity(lorenz_add(p,k2,dt*0.5f));
    const auto k4=lorenz_velocity(lorenz_add(p,k3,dt));
    return {p.x+dt*(k1.x+2*k2.x+2*k3.x+k4.x)/6.0f,
            p.y+dt*(k1.y+2*k2.y+2*k3.y+k4.y)/6.0f,
            p.z+dt*(k1.z+2*k2.z+2*k3.z+k4.z)/6.0f};
}
static void generate_lorenz(const vec3& center,float side) {
    constexpr int warmup=1500;
    constexpr int samples=2600;
    constexpr float dt=0.008f;
    LorenzPoint p={0.1f,0.0f,0.0f};
    for(int i=0;i<warmup;++i) p=lorenz_advance(p,dt);
    std::vector<LorenzPoint> path;
    path.reserve(samples);
    LorenzPoint low=p, high=p;
    for(int i=0;i<samples;++i) {
        p=lorenz_advance(p,dt);
        path.push_back(p);
        low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};
        high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};
    }
    const float extent=std::max({high.x-low.x,high.y-low.y,high.z-low.z,0.001f});
    const float scale=side*0.88f/extent;
    const LorenzPoint mid={(low.x+high.x)*0.5f,(low.y+high.y)*0.5f,
                           (low.z+high.z)*0.5f};
    // X,Y are the butterfly's horizontal axes, Z maps to world-up.
    // Small voxels keep the trajectory recognizable even for short gestures.
    const float size=std::max(0.003f,side/70.0f);
    vec3 prev{};
    bool have_prev=false;
    const int previous_sound=sound_mode;
    const float duration=generator_speed==0?20.0f:(generator_speed==2?4.0f:9.0f);
    const float start_time=animation_time;
    sound_mode=0; // Avoid hundreds of simultaneous spatial notes at commit.
    for(size_t point_index=0;point_index<path.size();++point_index) {
        const auto& q=path[point_index];
        if(voxels.size()>=max_voxels) break;
        const vec3 at={center.x+(q.x-mid.x)*scale,
                       center.y+(q.z-mid.z)*scale,
                       center.z+(q.y-mid.y)*scale};
        if(have_prev && dist(at,prev)<size*0.85f) continue;
        const size_t before=voxels.size();
        add_voxel(at,size);
        if(voxels.size()>before && generator_playback!=0) {
            auto& v=voxels.back();
            v.reveal_start=start_time;
            v.reveal_progress=static_cast<float>(point_index)/
                               static_cast<float>(samples);
            v.reveal_duration=duration;
            v.reveal_loop=(generator_playback==2);
        }
        prev=at;
        have_prev=true;
    }
    sound_mode=previous_sound;
    if(previous_sound>0 && tones[previous_sound-1])
        sound_play(tones[previous_sound-1],center,0.22f);
}
// A planar fractal is sampled in its native complex plane, never extruded into
// an arbitrary height field. The current preview is world-upright XY; a later
// iteration can orient the canvas using the controller's release pose.
static int complex_escape(float real, float imag, bool julia) {
    float zr = julia ? real : 0.0f;
    float zi = julia ? imag : 0.0f;
    const float cr = julia ? julia_real : real;
    const float ci = julia ? julia_imag : imag;
    const int max_iterations = fractal_iterations;
    for (int i = 0; i < max_iterations; ++i) {
        if (zr*zr + zi*zi > 4.0f) return i;
        const float next_real = zr*zr - zi*zi + cr;
        zi = 2.0f*zr*zi + ci;
        zr = next_real;
    }
    return max_iterations;
}
static void generate_planar_fractal(const vec3& center, float side, bool julia) {
    constexpr int resolution = 46;
    const float spacing = side / resolution;
    // One flat voxel layer with a maximum of 2116 candidates.
    const float voxel_edge = spacing * 0.86f;
    const int previous_sound = sound_mode;
    sound_mode = 0;
    for (int iy = 0; iy < resolution && voxels.size() < max_voxels; ++iy)
        for (int ix = 0; ix < resolution && voxels.size() < max_voxels; ++ix) {
            const float u = (ix + 0.5f) / resolution;
            const float v = (iy + 0.5f) / resolution;
            const float real = (julia ? 0.0f : -0.75f) + fractal_center_x
                               + (u-0.5f)*3.0f/fractal_zoom;
            const float imag = fractal_center_y + (v-0.5f)*3.0f/fractal_zoom;
            // Keep interior and nearby high-iteration boundary for a legible
            // silhouette; do not fill the entire exterior of the canvas.
            if (complex_escape(real, imag, julia) < fractal_iterations*3/8) continue;
            add_voxel({center.x + (ix + 0.5f - resolution*0.5f)*spacing,
                       center.y + (iy + 0.5f - resolution*0.5f)*spacing,
                       center.z}, voxel_edge);
        }
    sound_mode = previous_sound;
    if (previous_sound > 0 && tones[previous_sound-1])
        sound_play(tones[previous_sound-1], center, 0.22f);
}
// Rossler ODE integrated with RK4, using the same bounded representation
// strategy as Lorenz.
struct RPoint { float x,y,z; };
static RPoint rossler_velocity(RPoint p) {
    return {-p.y-p.z,p.x+rossler_a*p.y,
            rossler_b+p.z*(p.x-rossler_c)};
}
static RPoint rossler_step(RPoint p,float dt) {
    auto add=[](RPoint a,RPoint b,float k) -> RPoint {
        return {a.x+k*b.x,a.y+k*b.y,a.z+k*b.z};
    };
    const auto k1=rossler_velocity(p),k2=rossler_velocity(add(p,k1,dt*0.5f));
    const auto k3=rossler_velocity(add(p,k2,dt*0.5f));
    const auto k4=rossler_velocity(add(p,k3,dt));
    return {p.x+dt*(k1.x+2*k2.x+2*k3.x+k4.x)/6.0f,
            p.y+dt*(k1.y+2*k2.y+2*k3.y+k4.y)/6.0f,
            p.z+dt*(k1.z+2*k2.z+2*k3.z+k4.z)/6.0f};
}
static void generate_rossler(vec3 center,float side) {
    constexpr int warmup=1000,samples=2600;
    constexpr float dt=0.025f;
    RPoint p{1,0,0};
    for(int i=0;i<warmup;++i)p=rossler_step(p,dt);
    std::vector<RPoint> points; points.reserve(samples);
    RPoint low=p,high=p;
    for(int i=0;i<samples;++i) {
        p=rossler_step(p,dt);
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))break;
        points.push_back(p);
        low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};
        high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};
    }
    if(points.empty())return;
    const float extent=std::max({high.x-low.x,high.y-low.y,high.z-low.z,0.001f});
    const float scale=side*0.88f/extent,edge=std::max(0.003f,side/70.0f);
    const vec3 mid{(low.x+high.x)*0.5f,(low.z+high.z)*0.5f,
                   (low.y+high.y)*0.5f};
    const int old_sound=sound_mode; sound_mode=0;
    for(const auto& q:points) {
        if(voxels.size()>=max_voxels)break;
        add_voxel({center.x+q.x*scale-mid.x*scale,
                   center.y+q.z*scale-mid.y*scale,
                   center.z+q.y*scale-mid.z*scale},edge);
    }
    sound_mode=old_sound;
}
static void generate_fibonacci(vec3 center,float side) {
    // Logarithmic golden spiral in a vertical XY plane.
    constexpr float pi=3.14159265358979323846f;
    constexpr float golden=1.61803398875f;
    const int samples=1500;
    const float turns=fibonacci_turns;
    const float max_theta=2*pi*turns;
    const float growth=std::log(golden)/(pi*0.5f);
    const float max_radius=std::exp(growth*max_theta);
    const float edge=std::max(0.003f,side/100.0f);
    const int old_sound=sound_mode; sound_mode=0;
    for(int i=0;i<samples&&voxels.size()<max_voxels;++i) {
        const float theta=max_theta*i/(samples-1);
        const float radius=0.45f*side*fibonacci_spacing*
                           std::exp(growth*theta)/max_radius;
        add_voxel({center.x+radius*std::cos(theta),
                   center.y+radius*std::sin(theta),center.z},edge);
    }
    sound_mode=old_sound;
}
static void commit_volume() {
    if(!volume_active) return;
    volume_active=false;
    const vec3 center={(volume_start.x+volume_end.x)*0.5f,
                       (volume_start.y+volume_end.y)*0.5f,
                       (volume_start.z+volume_end.z)*0.5f};
    const float dx=std::fabs(volume_end.x-volume_start.x);
    const float dy=std::fabs(volume_end.y-volume_start.y);
    const float dz=std::fabs(volume_end.z-volume_start.z);
    // Require at least a small physical volume even on a tap.
    const float side=std::clamp(std::max({dx,dy,dz,voxel_size*3.0f}),
                                voxel_size*3.0f,0.8f);
    if(voxels.size()>=max_voxels) return;
    begin_artwork(center,side);
    switch(algorithm_mode) {
        case 6: generate_planar_fractal(center,side,false); break;
        case 7: generate_planar_fractal(center,side,true); break;
        case 10: generate_lorenz(center,side); break;
        case 11: generate_rossler(center,side); break;
        case 12: generate_fibonacci(center,side); break;
        default: break;
    }
    // Uniform reveal scheduler for every generator, preserving Lorenz's
    // trajectory order. Previous artworks keep their own playback settings.
    const float duration=generator_speed==0?20.0f:(generator_speed==2?4.0f:9.0f);
    const size_t first=pending_artwork.first_voxel;
    const size_t count=voxels.size()-first;
    for(size_t i=0;i<count;++i) {
        PaintedVoxel& v=voxels[first+i];
        v.reveal_start=animation_time;
        v.reveal_progress=count>1?static_cast<float>(i)/static_cast<float>(count-1):0.0f;
        v.reveal_duration=generator_playback==0?0.0f:duration;
        v.reveal_loop=generator_playback==2;
    }
    if(generator_sound>0 && tones[generator_sound-1])
        sound_play(tones[generator_sound-1],center,0.22f);
    finish_artwork();
    last_paint_valid=false;
}
// Versioned binary slot. A temporary file isn't used here because some
// Android app sandboxes restrict rename across storage locations.
static const char* scene_path() {
#ifdef __ANDROID__
    return "/data/data/org.voxen.xr/files/voxen_gallery.bin";
#else
    return "voxen_gallery.bin";
#endif
}
struct SceneHeader { uint32_t magic,version,count; };
struct SavedVoxel {
    float x,y,z,size,r,g,b,a,phase,progress,duration;
    int32_t shape,effect,shader;
    uint8_t loop;
};
static_assert(sizeof(SavedVoxel)==60,"Unexpected persisted voxel layout");
static void save_scene() {
    std::ofstream out(scene_path(),std::ios::binary|std::ios::trunc);
    if(!out) {storage_status="SAVE FAILED";return;}
    SceneHeader h{0x564f584eu,1,static_cast<uint32_t>(voxels.size())};
    out.write(reinterpret_cast<const char*>(&h),sizeof(h));
    for(const auto& v:voxels) {
        SavedVoxel p{v.position.x,v.position.y,v.position.z,v.size,
            v.tint.r,v.tint.g,v.tint.b,v.tint.a,v.phase,v.reveal_progress,
            v.reveal_duration,v.shape,v.effect,v.gpu_preset,
            static_cast<uint8_t>(v.reveal_loop)};
        out.write(reinterpret_cast<const char*>(&p),sizeof(p));
    }
    out.flush();
    storage_status=out.good()?"SAVED":"SAVE FAILED";
}
static void load_scene() {
    std::ifstream in(scene_path(),std::ios::binary);
    SceneHeader h{};
    if(!in.read(reinterpret_cast<char*>(&h),sizeof(h)) ||
       h.magic!=0x564f584eu || h.version!=1 || h.count>max_voxels) {
        storage_status="INVALID / NO FILE";return;
    }
    std::vector<PaintedVoxel> recovered;
    recovered.reserve(h.count);
    for(uint32_t i=0;i<h.count;++i) {
        SavedVoxel p{};
        if(!in.read(reinterpret_cast<char*>(&p),sizeof(p)) ||
           !std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||
           !std::isfinite(p.size)||p.size<=0 || p.size>2 ||
           p.shape<0||p.shape>=voxen_shapes::count||
           !voxen_effects::valid(p.shader)||p.effect<0||p.effect>3) {
            storage_status="CORRUPT FILE";return;
        }
        recovered.push_back({{p.x,p.y,p.z},p.size,{p.r,p.g,p.b,p.a},
            p.shape,p.effect,p.shader,p.phase,animation_time,
            p.progress,p.duration,p.loop!=0});
    }
    voxels.swap(recovered);
    artworks.clear();redo_artworks.clear();
    property_undo.clear();property_redo.clear();
    selected_artwork_id=0;stroke_active=false;volume_active=false;
    last_paint_valid=false;
    storage_status="RESTORED";
}
static color128 animated_tint(const PaintedVoxel& v) {
    if (v.effect==2) {
        const float h=std::fmod(animation_time*0.09f + v.phase*0.035f,1.0f);
        return color_hsv(h<0?h+1:h,0.9f,1.0f,1.0f);
    }
    if (v.effect==3) {
        const float k=0.65f+0.35f*std::sin(animation_time*5.0f+v.phase);
        return {v.tint.r*k,v.tint.g*k,v.tint.b*k,1.0f};
    }
    return v.tint;
}
static float animated_scale(const PaintedVoxel& v) {
    // Keep the original grid collision geometry. Visual size never exceeds
    // the collision/placement size, so neighboring cubes don't intersect.
    if (v.effect==1)
        return 0.82f+0.18f*(0.5f+0.5f*
            std::sin(animation_time*3.5f+v.phase));
    return 1.0f;
}
struct ArtworkBounds { vec3 lo,hi; };
static ArtworkBounds artwork_bounds(const voxen::Artwork& a) {
    ArtworkBounds b{{1e9f,1e9f,1e9f},{-1e9f,-1e9f,-1e9f}};
    const size_t end=std::min(voxels.size(),size_t(a.first_voxel)+a.voxel_count);
    for(size_t i=a.first_voxel;i<end;++i) {
        const auto& v=voxels[i]; const float h=v.size*0.5f;
        b.lo.x=std::min(b.lo.x,v.position.x-h); b.hi.x=std::max(b.hi.x,v.position.x+h);
        b.lo.y=std::min(b.lo.y,v.position.y-h); b.hi.y=std::max(b.hi.y,v.position.y+h);
        b.lo.z=std::min(b.lo.z,v.position.z-h); b.hi.z=std::max(b.hi.z,v.position.z+h);
    }
    constexpr float pad=0.012f;
    b.lo={b.lo.x-pad,b.lo.y-pad,b.lo.z-pad};
    b.hi={b.hi.x+pad,b.hi.y+pad,b.hi.z+pad};
    return b;
}
static voxen::Artwork* selected_artwork() {
    for(auto& a:artworks) if(a.id==selected_artwork_id) return &a;
    return nullptr;
}
static void edit_selected_shape(int shape) {
    auto* a=selected_artwork(); if(!a) return;
    if(a->shape==shape) return;
    if(!applying_property_history) {
        property_undo.push_back({a->id,false,a->shape,shape});
        property_redo.clear();
    }
    a->shape=shape;
    const size_t end=std::min(voxels.size(),size_t(a->first_voxel)+a->voxel_count);
    for(size_t i=a->first_voxel;i<end;++i) voxels[i].shape=shape;
}
static void edit_selected_shader(int shader) {
    auto* a=selected_artwork(); if(!a) return;
    if(a->shader==shader) return;
    if(!applying_property_history) {
        property_undo.push_back({a->id,true,a->shader,shader});
        property_redo.clear();
    }
    a->shader=shader;
    const size_t end=std::min(voxels.size(),size_t(a->first_voxel)+a->voxel_count);
    for(size_t i=a->first_voxel;i<end;++i) voxels[i].gpu_preset=shader;
}
static bool ray_aabb(vec3 origin,vec3 direction,const ArtworkBounds& b,float& hit) {
    float near=0.0f,far=4.0f;
    const float o[3]={origin.x,origin.y,origin.z};
    const float d[3]={direction.x,direction.y,direction.z};
    const float lo[3]={b.lo.x,b.lo.y,b.lo.z};
    const float hi[3]={b.hi.x,b.hi.y,b.hi.z};
    for(int k=0;k<3;++k) {
        if(std::fabs(d[k])<1e-6f) {
            if(o[k]<lo[k] || o[k]>hi[k]) return false;
        } else {
            float t0=(lo[k]-o[k])/d[k],t1=(hi[k]-o[k])/d[k];
            if(t0>t1) std::swap(t0,t1);
            near=std::max(near,t0); far=std::min(far,t1);
            if(near>far) return false;
        }
    }
    hit=near; return true;
}
static void select_from_ray(vec3 origin,vec3 direction) {
    float nearest=4.0f;
    selected_artwork_id=0;
    for(const auto& a:artworks) {
        if(!a.voxel_count) continue;
        float t=0;
        if(ray_aabb(origin,direction,artwork_bounds(a),t) && t<=nearest) {
            nearest=t; selected_artwork_id=a.id;
        }
    }
}
static void draw_selected_outline() {
    const auto* a=selected_artwork(); if(!a) return;
    const auto b=artwork_bounds(*a);
    const color32 tint{75,255,190,230};
    const vec3 p[8]={
        {b.lo.x,b.lo.y,b.lo.z},{b.hi.x,b.lo.y,b.lo.z},
        {b.hi.x,b.hi.y,b.lo.z},{b.lo.x,b.hi.y,b.lo.z},
        {b.lo.x,b.lo.y,b.hi.z},{b.hi.x,b.lo.y,b.hi.z},
        {b.hi.x,b.hi.y,b.hi.z},{b.lo.x,b.hi.y,b.hi.z}};
    constexpr int edges[12][2]={{0,1},{1,2},{2,3},{3,0},
       {4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
    for(const auto& e:edges) line_add(p[e[0]],p[e[1]],tint,tint,0.0015f);
}
static void update_brush() {
    const controller_t* right = input_controller(handed_right);
    if (!right || !(right->tracked & button_state_active)) {
        trigger_down = false;
        finish_artwork();
        last_paint_valid = false;
        volume_active = false;
        return;
    }
    // Right-stick horizontal: voxel size. Vertical: cycle shapes.
    // Continuous size adjustment, proportional to stick deflection.
    const float axis = right->stick.x;
    if (std::fabs(axis) > 0.18f) {
        const float input = (std::fabs(axis)-0.18f)/0.82f;
        voxel_size = std::clamp(voxel_size +
            (axis > 0 ? 1.0f : -1.0f)*input*0.065f*time_stepf(),
            0.01f, 0.12f);
    }
    if (std::fabs(right->stick.y) < 0.30f) shape_stick_latched = false;
    if (!shape_stick_latched && std::fabs(right->stick.y) > 0.70f) {
        selected_shape = (selected_shape + (right->stick.y > 0 ? 1 : voxen_shapes::count-1)) % voxen_shapes::count;
        if(selection_mode) edit_selected_shape(selected_shape);
        shape_stick_latched = true;
    }
    const bool pressed = trigger_down
        ? right->trigger > 0.45f : right->trigger > trigger_threshold;
    trigger_down = pressed;
    const vec3 fwd = aim_forward(right->aim.orientation);
    const vec3 tip = {
        right->aim.position.x + fwd.x*brush_distance,
        right->aim.position.y + fwd.y*brush_distance,
        right->aim.position.z + fwd.z*brush_distance
    };
    // UI owns the trigger while aiming at a panel.
    if(menu_aiming || menu_capture) {
        if(volume_active) volume_active=false;
        finish_artwork();
        last_paint_valid=false; trigger_down=false; return;
    }
    if(selection_mode) {
        finish_artwork();
        volume_active=false;
        last_paint_valid=false;
        if(pressed && !selection_trigger_latched)
            select_from_ray(right->aim.position,fwd);
        selection_trigger_latched=pressed;
        return;
    }
    selection_trigger_latched=false;
    if(algorithm_mode>=0) {
        finish_artwork();
        if(pressed) {
            if(!volume_active) { volume_start=tip; volume_active=true; }
            volume_end=tip;
            const vec3 middle={(volume_start.x+tip.x)*0.5f,
                               (volume_start.y+tip.y)*0.5f,
                               (volume_start.z+tip.z)*0.5f};
            const float side=std::max({std::fabs(tip.x-volume_start.x),
                std::fabs(tip.y-volume_start.y),std::fabs(tip.z-volume_start.z),
                voxel_size*3.0f});
            render_add_mesh(cube,carousel_mat,matrix_ts(middle,{side,side,side}),
                            {0.2f,0.9f,0.8f,0.16f});
        } else if(volume_active) commit_volume();
        last_paint_valid=false;
        return;
    }
    // Live fixed-size, axis-aligned brush cursor.
    render_add_mesh(shape_meshes[selected_shape], material,
        matrix_ts(tip, {voxel_size, voxel_size, voxel_size}),
        color_hsv(hue, 1.0f, 1.0f, 1.0f));
    if (!painting || !pressed) {
        finish_artwork();
        last_paint_valid = false;
        return;
    }

    if (!last_paint_valid) {
        begin_artwork(tip);
        paint_at(tip);
        last_paint = tip;
        last_paint_valid = true;
        return;
    }
    const float spacing = std::max(0.008f, voxel_size * 0.75f);
    const float d = dist(last_paint, tip);
    // Interpolate along controller motion, but bound per-frame work.
    const int steps = std::min(64, static_cast<int>(d / spacing));
    if (steps > 0) {
        const vec3 start = last_paint;
        for (int i=1; i<=steps && voxels.size()<max_voxels; ++i)
            paint_at(interpolate(start, tip, static_cast<float>(i)/steps));
        last_paint = tip;
    }
}


static bool undo_property() {
    if(property_undo.empty()) return false;
    const auto edit=property_undo.back(); property_undo.pop_back();
    selected_artwork_id=edit.artwork_id;
    applying_property_history=true;
    if(edit.is_shader) edit_selected_shader(edit.before);
    else edit_selected_shape(edit.before);
    applying_property_history=false;
    property_redo.push_back(edit);
    return true;
}
static bool redo_property() {
    if(property_redo.empty()) return false;
    const auto edit=property_redo.back(); property_redo.pop_back();
    selected_artwork_id=edit.artwork_id;
    applying_property_history=true;
    if(edit.is_shader) edit_selected_shader(edit.after);
    else edit_selected_shape(edit.after);
    applying_property_history=false;
    property_undo.push_back(edit);
    return true;
}
static void undo_stroke() {
    if(undo_property()) return;
    selected_artwork_id = 0;
    finish_artwork();
    if (artworks.empty()) return;
    auto artwork = artworks.back();
    artworks.pop_back();
    // All artworks are appended chronologically; the last one is a suffix.
    if (artwork.first_voxel + artwork.voxel_count != voxels.size()) {
        artworks.push_back(artwork);
        return;
    }
    voxen::ArtworkSnapshot<PaintedVoxel> snapshot;
    snapshot.artwork = artwork;
    snapshot.voxels.assign(voxels.begin()+artwork.first_voxel,voxels.end());
    redo_artworks.push_back(std::move(snapshot));
    voxels.resize(artwork.first_voxel);
    last_paint_valid = false;
}
static void redo_stroke() {
    if(redo_property()) return;
    selected_artwork_id = 0;
    if (redo_artworks.empty()) return;
    auto snapshot = std::move(redo_artworks.back());
    redo_artworks.pop_back();
    if (snapshot.voxels.size() > max_voxels - voxels.size()) return;
    snapshot.artwork.first_voxel = static_cast<uint32_t>(voxels.size());
    voxels.insert(voxels.end(),snapshot.voxels.begin(),snapshot.voxels.end());
    artworks.push_back(snapshot.artwork);
    last_paint_valid = false;
}
static void update_left_buttons() {
    const controller_t* left = input_controller(handed_left);
    if (!left || !(left->tracked & button_state_active)) return;
    if (left->x1 & button_state_just_active) undo_stroke();
    if (left->x2 & button_state_just_active) redo_stroke();
}

// Single source of truth for panel geometry. Face -Z is the outward
// surface in each face's local frame; the window starts at its upper-right.
// No per-page or ray-specific fudge offsets.
static constexpr float panel_width = 0.16f;
static constexpr float panel_height = 0.43f;


// A single controller-mounted face. Logical pages replace each other in
// place; never arrange five 46cm surfaces around a 17cm cube.
static pose_t face_panel_pose(int page) {
    const quat rotation=quat_mul(carousel_base.orientation,
                                  quat_from_angles(0,180.0f,0));
    const vec3 outward=quat_mul_vec(rotation,{0,0,0.005f});
    // ALGORITHMS and PARAMS are a paired side-by-side workspace.
    const bool pair=menu_page==2 || menu_page==4;
    const float shift=pair?(page==2?-0.087f:page==4?0.087f:0.0f):0.0f;
    const vec3 lateral=quat_mul_vec(rotation,{shift,0,0});
    return {{carousel_base.position.x+outward.x+lateral.x,
             carousel_base.position.y+outward.y+lateral.y,
             carousel_base.position.z+outward.z+lateral.z},rotation};
}
static vec3 panel_world(const pose_t& face,float x,float y,float z=0) {
    const vec3 o=quat_mul_vec(face.orientation,{x,y,z});
    return {face.position.x+o.x,face.position.y+o.y,face.position.z+o.z};
}
// face_panel_pose's local +Z points OUT of the cube.
// Render every surface and glyph in that exact orientation and on the
// positive-Z (viewer) side. No second 180-degree flip anywhere.
static void face_rect(const pose_t& face,float x,float y,float w,float h,
                      color128 color,float z=0.004f) {
    render_add_mesh(cube,carousel_mat,matrix_trs(
        panel_world(face,x,y,z),face.orientation,{w,h,0.002f}),color);
}
static void face_text(const pose_t& face,const char* label,float x,float y) {
    // Text uses the opposite front-facing convention to our mesh quads.
    // Rotate glyphs only; keep panel geometry and hit testing unchanged.
    const quat text_rotation=quat_mul(face.orientation,quat_from_angles(0,180,0));
    text_add_at(label,matrix_trs(panel_world(face,x,y,0.011f),
        text_rotation,{1,1,1}),panel_text_style);
}
namespace phosphor_pixel {
struct Glyph {char ch; std::array<uint8_t,7> rows;};
inline constexpr Glyph font[]={
{'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
{'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
{'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
{'G',{14,17,16,23,17,17,14}}, {'H',{17,17,17,31,17,17,17}},
{'I',{14,4,4,4,4,4,14}}, {'J',{7,2,2,2,18,18,12}},
{'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
{'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
{'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
{'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
{'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
{'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
{'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
{'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
{'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
{'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
{'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
{'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
{'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
{'/',{1,1,2,4,8,16,16}}, {'-',{0,0,0,31,0,0,0}},
{' ',{0,0,0,0,0,0,0}}
};

inline const Glyph& glyph(char c) {
    for (const auto& item : font) if (item.ch == c) return item;
    return font[sizeof(font)/sizeof(font[0])-1];
}
}
static void pixel_title(const pose_t& face,const char* label,float y) {
    const float cell=0.00275f, advance=cell*6.0f;
    size_t length=0; while(label[length]) ++length;
    // Keep long titles contained within the original panel width.
    const float width=std::min(0.151f,static_cast<float>(length)*advance);
    const float scale=length>9?width/(static_cast<float>(length)*6.0f):cell;
    const float start=-static_cast<float>(length)*scale*3.0f+scale*0.5f;
    const color128 green={0.22f,1.0f,0.46f,0.98f};
    for(size_t i=0;i<length;++i) {
        const auto& glyph=phosphor_pixel::glyph(label[i]);
        for(int row=0;row<7;++row)
            for(int col=0;col<5;++col)
                if(glyph.rows[row] & (1u<<(4-col)))
                    face_rect(face,start+i*scale*6+col*scale,
                              y+(3-row)*scale,scale*0.82f,scale*0.82f,green,0.013f);
    }
}
static void pixel_border(const pose_t& face) {
    const color128 rim={0.09f,0.84f,0.31f,0.9f};
    const float x=panel_width*0.5f, y=panel_height*0.5f;
    // Segmented border uses few meshes; no shader or XR runtime changes.
    for(int i=0;i<19;++i) {
        const float px=-x+(i+0.5f)*panel_width/19;
        face_rect(face,px,y-0.002f,panel_width/19*0.79f,0.003f,rim,0.012f);
        face_rect(face,px,-y+0.002f,panel_width/19*0.79f,0.003f,rim,0.012f);
    }
    for(int i=1;i<31;++i) {
        const float py=-y+(i+0.5f)*panel_height/31;
        face_rect(face,-x+0.002f,py,0.003f,panel_height/31*0.76f,rim,0.012f);
        face_rect(face,x-0.002f,py,0.003f,panel_height/31*0.76f,rim,0.012f);
    }
}
static bool panel_hover(int page,float x,float y,float w,float h) {
    return menu_aiming && hit_page==page &&
       std::fabs(menu_hit_x-x)<w*0.5f &&
       std::fabs(menu_hit_y-y)<h*0.5f;
}
static bool face_button(const pose_t& face,int page,const char* label,
                        float y,bool selected=false) {
    const bool hover=panel_hover(page,0,y,0.137f,0.026f);
    face_rect(face,0,y,0.137f,0.025f,
        hover ? color128{0.12f,0.62f,0.27f,0.95f} :
        selected ? color128{0.12f,0.42f,0.21f,0.92f} :
        color128{0.025f,0.10f,0.055f,0.88f});
    face_text(face,label,0,y);
    if(hover && menu_click && click_sound)
        sound_play(click_sound,menu_hit,0.35f);
    return hover && menu_click;
}

static void draw_face(int page) {
    const pose_t face=face_panel_pose(page);
    face_rect(face,0,0,panel_width,panel_height,
        {0.012f,0.035f,0.025f,0.87f},0.003f);
    pixel_border(face);
    static const char* titles[]={"COLORS","SHAPES","ALGORITHMS","SHADERS","PARAMS","ANIMATIONS","SAVE / LOAD","BACKGROUNDS"};
    pixel_title(face,titles[page],0.188f);
    if(page==0) {
        for(int i=0;i<16;++i) {
            const float x=-0.057f+(i%4)*0.038f;
            const float y=0.119f-(i/4)*0.042f;
            const bool hover=panel_hover(page,x,y,0.034f,0.034f);
            face_rect(face,x,y,hover?0.038f:0.032f,
                hover?0.038f:0.032f,color_hsv(i/16.0f,1,1,1));
            if(hover && menu_click) {
                hue=i/16.0f;
                if(click_sound) sound_play(click_sound,menu_hit,0.35f);
            }
        }
    } else if(page==1) {
        for(int i=0;i<voxen_shapes::count;++i) {
            const int col=i%2,row=i/2;
            const float x=col==0?-0.041f:0.041f;
            const float y=0.135f-row*0.051f;
            const bool hover=panel_hover(page,x,y,0.076f,0.038f);
            face_rect(face,x,y,0.075f,0.036f,
                hover?color128{0.12f,0.65f,0.26f,0.95f}:
                selected_shape==i?color128{0.12f,0.45f,0.22f,0.9f}:
                color128{0.025f,0.11f,0.06f,0.85f});
            face_text(face,voxen_shapes::labels[i],x,y);
            if(hover && menu_click) {
                selected_shape=i;
                if(selection_mode) edit_selected_shape(i);
                if(click_sound) sound_play(click_sound,menu_hit,0.35f);
            }
        }
        if(face_button(face,page,selection_mode?"AIM: ON":"AIM: OFF",-0.095f,selection_mode)) {
            selection_mode=!selection_mode;
            selection_trigger_latched=false;
            if(!selection_mode) selected_artwork_id=0;
        }
        if(selection_mode && selected_artwork())
            if(face_button(face,page,"DESELECT",-0.145f))
                selected_artwork_id=0;
    } else if(page==2) {
        static const char* algorithms[]={"LORENZ","ROSSLER","MANDELBROT",
                                         "JULIA","FIBONACCI"};
        static const int ids[]={10,11,6,7,12};
        for(int i=0;i<5;++i) {
            const int id=ids[i];
            if(face_button(face,page,algorithms[i],0.135f-i*0.060f,
                           algorithm_mode==id))
            {
                algorithm_mode=(algorithm_mode==id?-1:id);
                save_confirmation=false;
            }
        }
    } else if(page==3) {
        face_text(face,"GPU MATERIALS",0,0.153f);
        for(int i=0;i<voxen_effects::count;++i) {
            const float x=(i%2==0?-0.041f:0.041f);
            const float y=0.090f-(i/2)*0.058f;
            const bool hover=panel_hover(page,x,y,0.074f,0.033f);
            face_rect(face,x,y,0.074f,0.031f,
                hover?color128{0.12f,0.65f,0.26f,0.95f}:
                gpu_preset==i?color128{0.12f,0.45f,0.22f,0.9f}:
                color128{0.025f,0.11f,0.06f,0.85f});
            face_text(face,voxen_effects::get(i).label,x,y);
            if(hover && menu_click) {
                gpu_preset=i;
                if(selection_mode) edit_selected_shader(i);
                if(click_sound) sound_play(click_sound,menu_hit,0.35f);
            }
        }
    } else if(page==4) {
        const char* title="SELECT A GENERATOR";
        const char* names[5]={};
        float* values[5]={};
        float steps[5]={}, minima[5]={}, maxima[5]={};
        int count=0;
        auto add=[&](const char* name,float& value,float step,float lo,float hi) {
            names[count]=name; values[count]=&value;
            steps[count]=step; minima[count]=lo; maxima[count]=hi;
            ++count;
        };
        if(algorithm_mode==10) {
            title="LORENZ";
            add("SIGMA",lorenz_sigma,0.5f,0.1f,40.0f);
            add("RHO",lorenz_rho,1.0f,0.1f,80.0f);
            add("BETA",lorenz_beta,0.1f,0.1f,10.0f);
        } else if(algorithm_mode==11) {
            title="ROSSLER";
            add("A",rossler_a,0.05f,0.01f,1.0f);
            add("B",rossler_b,0.05f,0.01f,1.0f);
            add("C",rossler_c,0.5f,2.0f,12.0f);
        } else if(algorithm_mode==6 || algorithm_mode==7) {
            title=algorithm_mode==6?"MANDELBROT":"JULIA";
            add("ZOOM",fractal_zoom,0.5f,0.5f,12.0f);
            add("CENTER X",fractal_center_x,0.1f,-2.0f,2.0f);
            add("CENTER Y",fractal_center_y,0.1f,-2.0f,2.0f);
            if(algorithm_mode==7) {
                add("C REAL",julia_real,0.025f,-1.5f,1.5f);
                add("C IMAG",julia_imag,0.025f,-1.5f,1.5f);
            }
        } else if(algorithm_mode==12) {
            title="FIBONACCI";
            add("TURNS",fibonacci_turns,1.0f,1.0f,12.0f);
            add("SPACING",fibonacci_spacing,0.1f,0.3f,1.0f);
        }
        face_text(face,title,0,0.165f);
        if(count==0) face_text(face,"CHOOSE IN ALGORITHMS",0,0.07f);
        const bool is_lorenz=algorithm_mode==10;
        for(int i=0;i<count;++i) {
            const float y=0.137f-i*0.047f;
            char value[64];
            std::snprintf(value,sizeof(value),"%s %.3f",names[i],*values[i]);
            face_text(face,value,0,y);
            const float by=y-0.019f;
            const float bh=0.020f;
            const bool minus=panel_hover(page,-0.040f,by,0.070f,bh);
            const bool plus=panel_hover(page,0.040f,by,0.070f,bh);
            face_rect(face,-0.040f,by,0.070f,bh-0.001f,
                      minus?color128{0.2f,0.7f,0.8f,0.95f}:color128{0.1f,0.24f,0.35f,0.85f});
            face_rect(face,0.040f,by,0.070f,bh-0.001f,
                      plus?color128{0.2f,0.7f,0.8f,0.95f}:color128{0.1f,0.24f,0.35f,0.85f});
            face_text(face,"-", -0.040f,by);
            face_text(face,"+",0.040f,by);
            if(menu_click&&(minus||plus))
                *values[i]=std::clamp(*values[i]+(plus?steps[i]:-steps[i]),minima[i],maxima[i]);
        }
        if(count>0) {
            static const char* playback[]={"STATIC","ONCE","LOOP"};
            char label[64];
            std::snprintf(label,sizeof(label),"TIME: %s",playback[generator_playback]);
            if(face_button(face,page,label,-0.112f))
                generator_playback=(generator_playback+1)%3;
            static const char* speeds[]={"20 SEC","9 SEC","4 SEC"};
            std::snprintf(label,sizeof(label),"DURATION: %s",speeds[generator_speed]);
            if(face_button(face,page,label,-0.147f))
                generator_speed=(generator_speed+1)%3;
            static const char* sounds[]={"MUTE","BELL","PULSE","BASS"};
            std::snprintf(label,sizeof(label),"SOUND: %s",sounds[generator_sound]);
            if(face_button(face,page,label,-0.180f))
                generator_sound=(generator_sound+1)%4;
            if(face_button(face,page,"RESET PARAMS",-0.207f)) {
                lorenz_sigma=10; lorenz_rho=28; lorenz_beta=8.0f/3.0f;
                rossler_a=0.2f; rossler_b=0.2f; rossler_c=5.7f;
                fractal_zoom=1; fractal_center_x=0; fractal_center_y=0;
                julia_real=-0.745f; julia_imag=0.186f;
                fibonacci_turns=5; fibonacci_spacing=1;
                generator_playback=1; generator_speed=1; generator_sound=0;
            }
        }
    } else if(page==5) {
        face_text(face,"VOXEL ANIMATION",0,0.145f);
        static const char* effects[]={"SOLID","BREATHE","RAINBOW","SHIMMER"};
        for(int i=0;i<4;++i)
            if(face_button(face,page,effects[i],0.085f-i*0.065f,effect_mode==i))
                effect_mode=i;
    } else if(page==7) {
        face_text(face,"META SCENE PLANES",0,0.152f);
        if (!scene_background.supported) {
            face_text(face,"SCENE UNAVAILABLE",0,0.105f);
        } else {
            char status[48];
            std::snprintf(status,sizeof(status),"%s / %d PLANES",
                scene_background.loading?"SCANNING":scene_background.ready?"READY":"NO SCENE",
                scene_background.plane_count());
            face_text(face,status,0,0.111f);
            if(face_button(face,page,"SCAN AGAIN",0.071f)) scene_background.refresh();
            if(face_button(face,page,scene_background.preset<0?"SHADER: NONE":"SHADER: ACTIVE",0.034f))
                scene_background.preset = scene_background.preset<0?0:-1;
            if(face_button(face,page,"PREVIOUS SHADER",-0.004f))
                scene_background.preset = scene_background.preset<0?0:
                    (scene_background.preset+voxen_effects::count-1)%voxen_effects::count;
            if(face_button(face,page,"NEXT SHADER",-0.042f))
                scene_background.preset = scene_background.preset<0?0:
                    (scene_background.preset+1)%voxen_effects::count;
            if(scene_background.preset>=0)
                face_text(face,voxen_effects::get(scene_background.preset).label,0,-0.074f);
            if(face_button(face,page,(scene_background.mask&voxen_scene::Background::Walls)?"WALLS: ON":"WALLS: OFF",-0.111f))
                scene_background.mask ^= voxen_scene::Background::Walls;
            if(face_button(face,page,(scene_background.mask&voxen_scene::Background::Floor)?"FLOOR: ON":"FLOOR: OFF",-0.146f))
                scene_background.mask ^= voxen_scene::Background::Floor;
            if(face_button(face,page,(scene_background.mask&voxen_scene::Background::Ceiling)?"CEILING: ON":"CEILING: OFF",-0.181f))
                scene_background.mask ^= voxen_scene::Background::Ceiling;
        }
    } else if(page==6) {
        face_text(face,"SCENE SLOT 1",0,0.125f);
        face_text(face,storage_status,0,0.073f);
        if(face_button(face,page,save_confirmation?"CONFIRM SAVE":"SAVE",0.015f)) {
            if(save_confirmation) { save_scene(); save_confirmation=false; }
            else save_confirmation=true;
        }
        if(face_button(face,page,restore_confirmation?"CONFIRM LOAD":"RESTORE",-0.050f)) {
            if(restore_confirmation) { load_scene(); restore_confirmation=false; }
            else restore_confirmation=true;
        }
        if(face_button(face,page,"CANCEL",-0.115f)) {
            save_confirmation=false; restore_confirmation=false;
        }
    }
}
static void update_controller_palette() {
    const controller_t* left=input_controller(handed_left);
    if(!left || !(left->tracked & button_state_active)) return;
    if(std::fabs(left->stick.x)<0.35f) stick_latched=false;
    if(!stick_latched && std::fabs(left->stick.x)>0.7f) {
        const int direction=left->stick.x>0?1:-1;
        menu_page=(menu_page+direction+page_count)%page_count;
        stick_latched=true;
    }
    // Keep the tall panel anchored near the controller instead of projecting
    // its center 23.5 cm away. Orientation still faces the headset.
    // The panel remains upright and faces the viewer. Anchor its bottom
    // edge at controller height in world space, irrespective of wrist tilt.
    // panel_height is 0.43m; the center must sit half that above the hand.
    carousel_base.position={left->pose.position.x,
                            left->pose.position.y+panel_height*0.5f,
                            left->pose.position.z};
    // Position follows the controller; readability follows the headset.
    // Keep local Y upright and face the viewer instead of showing the panel
    // edge-on when the controller points forward or down.
    const pose_t* head=input_head();
    if(head) {
        const vec3 target={head->position.x,carousel_base.position.y,
                           head->position.z};
        const float dx=target.x-carousel_base.position.x;
        const float dz=target.z-carousel_base.position.z;
        if(dx*dx+dz*dz>0.0001f)
            carousel_base.orientation=quat_lookat_up(
                carousel_base.position,target,{0,1,0});
    }
    palette=face_panel_pose(menu_page);
}
// Controller ray to UI-plane intersection, independent of StereoKit's
// near-hand widget interaction. Local top-left is the window pose.
static void update_menu_ray() {
    menu_aiming = false;
    hit_page = -1;
    menu_click = false;
    const controller_t* right = input_controller(handed_right);
    if (!right || !(right->tracked & button_state_active)) {
        menu_trigger_down = false;
        menu_capture = false;
        menu_slider_capture = -1;
        return;
    }
    const bool trigger = right->trigger > 0.65f;
    const bool rising = trigger && !menu_trigger_down;
    menu_trigger_down = trigger;
    if (!trigger) {
        menu_capture = false;
        menu_slider_capture = -1;
    }
    const vec3 start = right->aim.position;
    const vec3 forward = aim_forward(right->aim.orientation);
    float closest = 2.0f;
    // Intersect each face in its own local coordinates. This uses exactly
    // the same transforms as each rendered StereoKit panel.
    for (int page=0; page<page_count; ++page) {
        if(page!=menu_page && !((menu_page==2 || menu_page==4) && (page==2 || page==4))) continue;
        const pose_t face = face_panel_pose(page);
        const quat inverse = quat_inverse(face.orientation);
        const vec3 face_origin = face.position;
        const vec3 relative = {start.x-face_origin.x,
                               start.y-face_origin.y,
                               start.z-face_origin.z};
        const vec3 o = quat_mul_vec(inverse, relative);
        const vec3 d = quat_mul_vec(inverse, forward);
        if (std::fabs(d.z) < 0.0001f) continue;
        const float t = -o.z / d.z;
        if (t <= 0.0f || t >= closest) continue;
        const float x = o.x + t*d.x, y = o.y + t*d.y;
        if (std::fabs(x)>panel_width*0.5f ||
            std::fabs(y)>panel_height*0.5f) continue;
        closest = t;
        hit_page = page;
        menu_aiming = true;
        menu_hit_x = x;
        menu_hit_y = y;
        menu_hit = {start.x+forward.x*t, start.y+forward.y*t,
                    start.z+forward.z*t};
    }
    if (rising && menu_aiming) {
        menu_click = true;
        menu_capture = true;
    }
}
static void draw_menu_ray() {
    if (!show_ui_ray || !menu_aiming) return;
    const controller_t* right = input_controller(handed_right);
    if (!right || !(right->tracked & button_state_active)) return;
    const vec3 start = right->aim.position;
    const vec3 end = menu_hit;
    const color32 ray_color = menu_aiming ? color32{80,255,190,220}
                                          : color32{110,215,255,140};
    line_add(start, end, ray_color, ray_color, 0.0015f);
    if (menu_aiming) {
        const vec3 marker_end = {menu_hit.x, menu_hit.y+0.012f, menu_hit.z};
        line_add(menu_hit, marker_end, ray_color, ray_color, 0.004f);
    }
}
// Fixed layout: the ray and widgets share identical local rectangles.
// Avoid relying on StereoKit's near-hand interaction for remote trigger clicks.
static constexpr float control_x = 0.008f;
static constexpr float control_w = 0.145f;
static constexpr float control_h = 0.03f;
static float control_y(int row) { return -0.045f - row * 0.04f; }
static bool hit_row(int row) {
    if (!menu_aiming) return false;
    const float top = control_y(row);
    return menu_hit_x >= control_x && menu_hit_x <= control_x + control_w &&
           menu_hit_y <= top && menu_hit_y >= top-control_h;
}
static bool menu_button(const char* caption, int row) {
    // Native near-field interaction stays usable; remote click is independent.
    const bool near_click = ui_button_at(caption,
        {control_x, control_y(row), 0}, {control_w, control_h});
    const bool remote_click = menu_click && hit_row(row);
    if (menu_aiming && hit_row(row)) {
        // Subtle luminous surface indication on the focused interactive row.
        const vec3 local = {-control_x-control_w*0.5f,
                             control_y(row)-control_h*0.5f, -0.016f};
        const vec3 offset = quat_mul_vec(palette.orientation,local);
        render_add_mesh(cube, carousel_mat,
            matrix_trs({palette.position.x+offset.x,
                        palette.position.y+offset.y,
                        palette.position.z+offset.z},
                       palette.orientation, {control_w,control_h,0.002f}),
            {0.25f,0.95f,1.0f,0.60f});
    }
    if ((near_click || remote_click) && click_sound)
        sound_play(click_sound, menu_hit, 0.35f);
    return near_click || remote_click;
}
static void menu_slider(const char* caption, float& value, float lo, float hi,
                        float step, int row) {
    ui_hslider_at(caption, value, lo, hi, step,
                  {control_x, control_y(row), 0}, {control_w, control_h});
    if (menu_click && hit_row(row)) menu_slider_capture = row;
    if (menu_slider_capture != row || !menu_trigger_down || !menu_aiming) return;
    const float x = std::clamp((menu_hit_x - control_x) / control_w, 0.0f, 1.0f);
    const float raw = lo + (hi-lo)*x;
    value = std::clamp(lo + std::round((raw-lo)/step)*step, lo, hi);
}

// Own our own passthrough layer; StereoKit still owns OpenXR instance/session.
// Failure is non-fatal: default alpha-blended passthrough remains available.
struct EdgePassthrough {
    XrPassthroughFB passthrough = XR_NULL_HANDLE;
    XrPassthroughLayerFB layer = XR_NULL_HANDLE;
    PFN_xrCreatePassthroughFB create = nullptr;
    PFN_xrDestroyPassthroughFB destroy = nullptr;
    PFN_xrCreatePassthroughLayerFB create_layer = nullptr;
    PFN_xrDestroyPassthroughLayerFB destroy_layer = nullptr;
    PFN_xrPassthroughLayerSetStyleFB set_style = nullptr;
    bool available = false;

    template <typename T> static bool load(const char* name, T& proc) {
        proc = reinterpret_cast<T>(backend_openxr_get_function(name));
        return proc != nullptr;
    }

    void initialize() {
        if (backend_xr_get_type() != backend_xr_type_openxr ||
            !backend_openxr_ext_enabled(XR_FB_PASSTHROUGH_EXTENSION_NAME))
            return;
        if (!load("xrCreatePassthroughFB", create) ||
            !load("xrDestroyPassthroughFB", destroy) ||
            !load("xrCreatePassthroughLayerFB", create_layer) ||
            !load("xrDestroyPassthroughLayerFB", destroy_layer) ||
            !load("xrPassthroughLayerSetStyleFB", set_style))
            return;

        const auto session = reinterpret_cast<XrSession>(backend_openxr_get_session());
        XrPassthroughCreateInfoFB info{XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
        info.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
        XrResult result = create(session, &info, &passthrough);
        if (XR_FAILED(result)) {
            std::printf("[Voxen] xrCreatePassthroughFB: %d\n", result);
            passthrough = XR_NULL_HANDLE;
            return;
        }
        XrPassthroughLayerCreateInfoFB layer_info{XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
        layer_info.passthrough = passthrough;
        layer_info.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
        layer_info.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
        result = create_layer(session, &layer_info, &layer);
        if (XR_FAILED(result)) {
            std::printf("[Voxen] xrCreatePassthroughLayerFB: %d\n", result);
            layer = XR_NULL_HANDLE;
            return;
        }
        available = true;
        std::printf("[Voxen] XR_FB_passthrough edge layer ready\n");
    }

    void frame(bool edges, float camera_alpha, color128 edge_color) {
        if (!available || !edges) return;
        XrPassthroughStyleFB style{XR_TYPE_PASSTHROUGH_STYLE_FB};
        style.textureOpacityFactor = camera_alpha;
        style.edgeColor = {edge_color.r, edge_color.g, edge_color.b, edge_color.a};
        const auto result = set_style(layer, &style);
        if (XR_FAILED(result)) return;
        XrCompositionLayerPassthroughFB composition{
            XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
        composition.flags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        composition.layerHandle = layer;
        // Put passthrough edges BEHIND StereoKit's projection layer.
        // Foreground voxels/panels then occlude camera contours.
        constexpr int32_t edge_background_order=-100;
        backend_openxr_composition_layer(
            &composition,sizeof(composition),edge_background_order);
    }

    void shutdown() {
        if (layer != XR_NULL_HANDLE && destroy_layer) destroy_layer(layer);
        if (passthrough != XR_NULL_HANDLE && destroy) destroy(passthrough);
        layer = XR_NULL_HANDLE;
        passthrough = XR_NULL_HANDLE;
        available = false;
    }
};

static EdgePassthrough edge_passthrough;

int voxen_stereokit_main() {
    app_settings.display_preference = display_mode_mixedreality;
    app_settings.blend_preference = display_blend_any_transparent;
    // Must happen before sk_init, which creates StereoKit's OpenXR instance.
    backend_openxr_ext_request(XR_FB_PASSTHROUGH_EXTENSION_NAME);
    backend_openxr_ext_request(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME);
    backend_openxr_ext_request(XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME);
    backend_openxr_ext_request(XR_FB_SCENE_EXTENSION_NAME);
    if (!sk_init(app_settings)) return 1;
    edge_passthrough.initialize();
    scene_background.initialize();
    cube = mesh_find(default_id_mesh_cube);
    shape_meshes = voxen_shapes::create(cube);
    material = material_copy(material_find(default_id_material));
    click_sound = sound_generate([](float t) -> float {
        const float envelope = std::exp(-42.0f*t);
        return 0.28f*envelope*std::sin(6.2831853f*880.0f*t);
    }, 0.09f);
    // Tiny synthesized spatial sounds: no asset bundles or audio downloads.
    tones[0]=sound_generate([](float t)->float {
        return 0.32f*std::exp(-8.0f*t)*
            (std::sin(6.2831853f*660*t)+0.35f*std::sin(6.2831853f*1320*t));
    },0.40f);
    tones[1]=sound_generate([](float t)->float {
        return 0.30f*std::exp(-18.0f*t)*std::sin(6.2831853f*990*t);
    },0.18f);
    tones[2]=sound_generate([](float t)->float {
        return 0.30f*std::exp(-9.0f*t)*std::sin(6.2831853f*165*t);
    },0.35f);
    voxel_shader=shader_create_mem((void*)sks_voxen_voxel_hlsl,
                                  sizeof(sks_voxen_voxel_hlsl));
    if(voxel_shader) {
        for(int i=0;i<voxen_effects::count;++i) {
            gpu_materials[i]=material_create(voxel_shader);
            if(gpu_materials[i]) material_set_float(gpu_materials[i],"mode",(float)i);
        }
    } else std::printf("[Voxen] GPU voxel shader unavailable; using default material\\n");
    if(voxel_shader) {
        for(int i=0;i<voxen_effects::count && i<(int)background_materials.size();++i) {
            background_materials[i]=material_create(voxel_shader);
            if(background_materials[i]) {
                material_set_float(background_materials[i],"mode",(float)i);
                material_set_transparency(background_materials[i],transparency_blend);
                material_set_cull(background_materials[i],cull_none);
            }
        }
    }
    voxels.reserve(max_voxels);
    carousel_mat = material_copy(material_find(default_id_material));
    material_set_transparency(carousel_mat, transparency_blend);
    material_set_cull(carousel_mat, cull_none);
    input_hand_visible(handed_left, false);
    input_hand_visible(handed_right, false);
    input_hand_solid(handed_left, false);
    input_hand_solid(handed_right, false);
    ui_enable_far_interact(false);
    panel_text_style=text_make_style(font_find(default_id_font),0.0085f,
        {0.55f,1.0f,0.66f,1});
    sk_run([] {
        update_controller_palette();
        update_left_buttons();
        update_menu_ray();
        // Never render overlapping inactive pages.
        if(menu_page==2 || menu_page==4) { draw_face(2); draw_face(4); }
        else draw_face(menu_page);

        animation_time += time_stepf();
        for(int i=0;i<voxen_effects::count;++i)
            if(gpu_materials[i]) material_set_float(gpu_materials[i],
                                                     "time",animation_time);
        for(int i=0;i<voxen_effects::count && i<(int)background_materials.size();++i)
            if(background_materials[i]) material_set_float(background_materials[i],"time",animation_time);
        scene_background.render(cube,background_materials);
        for(const PaintedVoxel& voxel:voxels) {
            if(voxel.reveal_duration>0.0f) {
                const float elapsed=std::max(0.0f,animation_time-voxel.reveal_start);
                const float progress_time=voxel.reveal_loop
                    ? std::fmod(elapsed,voxel.reveal_duration) : elapsed;
                if(progress_time<voxel.reveal_progress*voxel.reveal_duration)
                    continue;
            }
            const float side=voxel.size*animated_scale(voxel);
            material_t voxel_mat=gpu_materials[voxen_effects::valid(voxel.gpu_preset)?voxel.gpu_preset:0];
            render_add_mesh(shape_meshes[std::clamp(voxel.shape,0,voxen_shapes::count-1)],
                voxel_mat?voxel_mat:material,
                matrix_ts(voxel.position,{side,side,side}),
                voxel_mat?voxel.tint:animated_tint(voxel));
        }
        update_brush();
        draw_selected_outline();
        draw_menu_ray();
        if (edges_enabled && edge_passthrough.available) {
            render_enable_skytex(false);
            render_set_clear_color(
                color_hsv(background_hue, 1.0f, background_brightness, 0.18f));
        } else {
            render_set_clear_color({0, 0, 0, 0});
        }
        edge_passthrough.frame(edges_enabled, camera_opacity,
            color_hsv(edge_hue, 1.0f, 1.0f, 1.0f));
    }, [] {
        scene_background.shutdown();
        edge_passthrough.shutdown();
        for(mesh_t mesh:shape_meshes) if(mesh) mesh_release(mesh);
        material_release(material);
        material_release(carousel_mat);
        for(material_t mat:gpu_materials) if(mat) material_release(mat);
        for(material_t mat:background_materials) if(mat) material_release(mat);
        if(voxel_shader) shader_release(voxel_shader);
        if (click_sound) sound_release(click_sound);
        for (sound_t tone: tones) if(tone) sound_release(tone);
    });
    return 0;
}
#ifndef __ANDROID__
int main() { return voxen_stereokit_main(); }
#endif
