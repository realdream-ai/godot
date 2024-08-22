/**************************************************************************/
/*  gdextension_spx_ext.cpp                                               */
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

#include "gdextension_spx_ext.h"
#include "core/extension/gdextension.h"
#include "core/extension/gdextension_compat_hashes.h"
#include "core/variant/variant.h"
#include "gdextension_interface.h"
#include "scene/main/window.h"
#include "spx_engine.h"
#include "spx_audio_mgr.h"
#include "spx_input_mgr.h"
#include "spx_physic_mgr.h"
#include "spx_sprite_mgr.h"
#include "spx_ui_mgr.h"

#define audioMgr SpxEngine::get_singleton()->get_audio()
#define inputMgr SpxEngine::get_singleton()->get_input()
#define physicMgr SpxEngine::get_singleton()->get_physic()
#define spriteMgr SpxEngine::get_singleton()->get_sprite()
#define uiMgr SpxEngine::get_singleton()->get_ui()

#define REGISTER_SPX_INTERFACE_FUNC(m_name) GDExtension::register_interface_function( #m_name, (GDExtensionInterfaceFunctionPtr)&gdextension_##m_name)
static void gdextension_spx_global_register_callbacks(GDExtensionSpxCallbackInfoPtr callback_ptr) {
	SpxEngine::register_callbacks(callback_ptr);
}
static void gdextension_spx_audio_play_audio(GdString path) {
	 audioMgr->play_audio(path);
}
static void gdextension_spx_audio_set_audio_volume(GdFloat volume) {
	 audioMgr->set_audio_volume(volume);
}
static void gdextension_spx_audio_get_audio_volume(GdFloat* ret_val) {
	*ret_val = audioMgr->get_audio_volume();
}
static void gdextension_spx_audio_is_music_playing(GdBool* ret_val) {
	*ret_val = audioMgr->is_music_playing();
}
static void gdextension_spx_audio_play_music(GdString path) {
	 audioMgr->play_music(path);
}
static void gdextension_spx_audio_set_music_volume(GdFloat volume) {
	 audioMgr->set_music_volume(volume);
}
static void gdextension_spx_audio_get_music_volume(GdFloat* ret_val) {
	*ret_val = audioMgr->get_music_volume();
}
static void gdextension_spx_audio_pause_music() {
	 audioMgr->pause_music();
}
static void gdextension_spx_audio_resume_music() {
	 audioMgr->resume_music();
}
static void gdextension_spx_audio_get_music_timer(GdFloat* ret_val) {
	*ret_val = audioMgr->get_music_timer();
}
static void gdextension_spx_audio_set_music_timer(GdFloat time) {
	 audioMgr->set_music_timer(time);
}
static void gdextension_spx_input_get_mouse_pos(GdVec2* ret_val) {
	*ret_val = inputMgr->get_mouse_pos();
}
static void gdextension_spx_input_get_key(GdInt key,GdBool* ret_val) {
	*ret_val = inputMgr->get_key(key);
}
static void gdextension_spx_input_get_mouse_state(GdInt mouse_id,GdBool* ret_val) {
	*ret_val = inputMgr->get_mouse_state(mouse_id);
}
static void gdextension_spx_input_get_key_state(GdInt key,GdInt* ret_val) {
	*ret_val = inputMgr->get_key_state(key);
}
static void gdextension_spx_input_get_axis(GdString axis,GdFloat* ret_val) {
	*ret_val = inputMgr->get_axis(axis);
}
static void gdextension_spx_input_is_action_pressed(GdString action,GdBool* ret_val) {
	*ret_val = inputMgr->is_action_pressed(action);
}
static void gdextension_spx_input_is_action_just_pressed(GdString action,GdBool* ret_val) {
	*ret_val = inputMgr->is_action_just_pressed(action);
}
static void gdextension_spx_input_is_action_just_released(GdString action,GdBool* ret_val) {
	*ret_val = inputMgr->is_action_just_released(action);
}
static void gdextension_spx_physic_raycast(GdVec2 from,GdVec2 to,GdInt collision_mask,GdObj* ret_val) {
	*ret_val = physicMgr->raycast(from, to, collision_mask);
}
static void gdextension_spx_sprite_create_sprite(GdString path,GdObj* ret_val) {
	*ret_val = spriteMgr->create_sprite(path);
}
static void gdextension_spx_sprite_clone_sprite(GdObj obj,GdObj* ret_val) {
	*ret_val = spriteMgr->clone_sprite(obj);
}
static void gdextension_spx_sprite_destroy_sprite(GdObj obj,GdBool* ret_val) {
	*ret_val = spriteMgr->destroy_sprite(obj);
}
static void gdextension_spx_sprite_is_sprite_alive(GdObj obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_sprite_alive(obj);
}
static void gdextension_spx_sprite_set_position(GdObj obj,GdVec2 pos) {
	 spriteMgr->set_position(obj, pos);
}
static void gdextension_spx_sprite_set_rotation(GdObj obj,GdFloat rot) {
	 spriteMgr->set_rotation(obj, rot);
}
static void gdextension_spx_sprite_set_scale(GdObj obj,GdVec2 scale) {
	 spriteMgr->set_scale(obj, scale);
}
static void gdextension_spx_sprite_get_position(GdObj obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_position(obj);
}
static void gdextension_spx_sprite_get_rotation(GdObj obj,GdFloat* ret_val) {
	*ret_val = spriteMgr->get_rotation(obj);
}
static void gdextension_spx_sprite_get_scale(GdObj obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_scale(obj);
}
static void gdextension_spx_sprite_set_color(GdObj obj,GdColor color) {
	 spriteMgr->set_color(obj, color);
}
static void gdextension_spx_sprite_get_color(GdObj obj,GdColor* ret_val) {
	*ret_val = spriteMgr->get_color(obj);
}
static void gdextension_spx_sprite_set_texture(GdObj obj,GdString path) {
	 spriteMgr->set_texture(obj, path);
}
static void gdextension_spx_sprite_get_texture(GdObj obj,GdString* ret_val) {
	*ret_val = spriteMgr->get_texture(obj);
}
static void gdextension_spx_sprite_set_visible(GdObj obj,GdBool visible) {
	 spriteMgr->set_visible(obj, visible);
}
static void gdextension_spx_sprite_get_visible(GdObj obj,GdBool* ret_val) {
	*ret_val = spriteMgr->get_visible(obj);
}
static void gdextension_spx_sprite_get_z_index(GdObj obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_z_index(obj);
}
static void gdextension_spx_sprite_set_z_index(GdObj obj,GdInt z) {
	 spriteMgr->set_z_index(obj, z);
}
static void gdextension_spx_sprite_play_anim(GdObj obj,GdString p_name,GdFloat p_custom_scale,GdBool p_from_end) {
	 spriteMgr->play_anim(obj, p_name, p_custom_scale, p_from_end);
}
static void gdextension_spx_sprite_play_backwards_anim(GdObj obj,GdString p_name) {
	 spriteMgr->play_backwards_anim(obj, p_name);
}
static void gdextension_spx_sprite_pause_anim(GdObj obj) {
	 spriteMgr->pause_anim(obj);
}
static void gdextension_spx_sprite_stop_anim(GdObj obj) {
	 spriteMgr->stop_anim(obj);
}
static void gdextension_spx_sprite_is_playing_anim(GdObj obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_playing_anim(obj);
}
static void gdextension_spx_sprite_set_anim(GdObj obj,GdString p_name) {
	 spriteMgr->set_anim(obj, p_name);
}
static void gdextension_spx_sprite_get_anim(GdObj obj,GdString* ret_val) {
	*ret_val = spriteMgr->get_anim(obj);
}
static void gdextension_spx_sprite_set_anim_frame(GdObj obj,GdInt p_frame) {
	 spriteMgr->set_anim_frame(obj, p_frame);
}
static void gdextension_spx_sprite_get_anim_frame(GdObj obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_anim_frame(obj);
}
static void gdextension_spx_sprite_set_anim_speed_scale(GdObj obj,GdFloat p_speed_scale) {
	 spriteMgr->set_anim_speed_scale(obj, p_speed_scale);
}
static void gdextension_spx_sprite_get_anim_speed_scale(GdObj obj,GdFloat* ret_val) {
	*ret_val = spriteMgr->get_anim_speed_scale(obj);
}
static void gdextension_spx_sprite_get_anim_playing_speed(GdObj obj,GdFloat* ret_val) {
	*ret_val = spriteMgr->get_anim_playing_speed(obj);
}
static void gdextension_spx_sprite_set_anim_centered(GdObj obj,GdBool p_center) {
	 spriteMgr->set_anim_centered(obj, p_center);
}
static void gdextension_spx_sprite_is_anim_centered(GdObj obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_anim_centered(obj);
}
static void gdextension_spx_sprite_set_anim_offset(GdObj obj,GdVec2 p_offset) {
	 spriteMgr->set_anim_offset(obj, p_offset);
}
static void gdextension_spx_sprite_get_anim_offset(GdObj obj,GdVec2* ret_val) {
	*ret_val = spriteMgr->get_anim_offset(obj);
}
static void gdextension_spx_sprite_set_anim_flip_h(GdObj obj,GdBool p_flip) {
	 spriteMgr->set_anim_flip_h(obj, p_flip);
}
static void gdextension_spx_sprite_is_anim_flipped_h(GdObj obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_anim_flipped_h(obj);
}
static void gdextension_spx_sprite_set_anim_flip_v(GdObj obj,GdBool p_flip) {
	 spriteMgr->set_anim_flip_v(obj, p_flip);
}
static void gdextension_spx_sprite_is_anim_flipped_v(GdObj obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_anim_flipped_v(obj);
}
static void gdextension_spx_sprite_set_gravity(GdObj obj,GdFloat gravity) {
	 spriteMgr->set_gravity(obj, gravity);
}
static void gdextension_spx_sprite_get_gravity(GdObj obj,GdFloat* ret_val) {
	*ret_val = spriteMgr->get_gravity(obj);
}
static void gdextension_spx_sprite_set_mass(GdObj obj,GdFloat mass) {
	 spriteMgr->set_mass(obj, mass);
}
static void gdextension_spx_sprite_get_mass(GdObj obj,GdFloat* ret_val) {
	*ret_val = spriteMgr->get_mass(obj);
}
static void gdextension_spx_sprite_add_force(GdObj obj,GdVec2 force) {
	 spriteMgr->add_force(obj, force);
}
static void gdextension_spx_sprite_add_impulse(GdObj obj,GdVec2 impulse) {
	 spriteMgr->add_impulse(obj, impulse);
}
static void gdextension_spx_sprite_set_collision_layer(GdObj obj,GdInt layer) {
	 spriteMgr->set_collision_layer(obj, layer);
}
static void gdextension_spx_sprite_get_collision_layer(GdObj obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_collision_layer(obj);
}
static void gdextension_spx_sprite_set_collision_mask(GdObj obj,GdInt mask) {
	 spriteMgr->set_collision_mask(obj, mask);
}
static void gdextension_spx_sprite_get_collision_mask(GdObj obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_collision_mask(obj);
}
static void gdextension_spx_sprite_set_trigger_layer(GdObj obj,GdInt layer) {
	 spriteMgr->set_trigger_layer(obj, layer);
}
static void gdextension_spx_sprite_get_trigger_layer(GdObj obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_trigger_layer(obj);
}
static void gdextension_spx_sprite_set_trigger_mask(GdObj obj,GdInt mask) {
	 spriteMgr->set_trigger_mask(obj, mask);
}
static void gdextension_spx_sprite_get_trigger_mask(GdObj obj,GdInt* ret_val) {
	*ret_val = spriteMgr->get_trigger_mask(obj);
}
static void gdextension_spx_sprite_set_collider_rect(GdObj obj,GdVec2 center,GdVec2 size) {
	 spriteMgr->set_collider_rect(obj, center, size);
}
static void gdextension_spx_sprite_set_collider_circle(GdObj obj,GdVec2 center,GdFloat radius) {
	 spriteMgr->set_collider_circle(obj, center, radius);
}
static void gdextension_spx_sprite_set_collider_capsule(GdObj obj,GdVec2 center,GdVec2 size) {
	 spriteMgr->set_collider_capsule(obj, center, size);
}
static void gdextension_spx_sprite_set_collision_enabled(GdObj obj,GdBool enabled) {
	 spriteMgr->set_collision_enabled(obj, enabled);
}
static void gdextension_spx_sprite_is_collision_enabled(GdObj obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_collision_enabled(obj);
}
static void gdextension_spx_sprite_set_trigger_rect(GdObj obj,GdVec2 center,GdVec2 size) {
	 spriteMgr->set_trigger_rect(obj, center, size);
}
static void gdextension_spx_sprite_set_trigger_circle(GdObj obj,GdVec2 center,GdFloat radius) {
	 spriteMgr->set_trigger_circle(obj, center, radius);
}
static void gdextension_spx_sprite_set_trigger_capsule(GdObj obj,GdVec2 center,GdVec2 size) {
	 spriteMgr->set_trigger_capsule(obj, center, size);
}
static void gdextension_spx_sprite_set_trigger_enabled(GdObj obj,GdBool trigger) {
	 spriteMgr->set_trigger_enabled(obj, trigger);
}
static void gdextension_spx_sprite_is_trigger_enabled(GdObj obj,GdBool* ret_val) {
	*ret_val = spriteMgr->is_trigger_enabled(obj);
}
static void gdextension_spx_ui_create_button(GdString path,GdRect2 rect,GdString text,GdInt* ret_val) {
	*ret_val = uiMgr->create_button(path, rect, text);
}
static void gdextension_spx_ui_create_label(GdString path,GdRect2 rect,GdString text,GdInt* ret_val) {
	*ret_val = uiMgr->create_label(path, rect, text);
}
static void gdextension_spx_ui_create_image(GdString path,GdRect2 rect,GdColor color,GdInt* ret_val) {
	*ret_val = uiMgr->create_image(path, rect, color);
}
static void gdextension_spx_ui_create_slider(GdString path,GdRect2 rect,GdFloat value,GdInt* ret_val) {
	*ret_val = uiMgr->create_slider(path, rect, value);
}
static void gdextension_spx_ui_create_toggle(GdString path,GdRect2 rect,GdBool value,GdInt* ret_val) {
	*ret_val = uiMgr->create_toggle(path, rect, value);
}
static void gdextension_spx_ui_create_input(GdString path,GdRect2 rect,GdString text,GdInt* ret_val) {
	*ret_val = uiMgr->create_input(path, rect, text);
}
static void gdextension_spx_ui_get_type(GdObj obj,GdInt* ret_val) {
	*ret_val = uiMgr->get_type(obj);
}
static void gdextension_spx_ui_set_interactable(GdObj obj,GdBool interactable) {
	 uiMgr->set_interactable(obj, interactable);
}
static void gdextension_spx_ui_get_interactable(GdObj obj,GdBool* ret_val) {
	*ret_val = uiMgr->get_interactable(obj);
}
static void gdextension_spx_ui_set_text(GdObj obj,GdString text) {
	 uiMgr->set_text(obj, text);
}
static void gdextension_spx_ui_get_text(GdObj obj,GdString* ret_val) {
	*ret_val = uiMgr->get_text(obj);
}
static void gdextension_spx_ui_set_rect(GdObj obj,GdRect2 rect) {
	 uiMgr->set_rect(obj, rect);
}
static void gdextension_spx_ui_get_rect(GdObj obj,GdRect2* ret_val) {
	*ret_val = uiMgr->get_rect(obj);
}
static void gdextension_spx_ui_set_color(GdObj obj,GdColor color) {
	 uiMgr->set_color(obj, color);
}
static void gdextension_spx_ui_get_color(GdObj obj,GdColor* ret_val) {
	*ret_val = uiMgr->get_color(obj);
}
static void gdextension_spx_ui_set_font_size(GdObj obj,GdFloat size) {
	 uiMgr->set_font_size(obj, size);
}
static void gdextension_spx_ui_get_font_size(GdObj obj,GdFloat* ret_val) {
	*ret_val = uiMgr->get_font_size(obj);
}
static void gdextension_spx_ui_set_visible(GdObj obj,GdBool visible) {
	 uiMgr->set_visible(obj, visible);
}
static void gdextension_spx_ui_get_visible(GdObj obj,GdBool* ret_val) {
	*ret_val = uiMgr->get_visible(obj);
}



