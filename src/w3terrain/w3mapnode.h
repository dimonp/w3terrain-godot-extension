#ifndef _W3MAPNODE__H
#define _W3MAPNODE__H

#include <memory>
#include <optional>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/visual_instance3d.hpp>
#include <godot_cpp/classes/thread.hpp>

#include "w3defs.h"
#include "w3mapassets_impl.h"

namespace w3terr {

class W3MapInformatorImpl;
class W3MapRuntimeManagerImpl;
class W3MapSectionManagerImpl;
class W3MapCollectorImpl;
class W3MapBindings;
class W3MapBindingsEditor;
class W3SectionRenderedCache;

class W3MapNode final : public godot::VisualInstance3D, W3MapAssetsImpl { // NOLINT(fuchsia-multiple-inheritance)
    GDCLASS(W3MapNode, VisualInstance3D)
public:
    static constexpr auto kSignalMapInitialized  = "map_initialized";
    static constexpr auto kSignalMapDestroyed  = "map_destroyed";
    static constexpr auto kSignalMapInitializationProgress  = "map_initialization_progress";
    static constexpr auto kSignalMapAssetsChanged  = "map_assets_changed";
    static constexpr auto kSignalGroundAssetsChanged = "ground_assets_changed";
    static constexpr auto kSignalGeoAssetsChanged = "geo_assets_changed";

    W3MapNode();
    ~W3MapNode() final;

    bool create_empty_map(int32_t dim_2d_x, int32_t dim_2d_y, uint32_t ground_tilesets, uint32_t geo_tilesets);

    W3Ref<W3eResource> get_w3e_resource() const;
    void set_w3e_resource(const W3Ref<W3eResource>& map);
    godot::TypedArray<W3Texture> get_ground_textures() const;
    void set_ground_textures(const godot::TypedArray<W3Texture>& textures);
    godot::TypedArray<W3GeoResource> get_geo_resources() const;
    void set_geo_resources(const godot::TypedArray<W3GeoResource>& resources);

#ifdef EDITOR_SUPPORT_ENABLE
    bool is_editor_camera() const;
    void use_editor_camera(bool flag);
#endif

    godot::Camera3D* get_camera() const;
    void set_camera(godot::Camera3D* p_camera);
    bool is_camera_valid() const;

    W3MapBindings* get_bindings() const;
#ifdef EDITOR_SUPPORT_ENABLE
    W3MapBindingsEditor* get_bindings_editor() const;
#endif
    const W3MapAssets* get_assets() const;
    const W3MapRuntimeManagerImpl* get_runtime_manager() const;
    W3MapSectionManagerImpl* get_section_manager() const;
    const W3MapCollectorImpl* get_collector() const;
    W3SectionRenderedCache* get_rendered_sections_cache() const;

    std::optional<Coord2D> get_intersected_cell(const math::line3& line, math::vector3 &ipoint) const;

    void on_map_resource_changed();
    void on_ground_assets_changed();
    void on_geo_assets_changed();
    void on_geo_resource_changed();
    void on_frame_rendered();

    void _notification(int p_what);
    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

    godot::AABB _get_aabb() const override;

protected:
    static void _bind_methods();

private:
#ifdef W3MAP_STATS_ENABLE
    static constexpr auto kStatCachedSectionsCount = "W3Terrain/cached_sections_count";
    static constexpr auto kStatVisibleSectionsCount = "W3Terrain/visible_sections_count";
#endif

    void load_map();
    void reset_runtime();

    void collect_visible_sections();

    std::unique_ptr<W3MapInformatorImpl> informator_;
    std::unique_ptr<W3MapRuntimeManagerImpl> runtime_manager_;
    std::unique_ptr<W3MapSectionManagerImpl> sections_manager_;
    std::unique_ptr<W3MapCollectorImpl> collector_;
    std::unique_ptr<W3SectionRenderedCache> rendered_sections_cache_;

    std::unique_ptr<W3MapBindings, GodotObjectDeleter<W3MapBindings>> map_bindings_;
#ifdef EDITOR_SUPPORT_ENABLE
    std::unique_ptr<W3MapBindingsEditor, GodotObjectDeleter<W3MapBindingsEditor>> map_bindings_editor_;
#endif

    bool use_editor_camera_ = false;
    godot::ObjectID camera_id_;

    W3Ref<godot::Thread> load_thread_;
    std::atomic<bool> is_loading_ {false};
    std::atomic<bool> stop_loading_ {false};

#ifdef W3MAP_STATS_ENABLE
    uint64_t get_cached_sections_count() const;
    uint64_t get_visible_sections_count() const;
#endif

};

inline
const W3MapAssets*
W3MapNode::get_assets() const
{
    return this;
}

inline
const W3MapRuntimeManagerImpl*
W3MapNode::get_runtime_manager() const
{
    return runtime_manager_.get();
}

inline
W3MapSectionManagerImpl*
W3MapNode::get_section_manager() const
{
    return sections_manager_.get();
}

inline
const W3MapCollectorImpl*
W3MapNode::get_collector() const
{
    return collector_.get();
}

inline
W3SectionRenderedCache*
W3MapNode::get_rendered_sections_cache() const
{
    return rendered_sections_cache_.get();
}

inline
W3MapBindings*
W3MapNode::get_bindings() const
{
    return map_bindings_.get();
}

inline
W3MapBindingsEditor*
W3MapNode::get_bindings_editor() const
{
    return map_bindings_editor_.get();
}

inline
bool
W3MapNode::is_camera_valid() const
{
    const Object *obj = godot::ObjectDB::get_instance(camera_id_);
    return obj != nullptr;
}

}  // namespace w3terr

#endif  /// _W3MAPNODE__H

