#include "spx_draw_tiles.h"
#include "core/os/keyboard.h"
#include "servers/physics_server_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/2d/physics/animatable_body_2d.h"
#include "scene/2d/physics/collision_shape_2d.h"
#include "scene/resources/2d/rectangle_shape_2d.h"
#include "core/io/resource_loader.h"
#include "scene/resources/world_2d.h"
#include "scene/gui/color_rect.h"


void SpxDrawTiles::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_texture", "texture"), &SpxDrawTiles::set_texture);
    ClassDB::bind_method(D_METHOD("set_layer_index", "index"), &SpxDrawTiles::set_layer_index);
    ClassDB::bind_method(D_METHOD("place_tile", "coords"), &SpxDrawTiles::place_tile);
    ClassDB::bind_method(D_METHOD("erase_tile", "coords"), &SpxDrawTiles::erase_tile);
    ClassDB::bind_method(D_METHOD("undo"), &SpxDrawTiles::undo);
    ClassDB::bind_method(D_METHOD("redo"), &SpxDrawTiles::redo);
    ClassDB::bind_method(D_METHOD("handle_mouse_click", "pos", "erase"), &SpxDrawTiles::handle_mouse_click);
}

void SpxDrawTiles::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        _ready();
    }

    if (p_what == NOTIFICATION_DRAW) {
        _draw();
    }
}

void SpxDrawTiles::_ready() {
    set_process(true); 
    set_process_input(true);
    tileset.instantiate();
    tileset->set_tile_size(cell_size);
}

TileMapLayer* SpxDrawTiles::get_or_create_layer(int layer_index) {
    for (int i=0;i<get_child_count();i++) {
        TileMapLayer *child = Object::cast_to<TileMapLayer>(get_child(i));
        if (child && i == layer_index) return child;
    }

    TileMapLayer *layer = memnew(TileMapLayer);
    layer->set_tile_set(tileset);
    add_child(layer);

    return layer;
}

int SpxDrawTiles::get_or_create_source_id(Ref<Texture2D> texture) {
    if (texture_source_ids.find(texture) != texture_source_ids.end()) {
        return texture_source_ids[texture];
    }
    int id = next_source_id++;
    texture_source_ids[texture] = id;
   
    tileset->add_physics_layer(0);
    tileset->set_physics_layer_collision_layer(0, 0xFFFF);
    tileset->set_physics_layer_collision_mask(0, 0xFFFF);
 
    Ref<TileSetAtlasSource> atlas_source;
    atlas_source.instantiate();
    atlas_source->set_texture(texture);
    atlas_source->set_texture_region_size(cell_size);
    atlas_source->create_tile(Vector2i(0, 0));

    tileset->add_source(atlas_source, id);

    atlas_source->add_physics_layer(0);
    Vector2i tile_id(0, 0);
    auto tile_data = atlas_source->get_tile_data(tile_id, 0);

    Vector<Vector2> rect;
    rect.push_back(Vector2(0, 0));
    rect.push_back(Vector2(64, 0));
    rect.push_back(Vector2(64, 64));
    rect.push_back(Vector2(0, 64));
    tile_data->add_collision_polygon(0);
    tile_data->set_collision_polygon_points(0, 0, rect);

    return id;
}

void SpxDrawTiles::add_tile_collision(int source_id) {

}

void SpxDrawTiles::set_texture(Ref<Texture2D> texture) {
    if (texture.is_null()) return;
    current_texture = texture;
    get_or_create_source_id(texture);
}

void SpxDrawTiles::set_layer_index(int index) {
    current_layer_index = index;
    get_or_create_layer(index);
}

void SpxDrawTiles::place_tile(Vector2i coords) {
    if (!current_texture.is_valid()) return;
    TileMapLayer *layer = get_or_create_layer(current_layer_index);

    int source_id = get_or_create_source_id(current_texture);
    Vector2i atlas_coord(0,0);
    int alt_tile = 0;
    print_error("Placing tile at: " + itos(coords.x) + "," + itos(coords.y) + " source id: " + itos(source_id));
    layer->set_cell(coords, source_id, atlas_coord, alt_tile);

    TileAction action{current_layer_index, coords, true, source_id, atlas_coord, alt_tile};
    undo_stack.push_back(action);
    redo_stack.clear();
    queue_redraw();
}

void SpxDrawTiles::erase_tile(Vector2i coords) {
    TileMapLayer *layer = get_or_create_layer(current_layer_index);
    if (layer->get_cell_source_id(coords) != TileSet::INVALID_SOURCE) {
        TileAction action{current_layer_index, coords, false, 1, Vector2i(0,0),0};
        layer->erase_cell(coords);
        undo_stack.push_back(action);
        redo_stack.clear();
        queue_redraw();
    }
}

void SpxDrawTiles::undo() {
    if (undo_stack.empty()) return;
    TileAction action = undo_stack.back();
    undo_stack.pop_back();

    TileMapLayer *layer = get_or_create_layer(action.layer_index);
    if (action.placed) layer->erase_cell(action.coords);
    else layer->set_cell(action.coords, action.source_id, action.atlas_coord, action.alternative_tile);

    redo_stack.push_back(action);
    queue_redraw();
}

void SpxDrawTiles::redo() {
    if (redo_stack.empty()) return;
    TileAction action = redo_stack.back();
    redo_stack.pop_back();

    TileMapLayer *layer = get_or_create_layer(action.layer_index);
    if (action.placed) layer->set_cell(action.coords, action.source_id, action.atlas_coord, action.alternative_tile);
    else layer->erase_cell(action.coords);

    undo_stack.push_back(action);
    queue_redraw();
}

void SpxDrawTiles::handle_mouse_click(Vector2 pos, bool erase) {
    TileMapLayer *layer = get_or_create_layer(current_layer_index);
    Vector2 local_pos = layer->to_local(pos);
    Vector2i coords = layer->local_to_map(local_pos);

    if (erase) erase_tile(coords);
    else place_tile(coords);
}

void SpxDrawTiles::_draw() {
    Rect2 used_rect(0,0,0,0);
    TileMapLayer *layer = get_or_create_layer(current_layer_index);
    used_rect = layer->get_used_rect();
    for (int x=used_rect.position.x; x<used_rect.position.x+used_rect.size.x;x++) {
        for (int y=used_rect.position.y; y<used_rect.position.y+used_rect.size.y;y++) {
            Vector2 pos = layer->map_to_local(Vector2i(x,y));
            draw_rect(Rect2(pos - cell_size / 2,cell_size), Color(1,1,1,0.2), false);
        }
    }
}