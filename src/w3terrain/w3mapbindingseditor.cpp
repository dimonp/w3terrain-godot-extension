#include "w3defs.h"
#include "w3mapnode.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapbindingseditor.h"

namespace w3terr {

void
W3MapBindingsEditor::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("create_map", "size"), &W3MapBindingsEditor::create_map);
    godot::ClassDB::bind_method(godot::D_METHOD("increase_cellpoint_height_layer", "coord"), &W3MapBindingsEditor::increase_cellpoint_height_layer);
    godot::ClassDB::bind_method(godot::D_METHOD("decrease_cellpoint_height_layer", "coord"), &W3MapBindingsEditor::decrease_cellpoint_height_layer);
    godot::ClassDB::bind_method(godot::D_METHOD("set_cellpoint_ground_height", "coord", "height"), &W3MapBindingsEditor::set_cellpoint_ground_height);
}

inline
auto
W3MapBindingsEditor::w3e_map() const
{
    Expects(map_node_ != nullptr);
    return map_node_->get_w3e_resource();
}

void
W3MapBindingsEditor::create_map(const godot::Vector2i& size) const
{
    Expects(map_node_ != nullptr);
    if ((size.x - 1) % 4 != 0 || (size.y - 1) % 4 != 0) {
        w3_log_error("Map size must be multiple of 4 plus 1 (33, 65, 129, 192, 257, 401).");
        return;
    }

    map_node_->create_empty_map(size.x, size.y);
}

void
W3MapBindingsEditor::increase_cellpoint_height_layer(const godot::Vector2i& coords) const
{
    if (!w3e_map().is_valid()) {
        w3_log_error("W3E map is not valid.");
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        w3_log_error("Cellpoint is not valid.");
        return;
    }

    const uint8_t height_layer00 = w3e_map()->get_cellpoint_layer(coords.x,     coords.y);
    const uint8_t height_layer10 = w3e_map()->get_cellpoint_layer(coords.x + 1, coords.y);
    const uint8_t height_layer11 = w3e_map()->get_cellpoint_layer(coords.x + 1, coords.y + 1);
    const uint8_t height_layer01 = w3e_map()->get_cellpoint_layer(coords.x,     coords.y + 1);

    // height difference should not be more than 2
    const int32_t min_height = math::w3_min4(height_layer00, height_layer10, height_layer11, height_layer01);
    if (height_layer00 - min_height > 1) {
        return;
    }

    w3e_map()->set_cellpoint_layer(coords.x, coords.y, height_layer00 + 1);
    // update section mesh
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

void
W3MapBindingsEditor::decrease_cellpoint_height_layer(const godot::Vector2i& coords) const
{
    if (!w3e_map().is_valid()) {
        w3_log_error("W3E map is not valid.");
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        w3_log_error("Cellpoint is not valid.");
        return;
    }

    const uint8_t height_layer00 = w3e_map()->get_cellpoint_layer(coords.x,     coords.y);
    const uint8_t height_layer10 = w3e_map()->get_cellpoint_layer(coords.x + 1, coords.y);
    const uint8_t height_layer11 = w3e_map()->get_cellpoint_layer(coords.x + 1, coords.y + 1);
    const uint8_t height_layer01 = w3e_map()->get_cellpoint_layer(coords.x,     coords.y + 1);

    // height difference should not be more than 2
    const int32_t max_height = math::w3_max4(height_layer00, height_layer10, height_layer11, height_layer01);
    if (max_height - height_layer00 > 1) {
        return;
    }

    const uint8_t height_layer = w3e_map()->get_cellpoint_layer(coords.x, coords.y);
    w3e_map()->set_cellpoint_layer(coords.x, coords.y, height_layer - 1);
    // update section mesh
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

void
W3MapBindingsEditor::set_cellpoint_ground_height(const godot::Vector2i& coords, float height) const
{
    if (!w3e_map().is_valid()) {
        w3_log_error("W3E map is not valid.");
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        w3_log_error("Cellpoint is not valid.");
        return;
    }

    w3e_map()->set_cellpoint_ground_height(coords.x, coords.y, height);
    // update section mesh
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

}  // namespace w3terr
