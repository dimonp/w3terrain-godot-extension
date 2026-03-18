#include <godot_cpp/classes/world3d.hpp>

#include "w3mapruntimemanager_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapnode.h"
#include "w3surface.h"

namespace w3terr {

#define RS godot::RenderingServer::get_singleton()

void
W3Surface::_notification(int p_what)
{
    switch (p_what) {
    case NOTIFICATION_READY: {
        mesh_.instantiate();
        instance_rid_ = RS->instance_create();
        const godot::RID scenario_rid = get_world_3d()->get_scenario();
        RS->instance_set_scenario(instance_rid_, scenario_rid);
        RS->instance_set_base(instance_rid_, mesh_->get_rid());

        if (map_node_ptr_ == nullptr) {
            map_node_ptr_ = godot::Object::cast_to<W3MapNode>(get_parent());
        }
        break;
    }
    case NOTIFICATION_TRANSFORM_CHANGED: {
        if (instance_rid_.is_valid()) {
            RS->instance_set_transform(instance_rid_, get_global_transform());
        }
        break;
    }
    case NOTIFICATION_PREDELETE: {
        if (instance_rid_.is_valid()) {
            mesh_.unref();
            RS->free_rid(instance_rid_);
        }
        break;
    }
    case NOTIFICATION_VISIBILITY_CHANGED: {
        if (instance_rid_.is_valid()) {
            bool visible = is_visible_in_tree();
            RS->instance_set_visible(instance_rid_, visible);
        }
        break;
    }
    default:
        break;
    }
}

void
W3Surface::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("begin_render", "render_lines"), &W3Surface::begin_render, DEFVAL(false));
    godot::ClassDB::bind_method(godot::D_METHOD("end_render"), &W3Surface::end_render);

    godot::ClassDB::bind_method(godot::D_METHOD("get_mesh"), &W3Surface::get_mesh);
    ADD_PROPERTY(godot::PropertyInfo(
            godot::Variant::OBJECT, "_mesh",
            godot::PROPERTY_HINT_RESOURCE_TYPE, "ArrayMesh",
            godot::PROPERTY_USAGE_INTERNAL),
        "", "get_mesh");

    godot::ClassDB::bind_method(godot::D_METHOD("get_surface_tool"), &W3Surface::get_surface_tool);
    ADD_PROPERTY(godot::PropertyInfo(
            godot::Variant::OBJECT, "_surface_tool",
            godot::PROPERTY_HINT_RESOURCE_TYPE, "SurfaceTool",
            godot::PROPERTY_USAGE_INTERNAL),
        "", "get_surface_tool");

    godot::ClassDB::bind_method(godot::D_METHOD("get_map_node"), &W3Surface::get_map_node);
    godot::ClassDB::bind_method(godot::D_METHOD("set_map_node", "map_node"), &W3Surface::set_map_node);
    ADD_PROPERTY(godot::PropertyInfo(
            godot::Variant::OBJECT, "map_node",
            godot::PROPERTY_HINT_NODE_TYPE, "W3MapNode"),
        "set_map_node", "get_map_node");

}

W3Ref<W3Mesh>
W3Surface::get_mesh() const {
	return mesh_;
}

void
W3Surface::end_render()
{
    surface_tool_->commit(mesh_);
}

void
W3Surface::begin_render(bool render_lines)
{
    if (surface_tool_.is_null()) {
        surface_tool_.instantiate();
    }

    surface_tool_->clear();
    surface_tool_->begin(render_lines ? W3Mesh::PRIMITIVE_LINES: W3Mesh::PRIMITIVE_TRIANGLES);
    surface_tool_->set_custom_format(0, godot::SurfaceTool::CUSTOM_R_FLOAT);
    vertices_counter_ = 0;
}

W3Ref<godot::SurfaceTool>
W3Surface::get_surface_tool() const
{
    return surface_tool_;
}

bool
W3Surface::is_mesh_dirty() const
{
    return get_assets()->is_assets_dirty() ||
        get_runtime_manager() == nullptr ||
        get_runtime_manager()->is_dirty();
}

const
W3MapAssets*
W3Surface::get_assets() const
{
    return map_node_ptr_->get_assets();
}

const
W3MapRuntimeManagerImpl*
W3Surface::get_runtime_manager() const
{
    return map_node_ptr_->get_runtime_manager();
}

const
W3MapSectionManagerImpl*
W3Surface::get_section_manager() const
{
    return map_node_ptr_->get_section_manager();
}

const
W3MapCollectorImpl*
W3Surface::get_collector() const
{
    return map_node_ptr_->get_collector();
}

W3MapNode*
W3Surface::get_map_node() const
{
    return map_node_ptr_;
}

void
W3Surface::set_map_node(W3MapNode* map_node)
{
    map_node_ptr_ = map_node;
}

}  // namespace w3terr