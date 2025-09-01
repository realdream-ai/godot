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
    ClassDB::bind_method(D_METHOD("add_sprite_path", "path"), &SpxDrawTiles::add_sprite_path);
    ClassDB::bind_method(D_METHOD("switch_tile", "index"), &SpxDrawTiles::switch_tile);
    ClassDB::bind_method(D_METHOD("undo"), &SpxDrawTiles::undo);
    ClassDB::bind_method(D_METHOD("redo"), &SpxDrawTiles::redo);
}

void SpxDrawTiles::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) _ready();
    if (p_what == NOTIFICATION_PROCESS) _process(get_process_delta_time());
}

void SpxDrawTiles::_ready() {
    set_process(true);
    set_process_input(true);

    preview_sprite = memnew(Sprite2D);
    preview_sprite->set_modulate(Color(1,1,1,0.5));
    preview_sprite->set_z_index(101);
    add_child(preview_sprite);
}

void SpxDrawTiles::_process(double delta) {
    Vector2 mouse_pos = get_global_mouse_position();
    Vector2 aligned_pos(
        Math::floor(mouse_pos.x / tile_size.x) * tile_size.x,
        Math::floor(mouse_pos.y / tile_size.y) * tile_size.y
    );

    if (preview_sprite) preview_sprite->set_position(aligned_pos);

    if (drawing) place_tile(aligned_pos);
    else if (deleting) remove_tile(aligned_pos);
}

void SpxDrawTiles::input(const Ref<InputEvent> &event) {
    Ref<InputEventMouseButton> mb = event;
    if (mb.is_valid()) {
        Vector2 click_pos = mb->get_global_position();
        if (mb->is_pressed()) {
            for (int i = 0; i < palette_sprites.size(); i++) {
                Sprite2D *s = Object::cast_to<Sprite2D>(palette_sprites[i]);
                if (!s) continue;
                Rect2 rect(s->get_global_position() - s->get_texture()->get_size() * 0.5 * s->get_global_scale(),
                           s->get_texture()->get_size() * s->get_global_scale());
                if (rect.has_point(click_pos)) {
                    on_palette_click(i);
                    return;
                }
            }

            if (mb->get_button_index() == MouseButton::LEFT) drawing = true;
            else if (mb->get_button_index() == MouseButton::RIGHT) deleting = true;
        } else {
            if (mb->get_button_index() == MouseButton::LEFT) drawing = false;
            else if (mb->get_button_index() == MouseButton::RIGHT) deleting = false;
        }
    }

    Ref<InputEventKey> key = event;
    if (key.is_valid() && key->is_pressed()) {
        if (key->get_keycode() == Key::Z && Input::get_singleton()->is_key_pressed(Key::CTRL)) undo();
        else if (key->get_keycode() == Key::Y && Input::get_singleton()->is_key_pressed(Key::CTRL)) redo();
    }
}

Node2D *SpxDrawTiles::_get_tile_at_position(Vector2 pos) {
    PhysicsDirectSpaceState2D::PointParameters params;
    params.position = pos + tile_size * 0.5;
    params.collision_mask = 0xFFFFFFFF;
    params.exclude.clear();
    params.collide_with_bodies = true;
    params.collide_with_areas = false;

    PhysicsDirectSpaceState2D::ShapeResult results[1];
    PhysicsDirectSpaceState2D *space_state = get_world_2d()->get_direct_space_state();
    int hit_count = space_state->intersect_point(params, results, 1);

    if (hit_count > 0) {
        return Object::cast_to<Node2D>(results[0].collider);
    }
    return nullptr;
}

bool SpxDrawTiles::has_tile_at(Vector2 pos) {
    return _get_tile_at_position(pos) != nullptr;
}

// ================= API =================

void SpxDrawTiles::add_sprite_path(const String &path) {
    Ref<Texture2D> tex = ResourceLoader::load(path);
    if (!tex.is_valid()) {
        ERR_PRINT("Failed to load sprite: path = " + path);
        return;
    }

    Dictionary info;
    info["texture"] = tex;
    info["path"] = path;
    tile_textures.append(info);

    if (tile_textures.size() == 1) {
        tile_size = tex->get_size();
        if (preview_sprite) preview_sprite->set_texture(tex);
    }

    rebuild_palette();
}

void SpxDrawTiles::switch_tile(int index) {
    if (index < 0 || index >= tile_textures.size()) return;
    current_tile = index;
    Dictionary info = tile_textures[current_tile];
    Ref<Texture2D> tex = info["texture"];
    if (preview_sprite) preview_sprite->set_texture(tex);
    update_palette_highlight();
}

// ================= Tile =================

Node2D *SpxDrawTiles::place_tile(Vector2 aligned_pos, const String &sprite_path) {
    if (has_tile_at(aligned_pos)) return nullptr;

    String path_to_use = sprite_path;
    Ref<Texture2D> tex;
    if (sprite_path.is_empty()) {
        Dictionary info = tile_textures[current_tile];
        tex = info["texture"];
        path_to_use = info["path"];
    } else {
        tex = ResourceLoader::load(path_to_use);
        if (!tex.is_valid()) return nullptr;
    }

    StaticBody2D *tile = memnew(StaticBody2D);
    Sprite2D *sprite = memnew(Sprite2D);
    sprite->set_name("SpxSprite2D");
    sprite->set_texture(tex);
    tile->add_child(sprite);

    CollisionShape2D *collision = memnew(CollisionShape2D);
    Ref<RectangleShape2D> shape = memnew(RectangleShape2D);
    shape->set_size(tile_size);
    collision->set_shape(shape);
    collision->set_position(tile_size * 0.5);
    tile->add_child(collision);

    tile->set_position(aligned_pos);
    add_child(tile);

    Dictionary op;
    op["type"] = "add";
    op["node"] = tile;
    op["sprite_path"] = path_to_use;
    op["position"] = aligned_pos;
    undo_stack.append(op);
    redo_stack.clear();

    return tile;
}

