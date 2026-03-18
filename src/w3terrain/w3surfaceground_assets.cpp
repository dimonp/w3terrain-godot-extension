#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>

#include "w3mapassets.h"
#include "w3mapnode.h"
#include "w3surfaceground.h"

namespace w3terr {

W3Ref<W3Marerial>
W3SurfaceGround::get_ground_material() const
{
    return ground_material_asset_;
}

void
W3SurfaceGround::set_ground_material(const W3Ref<W3Marerial>& material)
{
    ground_material_asset_ = material;
    ground_assets_dirty_flag_ = true;
}

W3Ref<W3Marerial>
W3SurfaceGround::get_geo_materials() const
{
    return geo_material_asset_;
}

void
W3SurfaceGround::set_geo_materials(const W3Ref<W3Marerial>& material)
{
    geo_material_asset_ = material;
    geo_assets_dirty_flag_ = true;
}

bool
W3SurfaceGround::get_render_normals() const
{
    return render_normals_;
}

void
W3SurfaceGround::set_render_normals(bool flag)
{
    reset_rendered();
    render_normals_ = flag;
}

W3Ref<W3Marerial>
W3SurfaceGround::get_debug_material() const
{
    return debug_material_;
}

void
W3SurfaceGround::set_debug_material(const W3Ref<W3Marerial>& material)
{
    debug_material_ = material;
}

bool
W3SurfaceGround::load_ground_materials()
{
    ground_textures_array_.unref();
    if (ground_material_asset_.is_null()) {
        return false;
    }

    const auto* assets = get_map_node()->get_assets();
    const int64_t ground_tileset_size = assets->ground_assets_size_rt();

    godot::TypedArray<godot::Image> images;
    for(int64_t i = 0; i < ground_tileset_size; ++i) {
        W3Ref<W3Texture> ground_texture = assets->ground_asset_rt(i).texture;
        if (ground_texture.is_valid()) {
            images.push_back(ground_texture->get_image());
        }
    }
    ground_textures_array_.instantiate();
    ground_textures_array_->create_from_images(images);
    ground_material_asset_->set_shader_parameter("texture_array", ground_textures_array_);
    return true;
}

bool
W3SurfaceGround::load_geo_materials()
{
    geo_textures_array_.unref();
    if (geo_material_asset_.is_null()) {
        return false;
    }

    const auto* assets = get_map_node()->get_assets();
    const int64_t geo_tileset_size = assets->geo_assets_size_rt();

    godot::TypedArray<godot::Image> images;
    for(int64_t i = 0; i < geo_tileset_size; ++i) {
        const W3Ref<W3Texture>& geo_texture = assets->geo_asset_rt(i).texture;
        if (geo_texture.is_valid()) {
            images.push_back(geo_texture->get_image());
        }
    }
    geo_textures_array_.instantiate();
    geo_textures_array_->create_from_images(images);
    geo_material_asset_->set_shader_parameter("texture_array", geo_textures_array_);
    return true;
}

void
W3SurfaceGround::on_ground_assets_changed()
{
    w3_log_debug("W3MapSurfaceGround::on_ground_assets_changed");
    ground_assets_dirty_flag_ = true;
}

void
W3SurfaceGround::on_geo_assets_changed()
{
    w3_log_debug("W3MapSurfaceGround::on_geo_assets_changed");
    geo_assets_dirty_flag_ = true;
}

}  // namespace w3terr