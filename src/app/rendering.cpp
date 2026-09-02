#include "app/rendering.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace bfb {
namespace {

constexpr std::array<int, static_cast<std::size_t>(FontRole::Count)> kRoleSizes{24, 32, 32, 44, 72, 96};

constexpr const char* kGlyphText =
    "战线突围回合制术肉鸽玩家敌方关卡生命攻击射程移动暴主菜单开始新游戏人图鉴退出"
    "继续暂停设置音量静教目标操作选择确认取消重返近兵刺客醉汉狙手坦克逃斗天使狂士"
    "炮台追随机守卫支援初次交锋影袭酒馆乱叉火力钢铁阵背水一不要搏撕心裂肺穿越未竟"
    "之强壮体魄直觉利刃迅捷步伐稳固瞄准巨视野专注疾致长勇气崩地横冲撞精弹幕死身获"
    "得赋通失败胜鼠点格子数字键向按历史最佳当前用时伤害治疗备就绪意链路已建立第首完"
    "成记录间纪枚棋条读懂的然后行预览可化查看说明识别威胁测为控场结算性红表示即将橙"
    "悬信息规划下日志三掌握本能够其计位每只或切换也右侧钮指定黄色光快进入期声启项永"
    "久效破功终会谎全部十个经你被载巡逻离知大率逼基础发脆弱但极擅贴走原否则寻找高低"
    "是推中坚屏障濒远健康参与优先接并受友军半血提升无法较范令现在超占据上没有尚了员"
    "到亡请该必须坐界存复实叠配错误档式异常默创写替旧"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-/%:.!?()[] ·—›，。：“”！？、；（）《》";

std::string firstExistingPath(const char* bundledName, const char* systemFallback) {
    const std::filesystem::path relative = std::filesystem::path("assets/fonts") / bundledName;
    if (FileExists(relative.string().c_str())) return relative.string();
    const std::filesystem::path besideExecutable =
        std::filesystem::path(GetApplicationDirectory()) / "assets/fonts" / bundledName;
    if (FileExists(besideExecutable.string().c_str())) return besideExecutable.string();
    if (FileExists(systemFallback)) return systemFallback;
    return {};
}

float rasterBucket(float value) {
    return std::clamp(std::round(value * 4.0F) / 4.0F, 0.75F, 2.5F);
}

}  // namespace

void RenderMetrics::update(int virtualWidth, int virtualHeight) {
    virtualWidth_ = virtualWidth;
    virtualHeight_ = virtualHeight;
    const int screenWidth = std::max(GetScreenWidth(), 1);
    const int screenHeight = std::max(GetScreenHeight(), 1);
    canvasScale_ = std::min(static_cast<float>(screenWidth) / virtualWidth_,
                            static_cast<float>(screenHeight) / virtualHeight_);
    const float dpiX = static_cast<float>(std::max(GetRenderWidth(), 1)) / screenWidth;
    const float dpiY = static_cast<float>(std::max(GetRenderHeight(), 1)) / screenHeight;
    dpiScale_ = std::max(1.0F, std::min(dpiX, dpiY));
    offsetX_ = (screenWidth - virtualWidth_ * canvasScale_) * 0.5F;
    offsetY_ = (screenHeight - virtualHeight_ * canvasScale_) * 0.5F;
}

Vector2 RenderMetrics::virtualToScreen(Vector2 point) const {
    return {std::round(offsetX_ + point.x * canvasScale_),
            std::round(offsetY_ + point.y * canvasScale_)};
}

Vector2 RenderMetrics::screenToVirtual(Vector2 point) const {
    return {(point.x - offsetX_) / canvasScale_, (point.y - offsetY_) / canvasScale_};
}

Rectangle RenderMetrics::worldDestination(float shakeX, float shakeY) const {
    return {offsetX_ + shakeX, offsetY_ + shakeY,
            virtualWidth_ * canvasScale_, virtualHeight_ * canvasScale_};
}

FontManager::~FontManager() { shutdown(); }

void FontManager::update(float requestedRasterScale) {
    const float requestedBucket = rasterBucket(requestedRasterScale);
    if (std::abs(requestedBucket - rasterBucket_) < 0.01F) return;
    shutdown();
    load(requestedBucket);
}

void FontManager::shutdown() {
    for (auto& slot : fonts_) {
        if (slot.owned) UnloadFont(slot.font);
        slot = {};
    }
    rasterBucket_ = 0.0F;
}

void FontManager::load(float requestedBucket) {
    rasterBucket_ = requestedBucket;
    usingFallback_ = false;
    const std::string medium = firstExistingPath("NotoSansSC-Medium.ttf", "C:/Windows/Fonts/msyh.ttc");
    const std::string semibold = firstExistingPath("NotoSansSC-SemiBold.ttf", "C:/Windows/Fonts/msyhbd.ttc");
    int codepointCount = 0;
    int* codepoints = LoadCodepoints(kGlyphText, &codepointCount);

    for (std::size_t index = 0; index < fonts_.size(); ++index) {
        if (index == static_cast<std::size_t>(FontRole::Button)) {
            fonts_[index].font = fonts_[static_cast<std::size_t>(FontRole::Hud)].font;
            continue;
        }
        const bool emphasized = index >= static_cast<std::size_t>(FontRole::Subtitle);
        const std::string& path = emphasized && !semibold.empty() ? semibold : medium;
        const int pixelSize = std::max(12, static_cast<int>(std::round(kRoleSizes[index] * requestedBucket)));
        if (!path.empty()) {
            fonts_[index].font = LoadFontEx(path.c_str(), pixelSize, codepoints, codepointCount);
            fonts_[index].owned = fonts_[index].font.texture.id != 0;
        }
        if (!fonts_[index].owned) {
            fonts_[index].font = GetFontDefault();
            usingFallback_ = true;
        } else {
            SetTextureFilter(fonts_[index].font.texture, TEXTURE_FILTER_BILINEAR);
        }
    }
    UnloadCodepoints(codepoints);
}

const Font& FontManager::get(FontRole role) const {
    return fonts_[static_cast<std::size_t>(role)].font;
}

Vector2 FontManager::measure(const std::string& value, FontRole role, float size, float spacing) const {
    return MeasureTextEx(get(role), value.c_str(), size, spacing);
}

void FontManager::draw(const std::string& value, FontRole role, Vector2 position,
                       float size, float spacing, Color color) const {
    DrawTextEx(get(role), value.c_str(), position, size, spacing, color);
}

FontRole fontRoleForSize(float size) {
    if (size <= 25.0F) return FontRole::Body;
    if (size <= 34.0F) return FontRole::Hud;
    if (size <= 52.0F) return FontRole::Subtitle;
    if (size <= 82.0F) return FontRole::Title;
    return FontRole::Display;
}

}  // namespace bfb
