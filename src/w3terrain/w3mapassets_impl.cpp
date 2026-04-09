#include "w3mapassets_impl.h"

namespace w3terr {

void
W3GeoResource::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_geoset_config"), &W3GeoResource::get_geoset_config);
    godot::ClassDB::bind_method(godot::D_METHOD("set_geoset_config", "config"), &W3GeoResource::set_geoset_config);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "geoset_config", godot::PROPERTY_HINT_RESOURCE_TYPE, "JSON"),
        "set_geoset_config", "get_geoset_config");

    godot::ClassDB::bind_method(godot::D_METHOD("get_texture"), &W3GeoResource::get_texture);
    godot::ClassDB::bind_method(godot::D_METHOD("set_texture", "texture"), &W3GeoResource::set_texture);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "texture",godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"),
        "set_texture", "get_texture");

    godot::ClassDB::bind_method(godot::D_METHOD("get_cliff_geoset_mesh"), &W3GeoResource::get_cliff_geoset_mesh);
    godot::ClassDB::bind_method(godot::D_METHOD("set_cliff_geoset_mesh", "cliff_geoset_mesh"), &W3GeoResource::set_cliff_geoset_mesh);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "cliff_geoset_mesh",godot::PROPERTY_HINT_RESOURCE_TYPE, "Mesh"),
        "set_cliff_geoset_mesh", "get_cliff_geoset_mesh");

    godot::ClassDB::bind_method(godot::D_METHOD("get_ramp_geoset_mesh"), &W3GeoResource::get_ramp_geoset_mesh);
    godot::ClassDB::bind_method(godot::D_METHOD("set_ramp_geoset_mesh", "ramp_geoset_mesh"), &W3GeoResource::set_ramp_geoset_mesh);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "ramp_geoset_mesh",godot::PROPERTY_HINT_RESOURCE_TYPE, "Mesh"),
        "set_ramp_geoset_mesh", "get_ramp_geoset_mesh");

    godot::ClassDB::bind_method(godot::D_METHOD("get_ground_id"), &W3GeoResource::get_ground_tileset_id);
    godot::ClassDB::bind_method(godot::D_METHOD("set_ground_id", "ground_id"), &W3GeoResource::set_ground_tileset_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "ground_id", godot::PROPERTY_HINT_RANGE, "0,255"),
        "set_ground_id", "get_ground_id");
}

godot::TypedArray<W3Texture>
W3MapAssetsImpl::get_ground_assets() const
{
    godot::TypedArray<W3Texture> result;
    for(const auto& asset : ground_assets_rt_) {
        result.push_back(asset.texture);
    }
    return result;
}

bool
W3MapAssetsImpl::set_ground_assets(const godot::TypedArray<W3Texture>& assets)
{
    const size_t ground_tileset_size = assets.size();
    ground_assets_rt_.clear();
    ground_assets_rt_.resize(ground_tileset_size);
    assets_dirty_flag_ = true;

    // load ground resources
    for(int64_t i = 0; i < ground_tileset_size; ++i) {
        W3Ref<W3Texture> ground_texture = assets[i];
        if (!ground_texture.is_valid()) {
            w3_log_error("Ground texture %d is not defined.", i);
            continue;
        }

        w3_log_debug("Loaded ground texture %d: %s", i, ground_texture);

        auto& ground_type_rt = ground_assets_rt_[i];
        ground_type_rt.tile_tu_size = kGroundTextureTileSize / static_cast<float>(ground_texture->get_width());
        ground_type_rt.tile_tv_size = kGroundTextureTileSize / static_cast<float>(ground_texture->get_height());
        ground_type_rt.is_extended = ground_texture->get_width() > ground_texture->get_height();
        ground_type_rt.texture = ground_texture;
    }
    return true;
}

void
W3MapAssetsImpl::check_and_warning_geo_asset(size_t asset_idx) const
{
    W3Ref<W3GeoResource> geo_resource = geo_assets_[static_cast<int32_t>(asset_idx)];
    const auto& cliff_mesh = geo_resource->get_cliff_geoset_mesh();
    if (cliff_mesh.is_null()) {
        w3_log_error("Cliff mesh %d is not defined.", asset_idx);
    }

    const auto& ramp_mesh = geo_resource->get_ramp_geoset_mesh();
    if (ramp_mesh.is_null()) {
        w3_log_error("Ramp mesh %d is not defined.", asset_idx);
    }

    const auto& geo_asset_rt = geo_assets_rt_[asset_idx];
    if (geo_asset_rt.geo_cliff_keys_map.size() != cliff_mesh->get_surface_count()) {
        w3_log_error("Cliff mesh %d has %d surfaces, but %d cliff keys in config.",
            asset_idx,
            static_cast<size_t>(cliff_mesh->get_surface_count()),
            geo_asset_rt.geo_cliff_keys_map.size());
    }

    if (geo_asset_rt.geo_ramp_keys_map.size() != ramp_mesh->get_surface_count()) {
        w3_log_error("Ramp mesh %d has %d surfaces, but %d ramp keys in config",
            asset_idx,
            static_cast<size_t>(cliff_mesh->get_surface_count()),
            geo_asset_rt.geo_ramp_keys_map.size());
    }
}