void gdextension_spx_setup_interface() {
	REGISTER_SPX_INTERFACE_FUNC(spx_global_register_callbacks);

	REGISTER_SPX_INTERFACE_FUNC(spx_audio_play_audio);
	REGISTER_SPX_INTERFACE_FUNC(spx_audio_set_audio_volume);
	REGISTER_SPX_INTERFACE_FUNC(spx_audio_get_audio_volume);
	REGISTER_SPX_INTERFACE_FUNC(spx_audio_is_music_playing);
	REGISTER_SPX_INTERFACE_FUNC(spx_audio_play_music);
	REGISTER_SPX_INTERFACE_FUNC(spx_audio_set_music_volume);
	REGISTER_SPX_INTERFACE_FUNC(spx_audio_get_music_volume);
	REGISTER_SPX_INTERFACE_FUNC(spx_audio_pause_music);
	REGISTER_SPX_INTERFACE_FUNC(spx_audio_resume_music);
	REGISTER_SPX_INTERFACE_FUNC(spx_audio_get_music_timer);
	REGISTER_SPX_INTERFACE_FUNC(spx_audio_set_music_timer);
	REGISTER_SPX_INTERFACE_FUNC(spx_input_get_mouse_pos);
	REGISTER_SPX_INTERFACE_FUNC(spx_input_get_key);
	REGISTER_SPX_INTERFACE_FUNC(spx_input_get_mouse_state);
	REGISTER_SPX_INTERFACE_FUNC(spx_input_get_key_state);
	REGISTER_SPX_INTERFACE_FUNC(spx_input_get_axis);
	REGISTER_SPX_INTERFACE_FUNC(spx_input_is_action_pressed);
	REGISTER_SPX_INTERFACE_FUNC(spx_input_is_action_just_pressed);
	REGISTER_SPX_INTERFACE_FUNC(spx_input_is_action_just_released);
	REGISTER_SPX_INTERFACE_FUNC(spx_physic_raycast);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_create_sprite);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_clone_sprite);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_destroy_sprite);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_is_sprite_alive);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_position);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_rotation);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_scale);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_position);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_rotation);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_scale);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_color);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_color);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_texture);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_texture);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_visible);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_visible);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_z_index);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_z_index);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_play_anim);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_play_backwards_anim);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_pause_anim);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_stop_anim);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_is_playing_anim);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_anim);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_anim);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_anim_frame);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_anim_frame);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_anim_speed_scale);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_anim_speed_scale);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_anim_playing_speed);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_anim_centered);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_is_anim_centered);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_anim_offset);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_anim_offset);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_anim_flip_h);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_is_anim_flipped_h);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_anim_flip_v);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_is_anim_flipped_v);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_gravity);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_gravity);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_mass);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_mass);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_add_force);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_add_impulse);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_collision_layer);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_collision_layer);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_collision_mask);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_collision_mask);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_trigger_layer);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_trigger_layer);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_trigger_mask);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_get_trigger_mask);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_collider_rect);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_collider_circle);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_collider_capsule);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_collision_enabled);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_is_collision_enabled);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_trigger_rect);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_trigger_circle);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_trigger_capsule);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_set_trigger_enabled);
	REGISTER_SPX_INTERFACE_FUNC(spx_sprite_is_trigger_enabled);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_create_button);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_create_label);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_create_image);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_create_slider);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_create_toggle);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_create_input);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_get_type);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_set_interactable);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_get_interactable);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_set_text);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_get_text);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_set_rect);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_get_rect);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_set_color);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_get_color);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_set_font_size);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_get_font_size);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_set_visible);
	REGISTER_SPX_INTERFACE_FUNC(spx_ui_get_visible);
	
}
