#include "app/game_app.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace bfb {
namespace {

constexpr Color kBackground{7, 12, 27, 255};
constexpr Color kPanel{12, 23, 43, 238};
constexpr Color kPanelLight{18, 35, 58, 240};
constexpr Color kCyan{38, 226, 255, 255};
constexpr Color kBlue{70, 120, 255, 255};
constexpr Color kOrange{255, 111, 50, 255};
constexpr Color kRed{255, 61, 93, 255};
constexpr Color kGreen{68, 235, 153, 255};
constexpr Color kYellow{255, 215, 92, 255};
constexpr Color kText{238, 248, 255, 255};
constexpr Color kMuted{158, 187, 213, 255};

bool includes(const std::vector<GridPos>& values, GridPos value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

std::string formatTime(double seconds) {
    const int minutes = static_cast<int>(seconds) / 60;
    const double rest = seconds - minutes * 60;
    std::ostringstream stream;
    if (minutes > 0) stream << minutes << ':' << std::setw(5) << std::setfill('0');
    stream << std::fixed << std::setprecision(2) << rest << " s";
    return stream.str();
}

float clamp01(float value) { return std::clamp(value, 0.0F, 1.0F); }

}  // namespace

GameApp::GameApp() = default;
GameApp::~GameApp() { shutdown(); }

int GameApp::run(const std::string& qaScreenshot, bool smokeTest, const std::string& qaScene) {
    initialize();
    std::string screenshotPath = qaScreenshot;
    std::string sceneName = qaScene;
    const std::filesystem::path requestPath = std::filesystem::path(GetApplicationDirectory()) / "qa-request.txt";
    if (screenshotPath.empty() && std::filesystem::exists(requestPath)) {
        std::ifstream request(requestPath);
        std::getline(request, sceneName);
        std::getline(request, screenshotPath);
        request.close();
        if (!sceneName.empty() && sceneName.back() == '\r') sceneName.pop_back();
        if (!screenshotPath.empty() && screenshotPath.back() == '\r') screenshotPath.pop_back();
        std::error_code error;
        std::filesystem::remove(requestPath, error);
    }
    if (!screenshotPath.empty()) {
        if (sceneName == "bestiary") scene_ = Scene::Bestiary;
        else if (sceneName == "menu") scene_ = Scene::MainMenu;
        else {
            startNewGame(true);
            if (sceneName == "tutorial") {
                tutorialReturnScene_ = Scene::Battle;
                scene_ = Scene::Tutorial;
            }
        }
    } else if (smokeTest) {
        startNewGame(true);
    }
    int frameCount = 0;
    while (!shouldExit_ && !WindowShouldClose()) {
        const float dt = std::min(GetFrameTime(), 0.05F);
        update(dt);
        drawFrame();
        ++frameCount;
        if (!screenshotPath.empty() && frameCount == 2) {
            const auto parent = std::filesystem::path(screenshotPath).parent_path();
            if (!parent.empty()) std::filesystem::create_directories(parent);
            TakeScreenshot(screenshotPath.c_str());
            break;
        }
        if (smokeTest && frameCount >= 8) break;
    }
    shutdown();
    return 0;
}

void GameApp::initialize() {
    if (windowReady_) return;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(kVirtualWidth, kVirtualHeight, "战线突围 · Battlefront Breakout");
    SetWindowMinSize(1120, 630);
    SetTargetFPS(60);
    canvas_ = LoadRenderTexture(kVirtualWidth, kVirtualHeight);
    SetTextureFilter(canvas_.texture, TEXTURE_FILTER_BILINEAR);
    windowReady_ = true;

    std::string warning;
    profile_ = profileStore_.load(&warning);
    if (!warning.empty()) {
        toast_ = warning;
        toastTime_ = 4.0F;
    }
    renderMetrics_.update(kVirtualWidth, kVirtualHeight);
    fonts_.update(renderMetrics_.rasterScale());
    if (fonts_.usingFallback()) {
        toast_ = "字体加载失败，已使用系统降级字体";
        toastTime_ = 4.0F;
    }
    initializeAudio();
}

void GameApp::shutdown() {
    if (!windowReady_) return;
    persistProfile();
    if (audioReady_) {
        UnloadSound(menuSound_);
        UnloadSound(shotSound_);
        UnloadSound(hitSound_);
        UnloadSound(healSound_);
        UnloadSound(victorySound_);
        CloseAudioDevice();
        audioReady_ = false;
    }
    fonts_.shutdown();
    UnloadRenderTexture(canvas_);
    CloseWindow();
    windowReady_ = false;
}

void GameApp::initializeAudio() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    menuSound_ = makeTone(520.0F, 0.07F, 0.9F);
    shotSound_ = makeTone(170.0F, 0.16F, 2.8F);
    hitSound_ = makeTone(92.0F, 0.12F, 3.2F);
    healSound_ = makeTone(680.0F, 0.22F, 1.8F);
    victorySound_ = makeTone(880.0F, 0.36F, 1.2F);
    audioReady_ = true;
}

Sound GameApp::makeTone(float frequency, float duration, float decay) const {
    constexpr unsigned int sampleRate = 22050;
    const unsigned int frames = static_cast<unsigned int>(sampleRate * duration);
    std::vector<short> samples(frames);
    for (unsigned int index = 0; index < frames; ++index) {
        const float t = static_cast<float>(index) / sampleRate;
        const float envelope = std::exp(-decay * t / std::max(duration, 0.01F));
        const float fundamental = std::sin(2.0F * PI * frequency * t);
        const float overtone = 0.32F * std::sin(2.0F * PI * frequency * 2.0F * t);
        samples[index] = static_cast<short>(12000.0F * envelope * (fundamental + overtone));
    }
    Wave wave{};
    wave.frameCount = frames;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples.data();
    return LoadSoundFromWave(wave);
}

