#include "svgtextelement.h"
#include "svglayoutstate.h"
#include "svgrenderstate.h"
#include <lunasvg.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <unordered_map>
#include <utility>

#ifdef LUNASVG_ENABLE_HARFBUZZ
#include <hb-ot.h>
#include <hb.h>
#endif

namespace lunasvg {

static thread_local GraphemeBreakFunction graphemeBreakFunction = nullptr;
static thread_local void *graphemeBreakClosure = nullptr;

void setGraphemeBreakFunction(GraphemeBreakFunction callback, void *closure) {
	graphemeBreakFunction = callback;
	graphemeBreakClosure = closure;
}

namespace {

static bool isWhitespaceCodepoint(uint32_t codepoint) {
	switch (codepoint) {
		case 0x0009:
		case 0x000A:
		case 0x000B:
		case 0x000C:
		case 0x000D:
		case 0x0020:
		case 0x0085:
		case 0x00A0:
		case 0x1680:
		case 0x2028:
		case 0x2029:
		case 0x202F:
		case 0x205F:
		case 0x3000:
			return true;
		default:
			break;
	}

	return (codepoint >= 0x2000 && codepoint <= 0x200A);
}

struct EmbeddedSVGGlyphInfo {
	const char *svgData = nullptr;
	size_t svgLength = 0;
	Rect dstRect;
};

struct EmbeddedSVGGlyphBitmapCacheKey {
	const plutovg_font_face_t *face = nullptr;
	uint32_t glyphIndex = 0;

	bool operator==(const EmbeddedSVGGlyphBitmapCacheKey &other) const {
		return face == other.face && glyphIndex == other.glyphIndex;
	}
};

struct EmbeddedSVGGlyphBitmapCacheKeyHash {
	size_t operator()(const EmbeddedSVGGlyphBitmapCacheKey &key) const {
		auto faceHash = std::hash<const void *>{}(key.face);
		auto glyphHash = std::hash<uint32_t>{}(key.glyphIndex);
		return faceHash ^ (glyphHash + 0x9e3779b9 + (faceHash << 6) + (faceHash >> 2));
	}
};

using EmbeddedSVGGlyphBitmapCache = std::unordered_map<EmbeddedSVGGlyphBitmapCacheKey, Bitmap, EmbeddedSVGGlyphBitmapCacheKeyHash>;

static EmbeddedSVGGlyphBitmapCache &embeddedSVGGlyphBitmapCache() {
	static thread_local EmbeddedSVGGlyphBitmapCache cache;
	return cache;
}

static bool fontFaceHasGlyph(const FontFace &face, uint32_t codepoint) {
	return !face.isNull() && plutovg_font_face_has_glyph(face.get(), codepoint);
}

static bool isVariationSelector(uint32_t codepoint) {
	return (codepoint >= 0xFE00 && codepoint <= 0xFE0F) || (codepoint >= 0xE0100 && codepoint <= 0xE01EF);
}

static bool isClusterControl(uint32_t codepoint) {
	return codepoint == 0x200C || codepoint == 0x200D || isVariationSelector(codepoint) ||
			(codepoint >= 0xE0020 && codepoint <= 0xE007F);
}

static bool isGraphemeExtend(uint32_t codepoint) {
	return isVariationSelector(codepoint) || (codepoint >= 0x0300 && codepoint <= 0x036F) ||
			(codepoint >= 0x1AB0 && codepoint <= 0x1AFF) || (codepoint >= 0x1DC0 && codepoint <= 0x1DFF) ||
			(codepoint >= 0x20D0 && codepoint <= 0x20FF) || (codepoint >= 0xFE20 && codepoint <= 0xFE2F) ||
			(codepoint >= 0x1F3FB && codepoint <= 0x1F3FF) || (codepoint >= 0xE0020 && codepoint <= 0xE007F);
}

static bool isRegionalIndicator(uint32_t codepoint) {
	return codepoint >= 0x1F1E6 && codepoint <= 0x1F1FF;
}

static std::vector<size_t> fallbackGraphemeBreaks(std::u32string_view text) {
	std::vector<size_t> breaks;
	size_t regionalIndicatorCount = !text.empty() && isRegionalIndicator(text.front()) ? 1 : 0;
	for (size_t i = 1; i < text.size(); ++i) {
		const auto previous = text[i - 1];
		const auto current = text[i];
		bool shouldBreak = true;
		if (previous == '\r' && current == '\n') {
			shouldBreak = false;
		} else if (current == 0x200D || previous == 0x200D || isGraphemeExtend(current)) {
			shouldBreak = false;
		} else if (isRegionalIndicator(previous) && isRegionalIndicator(current)) {
			shouldBreak = regionalIndicatorCount % 2 == 0;
		}

		if (shouldBreak) {
			breaks.push_back(i);
			regionalIndicatorCount = 0;
		}
		if (isRegionalIndicator(current)) {
			++regionalIndicatorCount;
		} else if (!isGraphemeExtend(current) && current != 0x200D) {
			regionalIndicatorCount = 0;
		}
	}
	if (!text.empty()) {
		breaks.push_back(text.size());
	}
	return breaks;
}

static std::vector<size_t> graphemeBreaks(std::u32string_view text) {
	auto fallbackBreaks = fallbackGraphemeBreaks(text);
	if (text.empty() || graphemeBreakFunction == nullptr) {
		return fallbackBreaks;
	}

	std::vector<size_t> rawBreaks(text.size());
	auto count = graphemeBreakFunction(reinterpret_cast<const uint32_t *>(text.data()), text.size(), rawBreaks.data(),
			rawBreaks.size(), graphemeBreakClosure);
	if (count == 0) {
		return fallbackBreaks;
	}
	count = std::min(count, rawBreaks.size());
	std::vector<size_t> breaks;
	size_t previous = 0;
	for (size_t i = 0; i < count; ++i) {
		auto current = rawBreaks[i];
		// A TextServer can advertise break-iterator support while its runtime
		// support data is unavailable. In that state Godot reports every code
		// point as a character break. Treat the local segmenter as the minimum
		// atomicity guarantee: a richer callback may merge its clusters, but it
		// must not split combining marks, variation selectors, or ZWJ sequences.
		if (current > previous && current <= text.size() &&
				std::binary_search(fallbackBreaks.begin(), fallbackBreaks.end(), current)) {
			breaks.push_back(current);
			previous = current;
		}
	}
	if (breaks.empty() || breaks.back() != text.size()) {
		breaks.push_back(text.size());
	}
	return breaks;
}

static std::u32string drawableClusterText(std::u32string_view cluster) {
	std::u32string result;
	for (auto codepoint : cluster) {
		if (!isClusterControl(codepoint)) {
			result.push_back(codepoint);
		}
	}
	return result;
}

static bool isWhitespaceCluster(std::u32string_view cluster) {
	for (auto codepoint : cluster) {
		if (!isClusterControl(codepoint) && !isWhitespaceCodepoint(codepoint)) {
			return false;
		}
	}
	return true;
}

struct TextCluster {
	explicit TextCluster(std::u32string_view text) :
			text(text), drawableText(drawableClusterText(text)), isWhitespace(isWhitespaceCluster(text)) {}

