#ifndef _W3MAPSECTION__H
#define _W3MAPSECTION__H

#include <bitset>
#include <span>

#include "w3defs.h"
#include "w3map.h"

#include <godot_cpp/classes/rendering_server.hpp>


namespace w3terr {

class W3MapRuntimeManager;

#define RS godot::RenderingServer::get_singleton()

class W3_API W3MapSection {
public:
    static constexpr int32_t kNumberOfCells = kSectionDimension * kSectionDimension;

    W3MapSection(uint8_t  ground_tilesets_size, uint8_t geo_tilesets_size);
    W3MapSection(const W3MapSection&) = delete;
    W3MapSection& operator=(const W3MapSection&) = delete;
    W3MapSection(W3MapSection&&) noexcept;
    W3MapSection& operator=(W3MapSection&&) noexcept;
    ~W3MapSection() noexcept;

    void update_all_cells(const Coord2D& section_origin, W3MapRuntimeManager* runtime);
    bool refresh(const Coord2D& section_origin, W3MapRuntimeManager* runtime);

    void set_dirty(bool flag = true);
    bool is_dirty() const;

    static Coord2D calc_cell_coord_from_idx(const Coord2D& section_origin, size_t cell_idx);

    using TilesetUsage = std::bitset<kNumberOfCells>;

    std::span<uint32_t> get_ground_tileset_to_layer_map() const;
    std::span<const TilesetUsage> get_ground_tileset_usage() const;
    std::span<const TilesetUsage> get_geo_tileset_usage() const;
    const TilesetUsage& get_water_usage() const;

private:
    void update_cell(const Coord2D& cell_coords, size_t cell_idx, W3MapRuntimeManager* runtime);

    TilesetUsage* ground_usage_ptr() const;
    TilesetUsage* geo_usage_ptr() const;
    TilesetUsage* water_usage_ptr() const;

    uint32_t* ground_tileset_to_layer_map_;
    TilesetUsage* tilesets_usage_;

    uint8_t ground_tilesets_size_;
    uint8_t geo_tilesets_size_;

    bool dirty_ = true;
};

inline
W3MapSection::TilesetUsage*
W3MapSection::ground_usage_ptr() const
{
    return tilesets_usage_;
}

inline
W3MapSection::TilesetUsage*
W3MapSection::geo_usage_ptr() const
{
    //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return tilesets_usage_ + ground_tilesets_size_;
}

inline
W3MapSection::TilesetUsage*
W3MapSection::water_usage_ptr() const
{
    //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return tilesets_usage_ + ground_tilesets_size_ + geo_tilesets_size_;
}

inline
std::span<uint32_t>
W3MapSection::get_ground_tileset_to_layer_map() const
{
    return {ground_tileset_to_layer_map_, ground_tilesets_size_};
}


inline
std::span<const W3MapSection::TilesetUsage>
W3MapSection::get_ground_tileset_usage() const
{
    return { ground_usage_ptr(), ground_tilesets_size_ };
}

inline
std::span<const W3MapSection::TilesetUsage>
W3MapSection::get_geo_tileset_usage() const
{
    return { geo_usage_ptr(), geo_tilesets_size_ };
}

inline
const W3MapSection::TilesetUsage&
W3MapSection::get_water_usage() const
{
    return *water_usage_ptr();
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

inline
Coord2D
W3MapSection::calc_cell_coord_from_idx(const Coord2D& section_origin, size_t cell_idx)
{
    return {
        section_origin.x + static_cast<int32_t>(cell_idx % kSectionDimension),
        section_origin.y + static_cast<int32_t>(cell_idx / kSectionDimension)
    };
}

}  // namespace w3terr

#endif  /// _W3MAPSECTION__H

