#include "w3mapbindingseditor.h"

#include "w3defs.h"
#include "w3mapnode.h"
#include "w3mapsectionmanager_impl.h"

namespace w3terr {

void
W3MapBindingsEditor::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("create_map", "size", "ground_tilesets", "geo_tilesets"), &W3MapBindingsEditor::create_map);

    godot::ClassDB::bind_method(godot::D_METHOD("increase_cellpoint_layer", "coord"), &W3MapBindingsEditor::increase_cellpoint_layer);
    godot::ClassDB::bind_method(godot::D_METHOD("decrease_cellpoint_layer", "coord"), &W3MapBindingsEditor::decrease_cellpoint_layer);
    godot::ClassDB::bind_method(godot::D_METHOD("set_cellpoint_ground_height", "coord", "height"), &W3MapBindingsEditor::set_cellpoint_ground_height);
    godot::ClassDB::bind_method(godot::D_METHOD("set_cellpoint_water_height", "coord", "height"), &W3MapBindingsEditor::set_cellpoint_water_height);
    godot::ClassDB::bind_method(godot::D_METHOD("set_cellpoint_ground_tileset", "coord", "tileset", "variation"), &W3MapBindingsEditor::set_cellpoint_ground_tileset);
    godot::ClassDB::bind_method(godot::D_METHOD("set_cellpoint_geo_tileset", "coord", "tileset"), &W3MapBindingsEditor::set_cellpoint_geo_tileset);
    godot::ClassDB::bind_method(godot::D_METHOD("set_cellpoint_geo_variation", "coord", "variation"), &W3MapBindingsEditor::set_cellpoint_geo_variation);
    godot::ClassDB::bind_method(godot::D_METHOD("set_cellpoint_water", "coord", "flag"), &W3MapBindingsEditor::set_cellpoint_water);
    godot::ClassDB::bind_method(godot::D_METHOD("set_cellpoint_ramp", "coord", "flag"), &W3MapBindingsEditor::set_cellpoint_ramp);
}

inline
auto
W3MapBindingsEditor::w3e_map() const
{
    Expects(map_node_ != nullptr);
    return map_node_->get_w3e_resource();
}

void
W3MapBindingsEditor::create_map(const godot::Vector2i& size, uint8_t ground_tilesets, uint8_t geo_tilesets) const
{
    Expects(map_node_ != nullptr);
    if ((size.x - 1) % 4 != 0 || (size.y - 1) % 4 != 0) {
        w3_log_error("Map size must be multiple of 4 plus 1 (33, 65, 129, 192, 257, 401).");
        return;
    }
    map_node_->create_empty_map(size.x, size.y, ground_tilesets, geo_tilesets);
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
        // there should not be a difference in levels of more than two
        return;
    }

    auto& cell_point = w3e_map()->get_cellpoint(coords.x, coords.y);
    cell_point.height_layer = base_layer + 1;
    cell_point.clear_flag(W3eCell::Flags::RAMP);
    cell_point.clear_flag(W3eCell::Flags::WATER);
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
        // there should not be a difference in levels of more than two
        return;
    }

    auto& cell_point = w3e_map()->get_cellpoint(coords.x, coords.y);
    cell_point.height_layer = base_layer - 1;
    cell_point.clear_flag(W3eCell::Flags::RAMP);
    cell_point.clear_flag(W3eCell::Flags::WATER);
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
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

void
W3MapBindingsEditor::set_cellpoint_water_height(const godot::Vector2i& coords, float height) const
{
    if (!w3e_map().is_valid()) {
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return;
    }
    w3e_map()->set_cellpoint_water_height(coords.x, coords.y, height);
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

void
W3MapBindingsEditor::set_cellpoint_ground_tileset(const godot::Vector2i& coords, uint8_t tileset, uint8_t variation) const
{
    if (!w3e_map().is_valid()) {
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return;
    }

    auto& cell = w3e_map()->get_cellpoint(coords.x, coords.y);
    cell.ground_tileset = tileset;
    cell.ground_variation = variation;
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

void
W3MapBindingsEditor::set_cellpoint_geo_tileset(const godot::Vector2i& coords, uint8_t tileset) const
{
    if (!w3e_map().is_valid()) {
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return;
    }

    auto& cell = w3e_map()->get_cellpoint(coords.x, coords.y);
    cell.geo_tileset = tileset;
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

void
W3MapBindingsEditor::set_cellpoint_geo_variation(const godot::Vector2i& coords, uint8_t variation) const
{
    if (!w3e_map().is_valid()) {
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return;
    }

    auto& cell = w3e_map()->get_cellpoint(coords.x, coords.y);
    cell.geo_variation = variation;
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

void
W3MapBindingsEditor::set_cellpoint_water(const godot::Vector2i& coords, bool flag) const
{
    if (!w3e_map().is_valid()) {
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return;
    }

    auto& cell = w3e_map()->get_cellpoint(coords.x, coords.y);
    if (flag) {
        cell.set_flag(W3eCell::Flags::WATER);
    } else {
        cell.clear_flag(W3eCell::Flags::WATER);
    }
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}

void
W3MapBindingsEditor::set_cellpoint_ramp(const godot::Vector2i& coords, bool flag) const
{
    if (!w3e_map().is_valid()) {
        return;
    }
    if (!w3e_map()->is_valid_cellpoint(coords.x, coords.y)) {
        return;
    }

    auto& cell = w3e_map()->get_cellpoint(coords.x, coords.y);
    if (flag) {
        const uint8_t base_layer = w3e_map()->get_cellpoint_layer(coords.x, coords.y);
        if (!check_cellpoint_layer(coords, base_layer + 1) &&
            !check_cellpoint_layer(coords, base_layer - 1)) {
            // have to be a level difference
            return;
        }
        cell.set_flag(W3eCell::Flags::RAMP);
    } else {
        cell.clear_flag(W3eCell::Flags::RAMP);
    }
    map_node_->get_section_manager()->invalidate_sections_at_cellpoint(static_cast<Coord2D>(coords));
}


}  // namespace w3terr
