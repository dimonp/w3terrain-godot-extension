#ifndef _W3MAPSURFACE__H
#define _W3MAPSURFACE__H

#include <godot_cpp/classes/visual_instance3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

#include "w3defs.h"

namespace w3terr {

#include <array>
#include <cstdint>
#include <type_traits>
#include <cstring>
#include <optional>

template <typename K, size_t Capacity>
class LRUSetFast {
    static_assert(std::is_integral_v<K>, "Key must be an integral type");
    static_assert(Capacity <= 256, "Capacity out of optimized range");
    static_assert(std::is_trivially_copyable_v<K>, "Key must be trivially copyable for memmove");

    std::array<K, Capacity> data_;
    size_t current_size_ = 0;

public:
    void clear() {
        current_size_ = 0;
    }

    // Check if key exists without updating its LRU position
    bool has(K key) const {
        for (size_t i = 0; i < current_size_; ++i) {
            if (data_[i] == key) { return true; }
        }
        return false;
    }

    // Check if key exists and move it to the front (MRU position)
    bool touch(K key) {
        for (size_t i = 0; i < current_size_; ++i) {
            if (data_[i] == key) {
                if (i > 0) {
                    K found = data_[i];
                    std::memmove(&data_[1], data_.data(), i * sizeof(K));
                    data_[0] = found;
                }
                return true;
            }
        }
        return false;
    }

    // Insert key. If key exists, move to front. Returns evicted key if capacity was reached.
    std::optional<K> put(K key) {
        for (size_t i = 0; i < current_size_; ++i) {
            if (data_[i] == key) {
                if (i > 0) {
                    std::memmove(&data_[1], data_.data(), i * sizeof(K));
                    data_[0] = key;
                }
                return std::nullopt;
            }
        }

        std::optional<K> evicted = std::nullopt;
        if (current_size_ == Capacity) {
            evicted = data_[Capacity - 1];
        } else {
            current_size_++;
        }

        if (current_size_ > 1) {
            std::memmove(&data_[1], data_.data(), (current_size_ - 1) * sizeof(K));
        }
        data_[0] = key;
        return evicted;
    }

    // Remove key and collapse the gap
    bool remove(K key) {
        for (size_t i = 0; i < current_size_; ++i) {
            if (data_[i] == key) {
                if (i < current_size_ - 1) {
                    std::memmove(&data_[i], &data_[i + 1], (current_size_ - i - 1) * sizeof(K));
                }
                current_size_--;
                return true;
            }
        }
        return false;
    }

    auto begin() { return data_.begin(); }
    auto end() { return data_.begin() + current_size_; }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.begin() + current_size_; }

    size_t size() const { return current_size_; }
    bool empty() const { return current_size_ == 0; }
};

class W3MapAssets;
class W3MapRuntimeManagerImpl;
class W3MapSectionManagerImpl;
class W3MapCollectorImpl;
class W3MapNode;

#define RS godot::RenderingServer::get_singleton()

class W3Surface: public godot::VisualInstance3D {
    GDCLASS(W3Surface, VisualInstance3D)
public:
    void _notification(int p_what);

    godot::AABB _get_aabb() const override;
    void _enter_tree() override;

protected:
    static constexpr int32_t kMaxGPUMeshes = std::min(128, godot::RenderingServer::MAX_MESH_SURFACES);

    static void _bind_methods();

    bool get_render_debug() const;
    void set_render_debug(bool flag);

    W3Ref<W3Marerial> get_debug_material() const;
    void set_debug_material(const W3Ref<W3Marerial>& material);

    bool is_mesh_dirty() const;

    void begin_render(uint32_t section_id, bool render_lines = false);
    void end_render(uint32_t section_id);

    void clear_rendered();

    const W3MapAssets* get_assets() const;
    const W3MapRuntimeManagerImpl* get_runtime_manager() const;
    const W3MapSectionManagerImpl* get_section_manager() const;
    const W3MapCollectorImpl* get_collector() const;

    W3MapNode* get_map_node() const;
    void set_map_node(W3MapNode* map_node);

    LRUSetFast<uint32_t, kMaxGPUMeshes> rendered_sections_;

    W3Ref<godot::SurfaceTool> surface_tool_;
    int64_t vertices_counter_;

    bool render_debug_ = false;
    W3Ref<W3Marerial> debug_material_;

private:
    W3MapNode* map_node_ptr_ = nullptr;
};

}  // namespace w3terr

#endif // _W3MAP_SURFACE__H
