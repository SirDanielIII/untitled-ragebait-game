#include "engine/resources/resources.h"

#include <algorithm>
#include <vector>
#include <cmath>
#include "rlgl.h"

namespace rage {
    std::filesystem::path FindResourceDirectory() {
        const std::vector<std::filesystem::path> candidates = {
            std::filesystem::path(GetApplicationDirectory()) / "resources",
            std::filesystem::path(GetApplicationDirectory()).parent_path() / "resources",
            "resources", "game/resources"
        };
        for (const auto &candidate: candidates) {
            std::error_code error;
            if (std::filesystem::is_directory(candidate, error)) return candidate;
        }
        return candidates.front(); // Loaders diagnose missing files and retain defaults.
    }

    Resources::Resources(const std::filesystem::path &directory) {
        for (std::size_t i = 0; i < faces.size(); ++i) {
            const auto path = directory / fontPaths[i];
            int length = 0;
            unsigned char *data = FileExists(path.string().c_str()) ? LoadFileData(path.string().c_str(), &length) : nullptr;
            if (data && length > 0) faces[i].bytes.assign(data, data + length);
            if (data) UnloadFileData(data);
            if (faces[i].bytes.empty())
                TraceLog(LOG_WARNING, "Custom font unavailable: %s; using default font", path.string().c_str());
        }
        for (std::size_t i = 0; i < artwork.size(); ++i) {
            const auto path = directory / artPaths[i];
            if (FileExists(path.string().c_str())) {
                artwork[i] = LoadTexture(path.string().c_str());
                if (IsTextureValid(artwork[i])) SetTextureFilter(artwork[i], TEXTURE_FILTER_POINT);
            }
        }
        assets.Load(directory);
        audio.Load(directory);
    }

    Resources::~Resources() {
        for (const auto texture: artwork) if (IsTextureValid(texture)) UnloadTexture(texture);
        for (auto &face : faces)
            for (const auto &[size, entry] : face.sizes) UnloadFont(entry.font);
    }

    namespace {
        constexpr float FontCompensation(FontRole role) {
            return role == FontRole::Typewriter ? 1.40f : 1.43f;
        }

        // Raylib's raster size is the ascent-to-descent height, not the em size.
        // Align the new fonts' capital baseline with the old logical text box.
        constexpr float BaselineRatio(FontRole role) {
            return role == FontRole::Typewriter ? 1125.0f / 1500.0f : 1030.0f / 1280.0f;
        }

        const std::vector<int> &UiCodepoints() {
            static const auto values = [] {
                std::vector<int> result;
                for (int c = 32; c <= 126; ++c) result.push_back(c);
                for (int c = 160; c <= 255; ++c) result.push_back(c);
                for (int c : {0x2013, 0x2014, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2026})
                    result.push_back(c);
                return result;
            }();
            return values;
        }

        int ReadCodepoint(std::string_view text, std::size_t &offset) {
            // GetCodepointNext requires a terminated buffer. Avoid a string allocation
            // for every measure/draw, including non-terminated typewriter substrings.
            unsigned char first = static_cast<unsigned char>(text[offset]);
            if (first < 128) { ++offset; return first; }
            char buffer[5]{};
            const auto count = std::min<std::size_t>(4, text.size() - offset);
            std::copy_n(text.data() + offset, count, buffer);
            int length = 0;
            int codepoint = GetCodepointNext(buffer, &length);
            offset += std::clamp<std::size_t>(length > 0 ? length : 1, 1, count);
            return codepoint;
        }
    }

