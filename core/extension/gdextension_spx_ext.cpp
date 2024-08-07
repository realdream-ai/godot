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
#include "spx_audio_mgr.h"
#include "spx_engine.h"
#include "spx_input_mgr.h"
#include "spx_physic_mgr.h"
#include "spx_sprite_mgr.h"
#include "spx_ui_mgr.h"

#define inputMgr SpxEngine::get_singleton()->get_input()
#define audioMgr SpxEngine::get_singleton()->get_audio()
#define physicMgr SpxEngine::get_singleton()->get_physic()
#define spriteMgr SpxEngine::get_singleton()->get_sprite()
#define uiMgr SpxEngine::get_singleton()->get_ui()

static void gdextension_spx_global_register_callbacks(GDExtensionSpxCallbackInfoPtr callback_ptr) {
	SpxEngine::register_callbacks(callback_ptr);
}
// Input
static GdVec2 gdextension_spx_input_get_mouse_pos() { return inputMgr->get_mouse_pos(); }
static GdBool gdextension_spx_input_get_mouse_state(GdObj id) { return inputMgr->get_mouse_state(id); }
static GdInt gdextension_spx_input_get_key_state(GdInt key) { return inputMgr->get_key_state(key); }
static GdFloat gdextension_spx_input_get_axis(GdString axis) { return inputMgr->get_axis(axis); }
static GdBool gdextension_spx_input_is_action_pressed(GdString action) { return inputMgr->is_action_pressed(action); }
static GdBool gdextension_spx_input_is_action_just_pressed(GdString action) { return inputMgr->is_action_just_pressed(action); }
static GdBool gdextension_spx_input_is_action_just_released(GdString action) { return inputMgr->is_action_just_released(action); }
// Audio
static void gdextension_spx_audio_play_audio(GdString path) { audioMgr->play_audio(path); }
static void gdextension_spx_audio_set_audio_volume(GdFloat volume) { audioMgr->set_audio_volume(volume); }
static GdFloat gdextension_spx_audio_get_audio_volume() { return audioMgr->get_audio_volume(); }
static GdBool gdextension_spx_audio_is_music_playing() { return audioMgr->is_music_playing(); }
static void gdextension_spx_audio_play_music(GdString path) { audioMgr->play_music(path); }
static void gdextension_spx_audio_set_music_volume(GdFloat volume) { audioMgr->set_music_volume(volume); }
static GdFloat gdextension_spx_audio_get_music_volume() { return audioMgr->get_music_volume(); }
static void gdextension_spx_audio_pause_music() { audioMgr->pause_music(); }
static void gdextension_spx_audio_resume_music() { audioMgr->resume_music(); }
static GdFloat gdextension_spx_audio_get_music_timer() { return audioMgr->get_music_timer(); }
static void gdextension_spx_audio_set_music_timer(GdFloat time) { audioMgr->set_music_timer(time); }
// Physic
static void gdextension_spx_physic_set_gravity(GdFloat gravity) { physicMgr->set_gravity(gravity); }
static GdFloat gdextension_spx_physic_get_gravity() { return physicMgr->get_gravity(); }
static void gdextension_spx_physic_set_velocity(GdObj id, GdVec2 velocity) { physicMgr->set_velocity(id, velocity); }
static GdVec2 gdextension_spx_physic_get_velocity(GdObj id) { return physicMgr->get_velocity(id); }
static void gdextension_spx_physic_set_mass(GdObj id, GdFloat mass) { physicMgr->set_mass(id, mass); }
static GdFloat gdextension_spx_physic_get_mass(GdObj id) { return physicMgr->get_mass(id); }
static void gdextension_spx_physic_add_force(GdObj id, GdVec2 force) { physicMgr->add_force(id, force); }
static void gdextension_spx_physic_add_impulse(GdObj id, GdVec2 impulse) { physicMgr->add_impulse(id, impulse); }
static void gdextension_spx_physic_set_collision_layer(GdObj id, GdInt layer) { physicMgr->set_collision_layer(id, layer); }
static GdInt gdextension_spx_physic_get_collision_layer(GdObj id) { return physicMgr->get_collision_layer(id); }
static void gdextension_spx_physic_set_collision_mask(GdObj id, GdInt mask) { physicMgr->set_collision_mask(id, mask); }
static GdInt gdextension_spx_physic_get_collision_mask(GdObj id) { return physicMgr->get_collision_mask(id); }
static GdInt gdextension_spx_physic_get_collider_type(GdObj id) { return physicMgr->get_collider_type(id); }
static void gdextension_spx_physic_add_collider_rect(GdObj id, GdVec2 center, GdVec2 size) { physicMgr->add_collider_rect(id, center, size); }
static void gdextension_spx_physic_add_collider_circle(GdObj id, GdVec2 center, GdFloat radius) { physicMgr->add_collider_circle(id, center, radius); }
static void gdextension_spx_physic_add_collider_capsule(GdObj id, GdVec2 center, GdVec2 size) { physicMgr->add_collider_capsule(id, center, size); }
static void gdextension_spx_physic_set_trigger(GdObj id, GdBool trigger) { physicMgr->set_trigger(id, trigger); }
static GdBool gdextension_spx_physic_is_trigger(GdObj id) { return physicMgr->is_trigger(id); }
static void gdextension_spx_physic_set_collision_enabled(GdObj id, GdBool enabled) { physicMgr->set_collision_enabled(id, enabled); }
static GdBool gdextension_spx_physic_is_collision_enabled(GdObj id) { return physicMgr->is_collision_enabled(id); }
// Sprite
static GdInt gdextension_spx_sprite_create_sprite(GdString path) { return spriteMgr->create_sprite(path); }
static GdInt gdextension_spx_sprite_clone_sprite(GdObj id) { return spriteMgr->clone_sprite(id); }
static GdBool gdextension_spx_sprite_destroy_sprite(GdObj id) { return spriteMgr->destroy_sprite(id); }
static GdBool gdextension_spx_sprite_is_sprite_alive(GdObj id) { return spriteMgr->is_sprite_alive(id); }
static void gdextension_spx_sprite_set_position(GdObj id, GdVec2 pos) { spriteMgr->set_position(id, pos); }
static void gdextension_spx_sprite_set_rotation(GdObj id, GdFloat rot) { spriteMgr->set_rotation(id, rot); }
static void gdextension_spx_sprite_set_scale(GdObj id, GdVec2 scale) { spriteMgr->set_scale(id, scale); }
static GdVec2 gdextension_spx_sprite_get_position(GdObj id) { return spriteMgr->get_position(id); }
static GdFloat gdextension_spx_sprite_get_rotation(GdObj id) { return spriteMgr->get_rotation(id); }
static GdVec2 gdextension_spx_sprite_get_scale(GdObj id) { return spriteMgr->get_scale(id); }
static void gdextension_spx_sprite_set_color(GdObj id, GdColor color) { spriteMgr->set_color(id, color); }
static GdColor gdextension_spx_sprite_get_color(GdObj id) { return spriteMgr->get_color(id); }
static void gdextension_spx_sprite_update_texture(GdObj id, GdString path) { spriteMgr->update_texture(id, path); }
static GdString gdextension_spx_sprite_get_texture(GdObj id) { return spriteMgr->get_texture(id); }
static void gdextension_spx_sprite_set_visible(GdObj id, GdBool visible) { spriteMgr->set_visible(id, visible); }
static GdBool gdextension_spx_sprite_get_visible(GdObj id) { return spriteMgr->get_visible(id); }
static void gdextension_spx_sprite_update_z_index(GdObj id, GdInt z) { spriteMgr->update_z_index(id, z); }
// UI
static GdInt gdextension_spx_ui_create_button(GdString path, GdRect2 rect, GdString text) { return uiMgr->create_button(path, rect, text); }
static GdInt gdextension_spx_ui_create_label(GdString path, GdRect2 rect, GdString text) { return uiMgr->create_label(path, rect, text); }
static GdInt gdextension_spx_ui_create_image(GdString path, GdRect2 rect, GdColor color) { return uiMgr->create_image(path, rect, color); }
static GdInt gdextension_spx_ui_create_slider(GdString path, GdRect2 rect, GdFloat value) { return uiMgr->create_slider(path, rect, value); }
static GdInt gdextension_spx_ui_create_toggle(GdString path, GdRect2 rect, GdBool value) { return uiMgr->create_toggle(path, rect, value); }
static GdInt gdextension_spx_ui_create_input(GdString path, GdRect2 rect, GdString text) { return uiMgr->create_input(path, rect, text); }
static GdInt gdextension_spx_ui_get_type(GdObj id) { return uiMgr->get_type(id); }
static void gdextension_spx_ui_set_interactable(GdObj id, GdBool interactable) { uiMgr->set_interactable(id, interactable); }
static GdBool gdextension_spx_ui_get_interactable(GdObj id) { return uiMgr->get_interactable(id); }
static void gdextension_spx_ui_set_text(GdObj id, GdString text) { uiMgr->set_text(id, text); }
static GdString gdextension_spx_ui_get_text(GdObj id) { return uiMgr->get_text(id); }
static void gdextension_spx_ui_set_rect(GdObj id, GdRect2 rect) { return uiMgr->set_rect(id, rect); }
static GdRect2 gdextension_spx_ui_get_rect(GdObj id) { return uiMgr->get_rect(id); }
static void gdextension_spx_ui_set_color(GdObj id, GdColor color) { uiMgr->set_color(id, color); }
static GdColor gdextension_spx_ui_get_color(GdObj id) { return uiMgr->get_color(id); }
static void gdextension_spx_ui_set_font_size(GdObj id, GdFloat size) { uiMgr->set_font_size(id, size); }
static GdFloat gdextension_spx_ui_get_font_size(GdObj id) { return uiMgr->get_font_size(id); }
static void gdextension_spx_ui_set_visible(GdObj id, GdBool visible) { uiMgr->set_visible(id, visible); }
static GdBool gdextension_spx_ui_get_visible(GdObj id) { return uiMgr->get_visible(id); }

