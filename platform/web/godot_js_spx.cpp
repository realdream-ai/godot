/**************************************************************************/
/*  godot_js_spx.cpp                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "core/extension/gdextension_spx_ext.h"
#include "core/extension/gdextension.h"
#include "core/extension/gdextension_compat_hashes.h"
#include "core/variant/variant.h"
#include "core/extension/gdextension_interface.h"
#include "scene/main/window.h"
#include "core/extension/spx_engine.h"
#include "core/extension/spx_audio_mgr.h"
#include "core/extension/spx_camera_mgr.h"
#include "core/extension/spx_input_mgr.h"
#include "core/extension/spx_physic_mgr.h"
#include "core/extension/spx_platform_mgr.h"
#include "core/extension/spx_scene_mgr.h"
#include "core/extension/spx_sprite_mgr.h"
#include "core/extension/spx_ui_mgr.h"

#include <emscripten.h>
#define audioMgr SpxEngine::get_singleton()->get_audio()
#define cameraMgr SpxEngine::get_singleton()->get_camera()
#define inputMgr SpxEngine::get_singleton()->get_input()
#define physicMgr SpxEngine::get_singleton()->get_physic()
#define platformMgr SpxEngine::get_singleton()->get_platform()
#define sceneMgr SpxEngine::get_singleton()->get_scene()
#define spriteMgr SpxEngine::get_singleton()->get_sprite()
#define uiMgr SpxEngine::get_singleton()->get_ui()


extern "C" {
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_stop_all() {
	 audioMgr->stop_all();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_play_sfx(GdString* path) {
	 audioMgr->play_sfx(*path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_play_music(GdString* path) {
	 audioMgr->play_music(*path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_pause_music() {
	 audioMgr->pause_music();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_resume_music() {
	 audioMgr->resume_music();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_get_music_timer(GdFloat* ret_val) {
	*ret_val = audioMgr->get_music_timer();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_set_music_timer(GdFloat* time) {
	 audioMgr->set_music_timer(*time);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_is_music_playing(GdBool* ret_val) {
	*ret_val = audioMgr->is_music_playing();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_set_sfx_volume(GdFloat* volume) {
	 audioMgr->set_sfx_volume(*volume);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_get_sfx_volume(GdFloat* ret_val) {
	*ret_val = audioMgr->get_sfx_volume();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_set_music_volume(GdFloat* volume) {
	 audioMgr->set_music_volume(*volume);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_get_music_volume(GdFloat* ret_val) {
	*ret_val = audioMgr->get_music_volume();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_set_master_volume(GdFloat* volume) {
	 audioMgr->set_master_volume(*volume);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_audio_get_master_volume(GdFloat* ret_val) {
	*ret_val = audioMgr->get_master_volume();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_camera_get_camera_position(GdVec2* ret_val) {
	*ret_val = cameraMgr->get_camera_position();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_camera_set_camera_position(GdVec2* position) {
	 cameraMgr->set_camera_position(*position);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_camera_get_camera_zoom(GdVec2* ret_val) {
	*ret_val = cameraMgr->get_camera_zoom();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_camera_set_camera_zoom(GdVec2* size) {
	 cameraMgr->set_camera_zoom(*size);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_camera_get_viewport_rect(GdRect2* ret_val) {
	*ret_val = cameraMgr->get_viewport_rect();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_input_get_mouse_pos(GdVec2* ret_val) {
	*ret_val = inputMgr->get_mouse_pos();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_input_get_key(GdInt* key,GdBool* ret_val) {
	*ret_val = inputMgr->get_key(*key);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_input_get_mouse_state(GdInt* mouse_id,GdBool* ret_val) {
	*ret_val = inputMgr->get_mouse_state(*mouse_id);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_input_get_key_state(GdInt* key,GdInt* ret_val) {
	*ret_val = inputMgr->get_key_state(*key);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_input_get_axis(GdString* neg_action,GdString* pos_action,GdFloat* ret_val) {
	*ret_val = inputMgr->get_axis(*neg_action, *pos_action);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_input_is_action_pressed(GdString* action,GdBool* ret_val) {
	*ret_val = inputMgr->is_action_pressed(*action);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_input_is_action_just_pressed(GdString* action,GdBool* ret_val) {
	*ret_val = inputMgr->is_action_just_pressed(*action);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_input_is_action_just_released(GdString* action,GdBool* ret_val) {
	*ret_val = inputMgr->is_action_just_released(*action);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_physic_raycast(GdVec2* from,GdVec2* to,GdInt* collision_mask,GdObj* ret_val) {
	*ret_val = physicMgr->raycast(*from, *to, *collision_mask);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_physic_check_collision(GdVec2* from,GdVec2* to,GdInt* collision_mask,GdBool* collide_with_areas,GdBool* collide_with_bodies,GdBool* ret_val) {
	*ret_val = physicMgr->check_collision(*from, *to, *collision_mask, *collide_with_areas, *collide_with_bodies);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_platform_set_window_size(GdInt* width,GdInt* height) {
	 platformMgr->set_window_size(*width, *height);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_platform_get_window_size(GdVec2* ret_val) {
	*ret_val = platformMgr->get_window_size();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_platform_set_window_title(GdString* title) {
	 platformMgr->set_window_title(*title);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_platform_get_window_title(GdString* ret_val) {
	*ret_val = platformMgr->get_window_title();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_platform_set_window_fullscreen(GdBool* enable) {
	 platformMgr->set_window_fullscreen(*enable);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_platform_is_window_fullscreen(GdBool* ret_val) {
	*ret_val = platformMgr->is_window_fullscreen();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_platform_set_debug_mode(GdBool* enable) {
	 platformMgr->set_debug_mode(*enable);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_platform_is_debug_mode(GdBool* ret_val) {
	*ret_val = platformMgr->is_debug_mode();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_scene_change_scene_to_file(GdString* path) {
	 sceneMgr->change_scene_to_file(*path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_scene_reload_current_scene(GdInt* ret_val) {
	*ret_val = sceneMgr->reload_current_scene();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_scene_unload_current_scene() {
	 sceneMgr->unload_current_scene();
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_dont_destroy_on_load(GdObj* obj) {
	 spriteMgr->set_dont_destroy_on_load(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_process(GdObj* obj,GdBool* is_on) {
	 spriteMgr->set_process(*obj, *is_on);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_physic_process(GdObj* obj,GdBool* is_on) {
	 spriteMgr->set_physic_process(*obj, *is_on);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_child_position(GdObj* obj,GdString* path,GdVec2* pos) {
	 spriteMgr->set_child_position(*obj, *path, *pos);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_child_position(GdObj* obj,GdString* path,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_child_position(*obj, *path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_child_rotation(GdObj* obj,GdString* path,GdFloat* rot) {
	 spriteMgr->set_child_rotation(*obj, *path, *rot);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_child_rotation(GdObj* obj,GdString* path,GdFloat* ret_val) {
	*ret_val = spriteMgr->get_child_rotation(*obj, *path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_child_scale(GdObj* obj,GdString* path,GdVec2* scale) {
	 spriteMgr->set_child_scale(*obj, *path, *scale);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_child_scale(GdObj* obj,GdString* path,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_child_scale(*obj, *path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_check_collision(GdObj* obj,GdObj* target,GdBool* is_src_trigger,GdBool* is_dst_trigger,GdBool* ret_val) {
	*ret_val = spriteMgr->check_collision(*obj, *target, *is_src_trigger, *is_dst_trigger);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_check_collision_with_point(GdObj* obj,GdVec2* point,GdBool* is_trigger,GdBool* ret_val) {
	*ret_val = spriteMgr->check_collision_with_point(*obj, *point, *is_trigger);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_create_sprite(GdString* path,GdObj* ret_val) {
	*ret_val = spriteMgr->create_sprite(*path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_clone_sprite(GdObj* obj,GdObj* ret_val) {
	*ret_val = spriteMgr->clone_sprite(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_destroy_sprite(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->destroy_sprite(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_sprite_alive(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_sprite_alive(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_position(GdObj* obj,GdVec2* pos) {
	 spriteMgr->set_position(*obj, *pos);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_position(GdObj* obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_position(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_rotation(GdObj* obj,GdFloat* rot) {
	 spriteMgr->set_rotation(*obj, *rot);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_rotation(GdObj* obj,GdFloat* ret_val) {
	*ret_val = spriteMgr->get_rotation(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_scale(GdObj* obj,GdVec2* scale) {
	 spriteMgr->set_scale(*obj, *scale);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_scale(GdObj* obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_scale(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_color(GdObj* obj,GdColor* color) {
	 spriteMgr->set_color(*obj, *color);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_color(GdObj* obj,GdColor* ret_val) {
	*ret_val = spriteMgr->get_color(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_texture(GdObj* obj,GdString* path) {
	 spriteMgr->set_texture(*obj, *path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_texture(GdObj* obj,GdString* ret_val) {
	*ret_val = spriteMgr->get_texture(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_visible(GdObj* obj,GdBool* visible) {
	 spriteMgr->set_visible(*obj, *visible);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_visible(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->get_visible(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_z_index(GdObj* obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_z_index(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_z_index(GdObj* obj,GdInt* z) {
	 spriteMgr->set_z_index(*obj, *z);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_play_anim(GdObj* obj,GdString* p_name,GdFloat* p_custom_scale,GdBool* p_from_end) {
	 spriteMgr->play_anim(*obj, *p_name, *p_custom_scale, *p_from_end);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_play_backwards_anim(GdObj* obj,GdString* p_name) {
	 spriteMgr->play_backwards_anim(*obj, *p_name);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_pause_anim(GdObj* obj) {
	 spriteMgr->pause_anim(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_stop_anim(GdObj* obj) {
	 spriteMgr->stop_anim(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_playing_anim(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_playing_anim(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_anim(GdObj* obj,GdString* p_name) {
	 spriteMgr->set_anim(*obj, *p_name);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_anim(GdObj* obj,GdString* ret_val) {
	*ret_val = spriteMgr->get_anim(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_anim_frame(GdObj* obj,GdInt* p_frame) {
	 spriteMgr->set_anim_frame(*obj, *p_frame);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_anim_frame(GdObj* obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_anim_frame(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_anim_speed_scale(GdObj* obj,GdFloat* p_speed_scale) {
	 spriteMgr->set_anim_speed_scale(*obj, *p_speed_scale);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_anim_speed_scale(GdObj* obj,GdFloat* ret_val) {
	*ret_val = spriteMgr->get_anim_speed_scale(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_anim_playing_speed(GdObj* obj,GdFloat* ret_val) {
	*ret_val = spriteMgr->get_anim_playing_speed(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_anim_centered(GdObj* obj,GdBool* p_center) {
	 spriteMgr->set_anim_centered(*obj, *p_center);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_anim_centered(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_anim_centered(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_anim_offset(GdObj* obj,GdVec2* p_offset) {
	 spriteMgr->set_anim_offset(*obj, *p_offset);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_anim_offset(GdObj* obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_anim_offset(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_anim_flip_h(GdObj* obj,GdBool* p_flip) {
	 spriteMgr->set_anim_flip_h(*obj, *p_flip);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_anim_flipped_h(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_anim_flipped_h(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_anim_flip_v(GdObj* obj,GdBool* p_flip) {
	 spriteMgr->set_anim_flip_v(*obj, *p_flip);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_anim_flipped_v(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_anim_flipped_v(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_velocity(GdObj* obj,GdVec2* velocity) {
	 spriteMgr->set_velocity(*obj, *velocity);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_velocity(GdObj* obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_velocity(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_on_floor(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_on_floor(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_on_floor_only(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_on_floor_only(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_on_wall(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_on_wall(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_on_wall_only(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_on_wall_only(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_on_ceiling(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_on_ceiling(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_on_ceiling_only(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_on_ceiling_only(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_last_motion(GdObj* obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_last_motion(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_position_delta(GdObj* obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_position_delta(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_floor_normal(GdObj* obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_floor_normal(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_wall_normal(GdObj* obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_wall_normal(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_real_velocity(GdObj* obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_real_velocity(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_move_and_slide(GdObj* obj) {
	 spriteMgr->move_and_slide(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_gravity(GdObj* obj,GdFloat* gravity) {
	 spriteMgr->set_gravity(*obj, *gravity);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_gravity(GdObj* obj,GdFloat* ret_val) {
	*ret_val = spriteMgr->get_gravity(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_mass(GdObj* obj,GdFloat* mass) {
	 spriteMgr->set_mass(*obj, *mass);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_mass(GdObj* obj,GdFloat* ret_val) {
	*ret_val = spriteMgr->get_mass(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_add_force(GdObj* obj,GdVec2* force) {
	 spriteMgr->add_force(*obj, *force);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_add_impulse(GdObj* obj,GdVec2* impulse) {
	 spriteMgr->add_impulse(*obj, *impulse);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_collision_layer(GdObj* obj,GdInt* layer) {
	 spriteMgr->set_collision_layer(*obj, *layer);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_collision_layer(GdObj* obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_collision_layer(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_collision_mask(GdObj* obj,GdInt* mask) {
	 spriteMgr->set_collision_mask(*obj, *mask);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_collision_mask(GdObj* obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_collision_mask(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_trigger_layer(GdObj* obj,GdInt* layer) {
	 spriteMgr->set_trigger_layer(*obj, *layer);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_trigger_layer(GdObj* obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_trigger_layer(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_trigger_mask(GdObj* obj,GdInt* mask) {
	 spriteMgr->set_trigger_mask(*obj, *mask);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_get_trigger_mask(GdObj* obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_trigger_mask(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_collider_rect(GdObj* obj,GdVec2* center,GdVec2* size) {
	 spriteMgr->set_collider_rect(*obj, *center, *size);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_collider_circle(GdObj* obj,GdVec2* center,GdFloat* radius) {
	 spriteMgr->set_collider_circle(*obj, *center, *radius);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_collider_capsule(GdObj* obj,GdVec2* center,GdVec2* size) {
	 spriteMgr->set_collider_capsule(*obj, *center, *size);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_collision_enabled(GdObj* obj,GdBool* enabled) {
	 spriteMgr->set_collision_enabled(*obj, *enabled);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_collision_enabled(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_collision_enabled(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_trigger_rect(GdObj* obj,GdVec2* center,GdVec2* size) {
	 spriteMgr->set_trigger_rect(*obj, *center, *size);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_trigger_circle(GdObj* obj,GdVec2* center,GdFloat* radius) {
	 spriteMgr->set_trigger_circle(*obj, *center, *radius);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_trigger_capsule(GdObj* obj,GdVec2* center,GdVec2* size) {
	 spriteMgr->set_trigger_capsule(*obj, *center, *size);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_set_trigger_enabled(GdObj* obj,GdBool* trigger) {
	 spriteMgr->set_trigger_enabled(*obj, *trigger);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_sprite_is_trigger_enabled(GdObj* obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_trigger_enabled(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_bind_node(GdObj* obj,GdString* rel_path,GdObj* ret_val) {
	*ret_val = uiMgr->bind_node(*obj, *rel_path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_create_node(GdString* path,GdObj* ret_val) {
	*ret_val = uiMgr->create_node(*path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_create_button(GdString* path,GdString* text,GdObj* ret_val) {
	*ret_val = uiMgr->create_button(*path, *text);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_create_label(GdString* path,GdString* text,GdObj* ret_val) {
	*ret_val = uiMgr->create_label(*path, *text);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_create_image(GdString* path,GdObj* ret_val) {
	*ret_val = uiMgr->create_image(*path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_create_toggle(GdString* path,GdBool* value,GdObj* ret_val) {
	*ret_val = uiMgr->create_toggle(*path, *value);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_create_slider(GdString* path,GdFloat* value,GdObj* ret_val) {
	*ret_val = uiMgr->create_slider(*path, *value);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_create_input(GdString* path,GdString* text,GdObj* ret_val) {
	*ret_val = uiMgr->create_input(*path, *text);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_destroy_node(GdObj* obj,GdBool* ret_val) {
	*ret_val = uiMgr->destroy_node(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_type(GdObj* obj,GdInt* ret_val) {
	*ret_val = uiMgr->get_type(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_text(GdObj* obj,GdString* text) {
	 uiMgr->set_text(*obj, *text);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_text(GdObj* obj,GdString* ret_val) {
	*ret_val = uiMgr->get_text(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_texture(GdObj* obj,GdString* path) {
	 uiMgr->set_texture(*obj, *path);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_texture(GdObj* obj,GdString* ret_val) {
	*ret_val = uiMgr->get_texture(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_color(GdObj* obj,GdColor* color) {
	 uiMgr->set_color(*obj, *color);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_color(GdObj* obj,GdColor* ret_val) {
	*ret_val = uiMgr->get_color(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_font_size(GdObj* obj,GdInt* size) {
	 uiMgr->set_font_size(*obj, *size);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_font_size(GdObj* obj,GdInt* ret_val) {
	*ret_val = uiMgr->get_font_size(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_visible(GdObj* obj,GdBool* visible) {
	 uiMgr->set_visible(*obj, *visible);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_visible(GdObj* obj,GdBool* ret_val) {
	*ret_val = uiMgr->get_visible(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_interactable(GdObj* obj,GdBool* interactable) {
	 uiMgr->set_interactable(*obj, *interactable);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_interactable(GdObj* obj,GdBool* ret_val) {
	*ret_val = uiMgr->get_interactable(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_rect(GdObj* obj,GdRect2* rect) {
	 uiMgr->set_rect(*obj, *rect);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_rect(GdObj* obj,GdRect2* ret_val) {
	*ret_val = uiMgr->get_rect(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_layout_direction(GdObj* obj,GdInt* ret_val) {
	*ret_val = uiMgr->get_layout_direction(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_layout_direction(GdObj* obj,GdInt* value) {
	 uiMgr->set_layout_direction(*obj, *value);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_layout_mode(GdObj* obj,GdInt* ret_val) {
	*ret_val = uiMgr->get_layout_mode(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_layout_mode(GdObj* obj,GdInt* value) {
	 uiMgr->set_layout_mode(*obj, *value);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_anchors_preset(GdObj* obj,GdInt* ret_val) {
	*ret_val = uiMgr->get_anchors_preset(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_anchors_preset(GdObj* obj,GdInt* value) {
	 uiMgr->set_anchors_preset(*obj, *value);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_scale(GdObj* obj,GdVec2* ret_val) {
	*ret_val = uiMgr->get_scale(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_scale(GdObj* obj,GdVec2* value) {
	 uiMgr->set_scale(*obj, *value);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_position(GdObj* obj,GdVec2* ret_val) {
	*ret_val = uiMgr->get_position(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_position(GdObj* obj,GdVec2* value) {
	 uiMgr->set_position(*obj, *value);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_size(GdObj* obj,GdVec2* ret_val) {
	*ret_val = uiMgr->get_size(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_size(GdObj* obj,GdVec2* value) {
	 uiMgr->set_size(*obj, *value);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_global_position(GdObj* obj,GdVec2* ret_val) {
	*ret_val = uiMgr->get_global_position(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_global_position(GdObj* obj,GdVec2* value) {
	 uiMgr->set_global_position(*obj, *value);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_get_rotation(GdObj* obj,GdFloat* ret_val) {
	*ret_val = uiMgr->get_rotation(*obj);
}
EMSCRIPTEN_KEEPALIVE
void gdspx_ui_set_rotation(GdObj* obj,GdFloat* value) {
	 uiMgr->set_rotation(*obj, *value);
}

}