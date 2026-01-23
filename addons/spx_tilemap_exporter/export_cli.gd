@tool
extends SceneTree
## Command-line export script for SPX TileMap and Decorator data
##
## Usage: godot --headless --path <project_path> -s addons/spx_tilemap_exporter/export_cli.gd [-- --scene <scene_path>]
##
## Arguments:
##   --scene <path>  Path to the scene file to export (default: res://main.tscn)
##
## Examples:
##   godot --headless --path . -s addons/spx_tilemap_exporter/export_cli.gd
##   godot --headless --path . -s addons/spx_tilemap_exporter/export_cli.gd -- --scene res://levels/level1.tscn

const TileMapExtractor = preload("res://addons/spx_tilemap_exporter/tilemap_extractor.gd")
const DecoratorExtractor = preload("res://addons/spx_tilemap_exporter/decorator_extractor.gd")
const PreviewExporter = preload("res://addons/spx_tilemap_exporter/preview_exporter.gd")

# ============================================================================
# Configuration - default values (can be overridden via command line)
# ============================================================================
const DEFAULT_SCENE_PATH = "res://main.tscn"
const EXPORT_TILEMAP = true
const EXPORT_DECORATORS = true
const EXPORT_PREVIEW = true  # Export scene preview PNG
const PREVIEW_RENDER_DELAY = 0.5  # Delay in seconds for viewport rendering
# ============================================================================

# State for async export
var _scene_root: Node = null
var _export_base: String = ""
var _has_error: bool = false
var _node_offset: Vector2 = Vector2.ZERO
var _export_phase: int = 0  # 0=init, 1=waiting_preview, 2=done
var _preview_viewport: SubViewport = null
var _preview_elapsed: float = 0.0


func _init() -> void:
	# Parse command line arguments
	var scene_path = _parse_scene_argument()
	
	# Get export directory path (preserves scene path structure)
	# e.g., "res://main.tscn" -> "res://_export/main"
	# e.g., "res://levels/level1.tscn" -> "res://_export/levels/level1"
	var export_path = scene_path.get_file().get_basename()
	_export_base = "res://_export/" + export_path
	
	print("SPX Export CLI")
	print("==============")
	print("Scene:  ", scene_path)
	print("Output: ", _export_base)
	print("Export TileMap: ", EXPORT_TILEMAP)
	print("Export Decorators: ", EXPORT_DECORATORS)
	print("Export Preview: ", EXPORT_PREVIEW)
	print("")
	
	# Load the scene
	var packed_scene = load(scene_path)
	if not packed_scene:
		printerr("ERROR: Failed to load scene: ", scene_path)
		quit(1)
		return
	
	if not packed_scene is PackedScene:
		printerr("ERROR: Loaded resource is not a PackedScene: ", scene_path)
		quit(1)
		return
	
	_scene_root = packed_scene.instantiate()
	if not _scene_root:
		printerr("ERROR: Failed to instantiate scene")
		quit(1)
		return
	
	# Ensure export directory exists
	var global_export_dir = ProjectSettings.globalize_path(_export_base)
	if not DirAccess.dir_exists_absolute(global_export_dir):
		var err = DirAccess.make_dir_recursive_absolute(global_export_dir)
		if err != OK:
			printerr("ERROR: Failed to create export directory: ", global_export_dir)
			_scene_root.queue_free()
			quit(1)
			return
	
	# Export TileMap (synchronous)
	if EXPORT_TILEMAP:
		var tilemap_path = _export_base + "/tilemap.json"
		var global_tilemap_path = ProjectSettings.globalize_path(tilemap_path)
		var tilemap_result = _export_tilemap(_scene_root, global_tilemap_path)
		if tilemap_result.success:
			_node_offset = tilemap_result.node_offset
		elif tilemap_result.error != "skipped":
			_has_error = true
	
	# Export Decorators (synchronous)
	if EXPORT_DECORATORS:
		var decorator_path = _export_base + "/decorator.json"
		var global_decorator_path = ProjectSettings.globalize_path(decorator_path)
		var decorator_result = _export_decorators(_scene_root, global_decorator_path, _node_offset)
		if not decorator_result.success and decorator_result.error != "skipped":
			_has_error = true
	
	# Export Preview PNG (requires async rendering)
	if EXPORT_PREVIEW:
		_start_preview_export()
	else:
		_finish_export()


func _process(delta: float) -> bool:
	if _export_phase == 1:  # Waiting for preview render
		_preview_elapsed += delta
		if _preview_elapsed >= PREVIEW_RENDER_DELAY:
			_complete_preview_export()
			_finish_export()
	return false  # Continue processing


