#include "w3mapnode.h"
#include "w3mapruntimemanager_impl.h"
#include "w3mapbindings.h"

namespace w3terr {

void
W3MapBindings::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_name"), &W3MapBindings::get_map_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_size"), &W3MapBindings::get_map_size);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cell_bbox", "coord"), &W3MapBindings::get_cell_bbox);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cellpoint_ground_tilset", "coord"), &W3MapBindings::get_cellpoint_grond_tileset);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cellpoint_geo_tilset", "coord"), &W3MapBindings::get_cellpoint_geo_tileset);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cellpoint_type", "coord"), &W3MapBindings::get_cellpoint_type);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cellpoint_layer", "coord"), &W3MapBindings::get_cellpoint_layer);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cellpoint_ground_height", "coord"), &W3MapBindings::get_cellpoint_ground_height);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cellpoint_position", "coord"), &W3MapBindings::get_cellpoint_position);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cell_height", "coord"), &W3MapBindings::get_cell_height);
    godot::ClassDB::bind_method(godot::D_METHOD("get_height_at_point", "point"), &W3MapBindings::get_height_at_point);
    godot::ClassDB::bind_method(godot::D_METHOD("pick_cell_by_screen_position", "screen_position"), &W3MapBindings::pick_cell_by_screen_position);
}

inline
auto
W3MapBindings::w3e_map() const
{
    Expects(map_node_ != nullptr);
    return map_node_->get_w3e_resource();
}

inline
const W3MapRuntimeManagerImpl*
W3MapBindings::get_runtime() const
{
    w3_assert(map_node_ != nullptr);
    map_node_->refresh_runtime();
    return map_node_->get_runtime_manager();
}

godot::Variant
W3MapBindings::get_map_name() const
{
    if (w3e_map().is_null()) {
        return godot::Variant::NIL;
    }
    return w3e_map()->get_path().get_file();
}

godot::Variant
W3MapBindings::get_map_size() const
{
    if (w3e_map().is_null()) {
        return godot::Variant::NIL;
    }
    return godot::Vector2i {
        w3e_map()->get_map_2d_size_x(),
        w3e_map()->get_map_2d_size_y()
    };
}

godot::Variant
W3MapBindings::get_cell_bbox(const godot::Vector2i& coords) const
{
    if (w3e_map().is_null()) {
        return godot::Variant::NIL;
    }
    if (!w3e_map()->is_valid_cell(coords.x, coords.y)) {
        return godot::Variant::NIL;
    }
    return get_runtime()->get_cell_bbox(static_cast<Coord2D>(coords));
}

godot::Variant 
W3MapBindings::get_cellpoint_grond_tileset(const godot::Vector2i& coords) const
{
    if (w3e_map().is_null()) {
        return godot::Variant::NIL;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return godot::Variant::NIL;
    }
    return w3e_map()->get_cellpoint(coords.x, coords.y).ground_tileset;
}

godot::Variant 
W3MapBindings::get_cellpoint_geo_tileset(const godot::Vector2i& coords) const
{
    if (w3e_map().is_null()) {
        return godot::Variant::NIL;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return godot::Variant::NIL;
    }
    return w3e_map()->get_cellpoint(coords.x, coords.y).geo_tileset;
}

godot::Variant
W3MapBindings::get_cellpoint_type(const godot::Vector2i& coords) const
{
    if (w3e_map().is_null()) {
        return godot::Variant::NIL;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return godot::Variant::NIL;
    }
    return get_runtime()->get_cellpoint_rt(static_cast<Coord2D>(coords)).flags;
}

godot::Variant
W3MapBindings::get_cellpoint_layer(const godot::Vector2i& coords) const
{
    if (!w3e_map().is_valid()) {
        w3_log_error("W3E map is not valid.");
        return godot::Variant::NIL;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        w3_log_error("Cellpoint is not valid.");
        return godot::Variant::NIL;
    }
    return w3e_map()->get_cellpoint_layer(coords.x, coords.y);
}

godot::Variant
W3MapBindings::get_cellpoint_ground_height(const godot::Vector2i& coords) const
{
    if (!w3e_map().is_valid()) {
        w3_log_error("W3E map is not valid.");
        return godot::Variant::NIL;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        w3_log_error("Cellpoint is not valid.");
        return godot::Variant::NIL;
    }
    return w3e_map()->get_cellpoint_ground_height(coords.x, coords.y);
}

godot::Variant
W3MapBindings::get_cellpoint_position(const godot::Vector2i& coords) const
{
    if (w3e_map().is_null()) {
        return godot::Variant::NIL;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return godot::Variant::NIL;
    }
    return get_runtime()->get_cellpoint_position(static_cast<Coord2D>(coords));
}

godot::Variant
W3MapBindings::get_cell_height(const godot::Vector2i& coords) const
{
    // get height at center of cell
    return get_runtime()->get_cell_ground_height(static_cast<Coord2D>(coords), 0.5F, 0.5F);
}

godot::Variant
W3MapBindings::get_height_at_point(const godot::Vector3& point) const
{
    godot::Transform3D node_transform = map_node_->get_global_transform();
    math::line3 ray(
        node_transform.xform_inv({ point.x, 100000.0F, point.z }),
        { 0, -200000.0F, 0 });

    math::vector3 ipoint;
    std::optional<Coord2D> coords = map_node_->get_intersected_cell(ray, ipoint);
    if (coords.has_value()) {
        return ipoint.y;
    }
    return godot::Variant::NIL;
}

godot::Variant
W3MapBindings::pick_cell_by_screen_position(const math::vector2& screen_position) const
{
    if (!w3e_map().is_valid()) {
        w3_log_error("W3E map is not valid.");
    }
    const godot::Camera3D* p_camera = map_node_->get_camera();
    if (p_camera == nullptr) {
        return godot::Variant::NIL;
    }

    math::vector3 ray_origin = p_camera->project_ray_origin(screen_position);
    math::vector3 ray_direction= p_camera->project_ray_normal(screen_position) * 100000.0F;

    godot::Transform3D node_transform = map_node_->get_global_transform();
    math::line3 ray(node_transform.xform_inv(ray_origin), ray_direction);

    math::vector3 ipoint;
    std::optional<Coord2D> coords = map_node_->get_intersected_cell(ray, ipoint);
    if (coords.has_value()) {
        return godot::Vector2i { coords->x , coords->y };
    }
    return godot::Variant::NIL;
}

}  // namespace w3terr