void GameApp::playSoundFor(const GameEvent& item) {
    if (!audioReady_ || profile_.muted) return;
    Sound* sound = nullptr;
    switch (item.type) {
        case EventType::Shot: sound = &shotSound_; break;
        case EventType::Damaged:
        case EventType::Defeated:
        case EventType::PlayerDefeated: sound = &hitSound_; break;
        case EventType::Healed:
        case EventType::TalentApplied: sound = &healSound_; break;
        case EventType::LevelCompleted:
        case EventType::RunCompleted: sound = &victorySound_; break;
        default: break;
    }
    if (sound) {
        SetSoundVolume(*sound, profile_.volume);
        PlaySound(*sound);
    }
}

void GameApp::playMenuSound() {
    if (!audioReady_ || profile_.muted) return;
    SetSoundVolume(menuSound_, profile_.volume * 0.8F);
    PlaySound(menuSound_);
}

void GameApp::startNewGame(bool skipTutorial) {
    game_.startNewRun();
    scene_ = Scene::Battle;
    mode_ = PlayerMode::Move;
    keyboardCursor_ = game_.snapshot().player.pos;
    levelElapsed_ = 0.0;
    levelRecorded_ = false;
    animationQueue_.clear();
    combatLog_ = {"战术链路已建立", "第 1 关准备就绪"};
    if (!profile_.tutorialSeen && !skipTutorial) {
        tutorialReturnScene_ = Scene::Battle;
        scene_ = Scene::Tutorial;
    }
}

void GameApp::update(float dt) {
    toastTime_ = std::max(0.0F, toastTime_ - dt);
    screenShake_ = std::max(0.0F, screenShake_ - dt * 3.6F);
    updateParticles(dt);

    if (IsKeyPressed(KEY_F1) && (scene_ == Scene::Battle || scene_ == Scene::Pause)) {
        tutorialReturnScene_ = scene_;
        scene_ = Scene::Tutorial;
        playMenuSound();
        return;
    }
    if (scene_ == Scene::Tutorial) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_F1)) {
            profile_.tutorialSeen = true;
            scene_ = tutorialReturnScene_;
            persistProfile();
            playMenuSound();
        }
        return;
    }
    if (scene_ == Scene::Pause) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            scene_ = Scene::Battle;
            playMenuSound();
        }
        return;
    }
    if (scene_ == Scene::Battle) updateBattle(dt);
}

void GameApp::updateBattle(float dt) {
    auto snapshot = game_.snapshot();
    hoverCell_ = cellFromPoint(virtualMouse());
    if (snapshot.phase == TurnPhase::Player || snapshot.phase == TurnPhase::Enemy) {
        levelElapsed_ += dt;
        game_.setElapsedSeconds(levelElapsed_);
    }
    updateAnimation(dt);
    if (animationBusy()) return;

    snapshot = game_.snapshot();
    if (snapshot.phase == TurnPhase::Enemy) {
        queueEvents(game_.advanceEnemyTurn());
        return;
    }
    if (snapshot.phase == TurnPhase::LevelComplete) {
        recordLevelTime();
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) queueEvents(game_.apply({ActionType::Continue, {}, {}}));
        return;
    }
    if (snapshot.phase == TurnPhase::TalentChoice) {
        for (int index = 0; index < static_cast<int>(snapshot.talentChoices.size()); ++index) {
            if (IsKeyPressed(KEY_ONE + index)) {
                queueEvents(game_.apply({ActionType::ChooseTalent, {}, snapshot.talentChoices[index].id}));
                levelElapsed_ = 0.0;
                levelRecorded_ = false;
                keyboardCursor_ = game_.snapshot().player.pos;
                return;
            }
        }
        return;
    }
    if (snapshot.phase == TurnPhase::Defeat) {
        if (IsKeyPressed(KEY_R)) restartLevel();
        if (IsKeyPressed(KEY_ESCAPE)) scene_ = Scene::MainMenu;
        return;
    }
    if (snapshot.phase == TurnPhase::Victory) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) scene_ = Scene::MainMenu;
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        scene_ = Scene::Pause;
        playMenuSound();
        return;
    }
    if (IsKeyPressed(KEY_R)) {
        restartLevel();
        return;
    }
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_M)) mode_ = PlayerMode::Move;
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_F)) mode_ = PlayerMode::Attack;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) keyboardCursor_.x--;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) keyboardCursor_.x++;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) keyboardCursor_.y--;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) keyboardCursor_.y++;
    keyboardCursor_.x = std::clamp(keyboardCursor_.x, 0, kGridSize - 1);
    keyboardCursor_.y = std::clamp(keyboardCursor_.y, 0, kGridSize - 1);
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) issueBoardAction(keyboardCursor_);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoverCell_) issueBoardAction(*hoverCell_);
}

void GameApp::queueEvents(const CommandResult& result) {
    toast_ = result.message;
    toastTime_ = result.ok ? 1.8F : 2.6F;
    if (!result.ok) {
        playMenuSound();
        return;
    }
    if (!result.message.empty()) {
        combatLog_.push_back(result.message);
        if (combatLog_.size() > 7) combatLog_.erase(combatLog_.begin());
    }
    animationQueue_ = result.events;
    animationIndex_ = 0;
    animationClock_ = 0.0F;
    if (!animationQueue_.empty()) beginCurrentEvent();
}

