
#ifndef SPX_DRAW_TILES_H
#define SPX_DRAW_TILES_H

#include "scene/2d/node_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/2d/camera_2d.h"
#include "scene/2d/tile_map.h"
#include "scene/resources/2d/tile_set.h"
#include <map>
#include <vector>

struct TileAction {
    int layer_index;
    Vector2i coords;
    bool placed;
    int source_id;
    Vector2i atlas_coord;
    int alternative_tile;
};

class SpxDrawTiles : public Node2D {
    GDCLASS(SpxDrawTiles, Node2D);

private:
    Ref<TileSet> tileset;
    Ref<Texture2D> current_texture;

    std::vector<TileAction> undo_stack;
    std::vector<TileAction> redo_stack;

    std::map<Ref<Texture2D>, int> texture_source_ids;
    int next_source_id = 1;

    Vector2i cell_size = Vector2i(64, 64);
    int current_layer_index = 0;

protected:
    static void _bind_methods();
    void _notification(int p_what);
    void _ready();
    void _draw();

public:
    SpxDrawTiles() = default;
    ~SpxDrawTiles() = default;

    void set_texture(Ref<Texture2D> texture);
    void set_layer_index(int index);

    void place_tile(Vector2i coords);
    void erase_tile(Vector2i coords);

    void undo();
    void redo();

    void handle_mouse_click(Vector2 pos, bool erase);

private:
    TileMapLayer* get_or_create_layer(int layer_index);
    int get_or_create_source_id(Ref<Texture2D> texture);
    void add_tile_collision(int source_id);
};

#endif // SPX_DRAW_TILES_H