#define REGISTER_SPX_INTERFACE_FUNC(m_name) GDExtension::register_interface_function("spx_" #m_name, (GDExtensionInterfaceFunctionPtr)&gdextension_spx_##m_name)

void gdextension_spx_setup_interface() {
	REGISTER_SPX_INTERFACE_FUNC(global_register_callbacks);
	REGISTER_SPX_INTERFACE_FUNC(input_get_mouse_pos);
	REGISTER_SPX_INTERFACE_FUNC(input_get_mouse_state);
	REGISTER_SPX_INTERFACE_FUNC(input_get_key_state);
	REGISTER_SPX_INTERFACE_FUNC(input_get_axis);
	REGISTER_SPX_INTERFACE_FUNC(input_is_action_pressed);
	REGISTER_SPX_INTERFACE_FUNC(input_is_action_just_pressed);
	REGISTER_SPX_INTERFACE_FUNC(input_is_action_just_released);
	REGISTER_SPX_INTERFACE_FUNC(audio_play_audio);
	REGISTER_SPX_INTERFACE_FUNC(audio_set_audio_volume);
	REGISTER_SPX_INTERFACE_FUNC(audio_get_audio_volume);
	REGISTER_SPX_INTERFACE_FUNC(audio_is_music_playing);
	REGISTER_SPX_INTERFACE_FUNC(audio_play_music);
	REGISTER_SPX_INTERFACE_FUNC(audio_set_music_volume);
	REGISTER_SPX_INTERFACE_FUNC(audio_get_music_volume);
	REGISTER_SPX_INTERFACE_FUNC(audio_pause_music);
	REGISTER_SPX_INTERFACE_FUNC(audio_resume_music);
	REGISTER_SPX_INTERFACE_FUNC(audio_get_music_timer);
	REGISTER_SPX_INTERFACE_FUNC(audio_set_music_timer);
	REGISTER_SPX_INTERFACE_FUNC(physic_set_gravity);
	REGISTER_SPX_INTERFACE_FUNC(physic_get_gravity);
	REGISTER_SPX_INTERFACE_FUNC(physic_set_velocity);
	REGISTER_SPX_INTERFACE_FUNC(physic_get_velocity);
	REGISTER_SPX_INTERFACE_FUNC(physic_set_mass);
	REGISTER_SPX_INTERFACE_FUNC(physic_get_mass);
	REGISTER_SPX_INTERFACE_FUNC(physic_add_force);
	REGISTER_SPX_INTERFACE_FUNC(physic_add_impulse);
	REGISTER_SPX_INTERFACE_FUNC(physic_set_collision_layer);
	REGISTER_SPX_INTERFACE_FUNC(physic_get_collision_layer);
	REGISTER_SPX_INTERFACE_FUNC(physic_set_collision_mask);
	REGISTER_SPX_INTERFACE_FUNC(physic_get_collision_mask);
	REGISTER_SPX_INTERFACE_FUNC(physic_get_collider_type);
	REGISTER_SPX_INTERFACE_FUNC(physic_add_collider_rect);
	REGISTER_SPX_INTERFACE_FUNC(physic_add_collider_circle);
	REGISTER_SPX_INTERFACE_FUNC(physic_add_collider_capsule);
	REGISTER_SPX_INTERFACE_FUNC(physic_set_trigger);
	REGISTER_SPX_INTERFACE_FUNC(physic_is_trigger);
	REGISTER_SPX_INTERFACE_FUNC(physic_set_collision_enabled);
	REGISTER_SPX_INTERFACE_FUNC(physic_is_collision_enabled);
	REGISTER_SPX_INTERFACE_FUNC(sprite_create_sprite);
	REGISTER_SPX_INTERFACE_FUNC(sprite_clone_sprite);
	REGISTER_SPX_INTERFACE_FUNC(sprite_destroy_sprite);
	REGISTER_SPX_INTERFACE_FUNC(sprite_is_sprite_alive);
	REGISTER_SPX_INTERFACE_FUNC(sprite_set_position);
	REGISTER_SPX_INTERFACE_FUNC(sprite_set_rotation);
	REGISTER_SPX_INTERFACE_FUNC(sprite_set_scale);
	REGISTER_SPX_INTERFACE_FUNC(sprite_get_position);
	REGISTER_SPX_INTERFACE_FUNC(sprite_get_rotation);
	REGISTER_SPX_INTERFACE_FUNC(sprite_get_scale);
	REGISTER_SPX_INTERFACE_FUNC(sprite_set_color);
	REGISTER_SPX_INTERFACE_FUNC(sprite_get_color);
	REGISTER_SPX_INTERFACE_FUNC(sprite_update_texture);
	REGISTER_SPX_INTERFACE_FUNC(sprite_get_texture);
	REGISTER_SPX_INTERFACE_FUNC(sprite_set_visible);
	REGISTER_SPX_INTERFACE_FUNC(sprite_get_visible);
	REGISTER_SPX_INTERFACE_FUNC(sprite_update_z_index);
	REGISTER_SPX_INTERFACE_FUNC(ui_create_button);
	REGISTER_SPX_INTERFACE_FUNC(ui_create_label);
	REGISTER_SPX_INTERFACE_FUNC(ui_create_image);
	REGISTER_SPX_INTERFACE_FUNC(ui_create_slider);
	REGISTER_SPX_INTERFACE_FUNC(ui_create_toggle);
	REGISTER_SPX_INTERFACE_FUNC(ui_create_input);
	REGISTER_SPX_INTERFACE_FUNC(ui_get_type);
	REGISTER_SPX_INTERFACE_FUNC(ui_set_interactable);
	REGISTER_SPX_INTERFACE_FUNC(ui_get_interactable);
	REGISTER_SPX_INTERFACE_FUNC(ui_set_text);
	REGISTER_SPX_INTERFACE_FUNC(ui_get_text);
	REGISTER_SPX_INTERFACE_FUNC(ui_set_rect);
	REGISTER_SPX_INTERFACE_FUNC(ui_get_rect);
	REGISTER_SPX_INTERFACE_FUNC(ui_set_color);
	REGISTER_SPX_INTERFACE_FUNC(ui_get_color);
	REGISTER_SPX_INTERFACE_FUNC(ui_set_font_size);
	REGISTER_SPX_INTERFACE_FUNC(ui_get_font_size);
	REGISTER_SPX_INTERFACE_FUNC(ui_set_visible);
	REGISTER_SPX_INTERFACE_FUNC(ui_get_visible);
}
