/**************************************************************************/
/*  main_loop_phase_callback_bus.cpp                                      */
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

#include "main_loop_phase_callback_bus.h"

#include "core/error/error_macros.h"

bool MainLoopPhaseCallbackBus::Callbacks::is_empty() const {
	return start == nullptr && fixed_update == nullptr && update == nullptr && !has_movie_callbacks() && destroy == nullptr;
}

bool MainLoopPhaseCallbackBus::Callbacks::has_movie_callbacks() const {
	return movie_claim != nullptr || movie_requires_live_audio != nullptr || movie_begin != nullptr || movie_frame != nullptr || movie_end != nullptr;
}

bool MainLoopPhaseCallbackBus::Callbacks::has_valid_movie_callbacks() const {
	return !has_movie_callbacks() || (movie_claim != nullptr && movie_begin != nullptr && movie_end != nullptr);
}

MainLoopPhaseCallbackBus::RegistrationID MainLoopPhaseCallbackBus::_allocate_registration_id() {
	const RegistrationID first_candidate = next_registration_id;
	do {
		const RegistrationID candidate = next_registration_id++;
		if (next_registration_id == INVALID_REGISTRATION_ID) {
			next_registration_id = 1;
		}

		Callbacks unused;
		if (!_get_callbacks(candidate, unused)) {
			return candidate;
		}
	} while (next_registration_id != first_candidate);

	return INVALID_REGISTRATION_ID;
}

bool MainLoopPhaseCallbackBus::_get_callbacks(RegistrationID p_id, Callbacks &r_callbacks) const {
	for (const Registration &registration : registrations) {
		if (registration.id == p_id) {
			r_callbacks = registration.callbacks;
			return true;
		}
	}
	return false;
}

LocalVector<MainLoopPhaseCallbackBus::RegistrationID> MainLoopPhaseCallbackBus::_snapshot_registration_ids() const {
	LocalVector<RegistrationID> registration_ids;
	registration_ids.reserve(registrations.size());
	for (const Registration &registration : registrations) {
		registration_ids.push_back(registration.id);
	}
	return registration_ids;
}

void MainLoopPhaseCallbackBus::_clear_movie_route() {
	movie_route_state = MOVIE_ROUTE_IDLE;
	movie_route_owner = INVALID_REGISTRATION_ID;
	movie_route_live_audio = false;
	movie_route_path.clear();
}

bool MainLoopPhaseCallbackBus::_movie_owner_is_leased(RegistrationID p_id) const {
	return p_id != INVALID_REGISTRATION_ID && p_id == movie_route_owner && (movie_route_state == MOVIE_ROUTE_BEGINNING || movie_route_state == MOVIE_ROUTE_ACTIVE || movie_route_state == MOVIE_ROUTE_ENDING);
}

MainLoopPhaseCallbackBus::RegistrationID MainLoopPhaseCallbackBus::register_callbacks(const Callbacks &p_callbacks) {
	ERR_FAIL_COND_V_MSG(p_callbacks.is_empty(), INVALID_REGISTRATION_ID, "Cannot register an empty main loop phase callback set.");
	ERR_FAIL_COND_V_MSG(!p_callbacks.has_valid_movie_callbacks(), INVALID_REGISTRATION_ID, "Movie callbacks require claim, begin, and end callbacks as one complete set.");

	const RegistrationID id = _allocate_registration_id();
	ERR_FAIL_COND_V_MSG(id == INVALID_REGISTRATION_ID, INVALID_REGISTRATION_ID, "Main loop phase callback registration IDs are exhausted.");

	Registration registration;
	registration.id = id;
	registration.callbacks = p_callbacks;
	registrations.push_back(registration);
	return id;
}

bool MainLoopPhaseCallbackBus::unregister_callbacks(RegistrationID p_id) {
	if (p_id == INVALID_REGISTRATION_ID || _movie_owner_is_leased(p_id)) {
		return false;
	}

	for (uint32_t i = 0; i < registrations.size(); i++) {
		if (registrations[i].id == p_id) {
			if (p_id == movie_route_owner && movie_route_state == MOVIE_ROUTE_RESOLVED) {
				_clear_movie_route();
			}
			registrations.remove_at(i);
			return true;
		}
	}
	return false;
}

uint32_t MainLoopPhaseCallbackBus::get_registration_count() const {
	return registrations.size();
}

void MainLoopPhaseCallbackBus::notify_start(MainLoop *p_main_loop) {
	for (RegistrationID id : _snapshot_registration_ids()) {
		Callbacks callbacks;
		if (_get_callbacks(id, callbacks) && callbacks.start != nullptr) {
			callbacks.start(callbacks.userdata, p_main_loop);
		}
	}
}

void MainLoopPhaseCallbackBus::notify_fixed_update(double p_delta) {
	for (RegistrationID id : _snapshot_registration_ids()) {
		Callbacks callbacks;
		if (_get_callbacks(id, callbacks) && callbacks.fixed_update != nullptr) {
			callbacks.fixed_update(callbacks.userdata, p_delta);
		}
	}
}

void MainLoopPhaseCallbackBus::notify_update(double p_delta) {
	for (RegistrationID id : _snapshot_registration_ids()) {
		Callbacks callbacks;
		if (_get_callbacks(id, callbacks) && callbacks.update != nullptr) {
			callbacks.update(callbacks.userdata, p_delta);
		}
	}
}