void GameApp::beginCurrentEvent() {
    if (!animationBusy()) return;
    const auto& item = animationQueue_[animationIndex_];
    playSoundFor(item);
    if (item.type == EventType::Damaged || item.type == EventType::Critical ||
        item.type == EventType::Defeated || item.type == EventType::Healed) {
        const Color color = item.type == EventType::Healed ? kGreen : (item.type == EventType::Critical ? kYellow : kRed);
        const int count = item.type == EventType::Defeated ? 22 : 10;
        for (int i = 0; i < count; ++i) {
            const float angle = (2.0F * PI * i) / count;
            const float speed = 45.0F + static_cast<float>((i * 19) % 70);
            particles_.push_back({cellCenter(item.to), {std::cos(angle) * speed, std::sin(angle) * speed},
                                  0.65F, 0.65F, 2.5F + (i % 4), color});
        }
    }
    if (item.type == EventType::Damaged || item.type == EventType::Defeated || item.type == EventType::PlayerDefeated) {
        screenShake_ = 1.0F;
    }
    if (!item.text.empty() && item.type != EventType::TurnChanged) {
        combatLog_.push_back(item.text);
        if (combatLog_.size() > 7) combatLog_.erase(combatLog_.begin());
    }
}

float GameApp::currentEventDuration() const {
    if (!animationBusy()) return 0.0F;
    switch (animationQueue_[animationIndex_].type) {
        case EventType::Shot: return 0.24F;
        case EventType::Moved: return 0.18F;
        case EventType::Damaged:
        case EventType::Critical:
        case EventType::Healed: return 0.28F;
        case EventType::Defeated:
        case EventType::PlayerDefeated: return 0.42F;
        case EventType::LevelCompleted:
        case EventType::RunCompleted: return 0.55F;
        default: return 0.12F;
    }
}

bool GameApp::animationBusy() const {
    return animationIndex_ < animationQueue_.size();
}

void GameApp::updateAnimation(float dt) {
    if (!animationBusy()) return;
    animationClock_ += dt;
    if (animationClock_ < currentEventDuration()) return;
    animationClock_ = 0.0F;
    ++animationIndex_;
    if (animationBusy()) beginCurrentEvent();
    else animationQueue_.clear();
}

void GameApp::updateParticles(float dt) {
    for (auto& particle : particles_) {
        particle.life -= dt;
        particle.position.x += particle.velocity.x * dt;
        particle.position.y += particle.velocity.y * dt;
        particle.velocity.x *= 0.96F;
        particle.velocity.y *= 0.96F;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                    [](const Particle& particle) { return particle.life <= 0.0F; }),
                     particles_.end());
}

void GameApp::recordLevelTime() {
    if (levelRecorded_) return;
    levelRecorded_ = true;
    const int level = game_.snapshot().levelNumber;
    const auto found = profile_.bestTimes.find(level);
    if (found == profile_.bestTimes.end() || levelElapsed_ < found->second) {
        profile_.bestTimes[level] = levelElapsed_;
        toast_ = found == profile_.bestTimes.end() ? "首次完成，已记录最佳时间" : "新纪录！";
        toastTime_ = 3.0F;
    }
    persistProfile();
}

void GameApp::persistProfile() {
    std::string warning;
    if (!profileStore_.save(profile_, &warning) && !warning.empty()) {
        toast_ = warning;
        toastTime_ = 3.5F;
    }
}

void GameApp::issueBoardAction(GridPos target) {
    const ActionType type = mode_ == PlayerMode::Move ? ActionType::Move : ActionType::Attack;
    queueEvents(game_.apply({type, target, {}}));
}

void GameApp::restartLevel() {
    queueEvents(game_.apply({ActionType::Restart, {}, {}}));
    levelElapsed_ = 0.0;
    levelRecorded_ = false;
    keyboardCursor_ = game_.snapshot().player.pos;
}

void GameApp::drawFrame() {
    renderMetrics_.update(kVirtualWidth, kVirtualHeight);
    fonts_.update(renderMetrics_.rasterScale());
    textCommands_.clear();
    const Vector2 mouse = virtualMouse();
    BeginTextureMode(canvas_);
    ClearBackground(kBackground);
    drawBackground();
    switch (scene_) {
        case Scene::MainMenu: drawMainMenu(mouse); break;
        case Scene::Bestiary: drawBestiary(mouse); break;
        case Scene::Battle: drawBattle(mouse); break;
        case Scene::Tutorial:
            drawBattle(mouse);
            drawTutorial(mouse);
            break;
        case Scene::Pause:
            drawBattle(mouse);
            drawPause(mouse);
            break;
    }
    drawToast();
    EndTextureMode();

    float shakeX = 0.0F;
    float shakeY = 0.0F;
    if (screenShake_ > 0.0F) {
        shakeX = std::sin(static_cast<float>(GetTime()) * 91.0F) * 5.0F * screenShake_;
        shakeY = std::cos(static_cast<float>(GetTime()) * 77.0F) * 3.0F * screenShake_;
    }
    BeginDrawing();
    ClearBackground(BLACK);
    const Rectangle destination = renderMetrics_.worldDestination(shakeX, shakeY);
    DrawTexturePro(canvas_.texture, {0, 0, static_cast<float>(kVirtualWidth), -static_cast<float>(kVirtualHeight)},
                   destination, {0, 0}, 0.0F, WHITE);
    for (const auto& command : textCommands_) {
        fonts_.draw(command.value, command.role, renderMetrics_.virtualToScreen(command.position),
                    command.size * renderMetrics_.canvasScale(),
                    command.spacing * renderMetrics_.canvasScale(), command.color);
    }
    EndDrawing();
}