func _start_preview_export() -> void:
	# Use PreviewExporter static functions for TileMap-only rendering
	var layers = PreviewExporter.find_tilemap_layers(_scene_root)
	var bounds = PreviewExporter.calc_tilemap_bounds(layers)
	
	if not bounds.has_area():
		print("Preview: No TileMapLayer content found (skipped)")
		_finish_export()
		return
	
	# Create SubViewport for rendering
	_preview_viewport = SubViewport.new()
	_preview_viewport.size = Vector2i(bounds.size)
	_preview_viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	_preview_viewport.render_target_clear_mode = SubViewport.CLEAR_MODE_ALWAYS
	_preview_viewport.transparent_bg = true
	
	# Create a copy with only TileMapLayer nodes (uses PreviewExporter static function)
	var copy = PreviewExporter.create_tilemap_only_copy(_scene_root)
	if not copy:
		print("Preview: Failed to create scene copy (skipped)")
		_preview_viewport.queue_free()
		_preview_viewport = null
		_finish_export()
		return
	
	# Adjust position so content starts at (0, 0)
	if copy is Node2D:
		copy.position = copy.global_position - bounds.position
	
	_preview_viewport.add_child(copy)
	root.add_child(_preview_viewport)
	
	_export_phase = 1
	_preview_elapsed = 0.0
	print("Preview: Rendering...")


func _complete_preview_export() -> void:
	if not _preview_viewport:
		return
	
	var preview_path = _export_base + "/preview.png"
	var global_preview_path = ProjectSettings.globalize_path(preview_path)
	
	# Get viewport texture (may be null in headless mode)
	var texture = _preview_viewport.get_texture()
	if not texture:
		print("Preview: Viewport texture unavailable (headless mode?), skipped")
		_preview_viewport.queue_free()
		_preview_viewport = null
		_export_phase = 2
		return
	
	var image = texture.get_image()
	if not image:
		print("Preview: Failed to capture image (headless mode?), skipped")
		_preview_viewport.queue_free()
		_preview_viewport = null
		_export_phase = 2
		return
	
	var err = image.save_png(global_preview_path)
	if err != OK:
		printerr("ERROR: Failed to save preview PNG: ", err)
		_has_error = true
	else:
		print("Preview Export successful!")
		print("  Output: ", preview_path)
		print("  Size: ", _preview_viewport.size.x, "x", _preview_viewport.size.y)
	
	_preview_viewport.queue_free()
	_preview_viewport = null
	_export_phase = 2


func _finish_export() -> void:
	if _scene_root:
		_scene_root.queue_free()
		_scene_root = null
	
	if _has_error:
		print("")
		print("Export completed with errors")
		quit(1)
	else:
		print("")
		print("Export completed successfully!")
		quit(0)


# ============================================================================
# TileMap Export
# ============================================================================

class TileMapResult:
	var success: bool = false
	var error: String = ""
	var node_offset: Vector2 = Vector2.ZERO

func _export_tilemap(scene_root: Node, output_path: String) -> TileMapResult:
	var result = TileMapResult.new()
	
	# Find all TileMapLayer nodes
	var layers = _find_tilemap_layers(scene_root)
	if layers.is_empty():
		print("TileMap: No TileMapLayer nodes found (skipped)")
		result.error = "skipped"
		return result
	
	print("Found ", layers.size(), " TileMapLayer(s)")
	
	# Check if any layer has a TileSet
	var has_tileset = false
	for layer in layers:
		if layer.tile_set:
			has_tileset = true
			break
	
	if not has_tileset:
		printerr("ERROR: No TileSet found in TileMapLayer nodes")
		result.error = "No TileSet found"
		return result
	
	# Calculate node_offset for decorator alignment
	result.node_offset = _calculate_tilemap_offset(layers)
	
	# Export using TileMapExtractor with "tilemap" as textures subdirectory
	var extractor = TileMapExtractor.new()
	var export_result = extractor.export_tilemap(layers, output_path, "tilemap")
	
	if export_result.success:
		print("TileMap Export successful!")
		print("  Output: ", output_path)
		print("  Layers: ", export_result.layer_count)
		print("  Textures: ", export_result.texture_count)
		result.success = true
	else:
		printerr("ERROR: TileMap export failed: ", export_result.error)
		result.error = export_result.error
	
	return result


# ============================================================================
# Decorator Export
# ============================================================================

class DecoratorResult:
	var success: bool = false
	var error: String = ""

func _export_decorators(scene_root: Node, output_path: String, node_offset: Vector2) -> DecoratorResult:
	var result = DecoratorResult.new()
	
	# Export using DecoratorExtractor with "decorator" as textures subdirectory
	var extractor = DecoratorExtractor.new()
	var export_result = extractor.export_decorators(scene_root, output_path, node_offset, "decorator")
	
	if export_result.success:
		print("Decorator Export successful!")
		print("  Output: ", output_path)
		print("  Decorators: ", export_result.decorator_count)
		print("  Textures: ", export_result.texture_count)
		result.success = true
	else:
		if export_result.error == "No decorators found in scene":
			print("Decorators: None found (skipped)")
			result.error = "skipped"
		else:
			printerr("ERROR: Decorator export failed: ", export_result.error)
			result.error = export_result.error
	
	return result


# ============================================================================
# Helper Functions
# ============================================================================

