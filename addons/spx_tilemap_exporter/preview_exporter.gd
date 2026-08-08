@tool
extends RefCounted
## TileMap scene preview exporter - exports the scene as a PNG image
##
## Mirrors the C++ SpxSceneMgr::export_scene_as_png() functionality
## for use in the editor plugin environment.
##
## This class provides both instance methods (for editor use) and static
## utility functions (for CLI use) to avoid code duplication.

class_name PreviewExporter

## Export result structure
class ExportResult:
	var success: bool = false
	var error: String = ""
	var export_path: String = ""
	var image_size: Vector2i = Vector2i.ZERO


## Delay in seconds to wait for viewport rendering
const RENDER_DELAY_SECONDS = 0.5


## Reference to editor interface (needed for async operations)
var _editor_interface: EditorInterface
var _base_control: Control


func _init(editor_interface: EditorInterface = null):
	if editor_interface:
		_editor_interface = editor_interface
		_base_control = editor_interface.get_base_control()


# ============================================================================
# Main Export Function
# ============================================================================

## Export the current edited scene as a PNG preview image (async version)
## Waits for rendering and saves the image automatically
func export_scene_as_png_async(scene_root: Node) -> ExportResult:
	var result = ExportResult.new()
	
	if not scene_root:
		result.error = "No scene is currently open"
		return result
	
	# Find all TileMapLayers (use static function)
	var layers = PreviewExporter.find_tilemap_layers(scene_root)
	
	# Calculate scene bounds (TileMapLayers only, use static function)
	var bounds = PreviewExporter.calc_tilemap_bounds(layers)
	
	if not bounds.has_area():
		result.error = "No TileMapLayer content found in scene"
		return result
	
	# Generate export path
	var scene_name = scene_root.scene_file_path.get_file().get_basename()
	if scene_name.is_empty():
		scene_name = scene_root.name
	
	var export_dir = "res://_export/" + scene_name
	var export_path = export_dir + "/preview.png"
	var global_export_path = ProjectSettings.globalize_path(export_path)
	
	# Ensure export directory exists
	if not _ensure_export_dir(export_dir):
		result.error = "Failed to create export directory: " + export_dir
		return result
	
	# Create SubViewport for rendering
	var viewport = _create_preview_viewport(scene_root, bounds)
	if not viewport:
		result.error = "Failed to create preview viewport"
		return result
	
	# Add viewport to scene tree
	_base_control.add_child(viewport)
	
	# Wait for rendering to complete (multiple frames for safety)
	await _base_control.get_tree().create_timer(RENDER_DELAY_SECONDS).timeout
	
	# Get the rendered image and save
	var image = viewport.get_texture().get_image()
	if not image:
		viewport.queue_free()
		result.error = "Failed to capture viewport image"
		return result
	
	var err = image.save_png(global_export_path)
	
	# Cleanup
	viewport.queue_free()
	
	if err != OK:
		result.error = "Failed to save PNG: error code " + str(err)
		return result
	
	result.success = true
	result.export_path = export_path
	result.image_size = Vector2i(bounds.size)
	
	print_rich("[color=green]Preview PNG saved to: %s[/color]" % global_export_path)
	
	return result


# ============================================================================
# Static Utility Functions (can be called from CLI without EditorInterface)
# ============================================================================

## Calculate the bounding rectangle of all TileMapLayers
## Note: Only includes TileMapLayer bounds, excludes Sprite2D and other nodes
static func calc_tilemap_bounds(layers: Array[TileMapLayer]) -> Rect2:
	var total_rect = Rect2()
	
	for layer in layers:
		var layer_rect = calc_single_layer_bounds(layer)
		if layer_rect.has_area():
			var global_rect = layer.get_global_transform() * layer_rect
			total_rect = merge_rects(total_rect, global_rect)
	
	return total_rect


## Calculate bounds of a single TileMapLayer (mirrors SpxSceneMgr::get_tilemap_bounds)
static func calc_single_layer_bounds(layer: TileMapLayer) -> Rect2:
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


static func merge_rects(a: Rect2, b: Rect2) -> Rect2:
	if not a.has_area():
		return b
	if not b.has_area():
		return a
	return a.merge(b)


## Find all TileMapLayer nodes in the scene tree
static func find_tilemap_layers(node: Node) -> Array[TileMapLayer]:
	var layers: Array[TileMapLayer] = []
	_find_tilemap_layers_recursive(node, layers)
	return layers


static func _find_tilemap_layers_recursive(node: Node, layers: Array[TileMapLayer]) -> void:
	if node is TileMapLayer:
		layers.append(node)
	elif node is TileMap:
		for child in node.get_children(true):
			if child is TileMapLayer:
				layers.append(child)
	
	for child in node.get_children():
		_find_tilemap_layers_recursive(child, layers)