void GameApp::drawBackground() const {
    DrawRectangleGradientV(0, 0, kVirtualWidth, kVirtualHeight, Color{8, 15, 34, 255}, Color{3, 7, 18, 255});
    for (int i = 0; i < 90; ++i) {
        const int x = (i * 193 + 47) % kVirtualWidth;
        const int y = (i * 97 + 31) % kVirtualHeight;
        const float pulse = 0.45F + 0.35F * std::sin(static_cast<float>(GetTime()) * 0.8F + i);
        DrawCircle(x, y, 1.0F + (i % 3) * 0.35F, Fade(kCyan, pulse * 0.24F));
    }
    DrawCircleGradient({200, 170}, 360, Fade(kBlue, 0.10F), BLANK);
    DrawCircleGradient({1420, 740}, 440, Fade(kOrange, 0.07F), BLANK);
}

void GameApp::drawMainMenu(Vector2 mouse) {
    text("BATTLEFRONT // BREAKOUT", {112, 112}, 25, kCyan, 4.0F);
    text("战线突围", {105, 150}, 96, kText, 2.0F);
    text("回合制 · 战术 · ROGUELITE", {112, 272}, 28, kMuted, 2.0F);
    DrawRectangle(112, 328, 390, 3, kCyan);
    text("一枚棋子，一条战线。读懂敌人的意图，然后开火。", {112, 360}, 25, kText);

    panel({105, 455, 525, 290}, kBlue, Fade(kPanel, 0.94F));
    if (button({145, 500, 445, 64}, "开始新行动", mouse, kCyan, 30)) startNewGame(false);
    if (button({145, 582, 445, 64}, "敌人图鉴", mouse, kOrange, 30)) scene_ = Scene::Bestiary;
    if (button({145, 664, 445, 48}, "退出游戏", mouse, kMuted, 24)) shouldExit_ = true;

    panel({760, 120, 720, 625}, kCyan, Fade(kPanel, 0.86F));
    text("战术预览", {812, 165}, 30, kCyan);
    const Rectangle preview{815, 225, 610, 430};
    DrawRectangleRounded(preview, 0.04F, 8, Color{7, 18, 34, 255});
    for (int row = 0; row < 6; ++row) {
        for (int column = 0; column < 8; ++column) {
            const Rectangle cell{preview.x + column * 74.0F + 10, preview.y + row * 64.0F + 12, 62, 52};
            DrawRectangleRounded(cell, 0.08F, 4, Fade((row + column) % 2 ? kBlue : kCyan, 0.08F));
            DrawRectangleRoundedLinesEx(cell, 0.08F, 4, 1.0F, Fade(kCyan, 0.16F));
        }
    }
    DrawCircle(895, 320, 24, kCyan);
    DrawCircleLines(895, 320, 34, Fade(kCyan, 0.5F));
    DrawCircle(1280, 535, 24, kOrange);
    DrawLineEx({915, 330}, {1260, 525}, 5, Fade(kRed, 0.52F));
    text("敌方意图可视化", {1010, 675}, 23, kMuted);
    text("F1  随时查看操作说明", {112, 790}, 21, kMuted);
}

void GameApp::drawBestiary(Vector2 mouse) {
    text("敌人图鉴", {70, 50}, 54, kText);
    text("识别威胁 · 预测行为 · 控制战场", {72, 116}, 24, kMuted);
    const auto& entries = GameSession::bestiary();
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const int column = static_cast<int>(index % 3);
        const int row = static_cast<int>(index / 3);
        const Rectangle bounds{70.0F + column * 500.0F, 180.0F + row * 200.0F, 460, 168};
        const Color color = unitColor(entries[index].kind);
        panel(bounds, color, Fade(kPanel, 0.92F));
        DrawCircle(static_cast<int>(bounds.x + 62), static_cast<int>(bounds.y + 58), 31, Fade(color, 0.24F));
        DrawCircleLines(static_cast<int>(bounds.x + 62), static_cast<int>(bounds.y + 58), 31, color);
        centeredText(entries[index].callSign, {bounds.x + 31, bounds.y + 27, 62, 62}, 28, color);
        text(entries[index].name, {bounds.x + 114, bounds.y + 24}, 29, kText);
        text(aiLabel(entries[index].ai), {bounds.x + 114, bounds.y + 64}, 20, color);
        text(entries[index].description, {bounds.x + 28, bounds.y + 112}, 18, kMuted);
    }
    if (button({70, 806, 220, 50}, "返回主菜单", mouse, kCyan, 23) || IsKeyPressed(KEY_ESCAPE)) {
        scene_ = Scene::MainMenu;
    }
}

void GameApp::drawBattle(Vector2 mouse) {
    const auto snapshot = game_.snapshot();
    text("战线突围", {62, 34}, 38, kText);
    text("TACTICAL LINK ONLINE", {260, 48}, 17, kCyan, 2.5F);
    text("第 " + std::to_string(snapshot.levelNumber) + " / " + std::to_string(snapshot.levelCount) + " 关",
         {1165, 35}, 24, kMuted);
    text(snapshot.levelName, {1320, 32}, 30, kText);
    drawBoard(snapshot, mouse);
    drawHud(snapshot, mouse);
    drawParticles();
    drawAnimationOverlay();
    drawPhaseModal(snapshot, mouse);
}

Rectangle GameApp::gridRect() const { return {66, 130, 640, 640}; }

Rectangle GameApp::cellRect(GridPos pos) const {
    const auto grid = gridRect();
    return {grid.x + pos.x * 80.0F, grid.y + pos.y * 80.0F, 80, 80};
}

Vector2 GameApp::cellCenter(GridPos pos) const {
    const auto cell = cellRect(pos);
    return {cell.x + cell.width * 0.5F, cell.y + cell.height * 0.5F};
}