	std::u32string_view text;
	std::u32string drawableText;
	bool isWhitespace;
};

static bool fontFaceSupportsClusterWithoutShaping(const FontFace &face, const TextCluster &cluster) {
	if (face.isNull()) {
		return false;
	}
	for (auto codepoint : cluster.drawableText) {
		if (isWhitespaceCodepoint(codepoint)) {
			continue;
		}
		if (!fontFaceHasGlyph(face, codepoint)) {
			return false;
		}
	}
	return true;
}

struct ShapedCluster {
	std::vector<SVGShapedGlyph> glyphs;
	float width = 0;
	bool supported = false;
};

#ifdef LUNASVG_ENABLE_HARFBUZZ
class HarfBuzzFont {
public:
	explicit HarfBuzzFont(const FontFace &fontFace) {
		unsigned int dataLength = 0;
		int ttcIndex = 0;
		auto data = plutovg_font_face_get_data(fontFace.get(), &dataLength, &ttcIndex);
		if (data == nullptr || dataLength == 0) {
			return;
		}
		m_blob = hb_blob_create(static_cast<const char *>(data), dataLength, HB_MEMORY_MODE_READONLY, nullptr, nullptr);
		m_face = hb_face_create(m_blob, static_cast<unsigned int>(ttcIndex));
		m_upem = hb_face_get_upem(m_face);
		if (m_upem == 0) {
			return;
		}
		m_font = hb_font_create(m_face);
		hb_font_set_scale(m_font, static_cast<int>(m_upem), static_cast<int>(m_upem));
		hb_ot_font_set_funcs(m_font);
		m_buffer = hb_buffer_create();
	}

	~HarfBuzzFont() {
		if (m_buffer != nullptr) {
			hb_buffer_destroy(m_buffer);
		}
		if (m_font != nullptr) {
			hb_font_destroy(m_font);
		}
		if (m_face != nullptr) {
			hb_face_destroy(m_face);
		}
		if (m_blob != nullptr) {
			hb_blob_destroy(m_blob);
		}
	}

	HarfBuzzFont(const HarfBuzzFont &) = delete;
	HarfBuzzFont &operator=(const HarfBuzzFont &) = delete;
	HarfBuzzFont(HarfBuzzFont &&other) noexcept :
			m_blob(std::exchange(other.m_blob, nullptr)),
			m_face(std::exchange(other.m_face, nullptr)),
			m_font(std::exchange(other.m_font, nullptr)),
			m_buffer(std::exchange(other.m_buffer, nullptr)),
			m_upem(std::exchange(other.m_upem, 0)) {}

