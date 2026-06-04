#ifndef _W3MAPSECTIONRENDEREDCACHE__H
#define _W3MAPSECTIONRENDEREDCACHE__H


#include <array>
#include <optional>
#include <algorithm>

#include <godot_cpp/classes/rendering_server.hpp>
#include <utility>

#include "w3defs.h"

namespace w3terr {

#define RS godot::RenderingServer::get_singleton()

template <typename K, typename V, size_t Capacity>
class LRUHashFast {
    static_assert(std::is_integral_v<K>, "Key must be an integral type");
    static_assert(Capacity <= 256, "Capacity out of optimized range");

    // Stores keys in LRU order: [0] is Most Recently Used (MRU), [size-1] is Least Recently Used (LRU)
    std::array<K, Capacity> keys_priority_;
    // Fast value lookup using a contiguous sorted container
    W3FlatMap<K, V> values_;

public:
    LRUHashFast()
    {
        // Pre-allocate memory to avoid reallocations during runtime
#ifdef HAS_FLATMAP
        std::vector<K> key_cont;
        std::vector<V> value_cont;
        key_cont.reserve(Capacity);
        value_cont.reserve(Capacity);
        values_ = W3FlatMap<K, V>(std::sorted_unique, std::move(key_cont), std::move(value_cont));
#endif
        keys_priority_.fill({});
    }

    // Check if a key exists without updating its LRU position
    bool has(K key) const noexcept
    {
        return values_.contains(key);
    }

    // Check if a key exists and move it to the front (MRU position)
    bool touch(K key)
    {
        auto iter = values_.find(key);
        if (iter == values_.end()) { return false; }

        // Update key position in the priority array (Move to Front)
        for (size_t i = 0; i < values_.size(); ++i) {
            if (keys_priority_[i] == key) { // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                if (i > 0) {
                    // Shift elements to make room at the front
                    std::memmove(
                        &keys_priority_[1],
                        keys_priority_.data(),
                        i * sizeof(K));
                    keys_priority_[0] = key;
                }
                break;
            }
        }
        return true;
    }

    // Retrieve value and update its priority to MRU
    V* get(K key)
    {
        if (touch(key)) {
            return &(values_.find(key)->second);
        }
        return nullptr;
    }

    // Insert or update a key-value pair. Returns the evicted {key, value} if capacity is reached.
    std::optional<std::pair<K, V>> put(K key, V value)
    {
        // If key exists, update value and refresh priority
        if (auto iter = values_.find(key); iter != values_.end()) {
            iter->second = std::move(value);
            get(key);
            return std::nullopt;
        }

        std::optional<std::pair<K, V>> evicted = std::nullopt;

        // Handle eviction if at capacity
        if (values_.size() == Capacity) {
            K old_key = keys_priority_[Capacity - 1];
            auto iter = values_.find(old_key);
            if (iter != values_.end()) {
                evicted = std::make_pair(old_key, std::move(iter->second));
                values_.erase(iter);
            }
        }

        // Shift priority array and insert new key at the MRU position (index 0)
        size_t current_size = values_.size();
        if (current_size > 0) {
            std::memmove(
                &keys_priority_[1],
                keys_priority_.data(),
                std::min(current_size, Capacity - 1) * sizeof(K));
        }

        keys_priority_[0] = key;
        // insert_or_assign is efficient here due to reserve()
        values_.insert_or_assign(key, std::move(value));

        return evicted;
    }

    // Remove a key and its priority entry
    bool remove(K key)
    {
        auto iter = values_.find(key);
        if (iter == values_.end()) { return false; }

        size_t current_size = values_.size();
        values_.erase(iter);

        // Find the key in priority array and collapse the gap
        for (size_t i = 0; i < current_size; ++i) {
            if (keys_priority_[i] == key) {
                if (i < current_size - 1) {
                    std::memmove(
                        &keys_priority_[i],
                        &keys_priority_[i + 1],
                        (current_size - i - 1) * sizeof(K));
                }
                break;
            }
        }
        return true;
    }

    auto begin() noexcept { return values_.begin(); }
    auto end() noexcept { return values_.end(); }
    auto begin() const noexcept { return values_.begin(); }
    auto end() const noexcept { return values_.end(); }

    size_t size() const noexcept { return values_.size(); }
    bool empty() const noexcept { return values_.empty(); }
    void clear() noexcept { values_.clear(); }
};

class RenderedSection {
    godot::RID instance_rid_;
    godot::RID mesh_rid_;

public:
    int8_t surface_idx_ground = -1;
    int8_t surface_idx_geo = -1;
    int8_t surface_idx_water = -1;
    int8_t surface_idx_normal = -1;

    const auto& get_mesh_rid() const { return mesh_rid_; }
    const auto& get_instance_rid() const { return instance_rid_; }

    void free()
    {
        if (instance_rid_.is_valid()) {
            RS->free_rid(instance_rid_);
            instance_rid_ = {};
        }
        if (mesh_rid_.is_valid()) {
            RS->free_rid(mesh_rid_);
            mesh_rid_ = {};
        }
        surface_idx_ground = -1;
        surface_idx_geo = -1;
        surface_idx_water = -1;
        surface_idx_normal = -1;
    }

    friend class W3SectionRenderedCache;
};

class W3SectionRenderedCache {
    static constexpr int32_t kMaxGPUMeshes = std::min(256, godot::RenderingServer::MAX_MESH_SURFACES);

public:
    ~W3SectionRenderedCache()
    {
        clear_all();
    }

    bool has(uint32_t section_id) const
    {
        return rendered_meshes_.has(section_id);
    }

    bool touch(uint32_t section_id)
    {
        return rendered_meshes_.touch(section_id);
    }

    RenderedSection* get_rendered(uint32_t section_id)
    {
        return rendered_meshes_.get(section_id);
    }

    RenderedSection* get_or_create(uint32_t section_id)
    {
        auto* mesh = rendered_meshes_.get(section_id);
        if (mesh != nullptr) {
            return mesh;
        }

        RenderedSection rendered_mesh;
        rendered_mesh.mesh_rid_ = RS->mesh_create();
        rendered_mesh.instance_rid_ = RS->instance_create();

        auto evicted = rendered_meshes_.put(section_id, rendered_mesh);
        if (evicted.has_value()) {
            auto& [evicted_section_id, evicted_mesh] = evicted.value();
            evicted_mesh.free();
        }
        return rendered_meshes_.get(section_id);
    };

    void clear_all()
    {
        for (auto&& [section_id, mesh]  : rendered_meshes_) {
            mesh.free();
        }
        rendered_meshes_.clear();
    }

    size_t get_cached_count()
    {
        return rendered_meshes_.size();
    }

private:
    LRUHashFast<uint32_t, RenderedSection, kMaxGPUMeshes> rendered_meshes_;
};

} // namespace w3terr

#endif // _W3MAPSECTIONRENDEREDCACHE__H