std::optional<GridPos> GameApp::cellFromPoint(Vector2 point) const {
    const auto grid = gridRect();
    if (!CheckCollisionPointRec(point, grid)) return std::nullopt;
    return GridPos{static_cast<int>((point.x - grid.x) / 80.0F), static_cast<int>((point.y - grid.y) / 80.0F)};
}

void GameApp::drawBoard(const GameSnapshot& snapshot, Vector2 mouse) {
    const auto grid = gridRect();
    DrawRectangleRounded({grid.x - 18, grid.y - 18, grid.width + 36, grid.height + 36}, 0.035F, 6, Fade(kPanel, 0.96F));
    DrawRectangleRoundedLinesEx({grid.x - 18, grid.y - 18, grid.width + 36, grid.height + 36},
                                0.035F, 6, 2, Fade(kCyan, 0.34F));

    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            const GridPos pos{x, y};
            auto cell = cellRect(pos);
            const bool alternate = (x + y) % 2 == 0;
            DrawRectangleRec(cell, alternate ? Color{11, 27, 45, 255} : Color{8, 22, 39, 255});
            if (snapshot.phase == TurnPhase::Player && !animationBusy()) {
                if (mode_ == PlayerMode::Move && includes(snapshot.moveTargets, pos)) DrawRectangleRec(cell, Fade(kCyan, 0.13F));
                if (mode_ == PlayerMode::Attack && includes(snapshot.attackTargets, pos)) DrawRectangleRec(cell, Fade(kRed, 0.12F));
            }
            DrawRectangleLinesEx(cell, 1, Fade(kCyan, 0.12F));
        }
    }

    for (const auto& intent : snapshot.intents) {
        const Color color = intent.willHeal ? kGreen : (intent.willAttack ? kRed : kOrange);
        DrawLineEx(cellCenter(intent.from), cellCenter(intent.target), intent.willAttack ? 3.0F : 1.5F, Fade(color, 0.34F));
        DrawCircleV(cellCenter(intent.target), intent.willAttack ? 7.0F : 4.0F, Fade(color, 0.5F));
    }

    for (const auto& enemy : snapshot.enemies) drawUnit(enemy, cellRect(enemy.pos), false);
    drawUnit(snapshot.player, cellRect(snapshot.player.pos), true);

    if (hoverCell_) {
        const auto cell = cellRect(*hoverCell_);
        DrawRectangleLinesEx({cell.x + 3, cell.y + 3, cell.width - 6, cell.height - 6}, 3,
                             mode_ == PlayerMode::Move ? kCyan : kRed);
    }
    if (snapshot.phase == TurnPhase::Player) {
        const auto cell = cellRect(keyboardCursor_);
        DrawRectangleLinesEx({cell.x + 8, cell.y + 8, cell.width - 16, cell.height - 16}, 2, kYellow);
    }
    for (int index = 0; index < kGridSize; ++index) {
        text(std::to_string(index), {grid.x + index * 80.0F + 34, grid.y - 28}, 16, kMuted);
        text(std::to_string(index), {grid.x - 28, grid.y + index * 80.0F + 28}, 16, kMuted);
    }
    (void)mouse;
}

void GameApp::drawUnit(const Unit& unit, Rectangle cell, bool player) {
    const Color color = unitColor(unit.kind);
    const Vector2 center{cell.x + cell.width * 0.5F, cell.y + cell.height * 0.5F};
    const float pulse = 1.0F + 0.05F * std::sin(static_cast<float>(GetTime()) * 3.0F + unit.id);
    DrawCircleV(center, 31.0F * pulse, Fade(color, 0.11F));
    DrawCircleLinesV(center, 29.0F, Fade(color, 0.62F));
    DrawCircleV(center, player ? 22.0F : 20.0F, Fade(color, 0.78F));
    centeredText(unitGlyph(unit.kind), {center.x - 24, center.y - 24, 48, 48}, 25, player ? kBackground : kText);

    const Rectangle bar{cell.x + 12, cell.y + 67, 56, 5};
    DrawRectangleRec(bar, Color{3, 8, 17, 220});
    const float ratio = unit.stats.maxHp > 0 ? clamp01(static_cast<float>(unit.stats.hp) / unit.stats.maxHp) : 0.0F;
    DrawRectangleRec({bar.x, bar.y, bar.width * ratio, bar.height}, ratio < 0.34F ? kRed : (player ? kCyan : kOrange));
}