	bool isValid() const { return m_font != nullptr && m_buffer != nullptr && m_upem != 0; }
	hb_font_t *font() const { return m_font; }
	hb_buffer_t *buffer() const { return m_buffer; }
	unsigned int upem() const { return m_upem; }

private:
	hb_blob_t *m_blob = nullptr;
	hb_face_t *m_face = nullptr;
	hb_font_t *m_font = nullptr;
	hb_buffer_t *m_buffer = nullptr;
	unsigned int m_upem = 0;
};

static ShapedCluster shapeCluster(const HarfBuzzFont &font, float fontSize, const TextCluster &cluster) {
	ShapedCluster result;
	if (!font.isValid()) {
		return result;
	}

	auto buffer = font.buffer();
	hb_buffer_reset(buffer);
	hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
	hb_buffer_set_cluster_level(buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES);
	hb_buffer_set_flags(buffer, HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES);
	hb_buffer_add_utf32(buffer, reinterpret_cast<const uint32_t *>(cluster.text.data()),
			static_cast<int>(cluster.text.size()), 0, static_cast<int>(cluster.text.size()));
	hb_buffer_guess_segment_properties(buffer);
	hb_shape(font.font(), buffer, nullptr, 0);

	unsigned int glyphCount = 0;
	auto infos = hb_buffer_get_glyph_infos(buffer, &glyphCount);
	auto positions = hb_buffer_get_glyph_positions(buffer, &glyphCount);
	const auto scale = fontSize / static_cast<float>(font.upem());
	float penX = 0;
	float penY = 0;
	result.supported = cluster.isWhitespace || glyphCount > 0;
	result.glyphs.reserve(glyphCount);
	for (unsigned int i = 0; i < glyphCount; ++i) {
		if (infos[i].codepoint == 0 && !cluster.isWhitespace) {
			result.supported = false;
		}
		result.glyphs.push_back({ infos[i].codepoint, penX + positions[i].x_offset * scale,
				penY + positions[i].y_offset * scale });
		penX += positions[i].x_advance * scale;
		penY += positions[i].y_advance * scale;
	}
	result.width = penX;

	return result;
}
#endif

struct FontCandidate {
	explicit FontCandidate(FontFace fontFace) :
			face(std::move(fontFace))
#ifdef LUNASVG_ENABLE_HARFBUZZ
			, shapingFont(face)
#endif
	{}

