#ifndef _W3MAPRUNTIME_IMPL__H
#define _W3MAPRUNTIME_IMPL__H

#include <optional>
#include <gsl/gsl>

#include "w3defs.h"
#include "w3math.h"
#include "w3mapruntimemanager.h"

namespace w3terr {

class W3MapAssets;
class W3MapInformator;

class W3_API W3MapRuntimeManagerImpl final : public W3MapRuntimeManager {
public:
    W3MapRuntimeManagerImpl(const W3MapAssets* assets, const W3MapInformator* informator);

    const CellPointRT& get_cellpoint_rt(const Coord2D& coords) const override;

    float get_cellpoint_layer_height(const Coord2D& coords) const override;
    float get_cellpoint_ground_height(const Coord2D& coords) const override;
    math::vector3 get_cellpoint_position(const Coord2D& coords) const override;
    math::vector3 get_cellpoint_water_position(const Coord2D& coords) const override;
    math::bbox3 get_cell_bbox(const Coord2D& coords) const override;
    float get_cell_ground_height(const Coord2D& coords, float t_lerp, float s_lerp) const override;

    bool test_cell_intersection(const Coord2D& coords, const math::line3& line) const override;
    std::optional<math::vector3> get_cell_intersection_point(const Coord2D& coords, const math::line3& line) const override;

    void update_all_cells_rt() override;
    void update_cell_rt(const Coord2D& coords) override;
    void update_area_rt(const Coord2D& coords, int32_t area_margin) override;

    bool is_dirty() const override;
    void set_dirty(bool flag) override;

private:
    const auto* w3e_map() const;

    CellPointRT& get_cellpoint_rt(const Coord2D& coords);

    uint32_t get_cliff_geoset_id_for_geokey(size_t geo_tileset_id, uint32_t geo_key) const;
    uint32_t get_ramp_geoset_id_for_geokey(size_t geo_tileset_id, uint32_t geo_key) const;

    void update_runtime_ground(const auto& cell_info, W3MapRuntimeManagerImpl::CellPointRT& cell_rt) const;
    void update_runtime_cliff(const auto& cell_info, W3MapRuntimeManagerImpl::CellPointRT& cell_rt) const;
    void update_runtime_ramp(const auto& cell_info, W3MapRuntimeManagerImpl::CellPointRT& cell_rt) const;

    W3Array<CellPointRT> cellpoints_rt_;

    gsl::not_null<const W3MapAssets*> map_asset_;
    gsl::not_null<const W3MapInformator*> informator_;

    bool dirty_flag_ = true;
};

inline
bool
W3MapRuntimeManagerImpl::is_dirty() const
{
    return dirty_flag_;
}

inline
void
W3MapRuntimeManagerImpl::set_dirty(bool flag)
{
    dirty_flag_ = flag;
}

}  // namespace w3terr

#endif // _W3MAPRUNTIME_IMPL__H
