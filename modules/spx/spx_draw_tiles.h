
#ifndef SPX_DRAW_TILES_H
#define SPX_DRAW_TILES_H

#include "scene/2d/node_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/2d/camera_2d.h"


class SpxDrawTiles : public Node2D {
    GDCLASS(SpxDrawTiles, Node2D)

private:
    struct Operation {
        String type;         // "add" / "remove"
        Node2D *node;
        String sprite_path;
        Vector2 position;
    };

    bool drawing = false;
    bool deleting = false;
    int current_tile = 0;

    // tile
    Array tile_textures; // Dictionary { "texture": Ref<Texture2D>, "path": String }
    Vector2 tile_size = Vector2(64,64);

    // HUD
    Sprite2D *preview_sprite = nullptr;

    Array undo_stack;
    Array redo_stack;

    // Palette UI
    Array palette_sprites;        // Sprite2D
    Array palette_highlight_rects; // ColorRect
    int palette_margin = 10;
    int palette_size = 48;

private:
    Node2D *_get_tile_at_position(Vector2 pos);
    Camera2D *camera = nullptr;

protected:
    static void _bind_methods();

    void _process(double delta);
    void _draw();
    void _notification(int p_what);
    void _ready();
    void input(const Ref<InputEvent> &p_event) override;

public:
    SpxDrawTiles() = default;
    ~SpxDrawTiles() = default;

    // API
    void add_sprite_path(const String &path);
    void switch_tile(int index);

    // Tile 
    bool has_tile_at(Vector2 pos);
    Node2D *place_tile(Vector2 aligned_pos, const String &sprite_path = "");
    void remove_tile(Vector2 aligned_pos, bool record = true);

    void undo();
    void redo();

    // Palette UI
    void rebuild_palette();
    void on_palette_click(int index);
    void update_palette_highlight();
};

#endif // SPX_DRAW_TILES_H
