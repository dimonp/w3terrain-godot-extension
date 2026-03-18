#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/performance.hpp>

#include "w3mapinformator_impl.h"
#include "w3mapruntimemanager_impl.h"
#include "w3mapcollector_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapnode.h"
#include "w3mapbindings.h"

namespace w3terr {

void
W3MapNode::_bind_methods()
{
    ADD_SIGNAL(godot::MethodInfo(kSignalMapInitialized, godot::PropertyInfo(godot::Variant::OBJECT, "map_node", godot::PROPERTY_HINT_NODE_TYPE, "W3MapNode")));
    ADD_SIGNAL(godot::MethodInfo(kSignalMapDestroyed, godot::PropertyInfo(godot::Variant::OBJECT, "map_node", godot::PROPERTY_HINT_NODE_TYPE, "W3MapNode")));
    ADD_SIGNAL(godot::MethodInfo(kSignalMapAssetsChanged));
    ADD_SIGNAL(godot::MethodInfo(kSignalGroundAssetsChanged));
    ADD_SIGNAL(godot::MethodInfo(kSignalGeoAssetsChanged));

    godot::ClassDB::bind_method(godot::D_METHOD("get_map"), &W3MapNode::get_bindings);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "map",
            godot::PROPERTY_HINT_RESOURCE_TYPE,
            "W3MapBindings",
            godot::PROPERTY_USAGE_INTERNAL),
        "", "get_map");

    godot::ClassDB::bind_method(godot::D_METHOD("get_editor"), &W3MapNode::get_bindings_editor);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "editor",
            godot::PROPERTY_HINT_RESOURCE_TYPE,
            "W3MapBindingsEditor",
            godot::PROPERTY_USAGE_INTERNAL),
        "", "get_editor");

    godot::ClassDB::bind_method(godot::D_METHOD("get_w3e_map"), &W3MapNode::get_w3e_resource);
    godot::ClassDB::bind_method(godot::D_METHOD("set_w3e_map", "map"), &W3MapNode::set_w3e_resource);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "w3e_map",
            godot::PROPERTY_HINT_RESOURCE_TYPE,
            "W3eResource"),
        "set_w3e_map", "get_w3e_map");

    ADD_GROUP("Ground", "ground_");
    godot::ClassDB::bind_method(godot::D_METHOD("get_ground_textures"), &W3MapNode::get_ground_textures);
    godot::ClassDB::bind_method(godot::D_METHOD("set_ground_textures", "ground_textures"), &W3MapNode::set_ground_textures);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "ground_textures",
            godot::PROPERTY_HINT_ARRAY_TYPE,
            W3String::num(godot::Variant::OBJECT) + "/" + W3String::num(godot::PROPERTY_HINT_RESOURCE_TYPE) + ":Texture2D"),
        "set_ground_textures", "get_ground_textures");


    ADD_GROUP("Geo", "geo_");
    godot::ClassDB::bind_method(godot::D_METHOD("get_geo_resources"), &W3MapNode::get_geo_resources);
    godot::ClassDB::bind_method(godot::D_METHOD("set_geo_resources", "geo_resources"), &W3MapNode::set_geo_resources);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "geo_resources",
            godot::PROPERTY_HINT_ARRAY_TYPE,
            W3String::num(godot::Variant::OBJECT) + "/" + W3String::num(godot::PROPERTY_HINT_RESOURCE_TYPE) + ":W3GeoResource"),
        "set_geo_resources", "get_geo_resources");

    godot::ClassDB::bind_method(godot::D_METHOD("get_camera"), &W3MapNode::get_camera);
    godot::ClassDB::bind_method(godot::D_METHOD("set_camera", "p_camera"), &W3MapNode::set_camera);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "camera",
            godot::PROPERTY_HINT_NODE_TYPE,
            "Camera3D"
        ),
        "set_camera", "get_camera"
    );
}

W3MapNode::W3MapNode()
    : informator_(new W3MapInformatorImpl(this))
{}

W3MapNode::~W3MapNode() = default;

godot::AABB
W3MapNode::_get_aabb() const
{
    if (!collector_) {
        return {};
    }
    return collector_->get_bbox();
}

void
W3MapNode::on_map_resource_changed()
{
    w3_log_debug("on_map_resource_changed");
    reset_runtime();
}

void
W3MapNode::on_ground_assets_changed()
{
    w3_log_debug("W3MapNode::on_ground_assets_changed");
    assets_dirty_flag_ = true;
}

void
W3MapNode::on_geo_assets_changed()
{
    w3_log_debug("W3MapNode::on_geo_assets_changed");
    if (runtime_manager_) {
        runtime_manager_->update_all_cells_rt();
    }
    if (sections_manager_) {
        sections_manager_->set_dirty_all();
    }
    assets_dirty_flag_ = true;
}

void
W3MapNode::on_geo_resource_changed()
{
    w3_log_debug("W3MapNode::on_geo_resource_changed");
    prepare_geo_assets_rt();
    emit_signal(kSignalGeoAssetsChanged);
}

