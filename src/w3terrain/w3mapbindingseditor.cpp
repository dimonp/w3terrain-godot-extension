#include "w3defs.h"
#include "w3mapnode.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapbindingseditor.h"

namespace w3terr {

void
W3MapBindingsEditor::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("create_map", "size"), &W3MapBindingsEditor::create_map);
    godot::ClassDB::bind_method(godot::D_METHOD("increase_cellpoint_layer", "coord"), &W3MapBindingsEditor::increase_cellpoint_layer);
    godot::ClassDB::bind_method(godot::D_METHOD("decrease_cellpoint_layer", "coord"), &W3MapBindingsEditor::decrease_cellpoint_layer);
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

bool
W3MapBindingsEditor::check_cellpoint_layer(const godot::Vector2i& coords, uint8_t base_layer) const
{
    // near pattern:
    //  +++
    //  +*+
    //  +++
    static constexpr std::array<Delta2D, 8> kLayerCheckPattern = {{
        {0, -1}, {1, 0}, {0, 1}, {-1, 0},
        {-1, -1}, {1, -1}, {1, 1}, {-1, 1},
    }};

    for (const auto& pattern : kLayerCheckPattern) {
        const int32_t loc_idx_2d_x = coords.x + pattern.dx;
        const int32_t loc_idx_2d_y = coords.y + pattern.dy;

        if (!w3e_map()->is_valid_cellpoint(loc_idx_2d_x, loc_idx_2d_y)) {
            return false;
        }

        const uint8_t layer = w3e_map()->get_cellpoint_layer(loc_idx_2d_x, loc_idx_2d_y);
        if (std::abs(base_layer - layer) > 1) {
            return false;
        }
    }
    return true;
}


void
W3MapBindingsEditor::increase_cellpoint_layer(const godot::Vector2i& coords) const
{
    if (!w3e_map().is_valid()) {
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return;
    }

    const uint8_t base_layer = w3e_map()->get_cellpoint_layer(coords.x, coords.y);
    if (!check_cellpoint_layer(coords, base_layer + 1)) {
        return;
    }

    w3e_map()->set_cellpoint_layer(coords.x, coords.y, base_layer + 1);
    // update section mesh
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

void
W3MapBindingsEditor::decrease_cellpoint_layer(const godot::Vector2i& coords) const
{
    if (!w3e_map().is_valid()) {
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return;
    }

    const uint8_t base_layer = w3e_map()->get_cellpoint_layer(coords.x, coords.y);
    if (!check_cellpoint_layer(coords, base_layer - 1)) {
        return;
    }

    w3e_map()->set_cellpoint_layer(coords.x, coords.y, base_layer - 1);
    // update section mesh
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

void
W3MapBindingsEditor::set_cellpoint_ground_height(const godot::Vector2i& coords, float height) const
{
    if (!w3e_map().is_valid()) {
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return;
    }

    w3e_map()->set_cellpoint_ground_height(coords.x, coords.y, height);
    // update section mesh
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

}  // namespace w3terr
