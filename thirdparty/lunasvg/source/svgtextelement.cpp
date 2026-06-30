#include "svgtextelement.h"
#include "svglayoutstate.h"
#include "svgrenderstate.h"
#include <lunasvg.h>

#include <cassert>
#include <cmath>
#include <unordered_map>

namespace lunasvg {

namespace {

constexpr std::string_view kSPXDefaultFontFamily = "SPX Default";
constexpr std::string_view kSPXSymbolsFontFamily = "Symbols";
constexpr std::string_view kSPXEmojiFontFamily = "Emoji";

enum class TextRunKind : uint8_t {
    Base,
    DefaultFallback,
    SymbolsFallback,
    Emoji
};

static bool isWhitespaceCodepoint(uint32_t codepoint)
{
    switch(codepoint) {
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

struct ColorEmojiGlyphInfo {
    const char* svgData = nullptr;
    size_t svgLength = 0;
    Rect dstRect;
};

struct ColorEmojiBitmapCacheKey {
    const plutovg_font_face_t* face = nullptr;
    uint32_t codepoint = 0;

    bool operator==(const ColorEmojiBitmapCacheKey& other) const
    {
        return face == other.face && codepoint == other.codepoint;
    }
};

struct ColorEmojiBitmapCacheKeyHash {
    size_t operator()(const ColorEmojiBitmapCacheKey& key) const
    {
        auto faceHash = std::hash<const void*>{}(key.face);
        auto codeHash = std::hash<uint32_t>{}(key.codepoint);
        return faceHash ^ (codeHash + 0x9e3779b9 + (faceHash << 6) + (faceHash >> 2));
    }
};

static bool isEmojiCandidateCodepoint(uint32_t codepoint)
{
    if(codepoint == 0x00A9 || codepoint == 0x00AE || codepoint == 0x203C || codepoint == 0x2049 ||
        codepoint == 0x2122 || codepoint == 0x2139 || codepoint == 0x3030 || codepoint == 0x303D ||
        codepoint == 0x3297 || codepoint == 0x3299) {
        return true;
    }

    return (codepoint >= 0x2194 && codepoint <= 0x21AA) ||
        (codepoint >= 0x231A && codepoint <= 0x27BF) ||
        (codepoint >= 0x2934 && codepoint <= 0x2935) ||
        (codepoint >= 0x2B05 && codepoint <= 0x2B55) ||
        (codepoint >= 0x1F000 && codepoint <= 0x1FAFF) ||
        (codepoint >= 0x1FC00 && codepoint <= 0x1FFFD);
}

static bool isEmojiFormattingCodepoint(uint32_t codepoint)
{
    if(codepoint == 0x200D || codepoint == 0x20E3 || codepoint == 0xFE0E || codepoint == 0xFE0F)
        return true;
    if(codepoint >= 0xE0020 && codepoint <= 0xE007F)
        return true;
    return codepoint >= 0x1F3FB && codepoint <= 0x1F3FF;
}

static bool promotesEmojiPresentation(uint32_t codepoint)
{
    return codepoint != 0xFE0E && isEmojiFormattingCodepoint(codepoint);
}

static bool isDefaultEmojiPresentationCodepoint(uint32_t codepoint)
{
    return (codepoint >= 0x231A && codepoint <= 0x231B) ||
        (codepoint >= 0x23E9 && codepoint <= 0x23EC) ||
        codepoint == 0x23F0 ||
        codepoint == 0x23F3 ||
        (codepoint >= 0x25FD && codepoint <= 0x25FE) ||
        (codepoint >= 0x2614 && codepoint <= 0x2615) ||
        (codepoint >= 0x2648 && codepoint <= 0x2653) ||
        codepoint == 0x267F ||
        codepoint == 0x2693 ||
        codepoint == 0x26A1 ||
        (codepoint >= 0x26AA && codepoint <= 0x26AB) ||
        (codepoint >= 0x26BD && codepoint <= 0x26BE) ||
        (codepoint >= 0x26C4 && codepoint <= 0x26C5) ||
        codepoint == 0x26CE ||
        codepoint == 0x26D4 ||
        codepoint == 0x26EA ||
        (codepoint >= 0x26F2 && codepoint <= 0x26F3) ||
        codepoint == 0x26F5 ||
        codepoint == 0x26FA ||
        codepoint == 0x26FD ||
        codepoint == 0x2705 ||
        (codepoint >= 0x270A && codepoint <= 0x270B) ||
        codepoint == 0x2728 ||
        codepoint == 0x274C ||
        codepoint == 0x274E ||
        (codepoint >= 0x2753 && codepoint <= 0x2755) ||
        codepoint == 0x2757 ||
        (codepoint >= 0x2795 && codepoint <= 0x2797) ||
        codepoint == 0x27B0 ||
        codepoint == 0x27BF ||
        (codepoint >= 0x2B1B && codepoint <= 0x2B1C) ||
        codepoint == 0x2B50 ||
        codepoint == 0x2B55;
}

static bool isSymbolsFallbackCodepoint(uint32_t codepoint)
{
    return (codepoint >= 0x2190 && codepoint <= 0x21FF) ||
        (codepoint >= 0x2300 && codepoint <= 0x23FF) ||
        (codepoint >= 0x2460 && codepoint <= 0x27BF) ||
        (codepoint >= 0x2900 && codepoint <= 0x2BFF);
}

static bool isDefaultFallbackCodepoint(uint32_t codepoint)
{
    if(codepoint < 0x80 || isEmojiCandidateCodepoint(codepoint))
        return false;
    if(isWhitespaceCodepoint(codepoint))
        return false;

    return (codepoint >= 0x3000 && codepoint <= 0x303F) ||
        (codepoint >= 0x3040 && codepoint <= 0x30FF) ||
        (codepoint >= 0x3100 && codepoint <= 0x312F) ||
        (codepoint >= 0x31A0 && codepoint <= 0x31BF) ||
        (codepoint >= 0x31C0 && codepoint <= 0x31EF) ||
        (codepoint >= 0x3200 && codepoint <= 0x33FF) ||
        (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||
        (codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||
        (codepoint >= 0xA960 && codepoint <= 0xA97F) ||
        (codepoint >= 0xAC00 && codepoint <= 0xD7AF) ||
        (codepoint >= 0xD7B0 && codepoint <= 0xD7FF) ||
        (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||
        (codepoint >= 0xFE30 && codepoint <= 0xFE6F) ||
        (codepoint >= 0xFF00 && codepoint <= 0xFFEF) ||
        (codepoint >= 0x20000 && codepoint <= 0x323AF);
}

static TextRunKind classifyTextRunKind(uint32_t codepoint, TextRunKind previous, bool emojiPresentation)
{
    if(emojiPresentation || isDefaultEmojiPresentationCodepoint(codepoint))
        return TextRunKind::Emoji;
    if(isSymbolsFallbackCodepoint(codepoint))
        return TextRunKind::SymbolsFallback;
    if(isEmojiCandidateCodepoint(codepoint))
        return TextRunKind::Emoji;
    if(isDefaultFallbackCodepoint(codepoint))
        return TextRunKind::DefaultFallback;
    if(isWhitespaceCodepoint(codepoint))
        return previous;
    return TextRunKind::Base;
}

static Font resolveFragmentFont(const SVGTextPositioningElement* element, TextRunKind kind)
{
    if(kind == TextRunKind::Base)
        return element->font();

    const auto family = kind == TextRunKind::Emoji
        ? kSPXEmojiFontFamily
        : kind == TextRunKind::SymbolsFallback ? kSPXSymbolsFontFamily : kSPXDefaultFontFamily;
    auto face = fontFaceCache()->getFontFace(family, false, false);
    if(face.isNull())
        return element->font();
    return Font(face, element->font().size());
}

static bool tryResolveColorEmojiGlyph(const Font& font, uint32_t codepoint, const Point& origin, ColorEmojiGlyphInfo& glyph)
{
    auto face = font.face().get();
    if(face == nullptr)
        return false;

    auto svgLength = plutovg_font_face_get_glyph_svg(face, codepoint, &glyph.svgData);
    if(svgLength <= 0 || glyph.svgData == nullptr)
        return false;

    plutovg_rect_t glyphExtents = {0};
    plutovg_font_face_get_glyph_metrics(face, font.size(), codepoint, nullptr, nullptr, &glyphExtents);
    glyph.svgLength = static_cast<size_t>(svgLength);
    glyph.dstRect = Rect(origin.x + glyphExtents.x, origin.y + glyphExtents.y, glyphExtents.w, glyphExtents.h);
    return !glyph.dstRect.isEmpty();
}

static const Bitmap* getCachedColorEmojiBitmap(const Font& font, uint32_t codepoint, std::string_view svgDocument)
{
    auto face = font.face().get();
    if(face == nullptr || svgDocument.empty())
        return nullptr;

    // Cache rasterized SVG emoji glyphs per thread to avoid reparsing the
    // embedded SVG document on every frame.
    thread_local std::unordered_map<ColorEmojiBitmapCacheKey, Bitmap, ColorEmojiBitmapCacheKeyHash> bitmapCache;
    ColorEmojiBitmapCacheKey key{face, codepoint};
    auto it = bitmapCache.find(key);
    if(it != bitmapCache.end())
        return it->second.isNull() ? nullptr : &it->second;

    auto document = Document::loadFromData(svgDocument.data(), svgDocument.size());
    if(!document)
        return nullptr;
    auto bounds = Rect(document->boundingBox());
    if(bounds.isEmpty())
        return nullptr;

    auto bitmapWidth = std::max(1, static_cast<int>(std::ceil(bounds.w)));
    auto bitmapHeight = std::max(1, static_cast<int>(std::ceil(bounds.h)));
    Bitmap bitmap(bitmapWidth, bitmapHeight);
    bitmap.clear(0x00000000);
    document->render(bitmap, Matrix::translated(-bounds.x, -bounds.y));
    return &bitmapCache.emplace(key, std::move(bitmap)).first->second;
}

static bool tryRenderColorEmojiGlyph(std::u32string_view text, const Font& font, const Point& origin, const Transform& transform, SVGRenderState& state)
{
    if(text.size() != 1)
        return false;

    ColorEmojiGlyphInfo glyph;
    if(!tryResolveColorEmojiGlyph(font, text.front(), origin, glyph))
        return false;

    auto bitmap = getCachedColorEmojiBitmap(font, text.front(), std::string_view(glyph.svgData, glyph.svgLength));
    if(bitmap == nullptr)
        return false;

    state->drawImage(*bitmap, glyph.dstRect, Rect(0, 0, bitmap->width(), bitmap->height()), transform);
    return true;
}

} // namespace

inline const SVGTextNode* toSVGTextNode(const SVGNode* node)
{
    assert(node && node->isTextNode());
    return static_cast<const SVGTextNode*>(node);
}

inline const SVGTextPositioningElement* toSVGTextPositioningElement(const SVGNode* node)
{
    assert(node && node->isTextPositioningElement());
    return static_cast<const SVGTextPositioningElement*>(node);
}

static AlignmentBaseline resolveDominantBaseline(const SVGTextPositioningElement* element)
{
    switch(element->dominant_baseline()) {
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

static float calculateBaselineOffset(const SVGTextPositioningElement* element)
{
    auto offset = element->baseline_offset();
    for(auto parent = element->parent(); parent->isTextPositioningElement(); parent = parent->parent()) {
        offset += toSVGTextPositioningElement(parent)->baseline_offset();
    }

    auto baseline = element->alignment_baseline();
    if(baseline == AlignmentBaseline::Auto || baseline == AlignmentBaseline::Baseline) {
        baseline = resolveDominantBaseline(element);
    }

    const auto& font = element->font();
    switch(baseline) {
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

static bool needsTextAnchorAdjustment(const SVGTextPositioningElement* element)
{
    auto direction = element->direction();
    switch(element->text_anchor()) {
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

static float calculateTextAnchorOffset(const SVGTextPositioningElement* element, float width)
{
    auto direction = element->direction();
    switch(element->text_anchor()) {
    case TextAnchor::Start:
        if(direction == Direction::Ltr)
            return 0.f;
        return -width;
    case TextAnchor::Middle:
        return -width / 2.f;
    case TextAnchor::End:
        if(direction == Direction::Ltr)
            return -width;
        return 0.f;
    default:
        assert(false);
    }

    return 0.f;
}

SVGTextFragmentsBuilder::SVGTextFragmentsBuilder(std::u32string& text, SVGTextFragmentList& fragments)
    : m_text(text), m_fragments(fragments)
{
    m_text.clear();
    m_fragments.clear();
}

void SVGTextFragmentsBuilder::build(const SVGTextElement* textElement)
{
    handleElement(textElement);
    for(const auto& position : m_textPositions) {
        fillCharacterPositions(position);
    }

    std::u32string_view wholeText(m_text);
    for(const auto& textPosition : m_textPositions) {
        if(!textPosition.node->isTextNode())
            continue;
        auto element = toSVGTextPositioningElement(textPosition.node->parent());
        SVGTextFragment fragment(element);
        auto currentRunKind = TextRunKind::Base;
        auto recordTextFragment = [&](auto startOffset, auto endOffset, TextRunKind runKind) {
            if(startOffset == endOffset)
                return;
            auto text = wholeText.substr(startOffset, endOffset - startOffset);
            fragment.offset = startOffset;
            fragment.length = endOffset - startOffset;
            fragment.font = resolveFragmentFont(element, runKind);
            fragment.isEmojiRun = runKind == TextRunKind::Emoji;
            fragment.width = fragment.font.measureText(text);
            m_fragments.push_back(fragment);
            m_x += fragment.width;
        };

        auto baselineOffset = calculateBaselineOffset(element);
        auto startOffset = textPosition.startOffset;
        auto textOffset = textPosition.startOffset;
        auto didStartTextFragment = false;
        auto lastAngle = 0.f;
        while(textOffset < textPosition.endOffset) {
            auto hasEmojiPresentation = m_emojiPresentationOffsets.find(textOffset) != m_emojiPresentationOffsets.end();
            auto runKind = classifyTextRunKind(m_text[textOffset], currentRunKind, hasEmojiPresentation);
            SVGCharacterPosition characterPosition;
            auto positionIt = m_characterPositions.find(textOffset);
            if(positionIt != m_characterPositions.end()) {
                characterPosition = positionIt->second;
            }

            auto angle = characterPosition.rotate.value_or(0);
            auto dx = characterPosition.dx.value_or(0);
            auto dy = characterPosition.dy.value_or(0);

            auto shouldStartNewFragment = characterPosition.x || characterPosition.y || dx || dy || angle || angle != lastAngle || runKind != currentRunKind || runKind == TextRunKind::Emoji;
            if(shouldStartNewFragment && didStartTextFragment) {
                recordTextFragment(startOffset, textOffset, currentRunKind);
                startOffset = textOffset;
            }

            auto startsNewTextChunk = (characterPosition.x || characterPosition.y) && textOffset == textPosition.startOffset;
            if(startsNewTextChunk || shouldStartNewFragment || !didStartTextFragment) {
                m_x = dx + characterPosition.x.value_or(m_x);
                m_y = dy + characterPosition.y.value_or(m_y);
                fragment.x = m_x;
                fragment.y = m_y - baselineOffset;
                fragment.angle = angle;
                fragment.startsNewTextChunk = startsNewTextChunk;
                didStartTextFragment = true;
                currentRunKind = runKind;
            }

            lastAngle = angle;
            ++textOffset;
        }

        recordTextFragment(startOffset, textOffset, currentRunKind);
    }

    auto handleTextChunk = [](auto begin, auto end) {
        if(!needsTextAnchorAdjustment(begin->element))
            return;
        float chunkWidth = 0.f;
        const SVGTextFragment* lastFragment = nullptr;
        for(auto it = begin; it != end; ++it) {
            const SVGTextFragment& fragment = *it;
            chunkWidth += fragment.width;
            if(lastFragment)
                chunkWidth += fragment.x - (lastFragment->x + lastFragment->width);
            lastFragment = &fragment;
        }

        auto chunkOffset = calculateTextAnchorOffset(begin->element, chunkWidth);
        for(auto it = begin; it != end; ++it) {
            SVGTextFragment& fragment = *it;
            fragment.x += chunkOffset;
        }
    };

    if(m_fragments.empty())
        return;
    auto it = m_fragments.begin();
    auto begin = m_fragments.begin();
    auto end = m_fragments.end();
    for(++it; it != end; ++it) {
        const SVGTextFragment& fragment = *it;
        if(!fragment.startsNewTextChunk)
            continue;
        handleTextChunk(begin, it);
        begin = it;
    }

    handleTextChunk(begin, it);
}

void SVGTextFragmentsBuilder::handleText(const SVGTextNode* node)
{
    const auto& text = node->data();
    if(text.empty())
        return;
    auto element = toSVGTextPositioningElement(node->parent());
    const auto startOffset = m_text.length();
    uint32_t lastCharacter = ' ';
    if(!m_text.empty()) {
        lastCharacter = m_text.back();
    }

    plutovg_text_iterator_t it;
    plutovg_text_iterator_init(&it, text.data(), text.length(), PLUTOVG_TEXT_ENCODING_UTF8);
    while(plutovg_text_iterator_has_next(&it)) {
        auto currentCharacter = plutovg_text_iterator_next(&it);
        if(isEmojiFormattingCodepoint(currentCharacter)) {
            if(promotesEmojiPresentation(currentCharacter) && !m_text.empty())
                m_emojiPresentationOffsets.insert(m_text.length() - 1);
            continue;
        }
        if(currentCharacter == '\t' || currentCharacter == '\n' || currentCharacter == '\r')
            currentCharacter = ' ';
        if(currentCharacter == ' ' && lastCharacter == ' ' && element->white_space() == WhiteSpace::Default)
            continue;
        m_text.push_back(currentCharacter);
        lastCharacter = currentCharacter;
    }

    if(startOffset < m_text.length()) {
        m_textPositions.emplace_back(node, startOffset, m_text.length());
    }
}

void SVGTextFragmentsBuilder::handleElement(const SVGTextPositioningElement* element)
{
    auto itemIndex = m_textPositions.size();
    m_textPositions.emplace_back(element, m_text.length(), m_text.length());
    for(const auto& child : element->children()) {
        if(child->isTextNode()) {
            handleText(toSVGTextNode(child.get()));
        } else if(child->isTextPositioningElement()) {
            handleElement(toSVGTextPositioningElement(child.get()));
        }
    }

    auto& position = m_textPositions[itemIndex];
    assert(position.node == element);
    position.endOffset = m_text.length();
}

void SVGTextFragmentsBuilder::fillCharacterPositions(const SVGTextPosition& position)
{
    if(!position.node->isTextPositioningElement())
        return;
    auto element = toSVGTextPositioningElement(position.node);
    const auto& xList = element->x();
    const auto& yList = element->y();
    const auto& dxList = element->dx();
    const auto& dyList = element->dy();
    const auto& rotateList = element->rotate();

    auto xListSize = xList.size();
    auto yListSize = yList.size();
    auto dxListSize = dxList.size();
    auto dyListSize = dyList.size();
    auto rotateListSize = rotateList.size();
    if(!xListSize && !yListSize && !dxListSize && !dyListSize && !rotateListSize) {
        return;
    }

    LengthContext lengthContext(element);
    std::optional<float> lastRotation;
    for(auto offset = position.startOffset; offset < position.endOffset; ++offset) {
        auto index = offset - position.startOffset;
        if(index >= xListSize && index >= yListSize && index >= dxListSize && index >= dyListSize && index >= rotateListSize)
            break;
        auto& characterPosition = m_characterPositions[offset];
        if(index < xListSize)
            characterPosition.x = lengthContext.valueForLength(xList[index], LengthDirection::Horizontal);
        if(index < yListSize)
            characterPosition.y = lengthContext.valueForLength(yList[index], LengthDirection::Vertical);
        if(index < dxListSize)
            characterPosition.dx = lengthContext.valueForLength(dxList[index], LengthDirection::Horizontal);
        if(index < dyListSize)
            characterPosition.dy = lengthContext.valueForLength(dyList[index], LengthDirection::Vertical);
        if(index < rotateListSize) {
            characterPosition.rotate = rotateList[index];
            lastRotation = characterPosition.rotate;
        }
    }

    if(lastRotation == std::nullopt)
        return;
    auto offset = position.startOffset + rotateList.size();
    while(offset < position.endOffset) {
        m_characterPositions[offset++].rotate = lastRotation;
    }
}

SVGTextPositioningElement::SVGTextPositioningElement(Document* document, ElementID id)
    : SVGGraphicsElement(document, id)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_dx(PropertyID::Dx, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_dy(PropertyID::Dy, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_rotate(PropertyID::Rotate)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_dx);
    addProperty(m_dy);
    addProperty(m_rotate);
}

void SVGTextPositioningElement::layoutElement(const SVGLayoutState& state)
{
    m_font = state.font();
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

float SVGTextPositioningElement::convertBaselineOffset(const BaselineShift& baselineShift) const
{
    if(baselineShift.type() == BaselineShift::Type::Baseline)
        return 0.f;
    if(baselineShift.type() == BaselineShift::Type::Sub)
        return -m_font.height() / 2.f;
    if(baselineShift.type() == BaselineShift::Type::Super) {
        return m_font.height() / 2.f;
    }

    const auto& length = baselineShift.length();
    if(length.units() == LengthUnits::Percent)
        return length.value() * m_font.size() / 100.f;
    if(length.units() == LengthUnits::Ex)
        return length.value() * m_font.size() / 2.f;
    if(length.units() == LengthUnits::Em)
        return length.value() * m_font.size();
    return length.value();
}

SVGTSpanElement::SVGTSpanElement(Document* document)
    : SVGTextPositioningElement(document, ElementID::Tspan)
{
}

SVGTextElement::SVGTextElement(Document* document)
    : SVGTextPositioningElement(document, ElementID::Text)
{
}

void SVGTextElement::layout(SVGLayoutState& state)
{
    SVGTextPositioningElement::layout(state);
    SVGTextFragmentsBuilder fragmentsBuilder(m_text, m_fragments);
    fragmentsBuilder.build(this);
}

void SVGTextElement::render(SVGRenderState& state) const
{
    if(m_text.empty() || isVisibilityHidden() || isDisplayNone())
        return;
    SVGBlendInfo blendInfo(this);
    SVGRenderState newState(this, state, localTransform());
    newState.beginGroup(blendInfo);
    if(newState.mode() == SVGRenderMode::Clipping) {
        newState->setColor(Color::White);
    }

    std::u32string_view wholeText(m_text);
    for(const auto& fragment : m_fragments) {
        auto transform = newState.currentTransform() * Transform::rotated(fragment.angle, fragment.x, fragment.y);
        auto origin = Point(fragment.x, fragment.y);
        auto text = wholeText.substr(fragment.offset, fragment.length);

        const auto& font = fragment.font;
        if(newState.mode() == SVGRenderMode::Clipping) {
            newState->fillText(text, font, origin, transform);
        } else {
            if(fragment.isEmojiRun && tryRenderColorEmojiGlyph(text, font, origin, transform, newState))
                continue;
            const auto& fill = fragment.element->fill();
            const auto& stroke = fragment.element->stroke();
            auto stroke_width = fragment.element->stroke_width();
            if(fill.applyPaint(newState))
                newState->fillText(text, font, origin, transform);
            if(stroke.applyPaint(newState)) {
                newState->strokeText(text, stroke_width, font, origin, transform);
            }
        }
    }

    newState.endGroup(blendInfo);
}

Rect SVGTextElement::boundingBox(bool includeStroke) const
{
    auto boundingBox = Rect::Invalid;
    std::u32string_view wholeText(m_text);
    for(const auto& fragment : m_fragments) {
        const auto& font = fragment.font;
        const auto& stroke = fragment.element->stroke();
        auto fragmentTranform = Transform::rotated(fragment.angle, fragment.x, fragment.y);
        auto text = wholeText.substr(fragment.offset, fragment.length);
        auto origin = Point(fragment.x, fragment.y);
        auto fragmentRect = Rect(fragment.x, fragment.y - font.ascent(), fragment.width, fragment.element->font_size());
        if(fragment.isEmojiRun && text.size() == 1) {
            ColorEmojiGlyphInfo glyph;
            if(tryResolveColorEmojiGlyph(font, text.front(), origin, glyph))
                fragmentRect = glyph.dstRect;
        }
        if(includeStroke && stroke.isRenderable())
            fragmentRect.inflate(fragment.element->stroke_width() / 2.f);
        boundingBox.unite(fragmentTranform.mapRect(fragmentRect));
    }

    if(!boundingBox.isValid())
        boundingBox = Rect::Empty;
    return boundingBox;
}

} // namespace lunasvg
