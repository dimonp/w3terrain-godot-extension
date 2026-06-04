#ifndef _W3MAPSURFACE_GROUND__H
#define _W3MAPSURFACE_GROUND__H

#include <span>
#include <godot_cpp/classes/texture2d_array.hpp>
#include <godot_cpp/classes/shader_material.hpp>

#include "w3surface.h"
#include "w3math.h"

namespace w3terr {

class W3SurfaceTerrain final: public W3Surface {
    GDCLASS(W3SurfaceTerrain, W3Surface)
public:
    W3Ref<W3Marerial> get_ground_material() const;
    void set_ground_material(const W3Ref<W3Marerial>& material);

    W3Ref<W3Marerial> get_geo_materials() const;
    void set_geo_materials(const W3Ref<W3Marerial>& material);

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
    static constexpr auto kStatGroundTilesRenderedId = "W3Terrain/stat_ground_tiles_rendered";
    static constexpr auto kStatGeoTilesRenderedId = "W3Terrain/stat_geo_tiles_rendered";
#endif

    void render(const W3Array<uint32_t>& sections);

    void render_section_ground(uint32_t section_id);
    void render_section_geo(uint32_t section_id);
    void render_section_normals(uint32_t section_id);

    void render_ground_cells(uint32_t section_id, size_t tileset_id);
    void render_ground_cells_normals(uint32_t section_id);
    void render_geo_cells(uint32_t section_id, size_t tileset_id);
    void render_geo_cells_normals(uint32_t section_id);

    bool load_ground_materials();
    bool load_geo_materials();

    W3Ref<W3Marerial> get_debug_material() const;
    void set_debug_material(const W3Ref<W3Marerial>& material);

    W3Ref<godot::ShaderMaterial> ground_material_asset_;
    W3Ref<godot::Texture2DArray> ground_textures_array_;

    W3Ref<godot::ShaderMaterial> geo_material_asset_;
    W3Ref<godot::Texture2DArray> geo_textures_array_;

    bool ground_assets_dirty_flag_ = true;
    bool geo_assets_dirty_flag_ = true;

#ifdef W3MAP_STATS_ENABLE
    uint64_t get_stat_ground_tiles_rendered() const {
        return stat_ground_tiles_rendered_;
    }
    uint64_t get_stat_geo_tiles_rendered() const {
        return stat_geo_tiles_rendered_;
    }
    mutable uint64_t stat_ground_tiles_rendered_;
    mutable uint64_t stat_geo_tiles_rendered_;
#endif

};

}  // namespace w3terr

#endif // _W3MAPSURFACE_GROUND__H
