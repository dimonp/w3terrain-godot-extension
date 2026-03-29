#include "w3surfaceterrain.h"

#include <godot_cpp/classes/performance.hpp>

#include "w3mapcollector_impl.h"
#include "w3mapruntimemanager_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapnode.h"

namespace w3terr {

void
W3SurfaceTerrain::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_ground_material"), &W3SurfaceTerrain::get_ground_material);
    godot::ClassDB::bind_method(godot::D_METHOD("set_ground_material", "ground_material"), &W3SurfaceTerrain::set_ground_material);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "ground_material",
            godot::PROPERTY_HINT_RESOURCE_TYPE,
            "Material"),
        "set_ground_material", "get_ground_material");

    godot::ClassDB::bind_method(godot::D_METHOD("get_geo_material"), &W3SurfaceTerrain::get_geo_materials);
    godot::ClassDB::bind_method(godot::D_METHOD("set_geo_material", "geo_material"), &W3SurfaceTerrain::set_geo_materials);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "geo_material",
            godot::PROPERTY_HINT_RESOURCE_TYPE,
            "ShaderMaterial"),
        "set_geo_material", "get_geo_material");

    godot::ClassDB::bind_method(godot::D_METHOD("get_debug_material"), &W3SurfaceTerrain::get_debug_material);
    godot::ClassDB::bind_method(godot::D_METHOD("set_debug_material", "debug_material"), &W3SurfaceTerrain::set_debug_material);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "debug_material",
            godot::PROPERTY_HINT_RESOURCE_TYPE,
            "Material"),
        "set_debug_material", "get_debug_material");

    godot::ClassDB::bind_method(godot::D_METHOD("get_render_normals"), &W3SurfaceTerrain::get_render_normals);
    godot::ClassDB::bind_method(godot::D_METHOD("set_render_normals", "p_camera"), &W3SurfaceTerrain::set_render_normals);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "render_normals"),
        "set_render_normals", "get_render_normals"
    );
}

void
W3SurfaceTerrain::_notification(int p_what)
{
    // avoid to save dinamicaly created resources to scene file
    switch (p_what) {
    case NOTIFICATION_EDITOR_PRE_SAVE: {
        ground_material_asset_->set_shader_parameter("texture_array", nullptr);
        geo_material_asset_->set_shader_parameter("texture_array", nullptr);
        break;
    }
    case NOTIFICATION_EDITOR_POST_SAVE: {
        ground_material_asset_->set_shader_parameter("texture_array", ground_textures_array_);
        geo_material_asset_->set_shader_parameter("texture_array", geo_textures_array_);
        break;
    }
    default:
        break;
    }
}

void
W3SurfaceTerrain::_enter_tree() {
    const auto callable_ground_assets_changed = callable_mp(this, &W3SurfaceTerrain::on_ground_assets_changed);
    if (!get_map_node()->is_connected(W3MapNode::kSignalGroundAssetsChanged, callable_ground_assets_changed)) {
        get_map_node()->connect(W3MapNode::kSignalGroundAssetsChanged, callable_ground_assets_changed);
    }

    const auto callable_geo_assets_changed = callable_mp(this, &W3SurfaceTerrain::on_geo_assets_changed);
    if (!get_map_node()->is_connected(W3MapNode::kSignalGeoAssetsChanged, callable_geo_assets_changed)) {
        get_map_node()->connect(W3MapNode::kSignalGeoAssetsChanged, callable_geo_assets_changed);
    }

#ifdef W3MAP_STATS_ENABLE
    godot::Performance *perf = godot::Performance::get_singleton();
    const auto stat_ground_tiles_precached_callable = callable_mp(this, &W3SurfaceTerrain::get_stat_ground_tiles_precached);
    perf->add_custom_monitor(kStatGroundTilesPrecachedId, stat_ground_tiles_precached_callable);
    const auto stat_geo_tiles_precached_callable = callable_mp(this, &W3SurfaceTerrain::get_stat_geo_tiles_precached);
    perf->add_custom_monitor(kStatGeoTilesPrecachedId, stat_geo_tiles_precached_callable);
#endif
}

void
W3SurfaceTerrain::_exit_tree()
{
#ifdef W3MAP_STATS_ENABLE
    godot::Performance *perf = godot::Performance::get_singleton();
    if (perf->has_custom_monitor(kStatGroundTilesPrecachedId)) {
        perf->remove_custom_monitor(kStatGroundTilesPrecachedId);
    }
    if (perf->has_custom_monitor(kStatGeoTilesPrecachedId)) {
        perf->remove_custom_monitor(kStatGeoTilesPrecachedId);
    }
#endif
}

void
W3SurfaceTerrain::_process(double  /*delta*/)
{
    if (get_map_node() == nullptr) {
        return;
    }

    if (ground_assets_dirty_flag_) {
        load_ground_materials();
        reset_rendered();
        ground_assets_dirty_flag_ = false;
    }

    if (geo_assets_dirty_flag_) {
        load_geo_materials();
        reset_rendered();
        geo_assets_dirty_flag_ = false;
    }

    const auto* assets = get_assets();
    // release all early rendered meshes in the GPU only if the maximum count is reached
    // or runtime data is outdated
    const size_t total_tilesets = static_cast<size_t>(assets->ground_assets_size_rt()) + assets->geo_assets_size_rt();
    if (is_mesh_dirty() || mesh_->get_surface_count() + total_tilesets > kMaxGPUMeshes) {
        reset_rendered();
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
W3SurfaceTerrain::reset_rendered()
{
    if (mesh_.is_valid()) {
        mesh_->clear_surfaces();
    }
    section_rendered_flags_.clear();
}

void
W3SurfaceTerrain::render(const W3Array<uint32_t>& sections)
{
    not_rendered_sections_.clear();
    // filter already rendered sections
    std::ranges::copy_if (sections,
        std::back_inserter(not_rendered_sections_),
        [&rendered_flags = section_rendered_flags_](uint32_t section_id) {
            if (rendered_flags.size() <= section_id) {
                rendered_flags.resize(static_cast<size_t>(section_id) + 1);
            }
            return !rendered_flags[section_id];
        });

    if (not_rendered_sections_.empty()) {
        return;
    }

    // mark all sections as rendered
    for(const uint32_t section_id : not_rendered_sections_) {
        section_rendered_flags_[section_id] = true;
    }

    render_geos(not_rendered_sections_, false);
    render_grounds(not_rendered_sections_, false);

    if (render_normals_) {
        render_geos(not_rendered_sections_, true);
        render_grounds(not_rendered_sections_, true);
    }
}

void
W3SurfaceTerrain::render_cached_mesh(VertexSpan vertices, IndexSpan indices)
{
    for(const auto& vertex : vertices) {
        surface_tool_->set_uv(vertex.uv);
        surface_tool_->set_normal(vertex.norm);
        surface_tool_->add_vertex(vertex.pos);
    }

    for(const auto index : indices) {
        surface_tool_->add_index(static_cast<int32_t>(index + vertices_counter_));
    }
    vertices_counter_ += static_cast<int64_t>(vertices.size());
}

void
W3SurfaceTerrain::render_cached_mesh_normals(VertexSpan vertices)
{
    constexpr float kNormalScaleFactor = 10.0;
    for(const auto& vertex : vertices) {
        surface_tool_->add_vertex(vertex.pos);
        surface_tool_->add_vertex(vertex.pos + vertex.norm * kNormalScaleFactor);
    }
    vertices_counter_ += static_cast<int64_t>(vertices.size());
}

}  // namespace w3terr