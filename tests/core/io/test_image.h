/**************************************************************************/
/*  test_image.h                                                          */
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

#ifndef TEST_IMAGE_H
#define TEST_IMAGE_H

#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/os/os.h"

#include "tests/test_utils.h"

#include "modules/modules_enabled.gen.h"

#ifdef MODULE_SVG_ENABLED
#include "modules/svg/image_loader_svg.h"
#include "modules/svg/svg_utils.h"
#include "thirdparty/lunasvg/include/lunasvg.h"
#include "thirdparty/lunasvg/source/embedded_cnfont.h"
#include "thirdparty/lunasvg/source/graphics.h"
#include "thirdparty/lunasvg/source/svgtextelement.h"
#include <algorithm>
#include <cmath>
#include <thread>
#include <utility>
#include <vector>
#endif

#include "thirdparty/doctest/doctest.h"

namespace TestImage {

TEST_CASE("[Image] Instantiation") {
	Ref<Image> image = memnew(Image(8, 4, false, Image::FORMAT_RGBA8));
	CHECK_MESSAGE(
			!image->is_empty(),
			"An image created with specified size and format should not be empty at first.");
	CHECK_MESSAGE(
			image->is_invisible(),
			"A newly created image should be invisible.");
	CHECK_MESSAGE(
			!image->is_compressed(),
			"A newly created image should not be compressed.");
	CHECK(!image->has_mipmaps());

	PackedByteArray image_data = image->get_data();
	for (int i = 0; i < image_data.size(); i++) {
		CHECK_MESSAGE(
				image_data[i] == 0,
				"An image created without data specified should have its data zeroed out.");
	}

	Ref<Image> image_copy = memnew(Image());
	CHECK_MESSAGE(
			image_copy->is_empty(),
			"An image created without any specified size and format be empty at first.");
	image_copy->copy_internals_from(image);

	CHECK_MESSAGE(
			image->get_data() == image_copy->get_data(),
			"Duplicated images should have the same data.");

	image_data = image->get_data();
	Ref<Image> image_from_data = memnew(Image(8, 4, false, Image::FORMAT_RGBA8, image_data));
	CHECK_MESSAGE(
			image->get_data() == image_from_data->get_data(),
			"An image created from data of another image should have the same data of the original image.");
}

TEST_CASE("[Image] Saving and loading") {
	Ref<Image> image = memnew(Image(4, 4, false, Image::FORMAT_RGBA8));
	const String save_path_png = TestUtils::get_temp_path("image.png");
	const String save_path_exr = TestUtils::get_temp_path("image.exr");

	// Save PNG
	Error err;
	err = image->save_png(save_path_png);
	CHECK_MESSAGE(
			err == OK,
			"The image should be saved successfully as a .png file.");

	// Only available on editor builds.
#ifdef TOOLS_ENABLED
	// Save EXR
	err = image->save_exr(save_path_exr, false);
	CHECK_MESSAGE(
			err == OK,
			"The image should be saved successfully as an .exr file.");
#endif // TOOLS_ENABLED

	// Load using load()
	Ref<Image> image_load = memnew(Image());
	err = image_load->load(save_path_png);
	CHECK_MESSAGE(
			err == OK,
			"The image should load successfully using load().");
	CHECK_MESSAGE(
			image->get_data() == image_load->get_data(),
			"The loaded image should have the same data as the one that got saved.");

#ifdef MODULE_BMP_ENABLED
	// Load BMP
	Ref<Image> image_bmp = memnew(Image());
	Ref<FileAccess> f_bmp = FileAccess::open(TestUtils::get_data_path("images/icon.bmp"), FileAccess::READ, &err);
	REQUIRE(f_bmp.is_valid());
	PackedByteArray data_bmp;
	data_bmp.resize(f_bmp->get_length() + 1);
	f_bmp->get_buffer(data_bmp.ptrw(), f_bmp->get_length());
	CHECK_MESSAGE(
			image_bmp->load_bmp_from_buffer(data_bmp) == OK,
			"The BMP image should load successfully.");
#endif // MODULE_BMP_ENABLED

#ifdef MODULE_JPG_ENABLED
	// Load JPG
	Ref<Image> image_jpg = memnew(Image());
	Ref<FileAccess> f_jpg = FileAccess::open(TestUtils::get_data_path("images/icon.jpg"), FileAccess::READ, &err);
	REQUIRE(f_jpg.is_valid());
	PackedByteArray data_jpg;
	data_jpg.resize(f_jpg->get_length() + 1);
	f_jpg->get_buffer(data_jpg.ptrw(), f_jpg->get_length());
	CHECK_MESSAGE(
			image_jpg->load_jpg_from_buffer(data_jpg) == OK,
			"The JPG image should load successfully.");
#endif // MODULE_JPG_ENABLED

#ifdef MODULE_WEBP_ENABLED
	// Load WebP
	Ref<Image> image_webp = memnew(Image());
	Ref<FileAccess> f_webp = FileAccess::open(TestUtils::get_data_path("images/icon.webp"), FileAccess::READ, &err);
	REQUIRE(f_webp.is_valid());
	PackedByteArray data_webp;
	data_webp.resize(f_webp->get_length() + 1);
	f_webp->get_buffer(data_webp.ptrw(), f_webp->get_length());
	CHECK_MESSAGE(
			image_webp->load_webp_from_buffer(data_webp) == OK,
			"The WebP image should load successfully.");
#endif // MODULE_WEBP_ENABLED

	// Load PNG
	Ref<Image> image_png = memnew(Image());
	Ref<FileAccess> f_png = FileAccess::open(TestUtils::get_data_path("images/icon.png"), FileAccess::READ, &err);
	REQUIRE(f_png.is_valid());
	PackedByteArray data_png;
	data_png.resize(f_png->get_length() + 1);
	f_png->get_buffer(data_png.ptrw(), f_png->get_length());
	CHECK_MESSAGE(
			image_png->load_png_from_buffer(data_png) == OK,
			"The PNG image should load successfully.");

#ifdef MODULE_TGA_ENABLED
	// Load TGA
	Ref<Image> image_tga = memnew(Image());
	Ref<FileAccess> f_tga = FileAccess::open(TestUtils::get_data_path("images/icon.tga"), FileAccess::READ, &err);
	REQUIRE(f_tga.is_valid());
	PackedByteArray data_tga;
	data_tga.resize(f_tga->get_length() + 1);
	f_tga->get_buffer(data_tga.ptrw(), f_tga->get_length());
	CHECK_MESSAGE(
			image_tga->load_tga_from_buffer(data_tga) == OK,
			"The TGA image should load successfully.");
#endif // MODULE_TGA_ENABLED
}

TEST_CASE("[Image] Basic getters") {
	Ref<Image> image = memnew(Image(8, 4, false, Image::FORMAT_LA8));
	CHECK(image->get_width() == 8);
	CHECK(image->get_height() == 4);
	CHECK(image->get_size() == Vector2(8, 4));
	CHECK(image->get_format() == Image::FORMAT_LA8);
	CHECK(image->get_used_rect() == Rect2i(0, 0, 0, 0));
	Ref<Image> image_get_rect = image->get_region(Rect2i(0, 0, 2, 1));
	CHECK(image_get_rect->get_size() == Vector2(2, 1));
}

#ifdef MODULE_SVG_ENABLED
TEST_CASE("[Image] SVG loading keeps unpremultiplied RGBA data") {
	Ref<Image> image = memnew(Image());
	const String svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"2\" height=\"3\"><rect width=\"2\" height=\"3\" fill=\"#ff000080\"/></svg>";

	CHECK(ImageLoaderSVG::create_image_from_string(image, svg, 2.0f, false, HashMap<Color, Color>()) == OK);
	CHECK(image->get_width() == 4);
	CHECK(image->get_height() == 6);

	const Color pixel = image->get_pixel(0, 0);
	CHECK(pixel.r > 0.95f);
	CHECK(pixel.g < 0.05f);
	CHECK(pixel.b < 0.05f);
	CHECK(pixel.a == doctest::Approx(0.5f).epsilon(0.05f));
}

TEST_CASE("[Image] SVG loading rejects oversized rasterization") {
	Ref<Image> image = memnew(Image());
	const String svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"20000\" height=\"20000\"><rect width=\"20000\" height=\"20000\" fill=\"#000\"/></svg>";

	CHECK(ImageLoaderSVG::create_image_from_string(image, svg, 1.0f, false, HashMap<Color, Color>()) == ERR_INVALID_DATA);
}

struct LunaSVGTextConfigurationReset {
	~LunaSVGTextConfigurationReset() {
		lunasvg_set_grapheme_break_func(nullptr, nullptr);
		lunasvg_set_font_preferences(nullptr, 0);
		lunasvg::setShapingObserverFunction(nullptr, nullptr);
	}
};

struct SVGUtilsFontRegistryReset {
	SVGUtilsFontRegistryReset() {
		reset();
	}

