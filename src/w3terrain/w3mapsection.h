#ifndef _W3MAPSECTION__H
#define _W3MAPSECTION__H

#include <optional>
#include <bitset>
#include <span>

#include "w3defs.h"
#include "w3math.h"

namespace w3terr {

class W3MapRuntimeManager;

class W3_API W3MapSection {
public:
    static constexpr int32_t kCellsDimension = 4;
    static constexpr int32_t kNumberOfCells = kCellsDimension * kCellsDimension;

    explicit W3MapSection(const W3MapRuntimeManager* map_runtime): runtime_manager_(map_runtime) {}
    void initialize(size_t ground_tilesets_size, size_t geo_tilesets_size, const Coord2D& origin_2d, const math::bbox3& bbox);

    struct CachedMesh {
        CachedMesh() noexcept;
        ~CachedMesh() noexcept;

        template<typename VT, typename IT>
        W3Pair<std::span<const VT>, std::span<const IT>> get_cached_mesh_data() const noexcept
        {
            w3_assert(vertices_count_ > 0 && indices_count_ > 0);
            const void* cached_buffer = get_cached_buffer();
            if (cached_buffer == nullptr) { return {}; }
            return {
                { static_cast<const VT*>(cached_buffer), vertices_count_ },
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
                { reinterpret_cast<const IT*>(static_cast<const char*>(cached_buffer) + (vertices_count_ * sizeof(VT))), indices_count_ }
            };
        }

        template<typename VT, typename IT>
        W3Pair<std::span<VT>, std::span<IT>> allocate_mesh_data() const noexcept
        {
            w3_assert(vertices_count_ > 0 && indices_count_ > 0);
            const size_t alloc_size = (vertices_count_ * sizeof(VT)) + (indices_count_ * sizeof(IT));
            void* cached_buffer = alloc_cached_buffer(alloc_size);
            if (cached_buffer == nullptr) { return {}; }
            return {
                { static_cast<VT*>(cached_buffer), vertices_count_},
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
                { reinterpret_cast<IT*>(static_cast<char*>(cached_buffer) + (vertices_count_ * sizeof(VT))), indices_count_}
            };
        }

        bool is_used() const { return usage_flags_.any(); }
        bool is_used_by(size_t flag) const { return usage_flags_[flag]; }

        uint32_t vertices_count() const { return vertices_count_; }
        uint32_t indices_count() const { return indices_count_; }

    private:
        const void* get_cached_buffer() const;
        void* alloc_cached_buffer(size_t alloc_size) const;
        auto* get_cache_handle() const;

        uint32_t vertices_count_ = 0;
        uint32_t indices_count_ = 0;
        std::bitset<kNumberOfCells> usage_flags_;

        // fast pimpl storage for lru cache handle
        static constexpr std::size_t kPimplSize = sizeof(void*);
        static constexpr std::size_t kPimplAlign = alignof(void*);
        alignas(kPimplAlign) mutable std::array<std::byte, kPimplSize> storage_ = {};

        friend class W3MapSection;
    };

    bool refresh();

    uint32_t get_ground_vertices_count(size_t tileset_id) const;
    uint32_t get_geo_vertices_count(size_t tileset_id) const;
    uint32_t get_water_vertices_count() const;

    size_t get_ground_tilesets_size() const;
    size_t get_geo_tilesets_size() const;

    const CachedMesh& get_cached_ground_mesh(size_t tileset_id) const;
    const CachedMesh& get_cached_geo_mesh(size_t tileset_id) const;
    const CachedMesh& get_cached_waters_mesh() const;

    const Coord2D& get_origin_2d() const;
    const math::bbox3& get_bbox() const;

    uint32_t map_ground_tileset_to_layer(size_t tileset_id) const;
    Coord2D calc_cell_coord_from_idx(size_t cell_idx) const;

    std::optional<Coord2D> find_intersected_cell(const math::line3 &line, math::vector3& ipoint) const;

    void update_all_cells();
    void free_cached_data();
    void update_cell(size_t cell_idx);

    void set_dirty(bool flag = true);
    bool is_dirty() const;

private:
    Coord2D origin_2d_ = { 0, 0 };

    W3Array<CachedMesh> ground_cached_meshes_;
    W3Array<CachedMesh> geo_cached_meshes_;
    CachedMesh water_mesh_;

    W3Array<uint32_t> ground_tileset_to_layer_map_;

    math::bbox3 bbox_;
    bool dirty_ = true;

    const W3MapRuntimeManager* runtime_manager_;
};

inline
const math::bbox3&
W3MapSection::get_bbox() const
{
    return bbox_;
}

inline
const Coord2D&
W3MapSection::get_origin_2d() const
{
    return origin_2d_;
}

inline
Coord2D
W3MapSection::calc_cell_coord_from_idx(size_t cell_idx) const
{
    return {
        get_origin_2d().x + static_cast<int32_t>(cell_idx % kCellsDimension),
        get_origin_2d().y + static_cast<int32_t>(cell_idx / kCellsDimension)
    };
}

inline
uint32_t
W3MapSection::get_ground_vertices_count(size_t tileset_id) const
{
    w3_assert(tileset_id < get_ground_tilesets_size());
    return ground_cached_meshes_[tileset_id].vertices_count_;
}

inline
uint32_t
W3MapSection::get_geo_vertices_count(size_t tileset_id) const
{
    w3_assert(tileset_id < get_ground_tilesets_size());
    return geo_cached_meshes_[tileset_id].vertices_count_;
}

inline
uint32_t
W3MapSection::get_water_vertices_count() const
{
    return water_mesh_.vertices_count_;
}

inline
size_t
W3MapSection::get_ground_tilesets_size() const
{
    return ground_cached_meshes_.size();
}

inline
size_t
W3MapSection::get_geo_tilesets_size() const
{
    return geo_cached_meshes_.size();
}

inline
const W3MapSection::CachedMesh&
W3MapSection::get_cached_ground_mesh(size_t tileset_id) const
{
    return ground_cached_meshes_[tileset_id];
}

inline
const W3MapSection::CachedMesh&
W3MapSection::get_cached_geo_mesh(size_t tileset_id) const
{
    return geo_cached_meshes_[tileset_id];
}

inline
const W3MapSection::CachedMesh&
W3MapSection::get_cached_waters_mesh() const
{
    return water_mesh_;
}

inline
uint32_t
W3MapSection::map_ground_tileset_to_layer(size_t tileset_id) const
{
    return ground_tileset_to_layer_map_[tileset_id];
}

inline
void
W3MapSection::set_dirty(bool flag)
{
    dirty_ = flag;
}

inline
bool
W3MapSection::is_dirty() const
{
    return dirty_;
}

}  // namespace w3terr

#endif  /// _W3MAPSECTION__H