void GameApp::drawHud(const GameSnapshot& snapshot, Vector2 mouse) {
    const Rectangle hud{756, 100, 788, 700};
    panel(hud, kBlue, Fade(kPanel, 0.94F));
    text(snapshot.phase == TurnPhase::Player ? "玩家回合" : (snapshot.phase == TurnPhase::Enemy ? "敌方回合" : "战术结算"),
         {795, 130}, 30, snapshot.phase == TurnPhase::Player ? kCyan : kOrange);
    text(formatTime(levelElapsed_), {1328, 135}, 24, kText);

    text("生命", {795, 192}, 20, kMuted);
    const Rectangle hpBar{865, 198, 310, 16};
    DrawRectangleRounded(hpBar, 0.4F, 8, Color{4, 10, 20, 255});
    const float hpRatio = clamp01(static_cast<float>(snapshot.player.stats.hp) / snapshot.player.stats.maxHp);
    DrawRectangleRounded({hpBar.x, hpBar.y, hpBar.width * hpRatio, hpBar.height}, 0.4F, 8,
                         hpRatio < 0.34F ? kRed : kCyan);
    text(std::to_string(snapshot.player.stats.hp) + " / " + std::to_string(snapshot.player.stats.maxHp),
         {1190, 188}, 22, kText);

    const std::vector<std::pair<std::string, int>> statsView = {
        {"攻击", snapshot.player.stats.attack}, {"射程", snapshot.player.stats.range},
        {"移动", snapshot.player.stats.move}, {"暴击", snapshot.player.stats.critChance}};
    for (std::size_t index = 0; index < statsView.size(); ++index) {
        const Rectangle stat{795.0F + index * 170.0F, 242, 150, 62};
        DrawRectangleRounded(stat, 0.12F, 6, kPanelLight);
        text(statsView[index].first, {stat.x + 14, stat.y + 9}, 17, kMuted);
        text(std::to_string(statsView[index].second) + (index == 3 ? "%" : ""),
             {stat.x + 90, stat.y + 18}, 24, kText);
    }

    text("选择操作", {795, 340}, 22, kMuted);
    const bool interactive = scene_ == Scene::Battle && snapshot.phase == TurnPhase::Player && !animationBusy();
    const Rectangle moveButton{795, 378, 260, 62};
    const Rectangle attackButton{1075, 378, 260, 62};
    if (button(moveButton, "[1] 移动", mouse, mode_ == PlayerMode::Move ? kCyan : kMuted, 26) && interactive) mode_ = PlayerMode::Move;
    if (button(attackButton, "[2] 线性攻击", mouse, mode_ == PlayerMode::Attack ? kRed : kMuted, 26) && interactive) mode_ = PlayerMode::Attack;
    if (!interactive) DrawRectangleRec({moveButton.x, moveButton.y, attackButton.x + attackButton.width - moveButton.x,
                                        moveButton.height}, Fade(kBackground, 0.32F));

    panel({795, 475, 710, 120}, kOrange, Fade(kPanelLight, 0.86F));
    if (const auto* enemy = hoveredEnemy(snapshot)) {
        text(enemy->name + "  //  " + aiLabel(enemy->ai), {820, 493}, 23, unitColor(enemy->kind));
        text("生命 " + std::to_string(enemy->stats.hp) + "/" + std::to_string(enemy->stats.maxHp) +
             "   攻击 " + std::to_string(enemy->stats.attack) + "   射程 " + std::to_string(enemy->stats.range) +
             "   移动 " + std::to_string(enemy->stats.move), {820, 538}, 20, kText);
        text("红线表示即将攻击，橙线表示移动意图", {820, 568}, 17, kMuted);
    } else {
        text("悬停敌人查看战术信息", {820, 508}, 23, kMuted);
        text("利用意图线规划下一回合", {820, 548}, 19, kText);
    }

    text("战斗日志", {795, 625}, 20, kMuted);
    const int shown = std::min<int>(4, static_cast<int>(combatLog_.size()));
    for (int index = 0; index < shown; ++index) {
        const auto& line = combatLog_[combatLog_.size() - shown + index];
        text("› " + line, {812, 657.0F + index * 28.0F}, 17, index == shown - 1 ? kText : kMuted);
    }
    text("F1 教程   R 重开   ESC 暂停", {790, 820}, 19, kMuted);
}

void GameApp::drawAnimationOverlay() {
    if (!animationBusy()) return;
    const auto& item = animationQueue_[animationIndex_];
    const float progress = clamp01(animationClock_ / std::max(currentEventDuration(), 0.01F));
    if (item.type == EventType::Shot) {
        const Vector2 from = cellCenter(item.from);
        const Vector2 to = cellCenter(item.to);
        const Vector2 current{from.x + (to.x - from.x) * progress, from.y + (to.y - from.y) * progress};
        DrawLineEx(from, current, 11, Fade(item.actorId == 1 ? kCyan : kRed, 0.18F));
        DrawLineEx(from, current, 3, item.actorId == 1 ? kCyan : kRed);
        DrawCircleV(current, 7, WHITE);
    }
    if (item.type == EventType::Damaged || item.type == EventType::Critical || item.type == EventType::Healed) {
        const Color color = item.type == EventType::Healed ? kGreen : (item.type == EventType::Critical ? kYellow : kRed);
        const std::string value = item.type == EventType::Healed ? "+" + std::to_string(item.amount)
                                                                 : "-" + std::to_string(item.amount);
        const auto center = cellCenter(item.to);
        text(value, {center.x - 16, center.y - 58 - progress * 24}, 32, Fade(color, 1.0F - progress * 0.5F));
    }
}

void GameApp::drawParticles() const {
    for (const auto& particle : particles_) {
        DrawCircleV(particle.position, particle.radius,
                    Fade(particle.color, clamp01(particle.life / particle.maxLife)));
    }
}

void GameApp::drawTutorial(Vector2 mouse) {
    // The modal is opaque enough that background labels must not be replayed above it
    // by the native-resolution text pass.
    textCommands_.clear();
    DrawRectangle(0, 0, kVirtualWidth, kVirtualHeight, Color{2, 5, 12, 205});
    const Rectangle bounds{210, 105, 1180, 690};
    panel(bounds, kCyan, Color{8, 19, 36, 250});
    text("战术链路教程", {270, 155}, 48, kText);
    text("三步掌握战场", {1090, 170}, 24, kCyan);
    const std::vector<std::pair<std::string, std::string>> steps = {
        {"01  读取意图", "红线表示敌人本回合能够攻击；橙线表示其计划移动的位置。"},
        {"02  选择行动", "每回合只能移动或射击一次。按 1/2 切换，也可点击右侧按钮。"},
        {"03  指定目标", "鼠标点击格子，或用 WASD/方向键移动黄色光标，Enter 确认。"}};
    for (std::size_t index = 0; index < steps.size(); ++index) {
        const Rectangle card{270, 255.0F + index * 130.0F, 1060, 96};
        DrawRectangleRounded(card, 0.07F, 6, kPanelLight);
        text(steps[index].first, {card.x + 28, card.y + 18}, 25, index == 0 ? kCyan : (index == 1 ? kOrange : kYellow));
        text(steps[index].second, {card.x + 260, card.y + 24}, 21, kText);
    }
    text("快捷键：F1 教程 · R 重开 · ESC 暂停", {270, 670}, 20, kMuted);
    if (button({1070, 700, 260, 56}, "进入战场", mouse, kCyan, 26)) {
        profile_.tutorialSeen = true;
        scene_ = tutorialReturnScene_;
        persistProfile();
    }
}

