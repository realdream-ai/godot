/**************************************************************************/
/*  test_audio_stream_player.h                                            */
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

#ifndef TEST_AUDIO_STREAM_PLAYER_H
#define TEST_AUDIO_STREAM_PLAYER_H

#include "scene/2d/audio_stream_player_2d.h"
#include "scene/main/window.h"
#include "scene/resources/audio_stream_wav.h"
#include "scene/resources/world_2d.h"
#include "servers/audio/audio_stream.h"

#ifndef _3D_DISABLED
#include "scene/3d/audio_stream_player_3d.h"
#include "scene/3d/camera_3d.h"
#endif

#include "tests/test_macros.h"

namespace TestAudioStreamPlayer {

// Capture the parameters the browser would receive when it starts a sample.
// No real audio device or timing-dependent sound capture is needed.
class SampleDriver : public AudioDriver {
public:
	struct Start {
		ObjectID id;
		Vector<AudioFrame> volumes;
		StringName bus;
		float offset = 0.0f;
		float pitch = 1.0f;
	};
	Vector<Start> starts;
	HashSet<ObjectID> active;
	HashSet<ObjectID> paused;
	int stops = 0;

	const char *get_name() const override { return "Sample test driver"; }
	Error init() override { return OK; }
	void start() override {}
	int get_mix_rate() const override { return 44100; }
	SpeakerMode get_speaker_mode() const override { return SPEAKER_MODE_STEREO; }
	void lock() override {}
	void unlock() override {}
	void finish() override {}
	void mix() {
		Vector<int32_t> buffer;
		buffer.resize(1024 * 2);
		audio_server_process(1024, buffer.ptrw());
	}

	void start_sample_playback(const Ref<AudioSamplePlayback> &p_playback) override {
		Start started;
		started.id = p_playback->get_instance_id();
		started.volumes = p_playback->volume_vector;
		started.bus = p_playback->bus;
		started.offset = p_playback->offset;
		started.pitch = p_playback->pitch_scale;
		starts.push_back(started);
		active.insert(started.id);
	}
	void stop_sample_playback(const Ref<AudioSamplePlayback> &p_playback) override {
		active.erase(p_playback->get_instance_id());
		paused.erase(p_playback->get_instance_id());
		stops++;
	}
	void set_sample_playback_pause(const Ref<AudioSamplePlayback> &p_playback, bool p_paused) override {
		if (p_paused) {
			paused.insert(p_playback->get_instance_id());
		} else {
			paused.erase(p_playback->get_instance_id());
		}
	}
	bool is_sample_playback_active(const Ref<AudioSamplePlayback> &p_playback) override {
		return active.has(p_playback->get_instance_id());
	}
};

struct Fixture {
	SampleDriver driver;
	AudioDriver *previous_driver = nullptr;
	AudioServer *server = nullptr;
	SubViewport *viewport = nullptr;
	Ref<AudioStreamWAV> stream;

	Fixture() {
		// Reinitialize the standard dummy driver so AudioServer::finish() can
		// clean it up safely. Only our test driver is started by AudioServer.
		AudioDriverManager::initialize(AudioDriverManager::get_driver_count() - 1);
		previous_driver = AudioDriver::get_singleton();
		driver.set_singleton();
		server = memnew(AudioServer);
		server->init();
		server->add_bus();
		server->set_bus_name(1, "Effects");

		viewport = memnew(SubViewport);
		viewport->set_size(Vector2i(400, 200));
		viewport->set_world_2d(Ref<World2D>(memnew(World2D)));
		viewport->set_as_audio_listener_2d(true);
		SceneTree::get_singleton()->get_root()->add_child(viewport);

		stream.instantiate();
		stream->set_mix_rate(44100);
		Vector<uint8_t> data;
		data.resize(441); // 10 ms, shorter than one 60 Hz physics frame.
		data.fill(64);
		stream->set_data(data);
	}

	AudioStreamPlayer2D *create_player() {
		AudioStreamPlayer2D *player = memnew(AudioStreamPlayer2D);
		player->set_stream(stream);
		player->set_playback_type(AudioServer::PLAYBACK_TYPE_SAMPLE);
		player->set_position(Vector2(200, 100));
		viewport->add_child(player);
		return player;
	}

	void physics(Node *p_player) {
		PhysicsServer2D::get_singleton()->sync();
#ifndef _3D_DISABLED
		PhysicsServer3D::get_singleton()->sync();
#endif
		p_player->notification(Node::NOTIFICATION_INTERNAL_PHYSICS_PROCESS);
#ifndef _3D_DISABLED
		PhysicsServer3D::get_singleton()->end_sync();
#endif
		PhysicsServer2D::get_singleton()->end_sync();
	}