void SpxDrawTiles::remove_tile(Vector2 aligned_pos, bool record) {
    Node2D *tile = _get_tile_at_position(aligned_pos);
    if (!tile) return;

    if (record) {
        Node *node = tile->get_node(NodePath("SpxSprite2D"));
        Sprite2D *sprite = Object::cast_to<Sprite2D>(node);
        String path = "";
        if (sprite && sprite->get_texture().is_valid()) {
            for (int j = 0; j < tile_textures.size(); j++) {
                Dictionary info = tile_textures[j];
                if (info["texture"] == sprite->get_texture()) {
                    path = info["path"];
                    break;
                }
            }
        }

        Dictionary op;
        op["type"] = "remove";
        op["node"] = tile;
        op["sprite_path"] = path;
        op["position"] = tile->get_position();
        undo_stack.append(op);
        redo_stack.clear();
    }

    tile->queue_free();
}

// ================= 撤销/重做 =================

void SpxDrawTiles::undo() {
    if (undo_stack.is_empty()) return;
    Dictionary op = undo_stack.back();
    undo_stack.remove_at(undo_stack.size() - 1);

    String type = op["type"];
    Node2D *node = Object::cast_to<Node2D>(op["node"]);

    if (type == "add") {
        if (node) node->queue_free();
        redo_stack.append(op);
    } else if (type == "remove") {
        Node2D *new_tile = place_tile(op["position"], op["sprite_path"]);
        if (new_tile) {
            Dictionary redo_op = op;
            redo_op["node"] = new_tile;
            redo_stack.append(redo_op);
        }
    }
}

void SpxDrawTiles::redo() {
    if (redo_stack.is_empty()) return;
    Dictionary op = redo_stack.back();
    redo_stack.remove_at(redo_stack.size() - 1);

    String type = op["type"];
    Node2D *node = Object::cast_to<Node2D>(op["node"]);

    if (type == "add") {
        Node2D *new_tile = place_tile(op["position"], op["sprite_path"]);
        if (new_tile) {
            Dictionary undo_op = op;
            undo_op["node"] = new_tile;
            undo_stack.append(undo_op);
        }
    } else if (type == "remove") {
        if (node) remove_tile(node->get_position(), false);
        undo_stack.append(op);
    }
}

// ================= Palette UI =================

void SpxDrawTiles::rebuild_palette() {
    for (int i = 0; i < palette_sprites.size(); i++) {
        Node2D *n = Object::cast_to<Node2D>(palette_sprites[i]);
        if (n) n->queue_free();
    }

    for (int i = 0; i < palette_highlight_rects.size(); i++) {
        Node2D *n = Object::cast_to<Node2D>(palette_highlight_rects[i]);
        if (n) n->queue_free();
    }

    palette_sprites.clear();
    palette_highlight_rects.clear();

    Rect2 viewport = get_viewport_rect(); 
    camera = Object::cast_to<Camera2D>(get_tree()->get_current_scene()->get_node(NodePath("Camera2D")));
    Vector2 cam_pos = camera ? camera->get_global_position() : Vector2(viewport.size.x/2, viewport.size.y/2);
    Vector2 screen_top_right = cam_pos + Vector2(viewport.size.x/2, -viewport.size.y/2); 

    float offset_x = 20; 
    float offset_y = 20; 

    for (int i = 0; i < tile_textures.size(); i++) {
        Dictionary info = tile_textures[i];
        Ref<Texture2D> tex = info["texture"];

        Sprite2D *s = memnew(Sprite2D);
        s->set_texture(tex);

        float x = screen_top_right.x - offset_x - (i+1)*(palette_size + palette_margin);
        float y = screen_top_right.y + offset_y;
        s->set_position(Vector2(x, y));

        float scale = (float)palette_size / MAX(tex->get_size().x, tex->get_size().y);
        s->set_scale(Vector2(scale, scale));

        add_child(s);
        palette_sprites.append(s);

        ColorRect *highlight = memnew(ColorRect);
        highlight->set_color(Color(1,1,0,0.5));
        highlight->set_size(Vector2(palette_size, palette_size));
        highlight->set_position(s->get_position() - Vector2(palette_size/2, palette_size/2));
        highlight->set_visible(i == current_tile);
        add_child(highlight);
        palette_highlight_rects.append(highlight);
    }

}

void SpxDrawTiles::on_palette_click(int index) {
    switch_tile(index);
}

void SpxDrawTiles::update_palette_highlight() {
    for (int i = 0; i < palette_highlight_rects.size(); i++) {
        ColorRect *r = Object::cast_to<ColorRect>(palette_highlight_rects[i]);
        if (r) r->set_visible(i == current_tile);
    }
}