    Font Resources::ResolveFont(FontRole role, float size, float physicalScale) const {
        const auto &face = faces[static_cast<std::size_t>(role)];
        if (face.bytes.empty()) return GetFontDefault();
        const int pixels = std::clamp(static_cast<int>(std::round(std::clamp(
            size * FontCompensation(role) * physicalScale, 1.0f, 256.0f))), 1, 256);
        if (auto it = face.sizes.find(pixels); it != face.sizes.end()) {
            it->second.lastUsed = frameNumber;
            return it->second.font;
        }
        // Rasterize a binary-alpha atlas at the actual physical pixel height.
        // TTF data is read once; only new integer raster sizes incur GPU uploads.
        Font font{};
        font.baseSize = pixels;
        font.glyphPadding = 1;
        const auto &codepoints = UiCodepoints();
        font.glyphs = LoadFontData(face.bytes.data(), static_cast<int>(face.bytes.size()),
                                  pixels, codepoints.data(), static_cast<int>(codepoints.size()),
                                  FONT_BITMAP, &font.glyphCount);
        if (font.glyphs && font.glyphCount > 0) {
            Image atlas = GenImageFontAtlas(font.glyphs, &font.recs, font.glyphCount,
                                           pixels, font.glyphPadding, 0);
            if (atlas.data && font.recs) font.texture = LoadTextureFromImage(atlas);
            if (atlas.data) UnloadImage(atlas);
        }
        if (!IsFontValid(font) || !IsTextureValid(font.texture)) {
            if (font.glyphs) UnloadFontData(font.glyphs, font.glyphCount);
            if (font.recs) MemFree(font.recs);
            if (font.texture.id) UnloadTexture(font.texture);
            font = GetFontDefault();
        } else {
            SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
            ++fontBuildCount;
        }
        face.sizes.emplace(pixels, CachedFont{font, frameNumber});
        return font;
    }

    const std::vector<std::pair<int, float>> &Resources::LogicalAdvances(FontRole role) const {
        const auto &face = faces[static_cast<std::size_t>(role)];
        if (!face.logicalAdvances.empty()) return face.logicalAdvances;
        // Read reference advances once, with enough precision that logical layout
        // never depends on integer glyph advances at the current window resolution.
        // No reference atlas is uploaded to the GPU.
        constexpr int referenceSize = 256;
        int count = 0;
        const auto &codepoints = UiCodepoints();
        GlyphInfo *glyphs = face.bytes.empty() ? nullptr : LoadFontData(face.bytes.data(),
            static_cast<int>(face.bytes.size()), referenceSize, codepoints.data(),
            static_cast<int>(codepoints.size()), FONT_BITMAP, &count);
        if (glyphs && count > 0) {
            face.logicalAdvances.reserve(count);
            for (int i = 0; i < count; ++i) {
                const auto &g = glyphs[i];
                const float advance = g.advanceX > 0 ? g.advanceX : g.image.width;
                face.logicalAdvances.emplace_back(g.value, advance * FontCompensation(role) / referenceSize);
            }
            UnloadFontData(glyphs, count);
        } else {
            if (glyphs) UnloadFontData(glyphs, count);
            const Font fallback = GetFontDefault();
            face.logicalAdvances.reserve(fallback.glyphCount);
            for (int i = 0; i < fallback.glyphCount; ++i) {
                const auto &g = fallback.glyphs[i];
                const float advance = g.advanceX > 0 ? g.advanceX : fallback.recs[i].width;
                face.logicalAdvances.emplace_back(g.value, advance / fallback.baseSize);
            }
        }
        std::sort(face.logicalAdvances.begin(), face.logicalAdvances.end());
        return face.logicalAdvances;
    }

    bool Resources::HasCustomFont(FontRole role) const {
        const auto font = ResolveFont(role, 24, 1);
        return IsFontValid(font) && font.texture.id != GetFontDefault().texture.id;
    }

    std::size_t Resources::CachedFontCount() const {
        std::size_t count = 0;
        for (const auto &face : faces) count += face.sizes.size();
        return count;
    }

    void Resources::BeginFrame() {
        ++frameNumber;
        // Evict only between frames: an atlas may still be referenced by a queued draw.
        // Keep recently used sizes across nearby window resizes, with a per-face budget.
        constexpr std::size_t maxSizes = 48;
        constexpr std::size_t maxAtlasBytes = 16 * 1024 * 1024;
        for (auto &face : faces) {
            auto cost = [](const Font &font) -> std::size_t {
                return font.texture.id == GetFontDefault().texture.id ? 0 :
                    static_cast<std::size_t>(font.texture.width) * font.texture.height * 4;
            };
            std::size_t bytes = 0;
            for (const auto &[size, entry] : face.sizes) bytes += cost(entry.font);
            while (face.sizes.size() > maxSizes || bytes > maxAtlasBytes) {
                const auto oldest = std::min_element(face.sizes.begin(), face.sizes.end(),
                    [](const auto &a, const auto &b) { return a.second.lastUsed < b.second.lastUsed; });
                // Don't thrash when one unusually complex frame exceeds the soft budget.
                if (oldest->second.lastUsed + 1 >= frameNumber) break;
                bytes -= cost(oldest->second.font);
                UnloadFont(oldest->second.font);
                face.sizes.erase(oldest);
            }
        }
        viewport = {0, 0, canvasWidth, canvasHeight};
        viewportScale = {1, 1};
    }

