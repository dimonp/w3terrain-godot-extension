#ifndef _W3MAPBINDINGS_EDITOR__H
#define _W3MAPBINDINGS_EDITOR__H

#include <godot_cpp/classes/ref_counted.hpp>

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
    void increase_cellpoint_height_layer(const godot::Vector2i& coords) const;
    void decrease_cellpoint_height_layer(const godot::Vector2i& coords) const;
    void set_cellpoint_ground_height(const godot::Vector2i& coords, float height) const;
private:
    auto w3e_map() const;

    W3MapNode* map_node_ = nullptr;
};

}  // namespace w3terr

#endif // _W3MAPBINDINGS_EDITOR__H