	FontFace face;
#ifdef LUNASVG_ENABLE_HARFBUZZ
	HarfBuzzFont shapingFont;
#endif
};

static std::vector<FontCandidate> resolveFontCandidates(const SVGTextPositioningElement *element) {
	FontFamilyList families;
	if (!element->font_family().empty()) {
		families = parseFontFamilyList(element->font_family());
	} else if (fontPreferencesConfigured()) {
		families = fontPreferences();
	}

	std::vector<FontCandidate> candidates;
	for (const auto &family : families) {
		auto face = fontFaceCache()->getFontFace(family, element->font_bold(), element->font_italic());
		if (!face.isNull()) {
			candidates.emplace_back(std::move(face));
		}
	}
	if (candidates.empty() && !fontPreferencesConfigured() && !element->font().isNull()) {
		candidates.emplace_back(element->font().face());
	}
	return candidates;
}

static bool fontFaceHasEmbeddedSVGGlyph(const FontFace &face, const std::vector<SVGShapedGlyph> &glyphs) {
	if (face.isNull() || glyphs.size() != 1) {
		return false;
	}
	const char *svgData = nullptr;
	return plutovg_font_face_get_glyph_index_svg(face.get(), glyphs.front().index, &svgData) > 0 && svgData != nullptr;
}

struct SelectedClusterFont {
	Font font;
	std::vector<SVGShapedGlyph> glyphs;
	float width = 0;
	bool hasEmbeddedSVGGlyph = false;
	bool isMissingGlyph = false;
};

static SelectedClusterFont selectClusterFont(const SVGTextPositioningElement *element,
		const std::vector<FontCandidate> &candidates, const TextCluster &cluster) {
	const auto fontSize = element->font().size();
	for (const auto &candidate : candidates) {
#ifdef LUNASVG_ENABLE_HARFBUZZ
		auto shaped = shapeCluster(candidate.shapingFont, fontSize, cluster);
		if (shaped.supported) {
			auto hasEmbeddedSVG = fontFaceHasEmbeddedSVGGlyph(candidate.face, shaped.glyphs);
			return { Font(candidate.face, fontSize), std::move(shaped.glyphs), shaped.width, hasEmbeddedSVG, false };
		}
#else
		if (fontFaceSupportsClusterWithoutShaping(candidate.face, cluster)) {
			auto font = Font(candidate.face, fontSize);
			return { font, {}, font.measureText(cluster.drawableText), false, false };
		}
#endif
	}
	Font missingFont;
	if (!candidates.empty()) {
		missingFont = Font(candidates.front().face, fontSize);
	} else {
		missingFont = Font(FontFace(), fontSize);
	}
	return { missingFont, {}, 0, false, !cluster.isWhitespace };
}

static bool tryResolveEmbeddedSVGGlyph(const Font &font, uint32_t glyphIndex, const Point &origin, EmbeddedSVGGlyphInfo &glyph) {
	auto face = font.face().get();
	if (face == nullptr) {
		return false;
	}

	auto svgLength = plutovg_font_face_get_glyph_index_svg(face, glyphIndex, &glyph.svgData);
	if (svgLength <= 0 || glyph.svgData == nullptr) {
		return false;
	}

	plutovg_rect_t glyphExtents = { 0 };
	plutovg_font_face_get_glyph_index_metrics(face, font.size(), glyphIndex, nullptr, nullptr, &glyphExtents);
	glyph.svgLength = static_cast<size_t>(svgLength);
	glyph.dstRect = Rect(origin.x + glyphExtents.x, origin.y + glyphExtents.y, glyphExtents.w, glyphExtents.h);
	return !glyph.dstRect.isEmpty();
}

static const Bitmap *getCachedEmbeddedSVGGlyphBitmap(const Font &font, uint32_t glyphIndex, std::string_view svgDocument) {
	auto face = font.face().get();
	if (face == nullptr || svgDocument.empty()) {
		return nullptr;
	}

	// Cache rasterized embedded SVG glyphs per thread to avoid reparsing the
	// embedded SVG document on every frame.
	auto &bitmapCache = embeddedSVGGlyphBitmapCache();
	EmbeddedSVGGlyphBitmapCacheKey key{ face, glyphIndex };
	auto it = bitmapCache.find(key);
	if (it != bitmapCache.end()) {
		return it->second.isNull() ? nullptr : &it->second;
	}

	auto document = Document::loadFromData(svgDocument.data(), svgDocument.size());
	if (!document) {
		bitmapCache.emplace(key, Bitmap());
		return nullptr;
	}
	auto bounds = Rect(document->boundingBox());
	if (bounds.isEmpty()) {
		bitmapCache.emplace(key, Bitmap());
		return nullptr;
	}

	auto bitmapWidth = std::max(1, static_cast<int>(std::ceil(bounds.w)));
	auto bitmapHeight = std::max(1, static_cast<int>(std::ceil(bounds.h)));
	Bitmap bitmap(bitmapWidth, bitmapHeight);
	bitmap.clear(0x00000000);
	document->render(bitmap, Matrix::translated(-bounds.x, -bounds.y));
	return &bitmapCache.emplace(key, std::move(bitmap)).first->second;
}

static bool tryRenderEmbeddedSVGGlyph(const SVGTextFragment &fragment, const Transform &transform, SVGRenderState &state) {
	if (fragment.glyphs.size() != 1) {
		return false;
	}

	const auto &font = fragment.font;
	const auto &shapedGlyph = fragment.glyphs.front();
	auto origin = Point(fragment.x + shapedGlyph.x, fragment.y - shapedGlyph.y);
	EmbeddedSVGGlyphInfo glyph;
	if (!tryResolveEmbeddedSVGGlyph(font, shapedGlyph.index, origin, glyph)) {
		return false;
	}

	auto bitmap = getCachedEmbeddedSVGGlyphBitmap(font, shapedGlyph.index, std::string_view(glyph.svgData, glyph.svgLength));
	if (bitmap == nullptr) {
		return false;
	}

	state->drawImage(*bitmap, glyph.dstRect, Rect(0, 0, bitmap->width(), bitmap->height()), transform);
	return true;
}

static Path shapedGlyphPath(const SVGTextFragment &fragment) {
	Path path;
	auto face = fragment.font.face().get();
	if (face == nullptr) {
		return path;
	}
	for (const auto &glyph : fragment.glyphs) {
		path.addGlyph(face, fragment.font.size(), fragment.x + glyph.x, fragment.y - glyph.y, glyph.index);
	}
	return path;
}

static Rect shapedGlyphBounds(const SVGTextFragment &fragment) {
	auto bounds = Rect::Invalid;
	auto face = fragment.font.face().get();
	if (face == nullptr) {
		return Rect::Empty;
	}
	for (const auto &glyph : fragment.glyphs) {
		plutovg_rect_t extents = { 0 };
		plutovg_font_face_get_glyph_index_metrics(face, fragment.font.size(), glyph.index, nullptr, nullptr, &extents);
		bounds.unite(Rect(fragment.x + glyph.x + extents.x, fragment.y - glyph.y + extents.y, extents.w, extents.h));
	}
	return bounds.isValid() ? bounds : Rect::Empty;
}

static Rect missingGlyphRect(const SVGTextFragment &fragment) {
	const auto fontSize = fragment.element->font_size();
	const auto inset = std::max(0.5f, fontSize * 0.05f);
	return Rect(fragment.x + inset, fragment.y - fontSize * 0.8f, std::max(1.f, fragment.width - inset * 2.f),
			std::max(1.f, fontSize * 0.8f));
}

static Path missingGlyphPath(const SVGTextFragment &fragment) {
	auto outer = missingGlyphRect(fragment);
	auto inner = outer;
	inner.inflate(-std::max(1.f, fragment.element->font_size() * 0.08f));
	Path path;
	path.addRect(outer);
	if (!inner.isEmpty()) {
		path.addRect(inner);
	}
	return path;
}

} // namespace

void clearTextCaches() {
	embeddedSVGGlyphBitmapCache().clear();
}

inline const SVGTextNode *toSVGTextNode(const SVGNode *node) {
	assert(node && node->isTextNode());
	return static_cast<const SVGTextNode *>(node);
}

inline const SVGTextPositioningElement *toSVGTextPositioningElement(const SVGNode *node) {
	assert(node && node->isTextPositioningElement());
	return static_cast<const SVGTextPositioningElement *>(node);
}

static AlignmentBaseline resolveDominantBaseline(const SVGTextPositioningElement *element) {
	switch (element->dominant_baseline()) {
		case DominantBaseline::Auto:
		case DominantBaseline::UseScript:
		case DominantBaseline::NoChange:
		case DominantBaseline::ResetSize:
			return AlignmentBaseline::Auto;
		case DominantBaseline::Ideographic:
			return AlignmentBaseline::Ideographic;
		case DominantBaseline::Alphabetic:
			return AlignmentBaseline::Alphabetic;
		case DominantBaseline::Hanging:
			return AlignmentBaseline::Hanging;
		case DominantBaseline::Mathematical:
			return AlignmentBaseline::Mathematical;
		case DominantBaseline::Central:
			return AlignmentBaseline::Central;
		case DominantBaseline::Middle:
			return AlignmentBaseline::Middle;
		case DominantBaseline::TextAfterEdge:
			return AlignmentBaseline::TextAfterEdge;
		case DominantBaseline::TextBeforeEdge:
			return AlignmentBaseline::TextBeforeEdge;
		default:
			assert(false);
	}

	return AlignmentBaseline::Auto;
}

static float calculateBaselineOffset(const SVGTextPositioningElement *element) {
	auto offset = element->baseline_offset();
	for (auto parent = element->parent(); parent->isTextPositioningElement(); parent = parent->parent()) {
		offset += toSVGTextPositioningElement(parent)->baseline_offset();
	}

	auto baseline = element->alignment_baseline();
	if (baseline == AlignmentBaseline::Auto || baseline == AlignmentBaseline::Baseline) {
		baseline = resolveDominantBaseline(element);
	}

	const auto &font = element->font();
	switch (baseline) {
		case AlignmentBaseline::BeforeEdge:
		case AlignmentBaseline::TextBeforeEdge:
			offset -= font.ascent();
			break;
		case AlignmentBaseline::Middle:
			offset -= font.xHeight() / 2.f;
			break;
		case AlignmentBaseline::Central:
			offset -= (font.ascent() + font.descent()) / 2.f;
			break;
		case AlignmentBaseline::AfterEdge:
		case AlignmentBaseline::TextAfterEdge:
		case AlignmentBaseline::Ideographic:
			offset -= font.descent();
			break;
		case AlignmentBaseline::Hanging:
			offset -= font.ascent() * 8.f / 10.f;
			break;
		case AlignmentBaseline::Mathematical:
			offset -= font.ascent() / 2.f;
			break;
		default:
			break;
	}

	return offset;
}

static bool needsTextAnchorAdjustment(const SVGTextPositioningElement *element) {
	auto direction = element->direction();
	switch (element->text_anchor()) {
		case TextAnchor::Start:
			return direction == Direction::Rtl;
		case TextAnchor::Middle:
			return true;
		case TextAnchor::End:
			return direction == Direction::Ltr;
		default:
			assert(false);
	}

	return false;
}

static float calculateTextAnchorOffset(const SVGTextPositioningElement *element, float width) {
	auto direction = element->direction();
	switch (element->text_anchor()) {
		case TextAnchor::Start:
			if (direction == Direction::Ltr) {
				return 0.f;
			}
			return -width;
		case TextAnchor::Middle:
			return -width / 2.f;
		case TextAnchor::End:
			if (direction == Direction::Ltr) {
				return -width;
			}
			return 0.f;
		default:
			assert(false);
	}

	return 0.f;
}

static void adjustTextAnchor(SVGTextFragmentList::iterator begin, SVGTextFragmentList::iterator end) {
	if (!needsTextAnchorAdjustment(begin->element)) {
		return;
	}
	float chunkWidth = 0.f;
	const SVGTextFragment *lastFragment = nullptr;
	for (auto it = begin; it != end; ++it) {
		const SVGTextFragment &fragment = *it;
		chunkWidth += fragment.width;
		if (lastFragment) {
			chunkWidth += fragment.x - (lastFragment->x + lastFragment->width);
		}
		lastFragment = &fragment;
	}

	auto chunkOffset = calculateTextAnchorOffset(begin->element, chunkWidth);
	for (auto it = begin; it != end; ++it) {
		SVGTextFragment &fragment = *it;
		fragment.x += chunkOffset;
	}
}

SVGTextFragmentsBuilder::SVGTextFragmentsBuilder(std::u32string &text, SVGTextFragmentList &fragments) :
		m_text(text), m_fragments(fragments) {
	m_text.clear();
	m_fragments.clear();
}

void SVGTextFragmentsBuilder::build(const SVGTextElement *textElement) {
	handleElement(textElement);
	for (const auto &position : m_textPositions) {
		fillCharacterPositions(position);
	}

	for (const auto &position : m_textPositions) {
		buildTextNodeFragments(position);
	}
	adjustTextAnchors();
}

void SVGTextFragmentsBuilder::buildTextNodeFragments(const SVGTextPosition &textPosition) {
	if (!textPosition.node->isTextNode()) {
		return;
	}

	std::u32string_view wholeText(m_text);
	auto element = toSVGTextPositioningElement(textPosition.node->parent());
	const auto candidates = resolveFontCandidates(element);
	auto baselineOffset = calculateBaselineOffset(element);
	auto nodeText = wholeText.substr(textPosition.startOffset, textPosition.endOffset - textPosition.startOffset);
	auto clusterBreaks = graphemeBreaks(nodeText);
	size_t localStartOffset = 0;
	for (auto localEndOffset : clusterBreaks) {
		const auto startOffset = textPosition.startOffset + localStartOffset;
		const auto endOffset = textPosition.startOffset + localEndOffset;
		const TextCluster cluster(wholeText.substr(startOffset, endOffset - startOffset));
		SVGCharacterPosition characterPosition;
		auto positionIt = m_characterPositions.find(startOffset);
		if (positionIt != m_characterPositions.end()) {
			characterPosition = positionIt->second;
		}

		auto angle = characterPosition.rotate.value_or(0);
		auto dx = characterPosition.dx.value_or(0);
		auto dy = characterPosition.dy.value_or(0);

		m_x = dx + characterPosition.x.value_or(m_x);
		m_y = dy + characterPosition.y.value_or(m_y);

		auto selection = selectClusterFont(element, candidates, cluster);
		SVGTextFragment fragment(element);
		fragment.offset = startOffset;
		fragment.length = endOffset - startOffset;
		fragment.font = selection.font;
		fragment.glyphs = std::move(selection.glyphs);
		fragment.x = m_x;
		fragment.y = m_y - baselineOffset;
		fragment.angle = angle;
		fragment.startsNewTextChunk =
				(characterPosition.x || characterPosition.y) && startOffset == textPosition.startOffset;
		fragment.hasEmbeddedSVGGlyph = selection.hasEmbeddedSVGGlyph;
		fragment.isMissingGlyph = selection.isMissingGlyph;
		fragment.isWhitespace = cluster.isWhitespace;
		if (fragment.isMissingGlyph) {
			fragment.width = std::max(1.f, element->font_size() * 0.6f);
		} else if (fragment.font.isNull()) {
			fragment.width = fragment.isWhitespace ? std::max(1.f, element->font_size() * 0.33f) : 0.f;
		} else {
			fragment.width = selection.width;
		}
		m_fragments.push_back(std::move(fragment));
		m_x += fragment.width;
		localStartOffset = localEndOffset;
	}
}

void SVGTextFragmentsBuilder::adjustTextAnchors() {
	if (m_fragments.empty()) {
		return;
	}
	auto it = m_fragments.begin();
	auto begin = m_fragments.begin();
	auto end = m_fragments.end();
	for (++it; it != end; ++it) {
		const SVGTextFragment &fragment = *it;
		if (!fragment.startsNewTextChunk) {
			continue;
		}
		adjustTextAnchor(begin, it);
		begin = it;
	}

	adjustTextAnchor(begin, it);
}

void SVGTextFragmentsBuilder::handleText(const SVGTextNode *node) {
	const auto &text = node->data();
	if (text.empty()) {
		return;
	}
	auto element = toSVGTextPositioningElement(node->parent());
	const auto startOffset = m_text.length();
	uint32_t lastCharacter = ' ';
	if (!m_text.empty()) {
		lastCharacter = m_text.back();
	}

	plutovg_text_iterator_t it;
	plutovg_text_iterator_init(&it, text.data(), text.length(), PLUTOVG_TEXT_ENCODING_UTF8);
	while (plutovg_text_iterator_has_next(&it)) {
		auto currentCharacter = plutovg_text_iterator_next(&it);
		if (currentCharacter == '\t' || currentCharacter == '\n' || currentCharacter == '\r') {
			currentCharacter = ' ';
		}
		if (currentCharacter == ' ' && lastCharacter == ' ' && element->white_space() == WhiteSpace::Default) {
			continue;
		}
		m_text.push_back(currentCharacter);
		lastCharacter = currentCharacter;
	}

	if (startOffset < m_text.length()) {
		m_textPositions.emplace_back(node, startOffset, m_text.length());
	}
}

void SVGTextFragmentsBuilder::handleElement(const SVGTextPositioningElement *element) {
	auto itemIndex = m_textPositions.size();
	m_textPositions.emplace_back(element, m_text.length(), m_text.length());
	for (const auto &child : element->children()) {
		if (child->isTextNode()) {
			handleText(toSVGTextNode(child.get()));
		} else if (child->isTextPositioningElement()) {
			handleElement(toSVGTextPositioningElement(child.get()));
		}
	}

	auto &position = m_textPositions[itemIndex];
	assert(position.node == element);
	position.endOffset = m_text.length();
}

void SVGTextFragmentsBuilder::fillCharacterPositions(const SVGTextPosition &position) {
	if (!position.node->isTextPositioningElement()) {
		return;
	}
	auto element = toSVGTextPositioningElement(position.node);
	const auto &xList = element->x();
	const auto &yList = element->y();
	const auto &dxList = element->dx();
	const auto &dyList = element->dy();
	const auto &rotateList = element->rotate();

	auto xListSize = xList.size();
	auto yListSize = yList.size();
	auto dxListSize = dxList.size();
	auto dyListSize = dyList.size();
	auto rotateListSize = rotateList.size();
	if (!xListSize && !yListSize && !dxListSize && !dyListSize && !rotateListSize) {
		return;
	}

	LengthContext lengthContext(element);
	std::optional<float> lastRotation;
	for (auto offset = position.startOffset; offset < position.endOffset; ++offset) {
		auto index = offset - position.startOffset;
		if (index >= xListSize && index >= yListSize && index >= dxListSize && index >= dyListSize && index >= rotateListSize) {
			break;
		}
		auto &characterPosition = m_characterPositions[offset];
		if (index < xListSize) {
			characterPosition.x = lengthContext.valueForLength(xList[index], LengthDirection::Horizontal);
		}
		if (index < yListSize) {
			characterPosition.y = lengthContext.valueForLength(yList[index], LengthDirection::Vertical);
		}
		if (index < dxListSize) {
			characterPosition.dx = lengthContext.valueForLength(dxList[index], LengthDirection::Horizontal);
		}
		if (index < dyListSize) {
			characterPosition.dy = lengthContext.valueForLength(dyList[index], LengthDirection::Vertical);
		}
		if (index < rotateListSize) {
			characterPosition.rotate = rotateList[index];
			lastRotation = characterPosition.rotate;
		}
	}

	if (lastRotation == std::nullopt) {
		return;
	}
	auto offset = position.startOffset + rotateList.size();
	while (offset < position.endOffset) {
		m_characterPositions[offset++].rotate = lastRotation;
	}
}

SVGTextPositioningElement::SVGTextPositioningElement(Document *document, ElementID id) :
		SVGGraphicsElement(document, id), m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow), m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow), m_dx(PropertyID::Dx, LengthDirection::Horizontal, LengthNegativeMode::Allow), m_dy(PropertyID::Dy, LengthDirection::Vertical, LengthNegativeMode::Allow), m_rotate(PropertyID::Rotate) {
	addProperty(m_x);
	addProperty(m_y);
	addProperty(m_dx);
	addProperty(m_dy);
	addProperty(m_rotate);
}

