
#include "os_web.h"
#include "core/extension/gdextension_interface.h"
#include "core/extension/spx_engine.h"
#include "godot_js.h"
#include <emscripten.h>

#include "api/javascript_bridge_singleton.h"
#include "display_server_web.h"


void OS_Web::register_spx_callbacks() {
 	SpxCallbackInfo* callback_infos = memnew(SpxCallbackInfo);
// gdspx register callbacks
	callback_infos->func_on_engine_start = &godot_js_spx_on_engine_start;
	callback_infos->func_on_engine_update = &godot_js_spx_on_engine_update;
	callback_infos->func_on_engine_fixed_update = &godot_js_spx_on_engine_fixed_update;
	callback_infos->func_on_engine_destroy = &godot_js_spx_on_engine_destroy;
	callback_infos->func_on_scene_sprite_instantiated = &godot_js_spx_on_scene_sprite_instantiated;
	callback_infos->func_on_sprite_ready = &godot_js_spx_on_sprite_ready;
	callback_infos->func_on_sprite_updated = &godot_js_spx_on_sprite_updated;
	callback_infos->func_on_sprite_fixed_updated = &godot_js_spx_on_sprite_fixed_updated;
	callback_infos->func_on_sprite_destroyed = &godot_js_spx_on_sprite_destroyed;
	callback_infos->func_on_sprite_frames_set_changed = &godot_js_spx_on_sprite_frames_set_changed;
	callback_infos->func_on_sprite_animation_changed = &godot_js_spx_on_sprite_animation_changed;
	callback_infos->func_on_sprite_frame_changed = &godot_js_spx_on_sprite_frame_changed;
	callback_infos->func_on_sprite_animation_looped = &godot_js_spx_on_sprite_animation_looped;
	callback_infos->func_on_sprite_animation_finished = &godot_js_spx_on_sprite_animation_finished;
	callback_infos->func_on_sprite_vfx_finished = &godot_js_spx_on_sprite_vfx_finished;
	callback_infos->func_on_sprite_screen_exited = &godot_js_spx_on_sprite_screen_exited;
	callback_infos->func_on_sprite_screen_entered = &godot_js_spx_on_sprite_screen_entered;
	callback_infos->func_on_mouse_pressed = &godot_js_spx_on_mouse_pressed;
	callback_infos->func_on_mouse_released = &godot_js_spx_on_mouse_released;
	callback_infos->func_on_key_pressed = &godot_js_spx_on_key_pressed;
	callback_infos->func_on_key_released = &godot_js_spx_on_key_released;
	callback_infos->func_on_action_pressed = &godot_js_spx_on_action_pressed;
	callback_infos->func_on_action_just_pressed = &godot_js_spx_on_action_just_pressed;
	callback_infos->func_on_action_just_released = &godot_js_spx_on_action_just_released;
	callback_infos->func_on_axis_changed = &godot_js_spx_on_axis_changed;
	callback_infos->func_on_collision_enter = &godot_js_spx_on_collision_enter;
	callback_infos->func_on_collision_stay = &godot_js_spx_on_collision_stay;
	callback_infos->func_on_collision_exit = &godot_js_spx_on_collision_exit;
	callback_infos->func_on_trigger_enter = &godot_js_spx_on_trigger_enter;
	callback_infos->func_on_trigger_stay = &godot_js_spx_on_trigger_stay;
	callback_infos->func_on_trigger_exit = &godot_js_spx_on_trigger_exit;
	callback_infos->func_on_ui_ready = &godot_js_spx_on_ui_ready;
	callback_infos->func_on_ui_updated = &godot_js_spx_on_ui_updated;
	callback_infos->func_on_ui_destroyed = &godot_js_spx_on_ui_destroyed;
	callback_infos->func_on_ui_pressed = &godot_js_spx_on_ui_pressed;
	callback_infos->func_on_ui_released = &godot_js_spx_on_ui_released;
	callback_infos->func_on_ui_hovered = &godot_js_spx_on_ui_hovered;
	callback_infos->func_on_ui_clicked = &godot_js_spx_on_ui_clicked;
	callback_infos->func_on_ui_toggle = &godot_js_spx_on_ui_toggle;
	callback_infos->func_on_ui_text_changed = &godot_js_spx_on_ui_text_changed;
	
	SpxEngine::register_callbacks(callback_infos);
}




extern "C" {
	EMSCRIPTEN_KEEPALIVE
	float test_go_call_cpp(float val) {
		auto ret = val * 2;
		printf("C++ function called with value: %f return %f\n ", val,ret);
		return ret;
	}
}