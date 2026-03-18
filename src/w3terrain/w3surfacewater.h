#ifndef _W3MAPSURFACE_WATER__H
#define _W3MAPSURFACE_WATER__H

#include <span>

#include "w3surface.h"
#include "w3math.h"

namespace w3terr {

class W3MapRuntimeManagerImpl;

class W3SurfaceWater final: public W3Surface {
    GDCLASS(W3SurfaceWater, W3Surface)
public:
    void _process(double delta) override;

    godot::AABB _get_aabb() const override;

    W3Ref<W3Marerial> get_water_material() const;
    void set_water_material(const W3Ref<W3Marerial>& material);

    void _enter_tree() override;
    void _exit_tree() override;

protected:
    static void _bind_methods();

private:
#ifdef W3MAP_STATS_ENABLE
    static constexpr auto kStatWaterTilesPrecachedId = "W3Terrain/stat_water_tiles_precached";
#endif

    static constexpr int32_t kMaxGPUMeshes = 0;//godot::RenderingServer::MAX_MESH_SURFACES / 8;

    struct CachedVertex {
        math::vector3 pos;
    };

    using VertexSpan = std::span<const CachedVertex>;
    using IndexSpan = std::span<const uint16_t>;

    void render(const W3Array<uint32_t>& collected_sections);

    void render_cells(uint32_t section_id);
    W3Pair<VertexSpan, IndexSpan> precache_cells(uint32_t section_id) const;
    void render_cached_mesh(VertexSpan vertices, IndexSpan indices);

    W3HashMap<uint32_t, bool> section_rendered_flags_;
    W3Array<uint32_t> not_rendered_sections_;

    W3Ref<W3Marerial> water_material_asset_;
#ifdef W3MAP_STATS_ENABLE
    uint64_t get_stat_water_tiles_precached() const {
        return stat_water_tiles_precached_;
    }
    mutable uint64_t stat_water_tiles_precached_;
#endif
};

inline
godot::AABB
W3SurfaceWater::_get_aabb() const
{
    return mesh_->get_aabb();
}

}  // namespace w3terr

#endif // _W3MAPSURFACE_WATER__H