void SVGTextPositioningElement::layoutElement(const SVGLayoutState &state) {
	m_font = state.font();
	m_font_family = state.font_family();
	m_font_bold = state.font_weight() == FontWeight::Bold;
	m_font_italic = state.font_style() == FontStyle::Italic;
	m_fill = getPaintServer(state.fill(), state.fill_opacity());
	m_stroke = getPaintServer(state.stroke(), state.stroke_opacity());
	SVGGraphicsElement::layoutElement(state);

	LengthContext lengthContext(this);
	m_stroke_width = lengthContext.valueForLength(state.stroke_width(), LengthDirection::Diagonal);
	m_baseline_offset = convertBaselineOffset(state.baseline_shit());
	m_alignment_baseline = state.alignment_baseline();
	m_dominant_baseline = state.dominant_baseline();
	m_text_anchor = state.text_anchor();
	m_white_space = state.white_space();
	m_direction = state.direction();
}

float SVGTextPositioningElement::convertBaselineOffset(const BaselineShift &baselineShift) const {
	if (baselineShift.type() == BaselineShift::Type::Baseline) {
		return 0.f;
	}
	if (baselineShift.type() == BaselineShift::Type::Sub) {
		return -m_font.height() / 2.f;
	}
	if (baselineShift.type() == BaselineShift::Type::Super) {
		return m_font.height() / 2.f;
	}

	const auto &length = baselineShift.length();
	if (length.units() == LengthUnits::Percent) {
		return length.value() * m_font.size() / 100.f;
	}
	if (length.units() == LengthUnits::Ex) {
		return length.value() * m_font.size() / 2.f;
	}
	if (length.units() == LengthUnits::Em) {
		return length.value() * m_font.size();
	}
	return length.value();
}

