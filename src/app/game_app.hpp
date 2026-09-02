#pragma once

#include "core/game_core.hpp"
#include "core/profile_store.hpp"
#include "app/rendering.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace bfb {

class GameApp {
public:
    GameApp();
    ~GameApp();

    GameApp(const GameApp&) = delete;
    GameApp& operator=(const GameApp&) = delete;

    int run(const std::string& qaScreenshot = {}, bool smokeTest = false,
            const std::string& qaScene = "battle");

private:
    static constexpr int kVirtualWidth = 1600;
    static constexpr int kVirtualHeight = 900;

    enum class Scene { MainMenu, Battle, Bestiary, Tutorial, Pause };
    enum class PlayerMode { Move, Attack };

    struct Particle {
        Vector2 position{};
        Vector2 velocity{};
        float life = 0.0F;
        float maxLife = 1.0F;
        float radius = 3.0F;
        Color color = WHITE;
    };

    struct TextCommand {
        std::string value;
        Vector2 position{};
        float size = 24.0F;
        float spacing = 1.0F;
        Color color = WHITE;
        FontRole role = FontRole::Body;
    };

    GameSession game_;
    ProfileStore profileStore_;
    PlayerProfile profile_;
    Scene scene_ = Scene::MainMenu;
    Scene tutorialReturnScene_ = Scene::Battle;
    PlayerMode mode_ = PlayerMode::Move;
    GridPos keyboardCursor_{3, 0};
    std::optional<GridPos> hoverCell_;

    FontManager fonts_;
    RenderMetrics renderMetrics_;
    mutable std::vector<TextCommand> textCommands_;
    RenderTexture2D canvas_{};
    bool windowReady_ = false;
    bool shouldExit_ = false;
    bool audioReady_ = false;
    Sound menuSound_{};
    Sound shotSound_{};
    Sound hitSound_{};
    Sound healSound_{};
    Sound victorySound_{};

    double levelElapsed_ = 0.0;
    bool levelRecorded_ = false;
    std::vector<GameEvent> animationQueue_;
    std::size_t animationIndex_ = 0;
    float animationClock_ = 0.0F;
    float screenShake_ = 0.0F;
    std::vector<Particle> particles_;
    std::vector<std::string> combatLog_;
    std::string toast_;
    float toastTime_ = 0.0F;

    void initialize();
    void shutdown();
    void initializeAudio();
    Sound makeTone(float frequency, float duration, float decay) const;
    void playSoundFor(const GameEvent& event);
    void playMenuSound();

    void startNewGame(bool skipTutorial = false);
    void update(float dt);
    void updateBattle(float dt);
    void updateAnimation(float dt);
    void updateParticles(float dt);
    void recordLevelTime();
    void persistProfile();
    void queueEvents(const CommandResult& result);
    void beginCurrentEvent();
    float currentEventDuration() const;
    bool animationBusy() const;

    void drawFrame();
    void drawMainMenu(Vector2 mouse);
    void drawBestiary(Vector2 mouse);
    void drawBattle(Vector2 mouse);
    void drawBoard(const GameSnapshot& snapshot, Vector2 mouse);
    void drawUnit(const Unit& unit, Rectangle cell, bool player);
    void drawHud(const GameSnapshot& snapshot, Vector2 mouse);
    void drawAnimationOverlay();
    void drawParticles() const;
    void drawTutorial(Vector2 mouse);
    void drawPause(Vector2 mouse);
    void drawPhaseModal(const GameSnapshot& snapshot, Vector2 mouse);
    void drawBackground() const;
    void drawToast() const;

    Vector2 virtualMouse() const;
    Rectangle gridRect() const;
    Rectangle cellRect(GridPos pos) const;
    Vector2 cellCenter(GridPos pos) const;
    std::optional<GridPos> cellFromPoint(Vector2 point) const;
    Color unitColor(UnitKind kind) const;
    std::string unitGlyph(UnitKind kind) const;
    std::string aiLabel(AiMode mode) const;
    const Unit* hoveredEnemy(const GameSnapshot& snapshot) const;
    void issueBoardAction(GridPos target);
    void restartLevel();

    bool button(Rectangle bounds, const std::string& label, Vector2 mouse,
                Color accent, float fontSize = 27.0F);
    void text(const std::string& value, Vector2 position, float size, Color color,
              float spacing = 1.0F, std::optional<FontRole> role = std::nullopt) const;
    void centeredText(const std::string& value, Rectangle bounds, float size, Color color) const;
    void panel(Rectangle bounds, Color border, Color fill) const;
};

}  // namespace bfb