void GameApp::drawPause(Vector2 mouse) {
    textCommands_.clear();
    DrawRectangle(0, 0, kVirtualWidth, kVirtualHeight, Color{2, 5, 12, 205});
    const Rectangle bounds{500, 175, 600, 550};
    panel(bounds, kBlue, Color{9, 20, 38, 250});
    centeredText("行动暂停", {500, 220, 600, 70}, 45, kText);
    centeredText("暂停期间不计入关卡时间", {500, 292, 600, 40}, 20, kMuted);
    if (button({625, 365, 350, 58}, "继续行动", mouse, kCyan, 27)) scene_ = Scene::Battle;
    if (button({625, 440, 350, 58}, "重新开始本关", mouse, kOrange, 25)) {
        restartLevel();
        scene_ = Scene::Battle;
    }
    if (button({625, 515, 350, 58}, profile_.muted ? "声音：已静音" : "声音：开启", mouse, kYellow, 24)) {
        profile_.muted = !profile_.muted;
        persistProfile();
    }
    text("音量", {620, 606}, 21, kMuted);
    if (button({730, 594, 55, 45}, "-", mouse, kMuted, 26)) profile_.volume = std::max(0.0F, profile_.volume - 0.1F);
    text(std::to_string(static_cast<int>(profile_.volume * 100)) + "%", {815, 603}, 21, kText);
    if (button({905, 594, 55, 45}, "+", mouse, kCyan, 26)) profile_.volume = std::min(1.0F, profile_.volume + 0.1F);
    if (button({625, 660, 350, 40}, "返回主菜单", mouse, kMuted, 20)) scene_ = Scene::MainMenu;
}

void GameApp::drawPhaseModal(const GameSnapshot& snapshot, Vector2 mouse) {
    if (snapshot.phase == TurnPhase::Player || snapshot.phase == TurnPhase::Enemy) return;
    if (scene_ != Scene::Battle) return;
    textCommands_.clear();
    DrawRectangle(0, 0, kVirtualWidth, kVirtualHeight, Color{2, 5, 12, 185});
    if (snapshot.phase == TurnPhase::TalentChoice) {
        const Rectangle bounds{145, 125, 1310, 650};
        panel(bounds, kCyan, Color{8, 19, 36, 250});
        centeredText("选择一项战术天赋", {145, 160, 1310, 70}, 43, kText);
        centeredText("本次行动永久生效", {145, 225, 1310, 36}, 19, kMuted);
        for (std::size_t index = 0; index < snapshot.talentChoices.size(); ++index) {
            const auto& talent = snapshot.talentChoices[index];
            const Rectangle card{205.0F + index * 410.0F, 310, 370, 330};
            const Color color = index == 0 ? kCyan : (index == 1 ? kOrange : kYellow);
            panel(card, color, Fade(kPanelLight, 0.98F));
            text("0" + std::to_string(index + 1), {card.x + 28, card.y + 24}, 20, color);
            centeredText(talent.name, {card.x + 20, card.y + 82, card.width - 40, 60}, 31, kText);
            centeredText(talent.description, {card.x + 20, card.y + 158, card.width - 40, 70}, 23, color);
            if (button({card.x + 55, card.y + 255, card.width - 110, 50}, "选择", mouse, color, 23)) {
                queueEvents(game_.apply({ActionType::ChooseTalent, {}, talent.id}));
                levelElapsed_ = 0.0;
                levelRecorded_ = false;
                keyboardCursor_ = game_.snapshot().player.pos;
            }
        }
        return;
    }

    const Rectangle bounds{475, 205, 650, 490};
    const bool victory = snapshot.phase == TurnPhase::LevelComplete || snapshot.phase == TurnPhase::Victory;
    const Color color = victory ? kCyan : kRed;
    panel(bounds, color, Color{8, 19, 36, 250});
    if (snapshot.phase == TurnPhase::LevelComplete) {
        centeredText("关卡完成", {475, 250, 650, 70}, 50, kText);
        centeredText("第 " + std::to_string(snapshot.levelNumber) + " 关突破成功", {475, 330, 650, 46}, 26, kCyan);
        centeredText("本次用时  " + formatTime(levelElapsed_), {475, 400, 650, 42}, 24, kText);
        const auto best = profile_.bestTimes.find(snapshot.levelNumber);
        if (best != profile_.bestTimes.end()) centeredText("历史最佳  " + formatTime(best->second), {475, 450, 650, 38}, 20, kMuted);
        if (button({665, 555, 270, 58}, snapshot.levelNumber == snapshot.levelCount ? "查看最终结算" : "选择天赋", mouse, kCyan, 26)) {
            queueEvents(game_.apply({ActionType::Continue, {}, {}}));
        }
    } else if (snapshot.phase == TurnPhase::Defeat) {
        centeredText("行动失败", {475, 260, 650, 70}, 50, kRed);
        centeredText("重新规划战术，敌人意图不会说谎。", {475, 350, 650, 45}, 22, kMuted);
        if (button({645, 455, 310, 58}, "R 重新开始本关", mouse, kOrange, 25)) restartLevel();
        if (button({645, 535, 310, 48}, "返回主菜单", mouse, kMuted, 21)) scene_ = Scene::MainMenu;
    } else if (snapshot.phase == TurnPhase::Victory) {
        centeredText("战线突围", {475, 245, 650, 80}, 58, kCyan);
        centeredText("全部十个关卡已经完成", {475, 345, 650, 45}, 26, kText);
        centeredText("你的战术链路将被载入历史记录", {475, 405, 650, 40}, 21, kMuted);
        if (button({645, 505, 310, 58}, "开始新的行动", mouse, kCyan, 25)) startNewGame(true);
        if (button({645, 585, 310, 45}, "返回主菜单", mouse, kMuted, 20)) scene_ = Scene::MainMenu;
    }
}

