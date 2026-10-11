#include <stereokit.h>
#include <stereokit_ui.h>
#include <openxr/openxr.h>
#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace sk;

sk_settings_t app_settings = {"Voxen StereoKit"};
static pose_t palette = {{-0.25f, 0.0f, -0.55f}, quat_identity};
static int menu_page = 0;
static constexpr int page_count = 4;
static constexpr float menu_width = 0.24f;
static constexpr float menu_height = 0.32f;
static float carousel_angle = 0.0f;
static float target_angle = 0.0f;
static pose_t carousel_base = pose_identity;
static material_t carousel_mat = nullptr;
static bool stick_latched = false;
static bool show_ui_ray = true;
static float voxel_size = 0.045f;
static float hue = 0.45f;
static bool painting = true;
static bool edges_enabled = true;
static float camera_opacity = 1.0f;
static float edge_hue = 0.5f;
static float background_hue = 0.0f;
static float background_brightness = 0.0f;
static mesh_t cube = nullptr;
static material_t material = nullptr;
struct PaintedVoxel { vec3 position; float size; color128 tint; };
static std::vector<PaintedVoxel> voxels;
static constexpr size_t max_voxels = 4096;
static bool trigger_down = false;
static bool last_paint_valid = false;
static vec3 last_paint{};
static float brush_distance = 0.075f;
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
static void paint_at(vec3 position) {
    if (voxels.size() >= max_voxels) return;
    voxels.push_back({position, voxel_size,
        color_hsv(hue, 1.0f, 1.0f, 1.0f)});
}
static void update_brush() {
    const controller_t* right = input_controller(handed_right);
    if (!right || !(right->tracked & button_state_active)) {
        trigger_down = false;
        last_paint_valid = false;
        return;
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
    // Live fixed-size, axis-aligned brush cursor.
    render_add_mesh(cube, material,
        matrix_ts(tip, {voxel_size, voxel_size, voxel_size}),
        color_hsv(hue, 1.0f, 1.0f, 1.0f));
    if (!painting || !pressed) { last_paint_valid = false; return; }

    if (!last_paint_valid) {
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


static void update_controller_palette() {
    const controller_t* left = input_controller(handed_left);
    if (!left || !(left->tracked & button_state_active)) return;
    if (std::fabs(left->stick.x) < 0.35f) stick_latched = false;
    if (!stick_latched && std::fabs(left->stick.x) > 0.7f) {
        const int direction = left->stick.x > 0 ? 1 : -1;
        menu_page = (menu_page + direction + page_count) % page_count;
        target_angle += direction * 90.0f;
        stick_latched = true;
    }
    const vec3 local_offset = {0.0f, 0.16f, -0.11f};
    const vec3 offset = quat_mul_vec(left->pose.orientation, local_offset);
    carousel_base.position = {left->pose.position.x + offset.x,
                              left->pose.position.y + offset.y,
                              left->pose.position.z + offset.z};
    const pose_t* head = input_head();
    if (head) carousel_base.orientation =
        quat_lookat(carousel_base.position, head->position);

    // Rotate the transparent four-sided carousel, smoothly and independently
    // from the controller's six-degree-of-freedom position.
    const float delta = target_angle - carousel_angle;
    carousel_angle += delta * std::min(1.0f, time_stepf() * 12.0f);
    const quat spin = quat_from_angles(0, carousel_angle, 0);
    const quat orientation = quat_mul(carousel_base.orientation, spin);
    render_add_mesh(cube, carousel_mat,
        matrix_trs(carousel_base.position, orientation,
                   {menu_width, menu_height, menu_width}),
        {0.20f, 0.65f, 0.85f, 0.12f});

    // UI is mounted on the front face, with its lower edge above controller.
    // The active page swaps when the face rotation ends, preventing rear-face
    // panels from becoming unexpectedly interactive.
    const vec3 anchor = quat_mul_vec(carousel_base.orientation,
        {-menu_width*0.5f, menu_height*0.5f, -menu_width*0.5f - 0.008f});
    palette.position = {carousel_base.position.x + anchor.x,
                        carousel_base.position.y + anchor.y,
                        carousel_base.position.z + anchor.z};
    palette.orientation = carousel_base.orientation;
}
static void draw_menu_ray() {
    if (!show_ui_ray) return;
    const controller_t* right = input_controller(handed_right);
    if (!right || !(right->tracked & button_state_active)) return;
    const vec3 start = right->aim.position;
    const vec3 fwd = aim_forward(right->aim.orientation);
    const vec3 end = {start.x + fwd.x * 1.5f,
                      start.y + fwd.y * 1.5f,
                      start.z + fwd.z * 1.5f};
    const color32 ray_color = {110, 215, 255, 150};
    line_add(start, end, ray_color, ray_color, 0.0015f);
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
        // Behind the stereoscopic projection and UI.
        // Place edges above the opaque StereoKit background.
        backend_openxr_composition_layer(&composition, sizeof(composition), 100);
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
    if (!sk_init(app_settings)) return 1;
    edge_passthrough.initialize();
    cube = mesh_find(default_id_mesh_cube);
    material = material_copy(material_find(default_id_material));
    voxels.reserve(max_voxels);
    carousel_mat = material_copy(material_find(default_id_material));
    material_set_transparency(carousel_mat, transparency_blend);
    input_hand_visible(handed_left, false);
    input_hand_visible(handed_right, false);
    input_hand_solid(handed_left, false);
    input_hand_solid(handed_right, false);
    ui_enable_far_interact(true);
    sk_run([] {
        update_controller_palette();
        ui_window_begin("VOXEN | Palette", palette, {0.24f, 0},
                        ui_win_normal, ui_move_none);
        ui_label("Left stick: rotate cube");
        if (menu_page == 0) {
            ui_label("BRUSH - right trigger to paint");
            ui_hslider("Size (m)", voxel_size, 0.015f, 0.15f, 0.005f);
            ui_hslider("Hue", hue, 0.0f, 1.0f, 0.01f);
            ui_hslider("Tip offset", brush_distance, 0.02f, 0.20f, 0.005f);
            if (ui_button(painting ? "Brush: ON" : "Brush: OFF")) painting = !painting;
        } else if (menu_page == 1) {
            ui_label("SCENE");
            if (ui_button("Undo last voxel") && !voxels.empty()) voxels.pop_back();
            if (ui_button("Clear voxels")) { voxels.clear(); last_paint_valid = false; }
            char count[64];
            std::snprintf(count, sizeof(count), "Voxels: %zu / %zu", voxels.size(), max_voxels);
            ui_label(count);
            if (ui_button(show_ui_ray ? "UI ray: ON" : "UI ray: OFF"))
                show_ui_ray = !show_ui_ray;
        } else if (menu_page == 2) {
            ui_label("PASSTHROUGH / FX");
            if (edge_passthrough.available) {
                if (ui_button(edges_enabled ? "Contours: ON" : "Contours: OFF"))
                    edges_enabled = !edges_enabled;
                if (edges_enabled) {
                    ui_hslider("Camera", camera_opacity, 0.0f, 1.0f, 0.01f);
                    ui_hslider("Edge hue", edge_hue, 0.0f, 1.0f, 0.01f);
                    ui_hslider("Background hue", background_hue, 0.0f, 1.0f, 0.01f);
                    ui_hslider("Background brightness", background_brightness, 0.0f, 1.0f, 0.01f);
                }
            } else ui_label("Native contours unavailable");
        } else {
            ui_label("VOXEN / INFO");
            ui_label("Right trigger: paint voxels");
            ui_label("Left stick: rotate palette");
            ui_label("Four faces / four panels");
        }
        ui_window_end();

        for (const PaintedVoxel& voxel : voxels)
            render_add_mesh(cube, material,
                matrix_ts(voxel.position, {voxel.size, voxel.size, voxel.size}),
                voxel.tint);
        update_brush();
        draw_menu_ray();
        if (edges_enabled && edge_passthrough.available) {
            render_enable_skytex(false);
            render_set_clear_color(
                color_hsv(background_hue, 1.0f, background_brightness, 1.0f));
        } else {
            render_set_clear_color({0, 0, 0, 0});
        }
        edge_passthrough.frame(edges_enabled, camera_opacity,
            color_hsv(edge_hue, 1.0f, 1.0f, 1.0f));
    }, [] {
        edge_passthrough.shutdown();
        mesh_release(cube);
        material_release(material);
        material_release(carousel_mat);
    });
    return 0;
}
#ifndef __ANDROID__
int main() { return voxen_stereokit_main(); }
#endif
