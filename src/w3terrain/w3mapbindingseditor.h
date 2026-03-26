#ifndef _W3MAPBINDINGS_EDITOR__H
#define _W3MAPBINDINGS_EDITOR__H

#include <godot_cpp/classes/object.hpp>

namespace w3terr {

class W3MapNode;
class W3MapRuntimeManagerImpl;

class W3MapBindingsEditor final: public godot::Object {
    GDCLASS(W3MapBindingsEditor, godot::Object)
public:
    W3MapBindingsEditor() = default;
    explicit W3MapBindingsEditor(W3MapNode* map_node_ptr): map_node_(map_node_ptr) { }

protected:
    static void _bind_methods();

    void create_map(const godot::Vector2i& size) const;

    bool check_cellpoint_layer(const godot::Vector2i& coords, uint8_t base_layer) const;
    void increase_cellpoint_layer(const godot::Vector2i& coords) const;
    void decrease_cellpoint_layer(const godot::Vector2i& coords) const;
    void set_cellpoint_ground_height(const godot::Vector2i& coords, float height) const;
    void set_cellpoint_water_height(const godot::Vector2i& coords, float height) const;
    void set_cellpoint_ground_tileset(const godot::Vector2i& coords, uint8_t tileset, uint8_t variation = 0) const;
    void set_cellpoint_geo_tileset(const godot::Vector2i& coords, uint8_t tileset) const;
    void set_cellpoint_geo_variation(const godot::Vector2i& coords, uint8_t variation) const;
    void set_cellpoint_water(const godot::Vector2i& coords, bool flag) const;
    void set_cellpoint_ramp(const godot::Vector2i& coords, bool flag) const;

private:
    auto w3e_map() const;

    W3MapNode* map_node_ = nullptr;
};

}  // namespace w3terr

#endif // _W3MAPBINDINGS_EDITOR__H
