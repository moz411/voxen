#include <stereokit.h>
#include <stereokit_ui.h>
using namespace sk;

sk_settings_t app_settings = {"Voxen StereoKit"};
static pose_t palette = {{-0.25f, 0.0f, -0.55f}, quat_identity};
static float voxel_size = 0.045f;
static float hue = 0.45f;
static bool painting = true;
static mesh_t cube = nullptr;
static material_t material = nullptr;

int voxen_stereokit_main() {
    app_settings.display_preference = display_mode_mixedreality;
    app_settings.blend_preference = display_blend_any_transparent;
    if (!sk_init(app_settings)) return 1;
    cube = mesh_find(default_id_mesh_cube);
    material = material_find(default_id_material);
    sk_run([] {
        ui_window_begin("VOXEN | Palette", palette, {0.30f, 0});
        ui_label("Voxel brush - prototype");
        ui_hslider("Size (m)", voxel_size, 0.015f, 0.15f, 0.005f);
        ui_hslider("Hue", hue, 0.0f, 1.0f, 0.01f);
        if (ui_button(painting ? "Brush: ON" : "Brush: OFF")) painting = !painting;
        ui_window_end();

        // Reference voxel: follows size slider. Persistent painting is next milestone.
        if (painting)
            mesh_draw(cube, material, matrix_ts({0, 0, -0.8f},
                {voxel_size, voxel_size, voxel_size}));
    }, [] {
        mesh_release(cube);
        material_release(material);
    });
    return 0;
}
#ifndef __ANDROID__
int main() { return voxen_stereokit_main(); }
#endif