void
W3MapAssetsImpl::fill_mesh_counts_storage(size_t asset_idx)
{
    const W3Mesh *cliff_mesh = geo_assets_rt_[asset_idx].cliff_geoset_mesh.ptr();
    if (cliff_mesh != nullptr) {
        auto& cliff_geo_count= geo_assets_rt_[asset_idx].cliff_mesh_counts_storage;
        const int32_t surface_count = cliff_mesh->get_surface_count();
        cliff_geo_count.resize(surface_count);
        for(size_t cliff_geo_idx = 0; cliff_geo_idx < cliff_geo_count.size(); ++cliff_geo_idx) {
            const auto& surface = cliff_mesh->surface_get_arrays(static_cast<int32_t>(cliff_geo_idx));
            const godot::PackedVector3Array& vertices = surface[W3Mesh::ARRAY_VERTEX];
            const godot::PackedInt32Array& indices = surface[W3Mesh::ARRAY_INDEX];

            cliff_geo_count[cliff_geo_idx] = {
                vertices.size(),
                indices.size()
            };
        }
    }

    const W3Mesh *ramp_mesh = geo_assets_rt_[asset_idx].ramp_geoset_mesh.ptr();
    if (ramp_mesh != nullptr) {
        auto& ramp_geo_count= geo_assets_rt_[asset_idx].ramp_mesh_counts_storage;
        const int32_t surface_count = ramp_mesh->get_surface_count();
        ramp_geo_count.resize(surface_count);
        for(size_t ramp_geo_idx = 0; ramp_geo_idx < ramp_geo_count.size(); ++ramp_geo_idx) {
            const auto& surface = ramp_mesh->surface_get_arrays(static_cast<int32_t>(ramp_geo_idx));
            const godot::PackedVector3Array& vertices = surface[W3Mesh::ARRAY_VERTEX];
            const godot::PackedInt32Array& indices = surface[W3Mesh::ARRAY_INDEX];

            ramp_geo_count[ramp_geo_idx] = {
                vertices.size(),
                indices.size()
            };
        }
    }
}

void
W3MapAssetsImpl::prepare_geo_assets_rt()
{
    const size_t geo_assets_size = geo_assets_.size();
    geo_assets_rt_.clear();
    geo_assets_rt_.resize(geo_assets_size);
    assets_dirty_flag_ = true;

    // load cliff textures
    for(int64_t i = 0; i < geo_assets_size; ++i) {
        W3Ref<W3GeoResource> geo_resource = geo_assets_[i];
        if (!geo_resource.is_valid()) {
            w3_log_error("Geo resource %d is not defined.", i);
            continue;
        }

        w3_log_debug("Loaded geo texture %d: %s", i, geo_resource->get_texture());

        auto& geo_tileset_rt = geo_assets_rt_[i];
        geo_tileset_rt.texture = geo_resource->get_texture();
        geo_tileset_rt.cliff_geoset_mesh = geo_resource->get_cliff_geoset_mesh();
        geo_tileset_rt.ramp_geoset_mesh = geo_resource->get_ramp_geoset_mesh();
        geo_tileset_rt.ground_tileset_id = geo_resource->get_ground_tileset_id();

        load_geo_config(i, geo_resource->get_geoset_config());
        check_and_warning_geo_asset(i);
        fill_mesh_counts_storage(i);
    }
}

bool
W3MapAssetsImpl::load_geo_config(size_t tileset_id, const W3Ref<godot::JSON>& config)
{
    if (config.is_null()) {
        return false;
    }

    W3MapAssetsImpl::GeoAsset &geo_type_rt = this->geo_assets_rt_[tileset_id];

    const godot::Variant data = config->get_data();
    if (data.get_type() != godot::Variant::DICTIONARY) {
        w3_log_error("nTileMapNode::load_cliffs_config: unknown config format.");
        return false;
    }

    const godot::Dictionary geo_dict_data = data;
    for (const auto& key : geo_dict_data.keys()) {
        const godot::Variant& value_data = geo_dict_data[key];

        if (value_data.get_type() != godot::Variant::DICTIONARY) {
            w3_log_error("Geoset element not found.");
            return false;
        }

        const godot::Dictionary geoset_dict_data = value_data;
        const godot::String& geoset_type = static_cast<godot::String>(geoset_dict_data["type"]);
        const godot::String& geoset_mesh = static_cast<godot::String>(geoset_dict_data["mesh"]);

        if (geoset_type == "cliffs" && !geoset_mesh.is_empty()) {
            if (!parse_geoset_resource(geoset_dict_data, geo_type_rt.geo_cliff_keys_map)) {
                w3_log_error("nTileMapNode::load_cliffs_config: can't read cliffs geoset.");
                return false;
            }

        } else if (geoset_type == "ramps" && !geoset_mesh.is_empty()) {
            if (!parse_geoset_resource(geoset_dict_data, geo_type_rt.geo_ramp_keys_map)) {
                w3_log_error("nTileMapNode::load_cliffs_config: can't read ramps geoset.");
                return false;
            }
        }
    }
    return true;
}

template<typename TKeysMap>
bool
W3MapAssetsImpl::parse_geoset_resource(const godot::Dictionary& geoset, TKeysMap &geo_keys_map)
{
    size_t geoset_groups_count = static_cast<size_t>(geoset["count"]);
    if (geoset_groups_count == 0) {
        w3_log_error("XML geo config file error.");
        return false;
    }

    const godot::Variant gropups_data = geoset["groups"];
    if (gropups_data.get_type() != godot::Variant::DICTIONARY) {
        w3_log_error("Geoset gropus not found.");
        return false;
    }

    const godot::Dictionary groups_dict_data = gropups_data;
    for (const auto& mesh_idx : groups_dict_data.keys()) {

        const godot::Variant& key_data = groups_dict_data[mesh_idx];
        const godot::Variant key = static_cast<uint32_t>(key_data);
        geo_keys_map[key] = static_cast<uint32_t>(mesh_idx);

        --geoset_groups_count;
    }

    if (geoset_groups_count != 0) {
        w3_log_error("Geo config file error. Wrong geo groups count.");
        return false;
    }
    return true;
}

}  // namespace w3terr