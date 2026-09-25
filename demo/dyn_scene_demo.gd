extends Node3D

@export var terrain: W3MapNode

func _ready():
	terrain = W3MapNode.new()
	
	# Load a W3E file
	var w3e = load("res://assets/imported/Maps/Campaign/Demo01.w3e")
	terrain.w3e_map = w3e

	# Set ground textures
	var textures = [
		load("res://assets/imported/TerrainArt/LordaeronSummer/Lords_Dirt.dds"),
		load("res://assets/imported/TerrainArt/LordaeronSummer/Lords_DirtRough.dds"),
		load("res://assets/imported/TerrainArt/LordaeronSummer/Lords_Rock.dds"),
		load("res://assets/imported/TerrainArt/LordaeronSummer/Lords_DirtGrass.dds"),
		load("res://assets/imported/TerrainArt/LordaeronSummer/Lords_Grass.dds"),
		load("res://assets/imported/TerrainArt/LordaeronSummer/Lords_GrassDark.dds"),
	]
	terrain.ground_textures = textures

	# Set cliff resources
	var res0 = W3GeoResource.new()
	res0.geoset_config = load("res://assets/imported/Geosets/ground_keys.json")
	res0.texture = load("res://assets/imported/ReplaceableTextures/Cliff/Cliff1.dds")
	res0.cliff_geoset_mesh = load("res://assets/imported/Geosets/ground_cliffs.obj")
	res0.ramp_geoset_mesh = load("res://assets/imported/Geosets/ground_ramps.obj")

	var res1 = W3GeoResource.new()
	res1.geoset_config = load("res://assets/imported/Geosets/ground_keys.json")
	res1.texture = load("res://assets/imported/ReplaceableTextures/Cliff/Cliff1.dds")
	res1.cliff_geoset_mesh = load("res://assets/imported/Geosets/ground_cliffs.obj")
	res1.ramp_geoset_mesh = load("res://assets/imported/Geosets/ground_ramps.obj")
	res1.ground_id = 4
	
	var resources = [ res0, res1 ]
	terrain.geo_resources = resources

	# Set camera for culling
	terrain.camera = $FreeLookCamera
	
	# Set surfaces
	var terrain_surface = W3SurfaceTerrain.new()
	terrain.add_child(terrain_surface)
	
	var ground_material = ShaderMaterial.new()
	ground_material.shader = load("res://assets/shaders/terrain_ground.gdshader")
	ground_material.render_priority = 0
	ground_material.set_shader_parameter("albedo",Color(1, 1, 1, 1))
	ground_material.set_shader_parameter("alpha_scissor_threshold", 0.5)
	terrain_surface.ground_material = ground_material

	var geo_material = ShaderMaterial.new()
	geo_material.shader = load("res://assets/shaders/terrain_ground.gdshader")
	geo_material.render_priority = 0
	geo_material.set_shader_parameter("albedo",Color(1, 1, 1, 1))
	terrain_surface.geo_material = geo_material

	var water_surface = W3SurfaceWater.new()
	terrain.add_child(water_surface)
	
	var water_material = ShaderMaterial.new()
	water_material.shader = load("res://assets/shaders/water_simple.gdshader")
	water_surface.water_material = water_material
	
	add_child(terrain)