func _parse_scene_argument() -> String:
	"""Parse --scene argument from command line, return default if not specified"""
	# Use get_cmdline_user_args() for arguments after "--" separator
	var args = OS.get_cmdline_user_args()
	
	# Look for --scene argument
	for i in range(args.size()):
		if args[i] == "--scene" and i + 1 < args.size():
			var scene_arg = args[i + 1]
			# Validate the scene path
			if not scene_arg.begins_with("res://"):
				scene_arg = "res://" + scene_arg
			print("Using scene from command line: ", scene_arg)
			return scene_arg
	
	# Return default if not specified
	return DEFAULT_SCENE_PATH


func _find_tilemap_layers(node: Node) -> Array[TileMapLayer]:
	var layers: Array[TileMapLayer] = []
	_find_tilemap_layers_recursive(node, layers)
	return layers


func _find_tilemap_layers_recursive(node: Node, layers: Array[TileMapLayer]) -> void:
	if node is TileMapLayer:
		layers.append(node)
	elif node is TileMap:
		# TileMap node contains TileMapLayer as internal children
		for child in node.get_children(true):
			if child is TileMapLayer:
				layers.append(child)
	
	# Continue searching in regular children
	for child in node.get_children():
		_find_tilemap_layers_recursive(child, layers)


func _calculate_tilemap_offset(layers: Array[TileMapLayer]) -> Vector2:
	if layers.is_empty():
		return Vector2.ZERO
	
	var tileset = layers[0].tile_set
	if tileset == null:
		return Vector2.ZERO
	
	var tile_size = tileset.tile_size
	
	# Calculate bounds
	var min_x: int = 0x7FFFFFFF
	var max_x: int = -0x80000000
	var min_y: int = 0x7FFFFFFF
	var max_y: int = -0x80000000
	var has_tiles: bool = false
	
	for layer in layers:
		var global_pos = layer.position
		var layer_offset_x: int = floori(global_pos.x / tile_size.x)
		var layer_offset_y: int = floori(global_pos.y / tile_size.y)
		
		var used_cells = layer.get_used_cells()
		for cell in used_cells:
			has_tiles = true
			var world_x = cell.x + layer_offset_x
			var world_y = cell.y + layer_offset_y
			min_x = mini(min_x, world_x)
			max_x = maxi(max_x, world_x)
			min_y = mini(min_y, world_y)
			max_y = maxi(max_y, world_y)
	
	if not has_tiles:
		return Vector2.ZERO
	
	# Calculate center offset
	var center_x: int = (min_x + max_x) / 2
	var center_y: int = (min_y + max_y) / 2
	
	return Vector2(-center_x * tile_size.x, -center_y * tile_size.y)


# ============================================================================
# Preview Export Helper Functions
# ============================================================================

## Calculate the bounding rectangle of all renderable content in the scene
func _get_scene_bounds(node: Node, layers: Array[TileMapLayer]) -> Rect2:
	var total_rect = Rect2()
	
	# Calculate TileMapLayer bounds
	for layer in layers:
		var layer_rect = _get_tilemap_bounds(layer)
		if layer_rect.has_area():
			var global_rect = layer.get_global_transform() * layer_rect
			total_rect = _merge_rects(total_rect, global_rect)
	
	# Collect Sprite2D bounds recursively
	total_rect = _collect_sprite_bounds_recursive(node, total_rect)
	
	return total_rect


## Calculate bounds of a single TileMapLayer
func _get_tilemap_bounds(layer: TileMapLayer) -> Rect2:
	if not layer or not layer.tile_set:
		return Rect2()
	
	var used_rect = layer.get_used_rect()
	if used_rect.size == Vector2i.ZERO:
		return Rect2()
	
	var tile_size = layer.tile_set.tile_size
	
	# Convert tile coordinates to local pixel coordinates
	var top_left = layer.map_to_local(used_rect.position)
	var bottom_right = layer.map_to_local(used_rect.position + used_rect.size)
	
	# Adjust for half-tile offset (tiles are centered)
	var half_tile = Vector2(tile_size) / 2.0
	var rect = Rect2(top_left - half_tile, bottom_right - top_left)
	
	return rect


func _collect_sprite_bounds_recursive(node: Node, total_rect: Rect2) -> Rect2:
	# Skip TileMapLayer nodes (already handled)
	if node is TileMapLayer:
		return total_rect
	
	if node is Sprite2D:
		var sprite = node as Sprite2D
		var texture = sprite.texture
		if texture:
			var size = texture.get_size()
			var offset = -size / 2.0 if sprite.centered else Vector2.ZERO
			offset += sprite.offset
			var local_rect = Rect2(offset, size)
			var global_rect = sprite.get_global_transform() * local_rect
			total_rect = _merge_rects(total_rect, global_rect)
	
	for child in node.get_children():
		total_rect = _collect_sprite_bounds_recursive(child, total_rect)
	
	return total_rect


func _merge_rects(a: Rect2, b: Rect2) -> Rect2:
	if not a.has_area():
		return b
	if not b.has_area():
		return a
	return a.merge(b)


