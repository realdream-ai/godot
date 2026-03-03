const GodotGdspx = {
	$GodotGdspx__deps: ['$GodotConfig', '$GodotRuntime','$GodotFS'],
	$GodotGdspx: {
	},

	// godot gdspx extensions
	godot_js_spx_on_engine_start__sig: 'v',
	godot_js_spx_on_engine_start:async function () {
		FFI = null
		await self.initExtensionWasm()
	},


	godot_js_spx_on_engine_update__sig: 'vf',
	godot_js_spx_on_engine_update: function (delta) {
		if (!FFI) return;
		FFI.gdspx_dispatch("OnEngineUpdate", delta);
	},

	godot_js_spx_on_engine_fixed_update__sig: 'vf',
	godot_js_spx_on_engine_fixed_update: function (delta) {
		if (!FFI) return;
		FFI.gdspx_dispatch("OnEngineFixedUpdate", delta);
	},

	godot_js_spx_on_engine_destroy__sig: 'v',
	godot_js_spx_on_engine_destroy: function () {
		if (!FFI) return;
		FFI.gdspx_dispatch("OnEngineDestroy");
	},

	godot_js_spx_on_engine_reset__sig: 'v',
	godot_js_spx_on_engine_reset: function () {
		if (!FFI) return;
		FFI.gdspx_dispatch("OnEngineReset");
	},

	godot_js_spx_on_reset_done__sig: 'vi',
	godot_js_spx_on_reset_done: function (code) {
		if (!FFI || !FFI.gdspx_on_runtime_reset) {
			return;
		}
		FFI.gdspx_on_runtime_reset(code);
	},

	godot_js_spx_on_engine_pause__sig: 'vi',
	godot_js_spx_on_engine_pause : function (is_on) {
		FFI.gdspx_dispatch("OnEnginePause", is_on);
	},

	godot_js_spx_on_scene_sprite_instantiated__sig: 'vii',
	godot_js_spx_on_scene_sprite_instantiated: function (obj, type_name) {
		FFI.gdspx_dispatch("OnSceneSpriteInstantiated", GodotRuntime.ToJsObj(obj), GodotRuntime.parseString(type_name));
	},

	godot_js_spx_on_runtime_panic__sig: 'vi',
	godot_js_spx_on_runtime_panic: function (msg) {
		if (!FFI.gdspx_on_runtime_panic) {
			return;
		}
		FFI.gdspx_on_runtime_panic(GodotRuntime.parseString(msg));
	},

	godot_js_spx_on_runtime_exit__sig: 'vi',
	godot_js_spx_on_runtime_exit: function (code) {
		if (!FFI.gdspx_on_runtime_exit) {
			return;
		}
		FFI.gdspx_on_runtime_exit(code);
	},

	godot_js_spx_on_sprite_ready__sig: 'vi',
	godot_js_spx_on_sprite_ready: function (obj) {
		FFI.gdspx_dispatch("OnSpriteReady", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_sprite_updated__sig: 'vf',
	godot_js_spx_on_sprite_updated: function (delta) {
		FFI.gdspx_dispatch("OnSpriteUpdated", delta);
	},

	godot_js_spx_on_sprite_fixed_updated__sig: 'vf',
	godot_js_spx_on_sprite_fixed_updated: function (delta) {
		FFI.gdspx_dispatch("OnSpriteFixedUpdated", delta);
	},

	godot_js_spx_on_sprite_destroyed__sig: 'vi',
	godot_js_spx_on_sprite_destroyed: function (obj) {
		FFI.gdspx_dispatch("OnSpriteDestroyed", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_sprite_frames_set_changed__sig: 'vi',
	godot_js_spx_on_sprite_frames_set_changed: function (obj) {
		FFI.gdspx_dispatch("OnSpriteFramesSetChanged", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_sprite_animation_changed__sig: 'vi',
	godot_js_spx_on_sprite_animation_changed: function (obj) {
		FFI.gdspx_dispatch("OnSpriteAnimationChanged", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_sprite_frame_changed__sig: 'vi',
	godot_js_spx_on_sprite_frame_changed: function (obj) {
		FFI.gdspx_dispatch("OnSpriteFrameChanged", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_sprite_animation_looped__sig: 'vi',
	godot_js_spx_on_sprite_animation_looped: function (obj) {
		FFI.gdspx_dispatch("OnSpriteAnimationLooped", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_sprite_animation_finished__sig: 'vi',
	godot_js_spx_on_sprite_animation_finished: function (obj) {
		FFI.gdspx_dispatch("OnSpriteAnimationFinished", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_sprite_vfx_finished__sig: 'vi',
	godot_js_spx_on_sprite_vfx_finished: function (obj) {
		FFI.gdspx_dispatch("OnSpriteVfxFinished", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_sprite_screen_exited__sig: 'vi',
	godot_js_spx_on_sprite_screen_exited: function (obj) {
		FFI.gdspx_dispatch("OnSpriteScreenExited", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_sprite_screen_entered__sig: 'vi',
	godot_js_spx_on_sprite_screen_entered: function (obj) {
		FFI.gdspx_dispatch("OnSpriteScreenEntered", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_mouse_pressed__sig: 'vi',
	godot_js_spx_on_mouse_pressed: function (keyid) {
		FFI.gdspx_dispatch("OnMousePressed", GodotRuntime.ToJsInt(keyid));
	},

	godot_js_spx_on_mouse_released__sig: 'vi',
	godot_js_spx_on_mouse_released: function (keyid) {
		FFI.gdspx_dispatch("OnMouseReleased", GodotRuntime.ToJsInt(keyid));
	},

	godot_js_spx_on_key_pressed__sig: 'vi',
	godot_js_spx_on_key_pressed: function (keyid) {
		FFI.gdspx_dispatch("OnKeyPressed", GodotRuntime.ToJsInt(keyid));
	},

	godot_js_spx_on_key_released__sig: 'vi',
	godot_js_spx_on_key_released: function (keyid) {
		FFI.gdspx_dispatch("OnKeyReleased", GodotRuntime.ToJsInt(keyid));
	},

	godot_js_spx_on_action_pressed__sig: 'vi',
	godot_js_spx_on_action_pressed: function (action_name) {
		FFI.gdspx_dispatch("OnActionPressed", GodotRuntime.parseString(action_name));
	},

	godot_js_spx_on_action_just_pressed__sig: 'vi',
	godot_js_spx_on_action_just_pressed: function (action_name) {
		FFI.gdspx_dispatch("OnActionJustPressed", GodotRuntime.parseString(action_name));
	},

	godot_js_spx_on_action_just_released__sig: 'vi',
	godot_js_spx_on_action_just_released: function (action_name) {
		FFI.gdspx_dispatch("OnActionJustReleased", GodotRuntime.parseString(action_name));
	},

	godot_js_spx_on_axis_changed__sig: 'vif',
	godot_js_spx_on_axis_changed: function (action_name, value) {
		FFI.gdspx_dispatch("OnAxisChanged", GodotRuntime.parseString(action_name), value);
	},

	godot_js_spx_on_collision_enter__sig: 'vii',
	godot_js_spx_on_collision_enter: function (self_id, other_id) {
		FFI.gdspx_dispatch("OnCollisionEnter", GodotRuntime.ToJsInt(self_id), GodotRuntime.ToJsInt(other_id));
	},

	godot_js_spx_on_collision_stay__sig: 'vii',
	godot_js_spx_on_collision_stay: function (self_id, other_id) {
		FFI.gdspx_dispatch("OnCollisionStay", GodotRuntime.ToJsInt(self_id), GodotRuntime.ToJsInt(other_id));
	},

	godot_js_spx_on_collision_exit__sig: 'vii',
	godot_js_spx_on_collision_exit: function (self_id, other_id) {
		FFI.gdspx_dispatch("OnCollisionExit", GodotRuntime.ToJsInt(self_id), GodotRuntime.ToJsInt(other_id));
	},

	godot_js_spx_on_trigger_enter__sig: 'vii',
	godot_js_spx_on_trigger_enter: function (self_id, other_id) {
		FFI.gdspx_dispatch("OnTriggerEnter", GodotRuntime.ToJsInt(self_id), GodotRuntime.ToJsInt(other_id));
	},

	godot_js_spx_on_trigger_stay__sig: 'vii',
	godot_js_spx_on_trigger_stay: function (self_id, other_id) {
		FFI.gdspx_dispatch("OnTriggerStay", GodotRuntime.ToJsInt(self_id), GodotRuntime.ToJsInt(other_id));
	},

	godot_js_spx_on_trigger_exit__sig: 'vii',
	godot_js_spx_on_trigger_exit: function (self_id, other_id) {
		FFI.gdspx_dispatch("OnTriggerExit", GodotRuntime.ToJsInt(self_id), GodotRuntime.ToJsInt(other_id));
	},

	godot_js_spx_on_ui_ready__sig: 'vi',
	godot_js_spx_on_ui_ready: function (obj) {
		FFI.gdspx_dispatch("OnUiReady", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_ui_updated__sig: 'vi',
	godot_js_spx_on_ui_updated: function (obj) {
		FFI.gdspx_dispatch("OnUiUpdated", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_ui_destroyed__sig: 'vi',
	godot_js_spx_on_ui_destroyed: function (obj) {
		FFI.gdspx_dispatch("OnUiDestroyed", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_ui_pressed__sig: 'vi',
	godot_js_spx_on_ui_pressed: function (obj) {
		FFI.gdspx_dispatch("OnUiPressed", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_ui_released__sig: 'vi',
	godot_js_spx_on_ui_released: function (obj) {
		FFI.gdspx_dispatch("OnUiReleased", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_ui_hovered__sig: 'vi',
	godot_js_spx_on_ui_hovered: function (obj) {
		FFI.gdspx_dispatch("OnUiHovered", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_ui_clicked__sig: 'vi',
	godot_js_spx_on_ui_clicked: function (obj) {
		FFI.gdspx_dispatch("OnUiClicked", GodotRuntime.ToJsObj(obj));
	},

	godot_js_spx_on_ui_toggle__sig: 'vii',
	godot_js_spx_on_ui_toggle: function (obj, is_on) {
		FFI.gdspx_dispatch("OnUiToggle", GodotRuntime.ToJsObj(obj), is_on);
	},

	godot_js_spx_on_ui_text_changed__sig: 'vii',
	godot_js_spx_on_ui_text_changed: function (obj, text) {
		FFI.gdspx_dispatch("OnUiTextChanged", GodotRuntime.ToJsObj(obj), GodotRuntime.parseString(text));
	},
};

autoAddDeps(GodotGdspx, '$GodotGdspx');
mergeInto(LibraryManager.library, GodotGdspx);