    void Resources::SetViewport(Rectangle physicalDestination) {
        viewport = physicalDestination;
        viewportScale = {physicalDestination.width / canvasWidth, physicalDestination.height / canvasHeight};
    }

    void Resources::BeginClip(Rectangle bounds) const {
        // Use physical framebuffer coordinates directly: Raylib's BeginScissorMode
        // otherwise applies its own DPI scale and truncates fractional positions.
        // Match the pixel-center coverage of adjacent transformed rectangles.
        // Clamp to the logical viewport so fractional edges cannot leak into bars.
        const auto edge = [](float value) { return static_cast<int>(std::ceil(value - 0.5f)); };
        const int left = edge(viewport.x + std::clamp(bounds.x, 0.0f, float(canvasWidth)) * viewportScale.x);
        const int top = edge(viewport.y + std::clamp(bounds.y, 0.0f, float(canvasHeight)) * viewportScale.y);
        const int right = edge(viewport.x + std::clamp(bounds.x + bounds.width, 0.0f, float(canvasWidth)) * viewportScale.x);
        const int bottom = edge(viewport.y + std::clamp(bounds.y + bounds.height, 0.0f, float(canvasHeight)) * viewportScale.y);
        rlDrawRenderBatchActive();
        rlEnableScissorTest();
        const int framebufferHeight = rlGetActiveFramebuffer() ? rlGetFramebufferHeight() : GetRenderHeight();
        rlScissor(left, framebufferHeight - bottom, std::max(0, right - left), std::max(0, bottom - top));
    }

    Vector2 Resources::LayoutText(std::string_view text, Vector2 position, float size, float spacing,
                                  Color color, FontRole role, bool draw, Rectangle *inkBounds) const {
        if (text.empty() || !std::isfinite(size) || size <= 0 || viewportScale.x <= 0 || viewportScale.y <= 0) return {};
        // Measurement uses one logical metric set even when the raster size changes.
        const auto &advances = LogicalAdvances(role);
        const auto advanceFor = [&](int codepoint) {
            const int ascii = codepoint - 32;
            if (ascii >= 0 && static_cast<std::size_t>(ascii) < advances.size() && advances[ascii].first == codepoint)
                return advances[ascii].second;
            auto find = [&](int cp) { return std::lower_bound(advances.begin(), advances.end(), cp,
                [](const auto &entry, int value) { return entry.first < value; }); };
            auto it = find(codepoint);
            if (it == advances.end() || it->first != codepoint) it = find('?');
            return it != advances.end() ? it->second : 0.5f;
        };
        // Use the actual active camera/viewport matrix when drawing. UI uses the
        // logical viewport; world labels also include the camera zoom and translation.
        const bool rasterize = draw || inkBounds;
        const Matrix matrix = rasterize ? rlGetMatrixModelview() : MatrixIdentity();
        const float scaleX = std::abs(matrix.m0), scaleY = std::abs(matrix.m5);
        if (rasterize && (scaleX <= 0 || scaleY <= 0)) return {};
        const Font font = rasterize ? ResolveFont(role, size, scaleY) : Font{};
        const bool custom = rasterize && font.texture.id != GetFontDefault().texture.id;
        const float desiredPixels = size * FontCompensation(role) * scaleY;
        const float rasterScale = !rasterize ? 1.0f : custom ? std::max(1.0f, desiredPixels / 256.0f)
                                                      : size * scaleY / font.baseSize;
        const float glyphX = rasterScale / (rasterize ? scaleX : 1.0f);
        const float glyphY = rasterScale / (rasterize ? scaleY : 1.0f);
        const float offsetY = custom ? size * 0.8f - font.baseSize * glyphY * BaselineRatio(role) : 0;
        const float gap = spacing;
        float x = 0, width = 0, y = 0;
        bool hasGlyph = false;
        bool hasInk = false;
        for (std::size_t i = 0; i < text.size();) {
            int codepoint = ReadCodepoint(text, i);
            if (codepoint == 0xA0 || codepoint == '\t') codepoint = ' ';
            if (codepoint == '\n') {
                width = std::max(width, x - (hasGlyph ? gap : 0));
                x = 0; y += size + 7; hasGlyph = false;
                continue;
            }
            if (rasterize && codepoint != ' ' && codepoint != '\t' && color.a != 0) {
                const int asciiIndex = codepoint - 32;
                const int index = custom && asciiIndex >= 0 && asciiIndex < font.glyphCount &&
                                  font.glyphs[asciiIndex].value == codepoint ? asciiIndex : GetGlyphIndex(font, codepoint);
                const auto &glyph = font.glyphs[index];
                const auto &rec = font.recs[index];
                if (inkBounds && rec.width > 0 && rec.height > 0) {
                    const float left = position.x + x + glyph.offsetX * glyphX;
                    const float top = position.y + y + offsetY + glyph.offsetY * glyphY;
                    const float right = left + rec.width * glyphX;
                    const float bottom = top + rec.height * glyphY;
                    if (!hasInk) {
                        *inkBounds = {left, top, right - left, bottom - top};
                        hasInk = true;
                    } else {
                        const float oldRight = inkBounds->x + inkBounds->width;
                        const float oldBottom = inkBounds->y + inkBounds->height;
                        inkBounds->x = std::min(inkBounds->x, left);
                        inkBounds->y = std::min(inkBounds->y, top);
                        inkBounds->width = std::max(oldRight, right) - inkBounds->x;
                        inkBounds->height = std::max(oldBottom, bottom) - inkBounds->y;
                    }
                }
                if (draw) {
                    // Round each glyph origin, not every advance: rounding error does
                    // not accumulate along a line or change the logical text width.
                    const float px = (std::round(matrix.m12 + (position.x + x) * scaleX) - matrix.m12) / scaleX;
                    const float py = (std::round(matrix.m13 + (position.y + y + offsetY) * scaleY) - matrix.m13) / scaleY;
                    const float padding = static_cast<float>(font.glyphPadding);
                    DrawTexturePro(font.texture,
                        {rec.x - padding, rec.y - padding, rec.width + 2*padding, rec.height + 2*padding},
                        {px + (glyph.offsetX - padding)*glyphX, py + (glyph.offsetY - padding)*glyphY,
                         (rec.width + 2*padding)*glyphX, (rec.height + 2*padding)*glyphY}, {0, 0}, 0, color);
                }
            }
            x += advanceFor(codepoint) * size + gap;
            hasGlyph = true;
        }
        return {std::max(width, x - (hasGlyph ? gap : 0)), y + size};
    }