Error MainLoopPhaseCallbackBus::resolve_movie_route(const String &p_movie_path) {
	ERR_FAIL_COND_V_MSG(movie_route_state != MOVIE_ROUTE_IDLE, ERR_ALREADY_IN_USE, "A movie callback route has already been resolved.");
	ERR_FAIL_COND_V_MSG(p_movie_path.is_empty(), ERR_INVALID_PARAMETER, "Cannot resolve a movie callback route for an empty path.");

	movie_route_state = MOVIE_ROUTE_RESOLVING;
	movie_route_path = p_movie_path;
	for (RegistrationID id : _snapshot_registration_ids()) {
		Callbacks callbacks;
		if (!_get_callbacks(id, callbacks) || callbacks.movie_claim == nullptr) {
			continue;
		}

		const bool claimed = callbacks.movie_claim(callbacks.userdata, p_movie_path);
		Callbacks callbacks_after_claim;
		if (!_get_callbacks(id, callbacks_after_claim)) {
			continue;
		}
		if (!claimed) {
			continue;
		}

		bool live_audio = false;
		if (callbacks_after_claim.movie_requires_live_audio != nullptr) {
			live_audio = callbacks_after_claim.movie_requires_live_audio(callbacks_after_claim.userdata, p_movie_path);
			Callbacks callbacks_after_policy;
			if (!_get_callbacks(id, callbacks_after_policy)) {
				continue;
			}
		}

		movie_route_owner = id;
		movie_route_live_audio = live_audio;
		break;
	}

	movie_route_state = MOVIE_ROUTE_RESOLVED;
	return OK;
}

bool MainLoopPhaseCallbackBus::is_movie_route_resolved() const {
	return movie_route_state != MOVIE_ROUTE_IDLE && movie_route_state != MOVIE_ROUTE_RESOLVING;
}

bool MainLoopPhaseCallbackBus::is_movie_route_claimed() const {
	return is_movie_route_resolved() && movie_route_owner != INVALID_REGISTRATION_ID;
}

bool MainLoopPhaseCallbackBus::movie_route_requires_live_audio() const {
	return is_movie_route_claimed() && movie_route_live_audio;
}

Error MainLoopPhaseCallbackBus::notify_movie_begin(const Size2i &p_movie_size, uint32_t p_fps, const String &p_movie_path) {
	ERR_FAIL_COND_V_MSG(movie_route_state == MOVIE_ROUTE_BEGINNING || movie_route_state == MOVIE_ROUTE_ACTIVE || movie_route_state == MOVIE_ROUTE_ENDING, ERR_ALREADY_IN_USE, "The resolved movie callback route is already in use.");
	ERR_FAIL_COND_V_MSG(movie_route_state != MOVIE_ROUTE_RESOLVED || movie_route_owner == INVALID_REGISTRATION_ID, ERR_UNAVAILABLE, "No resolved movie callback route is available.");
	ERR_FAIL_COND_V_MSG(p_movie_path != movie_route_path, ERR_INVALID_PARAMETER, "The movie callback route was resolved for a different path.");

	Callbacks callbacks;
	if (!_get_callbacks(movie_route_owner, callbacks)) {
		_clear_movie_route();
		return ERR_DOES_NOT_EXIST;
	}

	movie_route_state = MOVIE_ROUTE_BEGINNING;
	const Error err = callbacks.movie_begin(callbacks.userdata, p_movie_size, p_fps, p_movie_path);
	if (err != OK) {
		_clear_movie_route();
		return err;
	}

	movie_route_state = MOVIE_ROUTE_ACTIVE;
	return OK;
}

void MainLoopPhaseCallbackBus::notify_movie_frame() {
	if (movie_route_state != MOVIE_ROUTE_ACTIVE) {
		return;
	}

	Callbacks callbacks;
	if (_get_callbacks(movie_route_owner, callbacks) && callbacks.movie_frame != nullptr) {
		callbacks.movie_frame(callbacks.userdata);
	}
}

void MainLoopPhaseCallbackBus::notify_movie_end() {
	if (movie_route_state == MOVIE_ROUTE_IDLE || movie_route_state == MOVIE_ROUTE_RESOLVING || movie_route_state == MOVIE_ROUTE_BEGINNING || movie_route_state == MOVIE_ROUTE_ENDING) {
		return;
	}

	if (movie_route_state == MOVIE_ROUTE_ACTIVE) {
		movie_route_state = MOVIE_ROUTE_ENDING;
		Callbacks callbacks;
		if (_get_callbacks(movie_route_owner, callbacks)) {
			callbacks.movie_end(callbacks.userdata);
		}
	}

	_clear_movie_route();
}

void MainLoopPhaseCallbackBus::notify_destroy() {
	for (RegistrationID id : _snapshot_registration_ids()) {
		Callbacks callbacks;
		if (_get_callbacks(id, callbacks) && callbacks.destroy != nullptr) {
			callbacks.destroy(callbacks.userdata);
		}
	}
}

MainLoopPhaseCallbackBus &get_main_loop_phase_callback_bus() {
	static MainLoopPhaseCallbackBus callback_bus;
	return callback_bus;
}
