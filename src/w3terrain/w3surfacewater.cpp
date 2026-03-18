#include <godot_cpp/classes/performance.hpp>

#include "w3mapruntimemanager_impl.h"
#include "w3mapcollector_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapsection.h"
#include "w3mapnode.h"
#include "w3surfacewater.h"

namespace w3terr {

void
W3SurfaceWater::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_water_material"), &W3SurfaceWater::get_water_material);
    godot::ClassDB::bind_method(godot::D_METHOD("set_water_material", "ground_material"), &W3SurfaceWater::set_water_material);
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
#ifdef W3MAP_STATS_ENABLE
    godot::Performance *perf = godot::Performance::get_singleton();
    const auto stat_ground_tiles_precached_callable = callable_mp(this, &W3SurfaceWater::get_stat_water_tiles_precached);
    perf->add_custom_monitor(kStatWaterTilesPrecachedId, stat_ground_tiles_precached_callable);
#endif
}

void
W3SurfaceWater::_exit_tree()
{
#ifdef W3MAP_STATS_ENABLE
    godot::Performance *perf = godot::Performance::get_singleton();
    if (perf->has_custom_monitor(kStatWaterTilesPrecachedId)) {
        perf->remove_custom_monitor(kStatWaterTilesPrecachedId);
    }
#endif
}

void
W3SurfaceWater::_process(double  /*delta*/)
{
    if (get_map_node() == nullptr) {
        return;
    }

    const auto* assets = get_assets();
    // release all early rendered meshes in the GPU only if the maximum count is reached
    // or runtime data is outdated
    if (is_mesh_dirty() || mesh_->get_surface_count() + 1 > kMaxGPUMeshes) {
        mesh_->clear_surfaces();
        section_rendered_flags_.clear();
    }

    const auto* collector = get_collector();
    if (collector == nullptr) {
        return;
    }

    const W3Array<uint32_t>& visible_sections = collector->get_visible_sections();
    if (visible_sections.empty()) {
        return;
    }

    render(visible_sections);
}

void
W3SurfaceWater::render(const W3Array<uint32_t>& collected_sections)
{
    not_rendered_sections_.clear();

    // filter already rendered sections
    std::ranges::copy_if (collected_sections,
        std::back_inserter(not_rendered_sections_),
        [&rendered_flags = section_rendered_flags_](uint32_t section_id) {
            return !rendered_flags[section_id];
        });

    if (not_rendered_sections_.empty()) {
        return;
    }

    begin_render();
    surface_tool_->set_material(water_material_asset_);
    for(const auto section_ptr : not_rendered_sections_) {
        section_rendered_flags_[section_ptr] = true;
        render_cells(section_ptr);
    }
    end_render();
}

void
W3SurfaceWater::render_cells(const uint32_t section_id)
{
    const auto* section_manager = get_section_manager();
    const W3MapSection& section = section_manager->get_section_by_id(section_id);
    if (section.get_water_vertices_count() > 0) {
        const W3MapSection::CachedMesh &cached_mesh = section.get_cached_waters_mesh();
        if (!cached_mesh.is_used()) {
            return;
        }

        auto [vertices, indices] = cached_mesh.get_cached_mesh_data<CachedVertex, uint16_t>();
        if (vertices.empty()) {

#ifdef W3MAP_STATS_ENABLE
    stat_water_tiles_precached_ = 0;
#endif
            std::tie(vertices, indices) = precache_cells(section_id);
        }

        render_cached_mesh(vertices, indices);
    }
}

W3Pair<W3SurfaceWater::VertexSpan, W3SurfaceWater::IndexSpan>
W3SurfaceWater::precache_cells(const uint32_t section_id) const
{
    const auto* section_manager = get_section_manager();

    const W3MapSection& section = section_manager->get_section_by_id(section_id);
    const W3MapSection::CachedMesh &cached_mesh = section.get_cached_waters_mesh();

    // alloc lru cache memory
    auto [vertices, indices] = cached_mesh.allocate_mesh_data<CachedVertex, uint16_t>();
    if (vertices.empty()) {
        w3_log_error("Can't allocate memory in cache.");
        return {};
    }

    uint16_t dest_vertex_idx = 0;
    uint16_t dest_index_idx = 0;

    const auto* runtime_manager = get_map_node()->get_runtime_manager();

    // for each cell in this section
    for(size_t cell_idx = 0; cell_idx < W3MapSection::kNumberOfCells; ++cell_idx) {
        if (!cached_mesh.is_used_by(cell_idx)) { // Is there water in the cell?
            continue;
        }

        const Coord2D cell_coord = section.calc_cell_coord_from_idx(cell_idx);

        const math::vector3 base_water_pos = runtime_manager->get_cellpoint_water_position(cell_coord);
        const float water_height = base_water_pos.y;

        // vertex 00
        vertices[dest_vertex_idx].pos = base_water_pos;
        ++dest_vertex_idx;

        math::vector3 cellpoint_position;
        // vertex 10
        cellpoint_position = runtime_manager->get_cellpoint_position({ cell_coord.x + 1, cell_coord.y });
        vertices[dest_vertex_idx].pos = cellpoint_position;
        vertices[dest_vertex_idx].pos.y = water_height;
        ++dest_vertex_idx;

        // vertex 11
        cellpoint_position = runtime_manager->get_cellpoint_position({ cell_coord.x + 1, cell_coord.y + 1 });
        vertices[dest_vertex_idx].pos = cellpoint_position;
        vertices[dest_vertex_idx].pos.y = water_height;
        ++dest_vertex_idx;

        // vertex 01
        cellpoint_position = runtime_manager->get_cellpoint_position({ cell_coord.x, cell_coord.y + 1 });
        vertices[dest_vertex_idx].pos = cellpoint_position;
        vertices[dest_vertex_idx].pos.y = water_height;
        ++dest_vertex_idx;

        // cell indices counterclockwise
        indices[dest_index_idx++] = dest_vertex_idx - 4; // 0
        indices[dest_index_idx++] = dest_vertex_idx - 2; // 2
        indices[dest_index_idx++] = dest_vertex_idx - 3; // 1
        indices[dest_index_idx++] = dest_vertex_idx - 4; // 0
        indices[dest_index_idx++] = dest_vertex_idx - 1; // 3
        indices[dest_index_idx++] = dest_vertex_idx - 2; // 2

#ifdef W3MAP_STATS_ENABLE
        stat_water_tiles_precached_++;
#endif
    }
    return { vertices, indices };
}

void
W3SurfaceWater::render_cached_mesh(VertexSpan vertices, IndexSpan indices)
{
    static const auto kNormalUp = math::vector3(0.0F, 1.0F, 0.0F);
    for(const auto& vertex : vertices) {
        surface_tool_->set_normal(kNormalUp);
        surface_tool_->add_vertex(vertex.pos);
    }

    for(const auto index : indices) {
        surface_tool_->add_index(static_cast<int32_t>(index + vertices_counter_));
    }

    vertices_counter_ += static_cast<int64_t>(vertices.size());
}

}  // namespace w3terr