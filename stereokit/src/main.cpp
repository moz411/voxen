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
static constexpr float menu_width = 0.17f;
static constexpr float menu_height = 0.23f;
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
static mesh_t sphere_mesh = nullptr;
static sound_t click_sound = nullptr;
static float size_velocity = 0.0f;
static float hue = 0.45f;
static bool painting = true;
static bool edges_enabled = true;
static float camera_opacity = 0.0f;
static float edge_hue = 0.5f;
static float background_hue = 0.64f;
static float background_brightness = 0.33f;
static mesh_t cube = nullptr;
static material_t material = nullptr;
struct PaintedVoxel { vec3 position; float size; color128 tint; int shape; };
static std::vector<PaintedVoxel> voxels;
static std::vector<size_t> stroke_starts;
static std::vector<std::vector<PaintedVoxel>> redo_strokes;
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
static void paint_at(vec3 position) {
    if (voxels.size() >= max_voxels) return;
    const float size = voxel_size;
    position = {std::round(position.x/size)*size,
                std::round(position.y/size)*size,
                std::round(position.z/size)*size};
    for (const auto& v : voxels) {
        const float min_gap = (v.size + size)*0.5f - 0.0001f;
        if (std::fabs(v.position.x-position.x) < min_gap &&
            std::fabs(v.position.y-position.y) < min_gap &&
            std::fabs(v.position.z-position.z) < min_gap) return;
    }
    redo_strokes.clear();
    voxels.push_back({position, size, color_hsv(hue,1,1,1), selected_shape});
}
static void update_brush() {
    const controller_t* right = input_controller(handed_right);
    if (!right || !(right->tracked & button_state_active)) {
        trigger_down = false;
        last_paint_valid = false;
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
        selected_shape = (selected_shape + (right->stick.y > 0 ? 1 : 1)) % 2;
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
    // Do not paint through the UI when the right ray targets the palette.
    if (menu_aiming || menu_capture) { last_paint_valid = false; trigger_down = false; return; }
    // Live fixed-size, axis-aligned brush cursor.
    render_add_mesh(selected_shape ? sphere_mesh : cube, material,
        matrix_ts(tip, {voxel_size, voxel_size, voxel_size}),
        color_hsv(hue, 1.0f, 1.0f, 1.0f));
    if (!painting || !pressed) { last_paint_valid = false; return; }

    if (!last_paint_valid) {
        if (voxels.size() < max_voxels) stroke_starts.push_back(voxels.size());
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


static void undo_stroke() {
    if (stroke_starts.empty()) return;
    const size_t start = stroke_starts.back();
    stroke_starts.pop_back();
    redo_strokes.emplace_back(voxels.begin() + start, voxels.end());
    voxels.resize(start);
    last_paint_valid = false;
}
static void redo_stroke() {
    if (redo_strokes.empty()) return;
    std::vector<PaintedVoxel> stroke = std::move(redo_strokes.back());
    redo_strokes.pop_back();
    stroke_starts.push_back(voxels.size());
    for (auto &v : stroke) {
        if (voxels.size() >= max_voxels) break;
        voxels.push_back(v);
    }
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
static constexpr float panel_height = 0.21f;
static constexpr float panel_lift = 0.002f;

static pose_t face_panel_pose(int page) {
    const quat rotation = quat_mul(
        quat_mul(carousel_base.orientation,quat_from_angles(0,carousel_angle,0)),
        quat_from_angles(0,-90.0f*page,0));
    const vec3 offset=quat_mul_vec(rotation,{0,0,-menu_width*0.5f-0.003f});
    return {{carousel_base.position.x+offset.x,
             carousel_base.position.y+offset.y,
             carousel_base.position.z+offset.z},rotation};
}
static vec3 panel_world(const pose_t& face,float x,float y,float z=0) {
    const vec3 o=quat_mul_vec(face.orientation,{x,y,z});
    return {face.position.x+o.x,face.position.y+o.y,face.position.z+o.z};
}
static void face_rect(const pose_t& face,float x,float y,float w,float h,
                      color128 color,float z=-0.002f) {
    render_add_mesh(cube,carousel_mat,matrix_trs(
        panel_world(face,x,y,z),face.orientation,{w,h,0.002f}),color);
}
static void face_text(const pose_t& face,const char* label,float x,float y) {
    const quat rotation=quat_mul(face.orientation,quat_from_angles(0,180,0));
    text_add_at(label,matrix_trs(panel_world(face,x,y,-0.006f),
        rotation,{1,1,1}),panel_text_style);
}
static bool panel_hover(int page,float x,float y,float w,float h) {
    return menu_aiming && hit_page==page &&
       std::fabs(menu_hit_x-x)<w*0.5f &&
       std::fabs(menu_hit_y-y)<h*0.5f;
}
static bool face_button(const pose_t& face,int page,const char* label,float y) {
    const bool hover=panel_hover(page,0,y,0.135f,0.042f);
    face_rect(face,0,y,0.135f,0.038f,
        hover?color128{0.15f,0.75f,0.9f,0.9f}:
              color128{0.12f,0.25f,0.35f,0.85f});
    face_text(face,label,0,y);
    if(hover && menu_click && click_sound)
        sound_play(click_sound,menu_hit,0.35f);
    return hover && menu_click;
}
static void draw_face(int page) {
    const pose_t face=face_panel_pose(page);
    face_rect(face,0,0,panel_width,panel_height,
        {0.06f,0.13f,0.22f,0.80f},0);
    static const char* titles[]={"COLORS","SHAPES","SOUNDS","SCENES"};
    face_text(face,titles[page],0,0.085f);
    if(page==0) {
        for(int i=0;i<16;++i) {
            const float x=-0.057f+(i%4)*0.038f;
            const float y=0.041f-(i/4)*0.038f;
            const bool hover=panel_hover(page,x,y,0.034f,0.034f);
            face_rect(face,x,y,hover?0.038f:0.032f,
                hover?0.038f:0.032f,color_hsv(i/16.0f,1,1,1));
            if(hover && menu_click) {
                hue=i/16.0f;
                if(click_sound) sound_play(click_sound,menu_hit,0.35f);
            }
        }
    } else if(page==1) {
        if(face_button(face,page,"CUBE",0.035f)) selected_shape=0;
        if(face_button(face,page,"SPHERE",-0.025f)) selected_shape=1;
    } else if(page==2) {
        face_text(face,"AUDIO: SOON",0,0.015f);
    } else {
        face_text(face,"SAVE/LOAD: SOON",0,0.015f);
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
    const vec3 local_offset = {0.0f, 0.06f, -0.025f};
    const vec3 offset = quat_mul_vec(left->pose.orientation, local_offset);
    carousel_base.position = {left->pose.position.x + offset.x,
                              left->pose.position.y + offset.y,
                              left->pose.position.z + offset.z};
    // Only yaw toward the headset. World Y stays upright even if the
    // controller rolls/pitches or the user's head is above the carousel.
    const pose_t* head = input_head();
    if (head) {
        vec3 target = {head->position.x, carousel_base.position.y,
                       head->position.z};
        const float dx = target.x - carousel_base.position.x;
        const float dz = target.z - carousel_base.position.z;
        if (dx*dx + dz*dz > 0.0001f)
            carousel_base.orientation = quat_lookat_up(
                carousel_base.position, target, {0, 1, 0});
    }

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

    palette = face_panel_pose(menu_page);
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
        // Behind the stereoscopic projection and UI.
        // Place edges above the opaque StereoKit background.
        // Composite the contours above the opaque color background.
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
    sphere_mesh = mesh_gen_sphere(1.0f,3);
    material = material_copy(material_find(default_id_material));
    click_sound = sound_generate([](float t) -> float {
        const float envelope = std::exp(-42.0f*t);
        return 0.28f*envelope*std::sin(6.2831853f*880.0f*t);
    }, 0.09f);
    voxels.reserve(max_voxels);
    carousel_mat = material_copy(material_find(default_id_material));
    material_set_transparency(carousel_mat, transparency_blend);
    input_hand_visible(handed_left, false);
    input_hand_visible(handed_right, false);
    input_hand_solid(handed_left, false);
    input_hand_solid(handed_right, false);
    ui_enable_far_interact(false);
    panel_text_style=text_make_style(font_find(default_id_font),0.012f,
        {0.95f,0.98f,1,1});
    sk_run([] {
        update_controller_palette();
        update_left_buttons();
        update_menu_ray();
        for(int page=0;page<page_count;++page) draw_face(page);

        for (const PaintedVoxel& voxel : voxels)
            render_add_mesh(voxel.shape ? sphere_mesh : cube, material,
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
        mesh_release(sphere_mesh);
        material_release(material);
        material_release(carousel_mat);
        if (click_sound) sound_release(click_sound);
    });
    return 0;
}
#ifndef __ANDROID__
int main() { return voxen_stereokit_main(); }
#endif