SVGTSpanElement::SVGTSpanElement(Document *document) :
		SVGTextPositioningElement(document, ElementID::Tspan) {
}

SVGTextElement::SVGTextElement(Document *document) :
		SVGTextPositioningElement(document, ElementID::Text) {
}

void SVGTextElement::layout(SVGLayoutState &state) {
	SVGTextPositioningElement::layout(state);
	SVGTextFragmentsBuilder fragmentsBuilder(m_text, m_fragments);
	fragmentsBuilder.build(this);
}

void SVGTextElement::render(SVGRenderState &state) const {
	if (m_text.empty() || isVisibilityHidden() || isDisplayNone()) {
		return;
	}
	SVGBlendInfo blendInfo(this);
	SVGRenderState newState(this, state, localTransform());
	newState.beginGroup(blendInfo);
	if (newState.mode() == SVGRenderMode::Clipping) {
		newState->setColor(Color::White);
	}

	std::u32string_view wholeText(m_text);
	for (const auto &fragment : m_fragments) {
		auto transform = newState.currentTransform() * Transform::rotated(fragment.angle, fragment.x, fragment.y);
		if (fragment.isWhitespace) {
			continue;
		}
		auto text = fragment.glyphs.empty() && !fragment.isMissingGlyph
				? drawableClusterText(wholeText.substr(fragment.offset, fragment.length))
				: std::u32string();

		if (newState.mode() == SVGRenderMode::Clipping) {
			if (fragment.isMissingGlyph) {
				newState->fillPath(missingGlyphPath(fragment), FillRule::EvenOdd, transform);
			} else if (!fragment.glyphs.empty()) {
				newState->fillPath(shapedGlyphPath(fragment), FillRule::NonZero, transform);
			} else {
				newState->fillText(text, fragment.font, Point(fragment.x, fragment.y), transform);
			}
		} else {
			const auto &fill = fragment.element->fill();
			const auto &stroke = fragment.element->stroke();
			auto stroke_width = fragment.element->stroke_width();
			if (fragment.isMissingGlyph) {
				auto path = missingGlyphPath(fragment);
				if (fill.applyPaint(newState)) {
					newState->fillPath(path, FillRule::EvenOdd, transform);
				}
				if (stroke.applyPaint(newState)) {
					newState->strokePath(path, StrokeData(stroke_width), transform);
				}
				continue;
			}
			if (fragment.hasEmbeddedSVGGlyph && tryRenderEmbeddedSVGGlyph(fragment, transform, newState)) {
				continue;
			}
			auto shapedPath = fragment.glyphs.empty() ? Path() : shapedGlyphPath(fragment);
			if (fill.applyPaint(newState)) {
				if (fragment.glyphs.empty()) {
					newState->fillText(text, fragment.font, Point(fragment.x, fragment.y), transform);
				} else {
					newState->fillPath(shapedPath, FillRule::NonZero, transform);
				}
			}
			if (stroke.applyPaint(newState)) {
				if (fragment.glyphs.empty()) {
					newState->strokeText(text, stroke_width, fragment.font, Point(fragment.x, fragment.y), transform);
				} else {
					newState->strokePath(shapedPath, StrokeData(stroke_width), transform);
				}
			}
		}
	}

	newState.endGroup(blendInfo);
}

