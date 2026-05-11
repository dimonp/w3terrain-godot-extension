#include "w3surfaceterrain.h"

#include <godot_cpp/classes/performance.hpp>

#include "w3mapnode.h"
#include "w3mapcollector_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapsection.h"

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
        if (ground_material_asset_.is_valid()) {
            ground_material_asset_->set_shader_parameter("texture_array", nullptr);
        }
        if (geo_material_asset_.is_valid()) {
            geo_material_asset_->set_shader_parameter("texture_array", nullptr);
        }
        break;
    }
    case NOTIFICATION_EDITOR_POST_SAVE: {
        if (ground_material_asset_.is_valid()) {
            ground_material_asset_->set_shader_parameter("texture_array", ground_textures_array_);
        }
        if (geo_material_asset_.is_valid()) {
            geo_material_asset_->set_shader_parameter("texture_array", geo_textures_array_);
        }
        break;
    }
    default:
        break;
    }
}

void
W3SurfaceTerrain::_enter_tree()
{
    W3Surface::_enter_tree();

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
    const auto stat_ground_tiles_precached_callable = callable_mp(this, &W3SurfaceTerrain::get_stat_ground_tiles_rendered);
    perf->add_custom_monitor(kStatGroundTilesRenderedId, stat_ground_tiles_precached_callable);
    const auto stat_geo_tiles_precached_callable = callable_mp(this, &W3SurfaceTerrain::get_stat_geo_tiles_rendered);
    perf->add_custom_monitor(kStatGeoTilesRenderedId, stat_geo_tiles_precached_callable);
#endif
}

void
W3SurfaceTerrain::_exit_tree()
{
#ifdef W3MAP_STATS_ENABLE
    godot::Performance *perf = godot::Performance::get_singleton();
    if (perf->has_custom_monitor(kStatGroundTilesRenderedId)) {
        perf->remove_custom_monitor(kStatGroundTilesRenderedId);
    }
    if (perf->has_custom_monitor(kStatGeoTilesRenderedId)) {
        perf->remove_custom_monitor(kStatGeoTilesRenderedId);
    }
#endif
    clear_rendered();
}

void
W3SurfaceTerrain::_process(double  /*delta*/)
{
    if (get_map_node() == nullptr) {
        return;
    }

    const auto* collector = get_collector();
    if (collector == nullptr) {
        return;
    }

    if (ground_assets_dirty_flag_) {
        load_ground_materials();
        clear_rendered();
        ground_assets_dirty_flag_ = false;
    }

    if (geo_assets_dirty_flag_) {
        load_geo_materials();
        clear_rendered();
        geo_assets_dirty_flag_ = false;
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
W3SurfaceTerrain::render(const W3Array<uint32_t>& sections)
{
    const auto* section_manager = get_section_manager();

#ifdef W3MAP_STATS_ENABLE
    stat_ground_tiles_rendered_ = 0;
    stat_geo_tiles_rendered_ = 0;
#endif

    for(const uint32_t section_id : sections) {
        const W3MapSection& section = section_manager->get_section_by_id(section_id);

        render_section_ground(section_id);
        render_section_geo(section_id);

        // Rendering normals for debugging purposes
        if (render_normals_) {
        }
    }
}

}  // namespace w3terr