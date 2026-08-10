/**************************************************************************/
/*  main_loop_phase_callback_bus.h                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
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

#ifndef MAIN_LOOP_PHASE_CALLBACK_BUS_H
#define MAIN_LOOP_PHASE_CALLBACK_BUS_H

#include "core/error/error_list.h"
#include "core/math/vector2i.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "core/typedefs.h"

class MainLoop;

// Main-thread-only callback bus for lifecycle phases owned by Main.
// General phase subscribers run in registration order. Movie routing is
// resolved exactly once, then leased to one registration until movie_end.
class MainLoopPhaseCallbackBus {
public:
	using RegistrationID = uint64_t;
	static constexpr RegistrationID INVALID_REGISTRATION_ID = 0;

	using StartCallback = void (*)(void *p_userdata, MainLoop *p_main_loop);
	using StepCallback = void (*)(void *p_userdata, double p_delta);
	using MovieClaimCallback = bool (*)(void *p_userdata, const String &p_movie_path);
	using MovieAudioPolicyCallback = bool (*)(void *p_userdata, const String &p_movie_path);
	using MovieBeginCallback = Error (*)(void *p_userdata, const Size2i &p_movie_size, uint32_t p_fps, const String &p_movie_path);
	using MovieFrameCallback = void (*)(void *p_userdata);
	using MovieEndCallback = void (*)(void *p_userdata);
	using DestroyCallback = void (*)(void *p_userdata);

	struct Callbacks {
		void *userdata = nullptr;
		StartCallback start = nullptr;
		StepCallback fixed_update = nullptr;
		StepCallback update = nullptr;
		MovieClaimCallback movie_claim = nullptr;
		MovieAudioPolicyCallback movie_requires_live_audio = nullptr;
		MovieBeginCallback movie_begin = nullptr;
		MovieFrameCallback movie_frame = nullptr;
		MovieEndCallback movie_end = nullptr;
		DestroyCallback destroy = nullptr;

		bool is_empty() const;
		bool has_movie_callbacks() const;
		bool has_valid_movie_callbacks() const;
	};

private:
	struct Registration {
		RegistrationID id = INVALID_REGISTRATION_ID;
		Callbacks callbacks;
	};

	enum MovieRouteState {
		MOVIE_ROUTE_IDLE,
		MOVIE_ROUTE_RESOLVING,
		MOVIE_ROUTE_RESOLVED,
		MOVIE_ROUTE_BEGINNING,
		MOVIE_ROUTE_ACTIVE,
		MOVIE_ROUTE_ENDING,
	};

	LocalVector<Registration> registrations;
	RegistrationID next_registration_id = 1;

	MovieRouteState movie_route_state = MOVIE_ROUTE_IDLE;
	RegistrationID movie_route_owner = INVALID_REGISTRATION_ID;
	bool movie_route_live_audio = false;
	String movie_route_path;

	RegistrationID _allocate_registration_id();
	bool _get_callbacks(RegistrationID p_id, Callbacks &r_callbacks) const;
	LocalVector<RegistrationID> _snapshot_registration_ids() const;
	void _clear_movie_route();
	bool _movie_owner_is_leased(RegistrationID p_id) const;

public:
	RegistrationID register_callbacks(const Callbacks &p_callbacks);
	bool unregister_callbacks(RegistrationID p_id);
	uint32_t get_registration_count() const;

	void notify_start(MainLoop *p_main_loop);
	void notify_fixed_update(double p_delta);
	void notify_update(double p_delta);

	Error resolve_movie_route(const String &p_movie_path);
	bool is_movie_route_resolved() const;
	bool is_movie_route_claimed() const;
	bool movie_route_requires_live_audio() const;
	Error notify_movie_begin(const Size2i &p_movie_size, uint32_t p_fps, const String &p_movie_path);
	void notify_movie_frame();
	void notify_movie_end();

	void notify_destroy();
};

MainLoopPhaseCallbackBus &get_main_loop_phase_callback_bus();

#endif // MAIN_LOOP_PHASE_CALLBACK_BUS_H
