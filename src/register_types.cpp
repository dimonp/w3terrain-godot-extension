#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

#include "w3terrain/w3mapnode.h"
#include "w3terrain/w3mapbindings.h"
#include "w3terrain/w3mapbindingseditor.h"
#include "w3terrain/w3surfaceground.h"
#include "w3terrain/w3surfacewater.h"

namespace {

// NOLINTNEXTLINE(cert-err58-cpp, readability-static-definition-in-anonymous-namespace, cppcoreguidelines-avoid-non-const-global-variables)
static godot::Ref<w3terr::W3eResourceLoader> w3map_resource_loader;

void initialize_gdextension_types(godot::ModuleInitializationLevel p_level)
{
	if (p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(w3terr::W3eResource);
	GDREGISTER_CLASS(w3terr::W3eResourceLoader);
	GDREGISTER_CLASS(w3terr::W3MapNode);
	GDREGISTER_CLASS(w3terr::W3GeoResource);
	GDREGISTER_CLASS(w3terr::W3Surface)
	GDREGISTER_CLASS(w3terr::W3SurfaceGround);
	GDREGISTER_CLASS(w3terr::W3SurfaceWater);
	GDREGISTER_CLASS(w3terr::W3MapBindings);
	GDREGISTER_CLASS(w3terr::W3MapBindingsEditor);

 	w3map_resource_loader.instantiate();
    godot::ResourceLoader::get_singleton()->add_resource_format_loader(w3map_resource_loader);
}

void uninitialize_gdextension_types(godot::ModuleInitializationLevel p_level)
{
	if (p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Unregister the loader when the module unloads (important for editor plugins)
	godot::ResourceLoader::get_singleton()->remove_resource_format_loader(w3map_resource_loader);
	w3map_resource_loader.unref();
}

extern "C"
{
	// Initialization
	GDExtensionBool GDE_EXPORT w3terrain_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);
		return init_obj.init();
	}
}

}  // namespace
