#ifndef _W3MAPRESOURCE__H
#define _W3MAPRESOURCE__H

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_format_saver.hpp>
#include "w3defs.h"
#include "w3e.h"

namespace w3terr {

class W3eResource final: public godot::Resource, public W3e {
	GDCLASS(W3eResource, Resource)
public:
    uint32_t ground_tilesets_count() const;
    uint32_t geo_tilesets_count() const;
    int32_t size_x() const;
    int32_t size_y() const;

protected:
    static void _bind_methods();
private:
    friend class W3eResourceLoader;
    friend class W3eResourceSaver;
};

class W3eResourceLoader : public godot::ResourceFormatLoader {
    GDCLASS(W3eResourceLoader, godot::ResourceFormatLoader);
protected:
    static void _bind_methods() {}
public:
    godot::PackedStringArray _get_recognized_extensions() const override {
        godot::PackedStringArray extensions;
        extensions.push_back("w3e");
        return extensions;
    }

    bool _handles_type(const godot::StringName &type) const override {
        return (type == godot::StringName("W3eResource"));
    }

    // Return the resource type name for a given path
    godot::String _get_resource_type(const godot::String &path) const override {
    	godot::String extesion = path.get_extension().to_lower();
        if (extesion == "w3e") {
            return "W3eResource";
        }
        return "";
    }

    /**
        Read w3e file (Warcraft III tile map format).
    */
    static godot::Variant load_w3e_file(const godot::Ref<godot::FileAccess>& file)
    {
        godot::Ref<W3eResource> resource;
        resource.instantiate();

        W3String magic = file->get_buffer(4).get_string_from_utf8();
        if (magic != W3e::kW3eMagik) {
            return godot::Error::ERR_FILE_UNRECOGNIZED;
        }

        uint32_t version = file->get_32();
        if (version != W3e::kW3eVersion) {
            return godot::Error::ERR_FILE_UNRECOGNIZED;
        }

        resource->main_tileset_id_ = file->get_8();
        resource->custom_tileset_flag_ = file->get_32();

        resource->number_of_ground_tilesets_ = file->get_32();
        for(size_t i = 0; i < resource->number_of_ground_tilesets_; ++i) {
            file->get_32();
        }

        resource->number_of_geo_tilesets_ = file->get_32();
        for(size_t i = 0; i < resource->number_of_geo_tilesets_; ++i) {
            file->get_32();
        }

        resource->map_size_x_ = static_cast<int32_t>(file->get_32());
        resource->map_size_y_ = static_cast<int32_t>(file->get_32());

        resource->map_3d_offset_x_ = file->get_float();
        resource->map_3d_offset_z_ = file->get_float();

        resource->initialize();

        for(int32_t i = 0; i < resource->map_size_y_; ++i) {
            for(int32_t j = 0; j < resource->map_size_x_; ++j) {
                W3eCell &cell_point = resource->get_cellpoint(j, i);

                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                auto *p_dst = reinterpret_cast<uint8_t*>(&cell_point);
                uint64_t bytes = file->get_buffer(p_dst, sizeof(W3eCell));
                if (bytes != sizeof(W3eCell)) {
                    w3_log_error("W3eResourceLoader::load_w3e_file: Read error!");
                    return godot::Error::ERR_FILE_CORRUPT;
                }
            }
        }
        return resource;
    }

    godot::Variant _load(
        const godot::String& p_path,
        const godot::String&  /*p_original_path*/,
        bool  /*p_use_sub_threads*/,
        int32_t  /*p_cache_mode*/) const override {

        godot::Ref<godot::FileAccess> file = godot::FileAccess::open(p_path, godot::FileAccess::READ);
        if (file.is_null()) {
            return godot::Error::ERR_CANT_OPEN;
        }
        return load_w3e_file(file);
    }
};

class W3eResourceSaver : public godot::ResourceFormatSaver {
    GDCLASS(W3eResourceSaver, godot::ResourceFormatSaver)
protected:
    static void _bind_methods() {}
public:
    bool _recognize(const godot::Ref<godot::Resource> &p_resource) const override {
        if (p_resource.is_null()) { return false; }
        return p_resource->is_class("W3eResource");
    }

    godot::PackedStringArray _get_recognized_extensions(const godot::Ref<godot::Resource>& p_resource) const override {
        godot::PackedStringArray extensions;
        if (p_resource.is_valid() && _recognize(p_resource)) {
            extensions.push_back("w3e");
        }
        return extensions;
    }

    /**
        Save w3e file (Warcraft III tile map format).
    */
    static godot::Error save_w3e_file(
        const W3eResource* resource,
        const godot::Ref<godot::FileAccess>& file)
    {
        if (!file.is_valid()) {
            return godot::Error::ERR_FILE_CANT_OPEN;
        }

        if (!file->store_string(W3e::kW3eMagik)) {
            return godot::Error::ERR_FILE_CANT_WRITE;
        }

        file->store_32(W3e::kW3eVersion);

        file->store_8(resource->main_tileset_id_);
        file->store_32(resource->custom_tileset_flag_);

        file->store_32(resource->number_of_ground_tilesets_);
        for(size_t i = 0; i < resource->number_of_ground_tilesets_; ++i) {
            file->store_32(0);
        }

        file->store_32(resource->number_of_geo_tilesets_);
        for(size_t i = 0; i < resource->number_of_geo_tilesets_; ++i) {
            file->store_32(0);
        }

        file->store_32(resource->map_size_x_);
        file->store_32(resource->map_size_y_);

        file->store_float(resource->map_3d_offset_x_);
        file->store_float(resource->map_3d_offset_z_);

        for(int32_t i = 0; i < resource->map_size_y_; ++i) {
            for(int32_t j = 0; j < resource->map_size_x_; ++j) {
                const W3eCell &cell_point = resource->get_cellpoint(j, i);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                const auto *p_src = reinterpret_cast<const uint8_t*>(&cell_point);
                file->store_buffer(p_src, sizeof(W3eCell));
            }
        }
        return godot::Error::OK;
    }

    godot::Error _save(
        const godot::Ref<godot::Resource> &p_resource,
        const godot::String &p_path,
        uint32_t /*p_flags*/) override {

        W3eResource *w3e_resource = Object::cast_to<W3eResource>(p_resource.ptr());
        if (w3e_resource == nullptr) {
            return godot::Error::FAILED;
        }

        // Open file using FileAccess
        godot::Ref<godot::FileAccess> file = godot::FileAccess::open(p_path, godot::FileAccess::WRITE);
        if (file.is_null()) {
            return godot::Error::ERR_CANT_OPEN;
        }
        return save_w3e_file(w3e_resource, file);
    }
};

}  // namespace w3terr

#endif  /// _W3MAPRESOURCE__H

