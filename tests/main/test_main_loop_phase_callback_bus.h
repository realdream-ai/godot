/**************************************************************************/
/*  test_main_loop_phase_callback_bus.h                                   */
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

#ifndef TEST_MAIN_LOOP_PHASE_CALLBACK_BUS_H
#define TEST_MAIN_LOOP_PHASE_CALLBACK_BUS_H

#include "main/main_loop_phase_callback_bus.h"
#include "tests/test_macros.h"

namespace TestMainLoopPhaseCallbackBus {

struct CallbackRecorder {
	LocalVector<int> *events = nullptr;
	int subscriber = 0;
	double fixed_delta = 0.0;
	double update_delta = 0.0;
	bool claims_movie = false;
	bool needs_live_audio = false;
	Error begin_result = OK;

	int claim_calls = 0;
	int policy_calls = 0;
	int begin_calls = 0;
	int frame_calls = 0;
	int end_calls = 0;

	MainLoopPhaseCallbackBus *bus = nullptr;
	MainLoopPhaseCallbackBus::RegistrationID registration_id = MainLoopPhaseCallbackBus::INVALID_REGISTRATION_ID;
	bool unregister_on_claim = false;
	bool unregister_on_policy = false;
	bool unregister_on_begin = false;
	bool unregister_on_frame = false;
	bool unregister_on_end = false;
	bool last_unregister_result = true;
	bool reenter_begin = false;
	Error reentrant_begin_result = OK;
};

static void _record_start(void *p_userdata, MainLoop *) {
	CallbackRecorder *recorder = static_cast<CallbackRecorder *>(p_userdata);
	recorder->events->push_back(recorder->subscriber * 10 + 1);
}

static void _record_fixed_update(void *p_userdata, double p_delta) {
	CallbackRecorder *recorder = static_cast<CallbackRecorder *>(p_userdata);
	recorder->fixed_delta = p_delta;
	recorder->events->push_back(recorder->subscriber * 10 + 2);
}

static void _record_update(void *p_userdata, double p_delta) {
	CallbackRecorder *recorder = static_cast<CallbackRecorder *>(p_userdata);
	recorder->update_delta = p_delta;
	recorder->events->push_back(recorder->subscriber * 10 + 3);
}

static void _record_destroy(void *p_userdata) {
	CallbackRecorder *recorder = static_cast<CallbackRecorder *>(p_userdata);
	recorder->events->push_back(recorder->subscriber * 10 + 4);
}

static bool _claim_movie(void *p_userdata, const String &) {
	CallbackRecorder *recorder = static_cast<CallbackRecorder *>(p_userdata);
	recorder->claim_calls++;
	if (recorder->unregister_on_claim) {
		recorder->last_unregister_result = recorder->bus->unregister_callbacks(recorder->registration_id);
	}
	return recorder->claims_movie;
}

static bool _needs_live_audio(void *p_userdata, const String &) {
	CallbackRecorder *recorder = static_cast<CallbackRecorder *>(p_userdata);
	recorder->policy_calls++;
	if (recorder->unregister_on_policy) {
		recorder->last_unregister_result = recorder->bus->unregister_callbacks(recorder->registration_id);
	}
	return recorder->needs_live_audio;
}

static Error _record_movie_begin(void *p_userdata, const Size2i &p_size, uint32_t p_fps, const String &p_path) {
	CallbackRecorder *recorder = static_cast<CallbackRecorder *>(p_userdata);
	recorder->begin_calls++;
	recorder->events->push_back(recorder->subscriber * 10 + 5);
	if (recorder->unregister_on_begin) {
		recorder->last_unregister_result = recorder->bus->unregister_callbacks(recorder->registration_id);
	}
	if (recorder->reenter_begin) {
		recorder->reentrant_begin_result = recorder->bus->notify_movie_begin(p_size, p_fps, p_path);
	}
	return recorder->begin_result;
}

static void _record_movie_frame(void *p_userdata) {
	CallbackRecorder *recorder = static_cast<CallbackRecorder *>(p_userdata);
	recorder->frame_calls++;
	recorder->events->push_back(recorder->subscriber * 10 + 6);
	if (recorder->unregister_on_frame) {
		recorder->last_unregister_result = recorder->bus->unregister_callbacks(recorder->registration_id);
	}
}

static void _record_movie_end(void *p_userdata) {
	CallbackRecorder *recorder = static_cast<CallbackRecorder *>(p_userdata);
	recorder->end_calls++;
	recorder->events->push_back(recorder->subscriber * 10 + 7);
	if (recorder->unregister_on_end) {
		recorder->last_unregister_result = recorder->bus->unregister_callbacks(recorder->registration_id);
	}
}

static MainLoopPhaseCallbackBus::Callbacks _callbacks_for(CallbackRecorder *p_recorder) {
	MainLoopPhaseCallbackBus::Callbacks callbacks;
	callbacks.userdata = p_recorder;
	callbacks.start = &_record_start;
	callbacks.fixed_update = &_record_fixed_update;
	callbacks.update = &_record_update;
	callbacks.destroy = &_record_destroy;
	return callbacks;
}

static MainLoopPhaseCallbackBus::Callbacks _movie_callbacks_for(CallbackRecorder *p_recorder) {
	MainLoopPhaseCallbackBus::Callbacks callbacks;
	callbacks.userdata = p_recorder;
	callbacks.movie_claim = &_claim_movie;
	callbacks.movie_requires_live_audio = &_needs_live_audio;
	callbacks.movie_begin = &_record_movie_begin;
	callbacks.movie_frame = &_record_movie_frame;
	callbacks.movie_end = &_record_movie_end;
	return callbacks;
}

TEST_CASE("[Main] Loop phase callbacks preserve registration and phase order") {
	MainLoopPhaseCallbackBus bus;
	LocalVector<int> events;
	CallbackRecorder first{ &events, 1 };
	CallbackRecorder second{ &events, 2 };

	const MainLoopPhaseCallbackBus::RegistrationID first_id = bus.register_callbacks(_callbacks_for(&first));
	const MainLoopPhaseCallbackBus::RegistrationID second_id = bus.register_callbacks(_callbacks_for(&second));
	CHECK(first_id != MainLoopPhaseCallbackBus::INVALID_REGISTRATION_ID);
	CHECK(second_id != MainLoopPhaseCallbackBus::INVALID_REGISTRATION_ID);
	CHECK(first_id != second_id);
	CHECK(bus.get_registration_count() == 2);

	bus.notify_start(nullptr);
	bus.notify_fixed_update(0.25);
	bus.notify_update(0.5);
	bus.notify_destroy();

	REQUIRE(events.size() == 8);
	CHECK(events[0] == 11);
	CHECK(events[1] == 21);
	CHECK(events[2] == 12);
	CHECK(events[3] == 22);
	CHECK(events[4] == 13);
	CHECK(events[5] == 23);
	CHECK(events[6] == 14);
	CHECK(events[7] == 24);
	CHECK(first.fixed_delta == 0.25);
	CHECK(second.fixed_delta == 0.25);
	CHECK(first.update_delta == 0.5);
	CHECK(second.update_delta == 0.5);

	CHECK(bus.unregister_callbacks(first_id));
	CHECK_FALSE(bus.unregister_callbacks(first_id));
	CHECK(bus.get_registration_count() == 1);
}

TEST_CASE("[Main] Movie callback registration requires a complete lifecycle") {
	MainLoopPhaseCallbackBus bus;
	MainLoopPhaseCallbackBus::Callbacks incomplete;
	incomplete.movie_claim = &_claim_movie;
	ERR_PRINT_OFF;
	const MainLoopPhaseCallbackBus::RegistrationID id = bus.register_callbacks(incomplete);
	ERR_PRINT_ON;
	CHECK(id == MainLoopPhaseCallbackBus::INVALID_REGISTRATION_ID);
}

TEST_CASE("[Main] Movie route resolves one stable owner and caches its audio policy") {
	MainLoopPhaseCallbackBus bus;
	LocalVector<int> events;
	CallbackRecorder declined{ &events, 1 };
	CallbackRecorder claimed{ &events, 2 };
	CallbackRecorder later_claim{ &events, 3 };
	declined.claims_movie = false;
	declined.needs_live_audio = true;
	claimed.claims_movie = true;
	claimed.needs_live_audio = true;
	later_claim.claims_movie = true;
	later_claim.needs_live_audio = false;

	bus.register_callbacks(_movie_callbacks_for(&declined));
	bus.register_callbacks(_movie_callbacks_for(&claimed));
	bus.register_callbacks(_movie_callbacks_for(&later_claim));

	CHECK(bus.resolve_movie_route("capture.avi") == OK);
	CHECK(bus.is_movie_route_resolved());
	CHECK(bus.is_movie_route_claimed());
	CHECK(bus.movie_route_requires_live_audio());
	CHECK(declined.claim_calls == 1);
	CHECK(declined.policy_calls == 0);
	CHECK(claimed.claim_calls == 1);
	CHECK(claimed.policy_calls == 1);
	CHECK(later_claim.claim_calls == 0);

	// Route ownership and audio policy remain stable after resolution.
	claimed.claims_movie = false;
	claimed.needs_live_audio = false;
	CHECK(bus.is_movie_route_claimed());
	CHECK(bus.movie_route_requires_live_audio());
	CHECK(bus.notify_movie_begin(Size2i(640, 480), 30, "capture.avi") == OK);
	bus.notify_movie_frame();
	bus.notify_movie_end();

	REQUIRE(events.size() == 3);
	CHECK(events[0] == 25);
	CHECK(events[1] == 26);
	CHECK(events[2] == 27);
	CHECK_FALSE(bus.is_movie_route_resolved());
}

TEST_CASE("[Main] Movie route safely skips callbacks which unregister while resolving") {
	LocalVector<int> events;

	SUBCASE("claim callback unregisters itself") {
		MainLoopPhaseCallbackBus bus;
		CallbackRecorder departing{ &events, 1 };
		CallbackRecorder fallback{ &events, 2 };
		departing.claims_movie = true;
		departing.needs_live_audio = true;
		departing.bus = &bus;
		departing.unregister_on_claim = true;
		fallback.claims_movie = true;
		departing.registration_id = bus.register_callbacks(_movie_callbacks_for(&departing));
		fallback.registration_id = bus.register_callbacks(_movie_callbacks_for(&fallback));

		CHECK(bus.resolve_movie_route("capture.avi") == OK);
		CHECK(departing.last_unregister_result);
		CHECK(departing.policy_calls == 0);
		CHECK(bus.notify_movie_begin(Size2i(1, 1), 30, "capture.avi") == OK);
		bus.notify_movie_end();
		CHECK(fallback.begin_calls == 1);
		CHECK(fallback.end_calls == 1);
	}

	SUBCASE("audio policy callback unregisters itself") {
		MainLoopPhaseCallbackBus bus;
		CallbackRecorder departing{ &events, 1 };
		CallbackRecorder fallback{ &events, 2 };
		departing.claims_movie = true;
		departing.needs_live_audio = true;
		departing.bus = &bus;
		departing.unregister_on_policy = true;
		fallback.claims_movie = true;
		departing.registration_id = bus.register_callbacks(_movie_callbacks_for(&departing));
		fallback.registration_id = bus.register_callbacks(_movie_callbacks_for(&fallback));

		CHECK(bus.resolve_movie_route("capture.avi") == OK);
		CHECK(departing.last_unregister_result);
		CHECK(bus.notify_movie_begin(Size2i(1, 1), 30, "capture.avi") == OK);
		bus.notify_movie_end();
		CHECK(fallback.begin_calls == 1);
		CHECK(fallback.end_calls == 1);
	}
}

TEST_CASE("[Main] Movie begin failure never activates the route") {
	MainLoopPhaseCallbackBus bus;
	LocalVector<int> events;
	CallbackRecorder recorder{ &events, 1 };
	recorder.claims_movie = true;
	recorder.begin_result = ERR_CANT_CREATE;
	recorder.registration_id = bus.register_callbacks(_movie_callbacks_for(&recorder));

	CHECK(bus.resolve_movie_route("capture.avi") == OK);
	CHECK(bus.notify_movie_begin(Size2i(1, 1), 30, "capture.avi") == ERR_CANT_CREATE);
	bus.notify_movie_frame();
	bus.notify_movie_end();
	CHECK(recorder.begin_calls == 1);
	CHECK(recorder.frame_calls == 0);
	CHECK(recorder.end_calls == 0);
	CHECK_FALSE(bus.is_movie_route_resolved());
	CHECK(bus.unregister_callbacks(recorder.registration_id));
}

TEST_CASE("[Main] Resolved movie owner can unregister before begin") {
	MainLoopPhaseCallbackBus bus;
	LocalVector<int> events;
	CallbackRecorder recorder{ &events, 1 };
	recorder.claims_movie = true;
	recorder.registration_id = bus.register_callbacks(_movie_callbacks_for(&recorder));

	CHECK(bus.resolve_movie_route("capture.avi") == OK);
	CHECK(bus.is_movie_route_claimed());
	CHECK(bus.unregister_callbacks(recorder.registration_id));
	CHECK(bus.get_registration_count() == 0);
	CHECK_FALSE(bus.is_movie_route_resolved());
	ERR_PRINT_OFF;
	CHECK(bus.notify_movie_begin(Size2i(1, 1), 30, "capture.avi") == ERR_UNAVAILABLE);
	ERR_PRINT_ON;
	CHECK(recorder.begin_calls == 0);
}

TEST_CASE("[Main] Movie begin must use the resolved path") {
	MainLoopPhaseCallbackBus bus;
	LocalVector<int> events;
	CallbackRecorder recorder{ &events, 1 };
	recorder.claims_movie = true;
	recorder.registration_id = bus.register_callbacks(_movie_callbacks_for(&recorder));

	CHECK(bus.resolve_movie_route("capture.avi") == OK);
	ERR_PRINT_OFF;
	CHECK(bus.notify_movie_begin(Size2i(1, 1), 30, "other.avi") == ERR_INVALID_PARAMETER);
	ERR_PRINT_ON;
	CHECK(bus.is_movie_route_claimed());
	CHECK(recorder.begin_calls == 0);
	CHECK(bus.notify_movie_begin(Size2i(1, 1), 30, "capture.avi") == OK);
	bus.notify_movie_end();
	CHECK(recorder.begin_calls == 1);
	CHECK(recorder.end_calls == 1);
}

TEST_CASE("[Main] Unregistering a non-owner does not end another movie route") {
	MainLoopPhaseCallbackBus bus;
	LocalVector<int> events;
	CallbackRecorder departing{ &events, 1 };
	CallbackRecorder owner{ &events, 2 };
	departing.claims_movie = false;
	owner.claims_movie = true;
	departing.registration_id = bus.register_callbacks(_movie_callbacks_for(&departing));
	owner.registration_id = bus.register_callbacks(_movie_callbacks_for(&owner));

	CHECK(bus.resolve_movie_route("capture.avi") == OK);
	CHECK(bus.notify_movie_begin(Size2i(1, 1), 30, "capture.avi") == OK);
	CHECK(bus.unregister_callbacks(departing.registration_id));
	CHECK(bus.is_movie_route_claimed());
	bus.notify_movie_frame();
	bus.notify_movie_end();
	CHECK(owner.begin_calls == 1);
	CHECK(owner.frame_calls == 1);
	CHECK(owner.end_calls == 1);
}

TEST_CASE("[Main] Active movie owner is leased through exactly one end callback") {
	MainLoopPhaseCallbackBus bus;
	LocalVector<int> events;
	CallbackRecorder recorder{ &events, 1 };
	recorder.claims_movie = true;
	recorder.bus = &bus;
	recorder.unregister_on_begin = true;
	recorder.unregister_on_frame = true;
	recorder.unregister_on_end = true;
	recorder.reenter_begin = true;
	recorder.registration_id = bus.register_callbacks(_movie_callbacks_for(&recorder));

	CHECK(bus.resolve_movie_route("capture.avi") == OK);
	ERR_PRINT_OFF;
	CHECK(bus.notify_movie_begin(Size2i(1, 1), 30, "capture.avi") == OK);
	ERR_PRINT_ON;
	CHECK_FALSE(recorder.last_unregister_result);
	CHECK(recorder.reentrant_begin_result == ERR_ALREADY_IN_USE);
	CHECK_FALSE(bus.unregister_callbacks(recorder.registration_id));

	ERR_PRINT_OFF;
	CHECK(bus.notify_movie_begin(Size2i(1, 1), 30, "capture.avi") == ERR_ALREADY_IN_USE);
	ERR_PRINT_ON;
	bus.notify_movie_frame();
	CHECK_FALSE(recorder.last_unregister_result);
	bus.notify_movie_end();
	CHECK_FALSE(recorder.last_unregister_result);
	bus.notify_movie_end();
	CHECK(recorder.begin_calls == 1);
	CHECK(recorder.frame_calls == 1);
	CHECK(recorder.end_calls == 1);
	CHECK(bus.unregister_callbacks(recorder.registration_id));
}

TEST_CASE("[Main] Unclaimed movie route never receives begin") {
	MainLoopPhaseCallbackBus bus;
	LocalVector<int> events;
	CallbackRecorder recorder{ &events, 1 };
	recorder.claims_movie = false;
	bus.register_callbacks(_movie_callbacks_for(&recorder));

	CHECK(bus.resolve_movie_route("capture.avi") == OK);
	CHECK_FALSE(bus.is_movie_route_claimed());
	ERR_PRINT_OFF;
	CHECK(bus.notify_movie_begin(Size2i(1, 1), 30, "capture.avi") == ERR_UNAVAILABLE);
	ERR_PRINT_ON;
	bus.notify_movie_end();
	CHECK(recorder.begin_calls == 0);
	CHECK_FALSE(bus.is_movie_route_resolved());
}

} // namespace TestMainLoopPhaseCallbackBus

#endif // TEST_MAIN_LOOP_PHASE_CALLBACK_BUS_H
