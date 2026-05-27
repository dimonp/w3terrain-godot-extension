#include "w3surface.h"

#include <godot_cpp/classes/world3d.hpp>

#include "w3mapruntimemanager_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapsectionrenderedcache.h"
#include "w3mapnode.h"

namespace w3terr {

void
W3Surface::_notification(int p_what)
{
    switch (p_what) {
    case NOTIFICATION_TRANSFORM_CHANGED:
    case NOTIFICATION_VISIBILITY_CHANGED: {
        clear_rendered();
        break;
    }
    default:
        break;
    }
}

void
W3Surface::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_node"), &W3Surface::get_map_node);
    godot::ClassDB::bind_method(godot::D_METHOD("set_map_node", "map_node"), &W3Surface::set_map_node);
    ADD_PROPERTY(godot::PropertyInfo(
            godot::Variant::OBJECT, "map_node",
            godot::PROPERTY_HINT_NODE_TYPE, "W3MapNode"),
        "set_map_node", "get_map_node");

    godot::ClassDB::bind_method(godot::D_METHOD("get_debug_material"), &W3Surface::get_debug_material);
    godot::ClassDB::bind_method(godot::D_METHOD("set_debug_material", "debug_material"), &W3Surface::set_debug_material);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "debug_material",
            godot::PROPERTY_HINT_RESOURCE_TYPE,
            "Material"),
        "set_debug_material", "get_debug_material");

    godot::ClassDB::bind_method(godot::D_METHOD("get_render_debug"), &W3Surface::get_render_debug);
    godot::ClassDB::bind_method(godot::D_METHOD("set_render_debug", "p_camera"), &W3Surface::set_render_debug);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "render_debug"),
        "set_render_debug", "get_render_debug"
    );
}

godot::AABB
W3Surface::_get_aabb() const
{
    return map_node_ptr_->get_aabb();
}

void
W3Surface::_enter_tree() {
    if (map_node_ptr_ == nullptr) {
        map_node_ptr_ = Object::cast_to<W3MapNode>(get_parent());
    }
}

bool
W3Surface::get_render_debug() const
{
    return render_debug_;
}

void
W3Surface::set_render_debug(bool flag)
{
    clear_rendered();
    render_debug_ = flag;
}

W3Ref<W3Marerial>
W3Surface::get_debug_material() const
{
    return debug_material_;
}

void
W3Surface::set_debug_material(const W3Ref<W3Marerial>& material)
{
    debug_material_ = material;
}

int8_t
W3Surface::begin_render(const uint32_t section_id, bool render_lines)
{
    if (surface_tool_.is_null()) {
        surface_tool_.instantiate();
    }

    surface_tool_->clear();
    surface_tool_->begin(render_lines ? W3Mesh::PRIMITIVE_LINES: W3Mesh::PRIMITIVE_TRIANGLES);
    surface_tool_->set_custom_format(0, godot::SurfaceTool::CUSTOM_R_FLOAT);

    vertices_counter_ = 0;

    auto* rendered_sections_cache = get_rendered_sections_cache();
    const auto* section = rendered_sections_cache->get_or_create(section_id);
    return static_cast<int8_t>(RS->mesh_get_surface_count(section->get_mesh_rid()));
}

RenderedSection*
W3Surface::end_render(const uint32_t section_id)
{
    auto mesh_array =  surface_tool_->commit_to_arrays();

    static const uint64_t kCustom0Type = godot::Mesh::ARRAY_CUSTOM_R_FLOAT;
    static const uint64_t kFormat = godot::Mesh::ARRAY_FORMAT_VERTEX |
        godot::Mesh::ARRAY_FORMAT_TEX_UV |
        godot::Mesh::ARRAY_FORMAT_NORMAL |
        godot::Mesh::ARRAY_FORMAT_CUSTOM0 |
        godot::Mesh::ARRAY_FORMAT_INDEX |
        (kCustom0Type << godot::Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT);

    auto* rendered_sections_cache = get_rendered_sections_cache();
    auto* mesh = rendered_sections_cache->get_rendered(section_id);
    assert(mesh != nullptr);

    const auto& mesh_rid = mesh->get_mesh_rid();
    RS->mesh_add_surface_from_arrays(
        mesh_rid,
        godot::RenderingServer::PRIMITIVE_TRIANGLES,
        mesh_array,
        godot::Array(),
        godot::Dictionary(),
        static_cast<int64_t>(kFormat)
    );

    const auto& instance_rid = mesh->get_instance_rid();
    RS->instance_set_base(instance_rid, mesh_rid);
    RS->instance_set_transform(instance_rid, get_global_transform());
    RS->instance_set_scenario(instance_rid, get_world_3d()->get_scenario());
    RS->instance_set_layer_mask(instance_rid, get_layer_mask());

    return mesh;
}

void
W3Surface::clear_rendered()
{
    if (map_node_ptr_ == nullptr) {
        return;
    }

    auto* rendered_sections_cache = get_rendered_sections_cache();
    if (rendered_sections_cache != nullptr) {
        rendered_sections_cache->clear_all();
    }
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

W3SectionRenderedCache*
W3Surface::get_rendered_sections_cache() const
{
    return map_node_ptr_->get_rendered_sections_cache();
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