    Vector2 Resources::MeasureText(std::string_view text, float size, float spacing, FontRole role) const {
        return LayoutText(text, {}, size, spacing, WHITE, role, false);
    }

    void Resources::DrawText(std::string_view text, Vector2 position, float size, float spacing, Color color,
                             FontRole role) const {
        LayoutText(text, position, size, spacing, color, role, true);
    }

    void Resources::DrawCenteredText(std::string_view text, Rectangle bounds, float size, float spacing,
                                     Color color, FontRole role) const {
        Rectangle ink{};
        LayoutText(text, {}, size, spacing, color, role, false, &ink);
        if (ink.width <= 0 || ink.height <= 0) return;
        DrawText(text, {bounds.x + (bounds.width - ink.width) / 2 - ink.x,
                        bounds.y + (bounds.height - ink.height) / 2 - ink.y}, size, spacing, color, role);
    }

    bool Resources::DrawArt(ArtId id, Rectangle bounds, ArtFit fit, Color tint) const {
        const auto index = static_cast<std::size_t>(id);
        if (index >= artwork.size()) return false;
        const Texture2D texture = artwork[index];
        if (!IsTextureValid(texture)) return false;
        if (fit == ArtFit::Contain) {
            const float scale = std::min(bounds.width / texture.width, bounds.height / texture.height);
            const Vector2 size{texture.width * scale, texture.height * scale};
            bounds.x += (bounds.width - size.x) / 2;
            bounds.y += bounds.height - size.y; // Bottom aligned: feet stay on the ground.
            bounds.width = size.x;
            bounds.height = size.y;
        }
        DrawTexturePro(texture, {0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)},
                       bounds, {0, 0}, 0, tint);
        return true;
    }

    void Resources::PlayClick(const GameConfig &config) {
        if (config.muted) return;
        audio.Play(uiClickSound); // Master volume is applied globally by GameWindow each frame.
    }
}
