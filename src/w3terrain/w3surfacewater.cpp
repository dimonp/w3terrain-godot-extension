#include "w3surfacewater.h"

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/world3d.hpp>

#include "w3mapruntimemanager_impl.h"
#include "w3mapcollector_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapsectionrenderedcache.h"
#include "w3mapsection.h"
#include "w3mapnode.h"

namespace w3terr {

void
W3SurfaceWater::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_water_material"), &W3SurfaceWater::get_water_material);
    godot::ClassDB::bind_method(godot::D_METHOD("set_water_material", "water_material"), &W3SurfaceWater::set_water_material);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "water_material",
            godot::PROPERTY_HINT_RESOURCE_TYPE,
            "Material"),
        "set_water_material", "get_water_material");
}

W3Ref<W3Marerial>
W3SurfaceWater::get_water_material() const
{
    return water_material_asset_;
}

void
W3SurfaceWater::set_water_material(const W3Ref<W3Marerial>& material)
{
    water_material_asset_ = material;
}

void
W3SurfaceWater::_enter_tree() {
    W3Surface::_enter_tree();

#ifdef W3MAP_STATS_ENABLE
    godot::Performance *perf = godot::Performance::get_singleton();
    const auto stat_ground_tiles_precached_callable = callable_mp(this, &W3SurfaceWater::get_stat_water_tiles_rendered);
    perf->add_custom_monitor(kStatWaterTilesRenderedId, stat_ground_tiles_precached_callable);
#endif
}

void
W3SurfaceWater::_exit_tree()
{
#ifdef W3MAP_STATS_ENABLE
    godot::Performance *perf = godot::Performance::get_singleton();
    if (perf->has_custom_monitor(kStatWaterTilesRenderedId)) {
        perf->remove_custom_monitor(kStatWaterTilesRenderedId);
    }
#endif
    clear_rendered();
}

void
W3SurfaceWater::_process(double  /*delta*/)
{
    if (get_map_node() == nullptr) {
        return;
    }

    const auto* collector = get_collector();
    if (collector == nullptr) {
        return;
    }

    if (is_mesh_dirty()) {
        clear_rendered();
    }

    const W3Array<uint32_t>& visible_sections = collector->get_visible_sections();
    if (visible_sections.empty()) {
        return;
    }

    if (is_visible_in_tree()) {
        render(visible_sections);
    }
}

void
W3SurfaceWater::render(const W3Array<uint32_t>& collected_sections)
{
#ifdef W3MAP_STATS_ENABLE
        stat_water_tiles_rendered_ = 0;
#endif

    auto* rendered_sections_cache = get_rendered_sections_cache();
    for(const auto section_id : collected_sections) {
        auto* rendered_section = rendered_sections_cache->get_rendered(section_id);
        if (rendered_section != nullptr && rendered_section->surface_idx_water >= 0) {
            continue;
        }

        render_section(section_id);
    }
}

void
W3SurfaceWater::render_section(const uint32_t section_id)
{
    const auto* section_manager = get_section_manager();
    const W3MapSection& section = section_manager->get_section_by_id(section_id);
    const auto& water_usage = section.get_water_usage();
    if (!water_usage.any()) {
        return;
    }

    int8_t surface_idx = begin_render(section_id);
    render_section_cells(section_id);
    auto* rendered_mesh = end_render(section_id);
    rendered_mesh->surface_idx_water = surface_idx;

    if (water_material_asset_.is_valid()) {
        RS->mesh_surface_set_material(rendered_mesh->get_mesh_rid(), surface_idx, water_material_asset_->get_rid());
    }
}

void
W3SurfaceWater::render_section_cells(const uint32_t section_id)
{
    const auto* section_manager = get_section_manager();
    const auto* runtime_manager = get_map_node()->get_runtime_manager();
    const W3MapSection& section = section_manager->get_section_by_id(section_id);
    const auto& water_usage = section.get_water_usage();

    // for each cell in this section
    for(size_t cell_idx = 0; cell_idx < W3MapSection::kNumberOfCells; ++cell_idx) {
        if (!water_usage[cell_idx]) { // Is there water in the cell?
            continue;
        }

        const auto section_origin = section_manager->calc_section_origin(section_id);
        const auto cell_coord = W3MapSection::calc_cell_coord_from_idx(section_origin, cell_idx);

        const math::vector3 base_water_pos = runtime_manager->get_cellpoint_water_position(cell_coord);
        const float water_height = base_water_pos.y;

        static const auto kNormalUp = math::vector3(0.0F, 1.0F, 0.0F);

        // vertex 00
        surface_tool_->set_normal(kNormalUp);
        surface_tool_->add_vertex(base_water_pos);

        math::vector3 cellpoint_position;
        // vertex 10
        cellpoint_position = runtime_manager->get_cellpoint_position({ cell_coord.x + 1, cell_coord.y });
        cellpoint_position.y = water_height;
        surface_tool_->set_normal(kNormalUp);
        surface_tool_->add_vertex(cellpoint_position);

        // vertex 01
        cellpoint_position = runtime_manager->get_cellpoint_position({ cell_coord.x, cell_coord.y + 1 });
        cellpoint_position.y = water_height;
        surface_tool_->set_normal(kNormalUp);
        surface_tool_->add_vertex(cellpoint_position);

        // vertex 11
        cellpoint_position = runtime_manager->get_cellpoint_position({ cell_coord.x + 1, cell_coord.y + 1 });
        cellpoint_position.y = water_height;
        surface_tool_->set_normal(kNormalUp);
        surface_tool_->add_vertex(cellpoint_position);

        // cell indices counterclockwise
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_));
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_ + 3));
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_ + 1));
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_));
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_ + 2));
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_ + 3));

        vertices_counter_ += 4;

#ifdef W3MAP_STATS_ENABLE
        stat_water_tiles_rendered_++;
#endif
    }
}

}  // namespace w3terr