void GameApp::drawToast() const {
    if (toastTime_ <= 0.0F || toast_.empty()) return;
    const Vector2 measured = fonts_.measure(toast_, FontRole::Body, 21, 1);
    const Rectangle bounds{(kVirtualWidth - measured.x - 54) * 0.5F, 824, measured.x + 54, 48};
    DrawRectangleRounded(bounds, 0.45F, 10, Color{5, 12, 25, 236});
    DrawRectangleRoundedLinesEx(bounds, 0.45F, 10, 1.5F, Fade(kCyan, 0.6F));
    text(toast_, {bounds.x + 27, bounds.y + 12}, 21, kText);
}

Vector2 GameApp::virtualMouse() const {
    return renderMetrics_.screenToVirtual(GetMousePosition());
}

Color GameApp::unitColor(UnitKind kind) const {
    switch (kind) {
        case UnitKind::Player: return kCyan;
        case UnitKind::Grunt: return kOrange;
        case UnitKind::Assassin: return Color{230, 78, 255, 255};
        case UnitKind::Drunk: return kYellow;
        case UnitKind::Sniper: return kRed;
        case UnitKind::Tank: return Color{141, 105, 255, 255};
        case UnitKind::Deserter: return Color{255, 169, 64, 255};
        case UnitKind::Healer: return kGreen;
        case UnitKind::Berserker: return Color{255, 48, 113, 255};
        case UnitKind::Turret: return Color{187, 204, 223, 255};
    }
    return WHITE;
}

std::string GameApp::unitGlyph(UnitKind kind) const {
    switch (kind) {
        case UnitKind::Player: return "P";
        case UnitKind::Grunt: return "E";
        case UnitKind::Assassin: return "A";
        case UnitKind::Drunk: return "Z";
        case UnitKind::Sniper: return "S";
        case UnitKind::Tank: return "T";
        case UnitKind::Deserter: return "F";
        case UnitKind::Healer: return "H";
        case UnitKind::Berserker: return "B";
        case UnitKind::Turret: return "C";
    }
    return "?";
}

std::string GameApp::aiLabel(AiMode mode) const {
    switch (mode) {
        case AiMode::Chase: return "追击";
        case AiMode::Patrol: return "巡逻";
        case AiMode::Random: return "随机";
        case AiMode::Guard: return "守卫";
        case AiMode::Sniper: return "狙击";
        case AiMode::Support: return "支援";
        case AiMode::Flee: return "逃离";
        case AiMode::Berserk: return "狂暴";
    }
    return "未知";
}

const Unit* GameApp::hoveredEnemy(const GameSnapshot& snapshot) const {
    if (!hoverCell_) return nullptr;
    const auto found = std::find_if(snapshot.enemies.begin(), snapshot.enemies.end(),
                                    [&](const Unit& unit) { return unit.pos == *hoverCell_; });
    return found == snapshot.enemies.end() ? nullptr : &*found;
}

bool GameApp::button(Rectangle bounds, const std::string& label, Vector2 mouse, Color accent, float fontSize) {
    const bool hovered = CheckCollisionPointRec(mouse, bounds);
    DrawRectangleRounded(bounds, 0.16F, 8, hovered ? Fade(accent, 0.20F) : Color{12, 27, 48, 245});
    DrawRectangleRoundedLinesEx(bounds, 0.16F, 8, hovered ? 2.5F : 1.5F, hovered ? accent : Fade(accent, 0.46F));
    const Vector2 measured = fonts_.measure(label, FontRole::Button, fontSize, 1.0F);
    text(label, {bounds.x + (bounds.width - measured.x) * 0.5F,
                 bounds.y + (bounds.height - measured.y) * 0.5F},
         fontSize, hovered ? WHITE : kText, 1.0F, FontRole::Button);
    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        playMenuSound();
        return true;
    }
    return false;
}

void GameApp::text(const std::string& value, Vector2 position, float size, Color color,
                   float spacing, std::optional<FontRole> role) const {
    textCommands_.push_back({value, position, size, spacing, color,
                             role.value_or(fontRoleForSize(size))});
}

void GameApp::centeredText(const std::string& value, Rectangle bounds, float size, Color color) const {
    const FontRole role = fontRoleForSize(size);
    const Vector2 measured = fonts_.measure(value, role, size, 1.0F);
    text(value, {bounds.x + (bounds.width - measured.x) * 0.5F,
                 bounds.y + (bounds.height - measured.y) * 0.5F}, size, color, 1.0F, role);
}

void GameApp::panel(Rectangle bounds, Color border, Color fill) const {
    DrawRectangleRounded(bounds, 0.035F, 8, fill);
    DrawRectangleRoundedLinesEx(bounds, 0.035F, 8, 1.5F, Fade(border, 0.55F));
    DrawLineEx({bounds.x + 24, bounds.y}, {bounds.x + 150, bounds.y}, 3, border);
}

}  // namespace bfb