	~Fixture() {
		memdelete(viewport);
		driver.mix();
		server->update();
		server->finish();
		memdelete(server);
		previous_driver->set_singleton();
	}
};

TEST_CASE("[SceneTree][AudioStreamPlayer2D] Sample starts with current spatial parameters") {
	Fixture fixture;
	AudioStreamPlayer2D *player = fixture.create_player();
	player->play();
	CHECK(fixture.driver.starts.is_empty());
	CHECK(player->is_playing());
	player->set_volume_linear(0.4f);
	player->set_pitch_scale(1.5f);
	player->set_bus("Effects");
	fixture.physics(player);
	REQUIRE(fixture.driver.starts.size() == 1);
	const SampleDriver::Start &started = fixture.driver.starts[0];
	REQUIRE(started.volumes.size() == 4);
	CHECK(started.volumes[0].left == doctest::Approx(0.2f));
	CHECK(started.volumes[0].right == doctest::Approx(0.2f));
	CHECK(started.pitch == doctest::Approx(1.5f));
	CHECK(started.offset == 0.0f);
	CHECK(started.bus == StringName("Effects"));

	// Replay from zero without another audio mix: refresh position and volume.
	player->stop();
	player->play();
	player->set_position(Vector2(300, 100));
	player->set_volume_linear(0.2f);
	fixture.physics(player);
	REQUIRE(fixture.driver.starts.size() == 2);
	const Vector<AudioFrame> &volumes = fixture.driver.starts[1].volumes;
	CHECK(volumes[0].left < volumes[0].right);
	CHECK(volumes[0].left + volumes[0].right == doctest::Approx(0.19f));
}

TEST_CASE("[SceneTree][AudioStreamPlayer2D] Pending samples retain pause and seek") {
	Fixture fixture;
	AudioStreamPlayer2D *player = fixture.create_player();
	player->play();
	player->set_stream_paused(true);
	player->seek(0.003f);
	fixture.physics(player);
	fixture.physics(player);
	CHECK(fixture.driver.starts.is_empty());
	CHECK(player->is_playing());
	CHECK(player->get_stream_paused());
	CHECK(player->get_playback_position() == doctest::Approx(0.003f));
	player->set_stream_paused(false);
	fixture.physics(player);
	REQUIRE(fixture.driver.starts.size() == 1);
	CHECK(fixture.driver.starts[0].offset == doctest::Approx(0.003f));
	CHECK(player->is_playing());
	player->set_stream_paused(true);
	CHECK(player->get_stream_paused());
	CHECK(fixture.driver.paused.size() == 1);
	player->set_stream_paused(false);
	CHECK_FALSE(player->get_stream_paused());
	CHECK(fixture.driver.paused.is_empty());
}

TEST_CASE("[SceneTree][AudioStreamPlayer2D] Cancelled samples do not start or retain references") {
	Fixture fixture;
	AudioStreamPlayer2D *player = fixture.create_player();
	SIGNAL_WATCH(player, "finished");
	player->play();
	Ref<AudioStreamPlayback> playback = player->get_stream_playback();
	ObjectID playback_id = playback->get_instance_id();
	ObjectID sample_id = playback->get_sample_playback()->get_instance_id();
	player->stop();
	CHECK(playback->get_sample_playback().is_null());
	playback.unref();
	CHECK(ObjectDB::get_instance(playback_id) == nullptr);
	CHECK(ObjectDB::get_instance(sample_id) == nullptr);
	fixture.physics(player);
	CHECK(fixture.driver.starts.is_empty());
	CHECK_FALSE(player->is_playing());
	SIGNAL_CHECK_FALSE("finished");

	player->play();
	playback = player->get_stream_playback();
	player->set_stream(fixture.stream);
	CHECK(playback->get_sample_playback().is_null());
	fixture.physics(player);
	CHECK(fixture.driver.starts.is_empty());

	player->play();
	playback = player->get_stream_playback();
	memdelete(player);
	CHECK(playback->get_sample_playback().is_null());
}

TEST_CASE("[SceneTree][AudioStreamPlayer2D] Same-frame sample polyphony and finished") {
	Fixture fixture;
	AudioStreamPlayer2D *player = fixture.create_player();
	SIGNAL_WATCH(player, "finished");
	player->set_max_polyphony(2);
	player->play();
	Ref<AudioStreamPlayback> discarded = player->get_stream_playback();
	player->play(0.001f);
	player->play(0.002f);
	CHECK(fixture.driver.starts.is_empty());
	fixture.physics(player);
	REQUIRE(fixture.driver.starts.size() == 2);
	CHECK(fixture.driver.starts[0].offset == doctest::Approx(0.001f));
	CHECK(fixture.driver.starts[1].offset == doctest::Approx(0.002f));
	CHECK(discarded->get_sample_playback().is_null());
	SIGNAL_CHECK_FALSE("finished");

	// Model the completion notification sent by the browser for the newest voice.
	Ref<AudioSamplePlayback> sample = player->get_stream_playback()->get_sample_playback();
	fixture.server->stop_sample_playback(sample);
	fixture.physics(player);
	CHECK(player->is_playing());
	Array finished_signals;
	finished_signals.push_back(Array());
	SIGNAL_CHECK("finished", finished_signals);
	player->stop();
	fixture.physics(player);
	CHECK_FALSE(player->is_playing());
	CHECK(fixture.driver.active.is_empty());
	SIGNAL_CHECK_FALSE("finished");
}

TEST_CASE("[SceneTree][AudioStreamPlayer2D] Finished samples do not consume a polyphony slot") {
	Fixture fixture;
	AudioStreamPlayer2D *player = fixture.create_player();
	player->set_max_polyphony(2);
	player->play();
	fixture.physics(player);
	Ref<AudioSamplePlayback> first = player->get_stream_playback()->get_sample_playback();
	player->play();
	fixture.physics(player);
	Ref<AudioSamplePlayback> second = player->get_stream_playback()->get_sample_playback();
	fixture.server->stop_sample_playback(second);
	player->play();
	fixture.physics(player);
	CHECK(fixture.driver.active.has(first->get_instance_id()));
	CHECK(fixture.driver.active.size() == 2);
}

TEST_CASE("[SceneTree][AudioStreamPlayer2D] Stream playback retains pending controls") {
	Fixture fixture;
	AudioStreamPlayer2D *player = fixture.create_player();
	player->set_playback_type(AudioServer::PLAYBACK_TYPE_STREAM);
	player->play();
	Ref<AudioStreamPlayback> playback = player->get_stream_playback();
	CHECK_FALSE(playback->is_playing());
	player->set_stream_paused(true);
	player->seek(0.004f);
	fixture.physics(player);
	CHECK_FALSE(playback->is_playing());
	CHECK(player->get_stream_paused());
	player->set_stream_paused(false);
	fixture.physics(player);
	CHECK(playback->is_playing());
	CHECK(player->get_playback_position() == doctest::Approx(0.004f).epsilon(0.01));
	CHECK(fixture.driver.starts.is_empty());
	player->stop();
	CHECK_FALSE(player->is_playing());
	fixture.driver.mix();
	fixture.physics(player);
	CHECK_FALSE(player->has_stream_playback());
}

#ifndef _3D_DISABLED
TEST_CASE("[SceneTree][AudioStreamPlayer3D] Sample starts with current spatial parameters") {
	Fixture fixture;
	fixture.viewport->set_as_audio_listener_3d(true);
	Camera3D *camera = memnew(Camera3D);
	fixture.viewport->add_child(camera);
	camera->make_current();
	AudioStreamPlayer3D *player = memnew(AudioStreamPlayer3D);
	player->set_stream(fixture.stream);
	player->set_playback_type(AudioServer::PLAYBACK_TYPE_SAMPLE);
	player->set_attenuation_model(AudioStreamPlayer3D::ATTENUATION_DISABLED);
	player->set_position(Vector3(0, 0, -10));
	fixture.viewport->add_child(player);
	player->play();
	CHECK(fixture.driver.starts.is_empty());
	player->set_pitch_scale(1.5f);
	player->set_volume_linear(0.4f);
	player->set_bus("Effects");
	fixture.physics(player);
	REQUIRE(fixture.driver.starts.size() == 1);
	const SampleDriver::Start &started = fixture.driver.starts[0];
	REQUIRE(started.volumes.size() == 4);
	CHECK(started.volumes[0].left == doctest::Approx(0.4f / Math_SQRT2));
	CHECK(started.volumes[0].right == doctest::Approx(0.4f / Math_SQRT2));
	CHECK(started.pitch == doctest::Approx(1.5f));
	CHECK(started.bus == StringName("Effects"));

	player->stop();
	fixture.viewport->set_as_audio_listener_3d(false);
	player->set_pitch_scale(2.0f);
	player->play();
	fixture.physics(player);
	REQUIRE(fixture.driver.starts.size() == 2);
	const SampleDriver::Start &silent = fixture.driver.starts[1];
	CHECK(silent.pitch == doctest::Approx(2.0f));
	REQUIRE(silent.volumes.size() == 4);
	for (const AudioFrame &volume : silent.volumes) {
		CHECK(volume.left == 0.0f);
		CHECK(volume.right == 0.0f);
	}
}
#endif

} // namespace TestAudioStreamPlayer

#endif // TEST_AUDIO_STREAM_PLAYER_H
