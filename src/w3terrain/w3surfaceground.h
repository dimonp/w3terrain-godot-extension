#ifndef _W3MAPSURFACE_GROUND__H
#define _W3MAPSURFACE_GROUND__H

#include <span>
#include <godot_cpp/classes/texture2d_array.hpp>
#include <godot_cpp/classes/shader_material.hpp>

#include "w3surface.h"
#include "w3math.h"

namespace w3terr {

class W3SurfaceGround final: public W3Surface {
    GDCLASS(W3SurfaceGround, W3Surface)
public:
    W3Ref<W3Marerial> get_ground_material() const;
    void set_ground_material(const W3Ref<W3Marerial>& material);

    W3Ref<W3Marerial> get_geo_materials() const;
    void set_geo_materials(const W3Ref<W3Marerial>& material);

    bool get_render_normals() const;
    void set_render_normals(bool flag);

    godot::AABB _get_aabb() const override;

    void on_ground_assets_changed();
    void on_geo_assets_changed();

    void _notification(int p_what);
    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

protected:
    static void _bind_methods();

private:
#ifdef W3MAP_STATS_ENABLE
    static constexpr auto kStatGroundTilesPrecachedId = "W3Terrain/stat_ground_tiles_precached";
    static constexpr auto kStatGeoTilesPrecachedId = "W3Terrain/stat_geo_tiles_precached";
#endif

    static constexpr int32_t kMaxGPUMeshes = godot::RenderingServer::MAX_MESH_SURFACES / 2;

    struct CachedVertex {
        math::vector3 pos;
        math::vector3 norm;
        math::vector2 uv;
    };

    using VertexSpan = std::span<const CachedVertex>;
    using IndexSpan = std::span<const uint16_t>;

    void render(const W3Array<uint32_t>& sections);

    void render_grounds(const W3Array<uint32_t>& sections, bool render_as_normals);
    void render_geos(const W3Array<uint32_t>& sections, bool render_as_normals);

    void render_ground_cells(uint32_t section_id, size_t tileset_id, bool render_as_normals);
    W3Pair<VertexSpan, IndexSpan> precache_ground_cells(uint32_t section_id, size_t tileset_id) const;

    void render_geo_cells(uint32_t section_id, size_t tileset_id, bool render_as_normals);
    W3Pair<VertexSpan, IndexSpan> precache_geo_cells(uint32_t section_id, size_t tileset_id) const;
    W3Pair<VertexSpan, IndexSpan> precache_geo_cells_fake(uint32_t section_id, size_t tileset_id) const;


    void render_cached_mesh(VertexSpan vertices, IndexSpan indices);
    void render_cached_mesh_normals(VertexSpan vertices);
    void reset_rendered();

    bool load_ground_materials();
    bool load_geo_materials();

    W3Ref<W3Marerial> get_debug_material() const;
    void set_debug_material(const W3Ref<W3Marerial>& material);

    bool render_normals_ = false;
    W3Ref<W3Marerial> debug_material_;

    W3HashMap<uint32_t, bool> section_rendered_flags_;
    W3Array<uint32_t> not_rendered_sections_;

    W3Ref<godot::ShaderMaterial> ground_material_asset_;
    W3Ref<godot::Texture2DArray> ground_textures_array_;

    W3Ref<godot::ShaderMaterial> geo_material_asset_;
    W3Ref<godot::Texture2DArray> geo_textures_array_;

    bool ground_assets_dirty_flag_ = true;
    bool geo_assets_dirty_flag_ = true;

#ifdef W3MAP_STATS_ENABLE
    uint64_t get_stat_ground_tiles_precached() const {
        return stat_ground_tiles_precached_;
    }
    uint64_t get_stat_geo_tiles_precached() const {
        return stat_geo_tiles_precached_;
    }
    mutable uint64_t stat_ground_tiles_precached_;
    mutable uint64_t stat_geo_tiles_precached_;
#endif

};

inline
godot::AABB
W3SurfaceGround::_get_aabb() const
{
    return mesh_->get_aabb();
}

}  // namespace w3terr

#endif // _W3MAPSURFACE_GROUND__H