Rect SVGTextElement::boundingBox(bool includeStroke) const {
	auto boundingBox = Rect::Invalid;
	for (const auto &fragment : m_fragments) {
		if (fragment.isWhitespace) {
			continue;
		}
		const auto &font = fragment.font;
		const auto &stroke = fragment.element->stroke();
		auto fragmentTranform = Transform::rotated(fragment.angle, fragment.x, fragment.y);
		auto fragmentRect = fragment.isMissingGlyph ? missingGlyphRect(fragment)
											: (fragment.glyphs.empty()
														  ? Rect(fragment.x, fragment.y - font.ascent(), fragment.width,
																	fragment.element->font_size())
														  : shapedGlyphBounds(fragment));
		if (!fragment.isMissingGlyph && fragment.hasEmbeddedSVGGlyph && fragment.glyphs.size() == 1) {
			EmbeddedSVGGlyphInfo glyph;
			const auto &shapedGlyph = fragment.glyphs.front();
			auto origin = Point(fragment.x + shapedGlyph.x, fragment.y - shapedGlyph.y);
			if (tryResolveEmbeddedSVGGlyph(font, shapedGlyph.index, origin, glyph)) {
				fragmentRect = glyph.dstRect;
			}
		}
		if (includeStroke && stroke.isRenderable()) {
			fragmentRect.inflate(fragment.element->stroke_width() / 2.f);
		}
		boundingBox.unite(fragmentTranform.mapRect(fragmentRect));
	}

	if (!boundingBox.isValid()) {
		boundingBox = Rect::Empty;
	}
	return boundingBox;
}

} // namespace lunasvg