	~SVGUtilsFontRegistryReset() {
		reset();
		lunasvg_set_grapheme_break_func(nullptr, nullptr);
	}

private:
	void reset() {
		SVGUtils::reset_font_registry();
		SVGUtils::ensure_font_faces_registered();
	}
};

static void luna_set_strict_empty_font_preferences() {
	const char *empty_preferences = nullptr;
	lunasvg_set_font_preferences(&empty_preferences, 0);
}

static void luna_set_font_preferences(const char *family) {
	const char *preferences[] = { family };
	lunasvg_set_font_preferences(preferences, 1);
}

static void luna_set_font_preferences(const std::vector<const char *> &families) {
	lunasvg_set_font_preferences(families.data(), families.size());
}

TEST_CASE("[Image] SVGUtils preserves explicitly empty font preferences") {
	SVGUtilsFontRegistryReset reset;
	Vector<String> preferences;
	SVGUtils::set_font_preferences(preferences);
	SVGUtils::ensure_font_faces_registered();

	CHECK(lunasvg::fontPreferencesConfigured());
}

static size_t codepoint_grapheme_breaks(const uint32_t *, size_t length, size_t *breaks, size_t capacity, void *) {
	const size_t count = MIN(length, capacity);
	for (size_t i = 0; i < count; i++) {
		breaks[i] = i + 1;
	}
	return count;
}

struct SVGShapingObservation {
	size_t start = 0;
	size_t end = 0;
	bool right_to_left = false;
	uint32_t script = 0;
	float width = 0;
	std::vector<uint32_t> glyph_indices;
	std::vector<size_t> clusters;
	std::vector<float> x_positions;
	std::vector<float> y_positions;
};

static void observe_svg_shaping(size_t start, size_t end, bool right_to_left,
		uint32_t script, const uint32_t *glyph_indices, const size_t *clusters,
		const float *x_positions, const float *y_positions, size_t glyph_count, float width, void *closure) {
	auto *observations = static_cast<std::vector<SVGShapingObservation> *>(closure);
	SVGShapingObservation observation;
	observation.start = start;
	observation.end = end;
	observation.right_to_left = right_to_left;
	observation.script = script;
	observation.width = width;
	for (size_t i = 0; i < glyph_count; ++i) {
		observation.glyph_indices.push_back(glyph_indices[i]);
		observation.clusters.push_back(clusters[i]);
		observation.x_positions.push_back(x_positions[i]);
		observation.y_positions.push_back(y_positions[i]);
	}
	observations->push_back(std::move(observation));
}

static bool svg_shaping_observation_is_supported(const SVGShapingObservation &observation) {
	return !observation.glyph_indices.empty() &&
			std::all_of(observation.glyph_indices.begin(), observation.glyph_indices.end(), [](uint32_t glyph_index) {
				return glyph_index != 0;
			});
}

static const SVGShapingObservation *find_supported_svg_shaping_observation(
		const std::vector<SVGShapingObservation> &observations, size_t start, size_t end, bool right_to_left) {
	for (const auto &observation : observations) {
		if (observation.start == start && observation.end == end && observation.right_to_left == right_to_left &&
				svg_shaping_observation_is_supported(observation)) {
			return &observation;
		}
	}
	return nullptr;
}

TEST_CASE("[Image] LunaSVG clears font faces on the current thread") {
	bool added = false;
	int destroy_calls_before_clear = -1;
	int destroy_calls_after_clear = -1;
	std::thread worker([&]() {
		int destroy_calls = 0;
		auto destroy = +[](void *closure) {
			(*static_cast<int *>(closure))++;
		};
		added = lunasvg_add_font_face_from_data("reset probe", false, false,
				lunasvg::embedded_cnfont_data, lunasvg::embedded_cnfont_size, destroy, &destroy_calls);
		destroy_calls_before_clear = destroy_calls;
		lunasvg_clear_font_faces();
		destroy_calls_after_clear = destroy_calls;
	});
	worker.join();

	CHECK(added);
	CHECK(destroy_calls_before_clear == 0);
	CHECK(destroy_calls_after_clear == 1);
}

TEST_CASE("[Image] SVG fallback keeps an extended grapheme cluster atomic") {
	LunaSVGTextConfigurationReset reset;
	struct GraphemeProbe {
		uint32_t text[8] = {};
		size_t length = 0;
		int calls = 0;
	};
	GraphemeProbe probe;
	auto callback = +[](const uint32_t *text, size_t length, size_t *breaks, size_t capacity, void *closure) -> size_t {
		GraphemeProbe *state = static_cast<GraphemeProbe *>(closure);
		state->calls++;
		state->length = length < 8 ? length : 8;
		for (size_t i = 0; i < state->length; i++) {
			state->text[i] = text[i];
		}
		if (capacity < 2) {
			return 0;
		}
		breaks[0] = 4;
		breaks[1] = length;
		return 2;
	};

	luna_set_strict_empty_font_preferences();
	lunasvg_set_grapheme_break_func(callback, &probe);
	const char svg[] = "<svg xmlns=\"http://www.w3.org/2000/svg\"><text x=\"0\" y=\"20\" font-size=\"20\">&#x2764;&#xFE0F;&#x200D;&#x1F525;A</text></svg>";
	auto document = lunasvg::Document::loadFromData(svg, sizeof(svg) - 1);

	CHECK(document != nullptr);
	lunasvg::Box bounds;
	if (document != nullptr) {
		bounds = document->boundingBox();
	}
	CHECK(probe.calls == 1);
	CHECK(probe.length == 5);
	CHECK(probe.text[0] == 0x2764);
	CHECK(probe.text[1] == 0xFE0F);
	CHECK(probe.text[2] == 0x200D);
	CHECK(probe.text[3] == 0x1F525);
	CHECK(probe.text[4] == 0x41);
	if (document != nullptr) {
		CHECK(bounds.w > 20.0f);
		CHECK(bounds.w < 25.0f);
	}
}

TEST_CASE("[Image] SVG fallback segments common clusters without a callback") {
	LunaSVGTextConfigurationReset reset;
	luna_set_strict_empty_font_preferences();
	lunasvg_set_grapheme_break_func(nullptr, nullptr);
	const char svg[] = "<svg xmlns=\"http://www.w3.org/2000/svg\"><text x=\"0\" y=\"20\" font-size=\"20\">A&#x0301;B</text></svg>";
	auto document = lunasvg::Document::loadFromData(svg, sizeof(svg) - 1);

	CHECK(document != nullptr);
	if (document != nullptr) {
		const lunasvg::Box bounds = document->boundingBox();
		CHECK(bounds.w > 20.0f);
		CHECK(bounds.w < 25.0f);
	}
}

TEST_CASE("[Image] LunaSVG trusts authoritative grapheme boundaries") {
	LunaSVGTextConfigurationReset reset;
	luna_set_strict_empty_font_preferences();
	auto callback = +[](const uint32_t *, size_t length, size_t *breaks, size_t capacity, void *) -> size_t {
		if (length != 3 || capacity < 2) {
			return 0;
		}
		// UAX #29 permits a break after a non-pictographic ZWJ sequence here.
		// The local emergency segmenter is deliberately not allowed to filter
		// an authoritative host result.
		breaks[0] = 2;
		breaks[1] = 3;
		return 2;
	};
	lunasvg_set_grapheme_break_func(callback, nullptr);
	const char svg[] = "<svg xmlns=\"http://www.w3.org/2000/svg\"><text x=\"0\" y=\"20\" font-size=\"20\">A&#x200D;B</text></svg>";
	auto document = lunasvg::Document::loadFromData(svg, sizeof(svg) - 1);
	REQUIRE(document != nullptr);
	CHECK(document->boundingBox().w > 20.0f);
}

TEST_CASE("[Image] LunaSVG parses CSS font family names") {
	auto families = lunasvg::parseFontFamilyList(R"(Basic Chinese, Basic\ Chinese, \64 efault, serif, 'default', "Comma\2c Family")");
	REQUIRE(families.size() == 4);
	CHECK(families[0] == "Basic Chinese");
	CHECK(families[1] == "Basic Chinese");
	CHECK(families[2] == "default");
	CHECK(families[3] == "Comma,Family");
}

TEST_CASE("[Image] LunaSVG renders embedded SVG glyphs for arbitrary family names") {
	LunaSVGTextConfigurationReset reset;
	const String font_path = TestUtils::get_data_path("fonts/twitter_color_emoji/TwitterColorEmoji-SVGinOT.ttf");
	CHECK(lunasvg_add_font_face_from_file("Heart Emoji Test", false, false, font_path.utf8().get_data()));
	luna_set_font_preferences("Heart Emoji Test");

	const char svg[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"64\" height=\"64\"><text x=\"4\" y=\"52\" font-size=\"48\">&#x2764;&#xFE0F;</text></svg>";
	auto document = lunasvg::Document::loadFromData(svg, sizeof(svg) - 1);
	REQUIRE(document != nullptr);

	auto bitmap = document->renderToBitmap(64, 64);
	REQUIRE(!bitmap.isNull());
	bitmap.convertToRGBA();
	int red_pixels = 0;
	for (int y = 0; y < bitmap.height(); y++) {
		const uint8_t *row = bitmap.data() + y * bitmap.stride();
		for (int x = 0; x < bitmap.width(); x++) {
			const uint8_t *pixel = row + x * 4;
			if (pixel[0] > 150 && pixel[1] < 100 && pixel[2] < 120 && pixel[3] > 100) {
				red_pixels++;
			}
		}
	}
	CHECK(red_pixels > 100);
}

TEST_CASE("[Image] PlutoVG rejects out-of-range SVG-in-OT glyph data") {
	const String font_path = TestUtils::get_data_path("fonts/twitter_color_emoji/TwitterColorEmoji-SVGinOT.ttf");
	plutovg_font_face_t *face = plutovg_font_face_load_from_file(font_path.utf8().get_data(), 0);
	REQUIRE(face != nullptr);
	const char *svg_data = nullptr;
	// Glyph 18 is outside every SVG document range in this subset. It must not
	// produce an unchecked pointer into unrelated font-table bytes.
	CHECK(plutovg_font_face_get_glyph_index_svg(face, 18, &svg_data) == 0);
	CHECK(svg_data == nullptr);
	plutovg_font_face_destroy(face);
}

TEST_CASE("[Image] LunaSVG renders adjacent embedded SVG glyphs inside one shaped run") {
	LunaSVGTextConfigurationReset reset;
	const String font_path = TestUtils::get_data_path("fonts/twitter_color_emoji/TwitterColorEmoji-SVGinOT.ttf");
	CHECK(lunasvg_add_font_face_from_file("Adjacent Emoji Test", false, false, font_path.utf8().get_data()));
	luna_set_font_preferences("Adjacent Emoji Test");

	const char svg[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"128\" height=\"64\"><text x=\"4\" y=\"52\" font-size=\"48\">&#x2764;&#xFE0F;&#x2764;&#xFE0F;</text></svg>";
	auto document = lunasvg::Document::loadFromData(svg, sizeof(svg) - 1);
	REQUIRE(document != nullptr);
	CHECK(document->boundingBox().w > 80.0f);

	auto bitmap = document->renderToBitmap(128, 64);
	REQUIRE(!bitmap.isNull());
	bitmap.convertToRGBA();
	int red_pixels = 0;
	for (int y = 0; y < bitmap.height(); y++) {
		const uint8_t *row = bitmap.data() + y * bitmap.stride();
		for (int x = 0; x < bitmap.width(); x++) {
			const uint8_t *pixel = row + x * 4;
			if (pixel[0] > 150 && pixel[1] < 100 && pixel[2] < 120 && pixel[3] > 100) {
				red_pixels++;
			}
		}
	}
	CHECK(red_pixels > 200);
}

TEST_CASE("[Image] LunaSVG shapes combining-mark clusters before fallback") {
	LunaSVGTextConfigurationReset reset;
	lunasvg_set_grapheme_break_func(codepoint_grapheme_breaks, nullptr);
	const String font_path = TestUtils::get_data_path("fonts/noto_sans_clusters/NotoSans-Medium.ttf");
	CHECK(lunasvg_add_font_face_from_file("Cluster Composition Test", false, false, font_path.utf8().get_data()));
	luna_set_font_preferences("Cluster Composition Test");

	const char decomposed_svg[] = "<svg xmlns=\"http://www.w3.org/2000/svg\"><text x=\"0\" y=\"40\" font-size=\"32\">Cafe&#x301;</text></svg>";
	const char composed_svg[] = "<svg xmlns=\"http://www.w3.org/2000/svg\"><text x=\"0\" y=\"40\" font-size=\"32\">Caf&#xE9;</text></svg>";
	auto decomposed = lunasvg::Document::loadFromData(decomposed_svg, sizeof(decomposed_svg) - 1);
	auto composed = lunasvg::Document::loadFromData(composed_svg, sizeof(composed_svg) - 1);
	REQUIRE(decomposed != nullptr);
	REQUIRE(composed != nullptr);

	const auto decomposed_bounds = decomposed->boundingBox();
	const auto composed_bounds = composed->boundingBox();
	CHECK(decomposed_bounds.x == doctest::Approx(composed_bounds.x));
	CHECK(decomposed_bounds.y == doctest::Approx(composed_bounds.y));
	CHECK(decomposed_bounds.w == doctest::Approx(composed_bounds.w));
	CHECK(decomposed_bounds.h == doctest::Approx(composed_bounds.h));
}

TEST_CASE("[Image] LunaSVG shapes complete script runs with inferred direction") {
	LunaSVGTextConfigurationReset reset;
	lunasvg_set_grapheme_break_func(codepoint_grapheme_breaks, nullptr);
	auto add_font = [](const char *family, const char *path) {
		const String font_path = TestUtils::get_data_path(path);
		return lunasvg_add_font_face_from_file(family, false, false, font_path.utf8().get_data());
	};
	CHECK(add_font("Latin Run Test", "fonts/noto_sans_clusters/NotoSans-Medium.ttf"));
	CHECK(add_font("Arabic Run Test", "fonts/noto_complex_scripts/NotoSansArabic-Subset.ttf"));
	CHECK(add_font("Hebrew Run Test", "fonts/noto_complex_scripts/NotoSansHebrew-Subset.ttf"));
	CHECK(add_font("Devanagari Run Test", "fonts/noto_complex_scripts/NotoSansDevanagari-Subset.ttf"));
	CHECK(add_font("Thai Run Test", "fonts/noto_complex_scripts/NotoSansThai-Subset.ttf"));

	std::vector<SVGShapingObservation> observations;
	lunasvg::setShapingObserverFunction(observe_svg_shaping, &observations);
	auto load = [&](const char *svg, size_t length) {
		observations.clear();
		auto document = lunasvg::Document::loadFromData(svg, length);
		REQUIRE(document != nullptr);
		CHECK(document->boundingBox().w > 0.0f);
	};

	luna_set_font_preferences("Arabic Run Test");
	const char arabic[] = "<svg xmlns=\"http://www.w3.org/2000/svg\"><text y=\"40\" font-size=\"32\">&#x633;&#x644;&#x627;&#x645;</text></svg>";
	load(arabic, sizeof(arabic) - 1);
	const auto *arabic_run = find_supported_svg_shaping_observation(observations, 0, 4, true);
	REQUIRE(arabic_run != nullptr);
	REQUIRE(arabic_run->clusters.size() == 3);
	CHECK(arabic_run->clusters.front() == 3);
	CHECK(arabic_run->clusters.back() == 0);

	luna_set_font_preferences(std::vector<const char *>{ "Latin Run Test", "Hebrew Run Test" });
	const char hebrew_mixed[] = "<svg xmlns=\"http://www.w3.org/2000/svg\"><text y=\"40\" font-size=\"32\">a &#x5E9;&#x5DC;&#x5D5;&#x5DD;</text></svg>";
	load(hebrew_mixed, sizeof(hebrew_mixed) - 1);
	CHECK(find_supported_svg_shaping_observation(observations, 0, 2, false) != nullptr);
	const auto *hebrew_run = find_supported_svg_shaping_observation(observations, 2, 6, true);
	REQUIRE(hebrew_run != nullptr);
	CHECK(hebrew_run->clusters.front() == 5);
	CHECK(hebrew_run->clusters.back() == 2);

	const char hebrew_rtl_mixed[] = "<svg xmlns=\"http://www.w3.org/2000/svg\"><text y=\"40\" direction=\"rtl\" font-size=\"32\">&#x5E9;&#x5DC;&#x5D5;&#x5DD; abc</text></svg>";
	load(hebrew_rtl_mixed, sizeof(hebrew_rtl_mixed) - 1);
	std::vector<std::pair<size_t, size_t>> visual_run_order;
	for (const auto &observation : observations) {
		if (!svg_shaping_observation_is_supported(observation)) {
			continue;
		}
		auto range = std::make_pair(observation.start, observation.end);
		if (range != std::make_pair<size_t, size_t>(5, 8) && range != std::make_pair<size_t, size_t>(0, 5)) {
			continue;
		}
		if (std::find(visual_run_order.begin(), visual_run_order.end(), range) == visual_run_order.end()) {
			visual_run_order.push_back(range);
		}
	}
	REQUIRE(visual_run_order.size() == 2);
	CHECK(visual_run_order[0].first == 5);
	CHECK(visual_run_order[0].second == 8);
	CHECK(visual_run_order[1].first == 0);
	CHECK(visual_run_order[1].second == 5);

	luna_set_font_preferences("Devanagari Run Test");
	const char devanagari[] = "<svg xmlns=\"http://www.w3.org/2000/svg\"><text y=\"40\" font-size=\"32\">&#x928;&#x92E;&#x938;&#x94D;&#x924;&#x947;</text></svg>";
	load(devanagari, sizeof(devanagari) - 1);
	const auto *devanagari_run = find_supported_svg_shaping_observation(observations, 0, 6, false);
	REQUIRE(devanagari_run != nullptr);
	CHECK(devanagari_run->glyph_indices.size() == 5);

	luna_set_font_preferences("Thai Run Test");
	// The second syllable stacks a tone mark above a vowel. It requires GPOS
	// context and has a non-zero vertical offset in Noto Sans Thai.
	const char thai[] = "<svg xmlns=\"http://www.w3.org/2000/svg\"><text y=\"40\" font-size=\"32\">&#xE40;&#xE01;&#xE49;&#xE32;&#xE01;&#xE34;&#xE48;</text></svg>";
	load(thai, sizeof(thai) - 1);
	const auto *thai_run = find_supported_svg_shaping_observation(observations, 0, 7, false);
	REQUIRE(thai_run != nullptr);
	CHECK(thai_run->glyph_indices.size() == 7);
	CHECK(std::any_of(thai_run->y_positions.begin(), thai_run->y_positions.end(), [](float position) {
		return std::abs(position) > 0.01f;
	}));
}

TEST_CASE("[Image] LunaSVG preserves kerning across grapheme clusters") {
	LunaSVGTextConfigurationReset reset;
	lunasvg_set_grapheme_break_func(codepoint_grapheme_breaks, nullptr);
	const String font_path = TestUtils::get_data_path("fonts/lunasvg_kerning.ttf");
	CHECK(lunasvg_add_font_face_from_file("Kerning Run Test", false, false, font_path.utf8().get_data()));
	luna_set_font_preferences("Kerning Run Test");

	std::vector<SVGShapingObservation> observations;
	lunasvg::setShapingObserverFunction(observe_svg_shaping, &observations);
	auto shape_width = [&](const char *text, size_t text_length) {
		observations.clear();
		std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\"><text y=\"40\" font-size=\"32\">";
		svg.append(text, text_length);
		svg += "</text></svg>";
		auto document = lunasvg::Document::loadFromData(svg.data(), svg.size());
		if (document == nullptr) {
			return -1.0f;
		}
		document->boundingBox();
		const auto *run = find_supported_svg_shaping_observation(observations, 0, text_length, false);
		return run == nullptr ? -1.0f : run->width;
	};

	const auto pair_width = shape_width("AV", 2);
	const auto separate_width = shape_width("A", 1) + shape_width("V", 1);
	REQUIRE(pair_width >= 0.0f);
	REQUIRE(separate_width >= 0.0f);
	CHECK(pair_width < separate_width);
}

TEST_CASE("[Image] LunaSVG renders an emoji ZWJ sequence as one shaped SVG glyph") {
	LunaSVGTextConfigurationReset text_reset;
	SVGUtilsFontRegistryReset registry_reset;
	const String font_path = TestUtils::get_data_path("fonts/twitter_color_emoji/TwitterColorEmoji-SVGinOT.ttf");
	const Vector<uint8_t> font_data = FileAccess::get_file_as_bytes(font_path);
	REQUIRE_FALSE(font_data.is_empty());
	SVGUtils::add_font_face("Emoji Cluster Test", font_data.ptr(), font_data.size());
	SVGUtils::set_font_preferences(Vector<String>{ "Emoji Cluster Test" });
	REQUIRE(SVGUtils::ensure_font_faces_registered());
	std::vector<SVGShapingObservation> observations;
	lunasvg::setShapingObserverFunction(observe_svg_shaping, &observations);

	const char svg[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"64\" height=\"64\"><text x=\"4\" y=\"52\" font-size=\"48\">&#x1F469;&#x200D;&#x1F4BB;</text></svg>";
	auto document = lunasvg::Document::loadFromData(svg, sizeof(svg) - 1);
	REQUIRE(document != nullptr);
	const auto bounds = document->boundingBox();
	CHECK(bounds.w > 40.0f);
	CHECK(bounds.w < 55.0f);
	CHECK(std::any_of(observations.begin(), observations.end(), [](const SVGShapingObservation &observation) {
		return observation.start == 0 && observation.end == 3 && observation.glyph_indices == std::vector<uint32_t>{ 2795 } &&
				observation.clusters == std::vector<size_t>{ 0 };
	}));

	auto bitmap = document->renderToBitmap(64, 64);
	REQUIRE(!bitmap.isNull());
	bitmap.convertToRGBA();
	int colored_pixels = 0;
	for (int y = 0; y < bitmap.height(); y++) {
		const uint8_t *row = bitmap.data() + y * bitmap.stride();
		for (int x = 0; x < bitmap.width(); x++) {
			const uint8_t *pixel = row + x * 4;
			const int max_channel = MAX(pixel[0], MAX(pixel[1], pixel[2]));
			const int min_channel = MIN(pixel[0], MIN(pixel[1], pixel[2]));
			if (pixel[3] > 100 && max_channel - min_channel > 40) {
				colored_pixels++;
			}
		}
	}
	CHECK(colored_pixels > 100);
}
#endif

TEST_CASE("[Image] Resizing") {
	Ref<Image> image = memnew(Image(8, 8, false, Image::FORMAT_RGBA8));
	// Crop
	image->crop(4, 4);
	CHECK_MESSAGE(
			image->get_size() == Vector2(4, 4),
			"get_size() should return the correct size after cropping.");
	image->set_pixel(0, 0, Color(1, 1, 1, 1));

	// Resize
	for (int i = 0; i < 5; i++) {
		Ref<Image> image_resized = memnew(Image());
		image_resized->copy_internals_from(image);
		Image::Interpolation interpolation = static_cast<Image::Interpolation>(i);
		image_resized->resize(8, 8, interpolation);
		CHECK_MESSAGE(
				image_resized->get_size() == Vector2(8, 8),
				"get_size() should return the correct size after resizing.");
		CHECK_MESSAGE(
				image_resized->get_pixel(1, 1).a > 0,
				"Resizing an image should also affect its content.");
	}

	// shrink_x2()
	image->shrink_x2();
	CHECK_MESSAGE(
			image->get_size() == Vector2(2, 2),
			"get_size() should return the correct size after shrink_x2().");

	// resize_to_po2()
	Ref<Image> image_po_2 = memnew(Image(14, 28, false, Image::FORMAT_RGBA8));
	image_po_2->resize_to_po2();
	CHECK_MESSAGE(
			image_po_2->get_size() == Vector2(16, 32),
			"get_size() should return the correct size after resize_to_po2().");
}

TEST_CASE("[Image] Modifying pixels of an image") {
	Ref<Image> image = memnew(Image(3, 3, false, Image::FORMAT_RGBA8));
	image->set_pixel(0, 0, Color(1, 1, 1, 1));
	CHECK_MESSAGE(
			!image->is_invisible(),
			"Image should not be invisible after drawing on it.");
	CHECK_MESSAGE(
			image->get_pixelv(Vector2(0, 0)).is_equal_approx(Color(1, 1, 1, 1)),
			"Image's get_pixel() should return the same color value as the one being set with set_pixel() in the same position.");
	CHECK_MESSAGE(
			image->get_used_rect() == Rect2i(0, 0, 1, 1),
			"Image's get_used_rect should return the expected value, larger than Rect2i(0, 0, 0, 0) if it's visible.");

	image->set_pixelv(Vector2(0, 0), Color(0.5, 0.5, 0.5, 0.5));
	Ref<Image> image2 = memnew(Image(3, 3, false, Image::FORMAT_RGBA8));

	// Fill image with color
	image2->fill(Color(0.5, 0.5, 0.5, 0.5));
	for (int y = 0; y < image2->get_height(); y++) {
		for (int x = 0; x < image2->get_width(); x++) {
			CHECK_MESSAGE(
					image2->get_pixel(x, y).r > 0.49,
					"fill() should colorize all pixels of the image.");
		}
	}

	// Fill rect with color
	{
		const int img_width = 3;
		const int img_height = 3;
		Vector<Rect2i> rects;
		rects.push_back(Rect2i());
		rects.push_back(Rect2i(-5, -5, 3, 3));
		rects.push_back(Rect2i(img_width, 0, 12, 12));
		rects.push_back(Rect2i(0, img_height, 12, 12));
		rects.push_back(Rect2i(img_width + 1, img_height + 2, 12, 12));
		rects.push_back(Rect2i(1, 1, 1, 1));
		rects.push_back(Rect2i(0, 1, 2, 3));
		rects.push_back(Rect2i(-5, 0, img_width + 10, 2));
		rects.push_back(Rect2i(0, -5, 2, img_height + 10));
		rects.push_back(Rect2i(-1, -1, img_width + 1, img_height + 1));

		for (const Rect2i &rect : rects) {
			Ref<Image> img = memnew(Image(img_width, img_height, false, Image::FORMAT_RGBA8));
			img->fill_rect(rect, Color(1, 1, 1, 1));
			for (int y = 0; y < img->get_height(); y++) {
				for (int x = 0; x < img->get_width(); x++) {
					if (rect.abs().has_point(Point2(x, y))) {
						CHECK_MESSAGE(
								img->get_pixel(x, y).is_equal_approx(Color(1, 1, 1, 1)),
								"fill_rect() should colorize all image pixels within rect bounds.");
					} else {
						CHECK_MESSAGE(
								!img->get_pixel(x, y).is_equal_approx(Color(1, 1, 1, 1)),
								"fill_rect() shouldn't colorize any image pixel out of rect bounds.");
					}
				}
			}
		}
	}

	// Blend two images together
	image->blend_rect(image2, Rect2i(Vector2i(0, 0), image2->get_size()), Vector2i(0, 0));
	CHECK_MESSAGE(
			image->get_pixel(0, 0).a > 0.7,
			"blend_rect() should blend the alpha values of the two images.");
	CHECK_MESSAGE(
			image->get_used_rect().size == image->get_size(),
			"get_used_rect() should return the expected value, its Rect size should be the same as get_size() if there are no transparent pixels.");

	Ref<Image> image3 = memnew(Image(2, 2, false, Image::FORMAT_RGBA8));
	image3->set_pixel(0, 0, Color(0, 1, 0, 1));

	//blit_rect() two images together
	image->blit_rect(image3, Rect2i(Vector2i(0, 0), image3->get_size()), Vector2i(0, 0));
	CHECK_MESSAGE(
			image->get_pixel(0, 0).is_equal_approx(Color(0, 1, 0, 1)),
			"blit_rect() should replace old colors and not blend them.");
	CHECK_MESSAGE(
			!image->get_pixel(2, 2).is_equal_approx(Color(0, 1, 0, 1)),
			"blit_rect() should not affect the area of the image that is outside src_rect.");

	// Flip image
	image3->flip_x();
	CHECK(image3->get_pixel(1, 0).is_equal_approx(Color(0, 1, 0, 1)));
	CHECK_MESSAGE(
			image3->get_pixel(0, 0).is_equal_approx(Color(0, 0, 0, 0)),
			"flip_x() should not leave old pixels behind.");
	image3->flip_y();
	CHECK(image3->get_pixel(1, 1).is_equal_approx(Color(0, 1, 0, 1)));
	CHECK_MESSAGE(
			image3->get_pixel(1, 0).is_equal_approx(Color(0, 0, 0, 0)),
			"flip_y() should not leave old pixels behind.");

	// Pre-multiply Alpha then Convert from RGBA to L8, checking alpha
	{
		Ref<Image> gray_image = memnew(Image(3, 3, false, Image::FORMAT_RGBA8));
		gray_image->fill_rect(Rect2i(0, 0, 3, 3), Color(1, 1, 1, 0));
		gray_image->set_pixel(1, 1, Color(1, 1, 1, 1));
		gray_image->set_pixel(1, 2, Color(0.5, 0.5, 0.5, 0.5));
		gray_image->set_pixel(2, 1, Color(0.25, 0.05, 0.5, 1.0));
		gray_image->set_pixel(2, 2, Color(0.5, 0.25, 0.95, 0.75));
		gray_image->premultiply_alpha();
		gray_image->convert(Image::FORMAT_L8);
		CHECK_MESSAGE(gray_image->get_pixel(0, 0).is_equal_approx(Color(0, 0, 0, 1)), "convert() RGBA to L8 should be black.");
		CHECK_MESSAGE(gray_image->get_pixel(0, 1).is_equal_approx(Color(0, 0, 0, 1)), "convert() RGBA to L8 should be black.");
		CHECK_MESSAGE(gray_image->get_pixel(0, 2).is_equal_approx(Color(0, 0, 0, 1)), "convert() RGBA to L8 should be black.");
		CHECK_MESSAGE(gray_image->get_pixel(1, 0).is_equal_approx(Color(0, 0, 0, 1)), "convert() RGBA to L8 should be black.");
		CHECK_MESSAGE(gray_image->get_pixel(1, 1).is_equal_approx(Color(1, 1, 1, 1)), "convert() RGBA to L8 should be white.");
		CHECK_MESSAGE(gray_image->get_pixel(1, 2).is_equal_approx(Color(0.250980407, 0.250980407, 0.250980407, 1)), "convert() RGBA to L8 should be around 0.250980407 (64).");
		CHECK_MESSAGE(gray_image->get_pixel(2, 0).is_equal_approx(Color(0, 0, 0, 1)), "convert() RGBA to L8 should be black.");
		CHECK_MESSAGE(gray_image->get_pixel(2, 1).is_equal_approx(Color(0.121568628, 0.121568628, 0.121568628, 1)), "convert() RGBA to L8 should be around 0.121568628 (31).");
		CHECK_MESSAGE(gray_image->get_pixel(2, 2).is_equal_approx(Color(0.266666681, 0.266666681, 0.266666681, 1)), "convert() RGBA to L8 should be around 0.266666681 (68).");
	}
}

TEST_CASE("[Image] Custom mipmaps") {
	Ref<Image> image = memnew(Image(100, 100, false, Image::FORMAT_RGBA8));

	REQUIRE(!image->has_mipmaps());
	image->generate_mipmaps();
	REQUIRE(image->has_mipmaps());

	const int mipmaps = image->get_mipmap_count() + 1;
	REQUIRE(mipmaps == 7);

	// Initialize reference mipmap data.
	// Each byte is given value "mipmap_index * 5".

	{
		PackedByteArray data = image->get_data();
		uint8_t *data_ptr = data.ptrw();

		for (int mip = 0; mip < mipmaps; mip++) {
			int64_t mip_offset = 0;
			int64_t mip_size = 0;
			image->get_mipmap_offset_and_size(mip, mip_offset, mip_size);

			for (int i = 0; i < mip_size; i++) {
				data_ptr[mip_offset + i] = mip * 5;
			}
		}
		image->set_data(image->get_width(), image->get_height(), image->has_mipmaps(), image->get_format(), data);
	}

	// Byte format conversion.

	for (int format = Image::FORMAT_L8; format <= Image::FORMAT_RGBA8; format++) {
		Ref<Image> image_bytes = memnew(Image());
		image_bytes->copy_internals_from(image);
		image_bytes->convert((Image::Format)format);
		REQUIRE(image_bytes->has_mipmaps());

		PackedByteArray data = image_bytes->get_data();
		const uint8_t *data_ptr = data.ptr();

		for (int mip = 0; mip < mipmaps; mip++) {
			int64_t mip_offset = 0;
			int64_t mip_size = 0;
			image_bytes->get_mipmap_offset_and_size(mip, mip_offset, mip_size);

			for (int i = 0; i < mip_size; i++) {
				if (data_ptr[mip_offset + i] != mip * 5) {
					REQUIRE_MESSAGE(false, "Byte format conversion error.");
				}
			}
		}
	}

	// Floating point format conversion.

	for (int format = Image::FORMAT_RF; format <= Image::FORMAT_RGBAF; format++) {
		Ref<Image> image_rgbaf = memnew(Image());
		image_rgbaf->copy_internals_from(image);
		image_rgbaf->convert((Image::Format)format);
		REQUIRE(image_rgbaf->has_mipmaps());

		PackedByteArray data = image_rgbaf->get_data();
		const uint8_t *data_ptr = data.ptr();

		for (int mip = 0; mip < mipmaps; mip++) {
			int64_t mip_offset = 0;
			int64_t mip_size = 0;
			image_rgbaf->get_mipmap_offset_and_size(mip, mip_offset, mip_size);

			for (int i = 0; i < mip_size; i += 4) {
				float value = *(float *)(data_ptr + mip_offset + i);
				if (!Math::is_equal_approx(value * 255.0f, mip * 5)) {
					REQUIRE_MESSAGE(false, "Floating point conversion error.");
				}
			}
		}
	}
}

TEST_CASE("[Image] Convert image") {
	for (int format = Image::FORMAT_RF; format < Image::FORMAT_RGBE9995; format++) {
		for (int new_format = Image::FORMAT_RF; new_format < Image::FORMAT_RGBE9995; new_format++) {
			Ref<Image> image = memnew(Image(4, 4, false, (Image::Format)format));
			image->convert((Image::Format)new_format);
			String format_string = Image::format_names[(Image::Format)format];
			String new_format_string = Image::format_names[(Image::Format)new_format];
			format_string = "Error converting from " + format_string + " to " + new_format_string + ".";
			CHECK_MESSAGE(image->get_format() == new_format, format_string);
		}
	}

	Ref<Image> image = memnew(Image(4, 4, false, Image::FORMAT_RGBA8));
	PackedByteArray image_data = image->get_data();
	ERR_PRINT_OFF;
	image->convert((Image::Format)-1);
	ERR_PRINT_ON;
	CHECK_MESSAGE(image->get_data() == image_data, "Image conversion to invalid type (-1) should not alter image.");
	Ref<Image> image2 = memnew(Image(4, 4, false, Image::FORMAT_RGBA8));
	image_data = image2->get_data();
	ERR_PRINT_OFF;
	image2->convert((Image::Format)(Image::FORMAT_MAX + 1));
	ERR_PRINT_ON;
	CHECK_MESSAGE(image2->get_data() == image_data, "Image conversion to invalid type (Image::FORMAT_MAX + 1) should not alter image.");
}

} // namespace TestImage

#endif // TEST_IMAGE_H