## Create a duplicate of the scene with only TileMapLayer nodes (all others removed)
## Returns the cleaned scene copy, or null on failure
## Set debug_log=true to print scene tree before/after cleanup
static func create_tilemap_only_copy(scene_root: Node, debug_log: bool = false) -> Node:
	var copy = scene_root.duplicate(Node.DUPLICATE_USE_INSTANTIATION)
	if not copy:
		return null
	
	if debug_log:
		print("=== Original Scene Tree (before cleanup) ===")
		print_scene_tree(copy, 0)
	
	# Remove all non-TileMapLayer nodes
	remove_non_tilemap_nodes(copy, debug_log)
	
	if debug_log:
		print("=== Cleaned Scene Tree (after cleanup) ===")
		print_scene_tree(copy, 0)
	
	return copy


## Print the scene tree for debugging
static func print_scene_tree(node: Node, indent: int) -> void:
	var indent_str = "  ".repeat(indent)
	var node_type = node.get_class()
	var visible_str = ""
	if node is CanvasItem:
		visible_str = " [visible=%s]" % str(node.visible)
	print("%s%s (%s)%s" % [indent_str, node.name, node_type, visible_str])
	
	for child in node.get_children():
		print_scene_tree(child, indent + 1)


## Remove all nodes that are not TileMapLayer or necessary parent containers
static func remove_non_tilemap_nodes(root: Node, debug_log: bool = false) -> void:
	# First, collect all TileMapLayer nodes and their ancestor paths
	var nodes_to_keep: Dictionary = {}
	_collect_tilemap_ancestors(root, nodes_to_keep)
	
	if debug_log:
		print("Nodes to keep: %d" % nodes_to_keep.size())
		for node in nodes_to_keep.keys():
			print("  - %s (%s)" % [node.name, node.get_class()])
	
	# Remove all nodes not in the keep list
	_delete_unwanted_nodes(root, nodes_to_keep, debug_log)


static func _collect_tilemap_ancestors(node: Node, nodes_to_keep: Dictionary) -> bool:
	var has_tilemap_descendant = false
	
	if node is TileMapLayer or node is TileMap:
		has_tilemap_descendant = true
	
	for child in node.get_children():
		if _collect_tilemap_ancestors(child, nodes_to_keep):
			has_tilemap_descendant = true
	
	if has_tilemap_descendant:
		nodes_to_keep[node] = true
	
	return has_tilemap_descendant


static func _delete_unwanted_nodes(node: Node, nodes_to_keep: Dictionary, debug_log: bool) -> void:
	var children = node.get_children()
	for child in children:
		if nodes_to_keep.has(child):
			_delete_unwanted_nodes(child, nodes_to_keep, debug_log)
		else:
			if debug_log:
				print("Deleting: %s (%s)" % [child.name, child.get_class()])
			node.remove_child(child)
			child.free()


# ============================================================================
# Instance Methods (for editor use with EditorInterface)
# ============================================================================

## Calculate the bounding rectangle of all TileMapLayers in the scene (instance method wrapper)
func get_scene_bounds(node: Node, layers: Array[TileMapLayer] = []) -> Rect2:
	return PreviewExporter.calc_tilemap_bounds(layers)


## Calculate bounds of a single TileMapLayer (instance method wrapper)
func get_tilemap_bounds(layer: TileMapLayer) -> Rect2:
	return PreviewExporter.calc_single_layer_bounds(layer)


# ============================================================================
# SubViewport Creation (uses static utility functions)
# ============================================================================

## Create a SubViewport configured for preview rendering
## Only renders TileMapLayer nodes, all other nodes are removed
func _create_preview_viewport(scene_root: Node, bounds: Rect2) -> SubViewport:
	var viewport = SubViewport.new()
	viewport.size = Vector2i(bounds.size)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	viewport.render_target_clear_mode = SubViewport.CLEAR_MODE_ALWAYS
	viewport.transparent_bg = true
	
	# Create a copy with only TileMapLayer nodes
	var copy = PreviewExporter.create_tilemap_only_copy(scene_root)
	if not copy:
		viewport.queue_free()
		return null
	
	# Adjust position so content starts at (0, 0)
	if copy is Node2D:
		copy.position = copy.global_position - bounds.position
	
	viewport.add_child(copy)
	
	return viewport


func _ensure_export_dir(export_dir: String) -> bool:
	var global_path = ProjectSettings.globalize_path(export_dir)
	if not DirAccess.dir_exists_absolute(global_path):
		var err = DirAccess.make_dir_recursive_absolute(global_path)
		if err != OK:
			push_error("Failed to create export directory: %s (error: %d)" % [global_path, err])
			return false
	return true