void
W3MapNode::_enter_tree() {
    const auto callable_map_resource_changed = callable_mp(this, &W3MapNode::on_map_resource_changed);
    if (!is_connected(kSignalMapAssetsChanged, callable_map_resource_changed)) {
        connect(kSignalMapAssetsChanged, callable_map_resource_changed);
    }

    const auto callable_ground_assets_changed = callable_mp(this, &W3MapNode::on_ground_assets_changed);
    if (!is_connected(kSignalGroundAssetsChanged, callable_ground_assets_changed)) {
        connect(kSignalGroundAssetsChanged, callable_ground_assets_changed);
    }

    const auto callable_geo_assets_changed = callable_mp(this, &W3MapNode::on_geo_assets_changed);
    if (!is_connected(kSignalGeoAssetsChanged, callable_geo_assets_changed)) {
        connect(kSignalGeoAssetsChanged, callable_geo_assets_changed);
    }

    if (!map_bindings_) {
        map_bindings_.reset(memnew(W3MapBindings(this)));
    }

    if (!map_bindings_editor_) {
        map_bindings_editor_.reset(memnew(W3MapBindingsEditor(this)));
    }

    const godot::Camera3D* p_camera = get_camera();
    if (p_camera != nullptr) {
        w3_log_info("Camera: %s", p_camera->get_name());
    } else {
        w3_log_info("Camera is not set.");
    }

#ifdef W3MAP_STATS_ENABLE
    godot::Performance *perf = godot::Performance::get_singleton();
    const auto cache_allocation_size_callable = callable_mp_static(&W3MapNode::get_cache_allocation_size);
    perf->add_custom_monitor(kStatCacheAllocationSizeId, cache_allocation_size_callable);
#endif
}

void
W3MapNode::_exit_tree()
{
#ifdef W3MAP_STATS_ENABLE
    godot::Performance *perf = godot::Performance::get_singleton();
    if (perf->has_custom_monitor(kStatCacheAllocationSizeId)) {
        perf->remove_custom_monitor(kStatCacheAllocationSizeId);
    }
#endif
}

void 
W3MapNode::_ready() {
    auto* rs = godot::RenderingServer::get_singleton();

    const auto callable_frame_post_draw = callable_mp(this, &W3MapNode::on_frame_rendered);
    if (!rs->is_connected("frame_post_draw", callable_frame_post_draw)) {
        rs->connect("frame_post_draw", callable_frame_post_draw);
    }
}

void
W3MapNode::_process(double  /*delta*/)
{
    if (!is_map_loaded()) {
        return;
    }
    if (!is_camera_valid()) {
        return;
    }
    if (assets_dirty_flag_ && !refresh_runtime()) {
        return;
    }
    collect_visible_sections();
}

void
W3MapNode::on_frame_rendered()
{
    assets_dirty_flag_ = false;
    if (runtime_manager_) {
        runtime_manager_->set_dirty(false);
    }
}

W3Ref<W3eResource>
W3MapNode::get_w3e_resource() const
{
    return map_w3e_;
}

void
W3MapNode::set_w3e_resource(const W3Ref<W3eResource>& map)
{
    map_w3e_ = map;
    emit_signal(kSignalMapAssetsChanged);
}

godot::TypedArray<W3Texture>
W3MapNode::get_ground_textures() const
{
    return get_ground_assets();
}

void
W3MapNode::set_ground_textures(const godot::TypedArray<W3Texture>& textures)
{
    set_ground_assets(textures);
    emit_signal(kSignalGroundAssetsChanged);
}

godot::TypedArray<W3GeoResource>
W3MapNode::get_geo_resources() const
{
    return geo_assets_;
}

void
W3MapNode::set_geo_resources(const godot::TypedArray<W3GeoResource>& resources)
{
    static constexpr auto kGeoResourceChanged = "changed";
    const auto callable_geo_resource_changed = callable_mp(this, &W3MapNode::on_geo_resource_changed);

    // disconnect before update
    for (const auto& geo_resource : geo_assets_) {
        W3Ref<W3GeoResource> resource = geo_resource;
        if (resource.is_valid() && resource->is_connected(kGeoResourceChanged, callable_geo_resource_changed)) {
            resource->disconnect(kGeoResourceChanged, callable_geo_resource_changed);
        }
    }

    geo_assets_ = resources;

    // reconnect after update
    for (const auto& geo_resource : geo_assets_) {
        W3Ref<W3GeoResource> resource = geo_resource;
        if (resource.is_valid() && !resource->is_connected(kGeoResourceChanged, callable_geo_resource_changed)) {
            resource->connect(kGeoResourceChanged, callable_geo_resource_changed);
        }
    }
    prepare_geo_assets_rt();
    emit_signal(kSignalGeoAssetsChanged);
}

godot::Camera3D*
W3MapNode::get_camera() const {
    return Object::cast_to<godot::Camera3D>(godot::ObjectDB::get_instance(camera_id_));
}

void
W3MapNode::set_camera(godot::Camera3D* p_camera) {
    if (p_camera != nullptr) {
        camera_id_ = p_camera->get_instance_id();
    } else {
        camera_id_ = godot::ObjectID();
    }
}

uint64_t
W3MapNode::get_cache_allocation_size() {
    return W3MapSectionManagerImpl::get_cache_allocation_size();
}


}  // namespace w3terr