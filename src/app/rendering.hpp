#pragma once

#include <raylib.h>

#include <array>
#include <string>

namespace bfb {

enum class FontRole { Body, Hud, Button, Subtitle, Title, Display, Count };

class RenderMetrics {
public:
    void update(int virtualWidth, int virtualHeight);

    [[nodiscard]] Vector2 virtualToScreen(Vector2 point) const;
    [[nodiscard]] Vector2 screenToVirtual(Vector2 point) const;
    [[nodiscard]] Rectangle worldDestination(float shakeX = 0.0F, float shakeY = 0.0F) const;
    [[nodiscard]] float canvasScale() const { return canvasScale_; }
    [[nodiscard]] float dpiScale() const { return dpiScale_; }
    [[nodiscard]] float rasterScale() const { return canvasScale_ * dpiScale_; }

private:
    int virtualWidth_ = 1600;
    int virtualHeight_ = 900;
    float canvasScale_ = 1.0F;
    float dpiScale_ = 1.0F;
    float offsetX_ = 0.0F;
    float offsetY_ = 0.0F;
};

class FontManager {
public:
    FontManager() = default;
    ~FontManager();

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    void update(float rasterScale);
    void shutdown();

    [[nodiscard]] Vector2 measure(const std::string& value, FontRole role,
                                  float size, float spacing = 1.0F) const;
    void draw(const std::string& value, FontRole role, Vector2 position,
              float size, float spacing, Color color) const;
    [[nodiscard]] bool usingFallback() const { return usingFallback_; }

private:
    struct FontSlot {
        Font font{};
        bool owned = false;
    };

    std::array<FontSlot, static_cast<std::size_t>(FontRole::Count)> fonts_{};
    float rasterBucket_ = 0.0F;
    bool usingFallback_ = false;

    void load(float rasterBucket);
    [[nodiscard]] const Font& get(FontRole role) const;
};

[[nodiscard]] FontRole fontRoleForSize(float size);

}  // namespace bfb
