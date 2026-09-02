#include "core/game_core.hpp"
#include "core/profile_store.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "[FAIL] " << message << '\n';
    }
}

bfb::Unit makeEnemy(int id, bfb::GridPos pos, bfb::AiMode ai, bfb::Stats stats,
                    bfb::UnitKind kind = bfb::UnitKind::Grunt) {
    return bfb::Unit{id, kind, "测试敌人", pos, stats, ai, true};
}

void testContent() {
    check(bfb::GameSession::levels().size() == 10, "应该保留 10 个关卡");
    check(bfb::GameSession::bestiary().size() == 9, "图鉴应该包含 9 类敌人");
    const auto errors = bfb::GameSession::validateContent();
    for (const auto& error : errors) std::cerr << "[CONTENT] " << error << '\n';
    check(errors.empty(), "关卡、敌人和天赋内容必须通过验证");
}

void testMovementAndOccupancy() {
    bfb::GameSession game(1);
    game.debugSetPlayer({0, 0}, {5, 5, 1, 5, 2, 0});
    game.debugSetEnemies({makeEnemy(2, {1, 0}, bfb::AiMode::Guard, {2, 2, 1, 1, 0, 0})});
    check(!game.apply({bfb::ActionType::Move, {-1, 0}, {}}).ok, "不能移动到棋盘外");
    check(!game.apply({bfb::ActionType::Move, {1, 0}, {}}).ok, "不能移动到被占据的格子");
    check(!game.apply({bfb::ActionType::Move, {3, 0}, {}}).ok, "不能移动到范围外");
    const auto result = game.apply({bfb::ActionType::Move, {0, 2}, {}});
    check(result.ok, "范围内移动应该成功");
    check(game.snapshot().player.pos == bfb::GridPos{0, 2}, "玩家坐标应该更新");
    check(game.snapshot().phase == bfb::TurnPhase::Enemy, "移动后应进入敌方回合");
}

void testAttackAndCritical() {
    bfb::GameSession game(2);
    game.debugSetPlayer({0, 0}, {5, 5, 2, 5, 2, 100});
    game.debugSetEnemies({makeEnemy(2, {3, 0}, bfb::AiMode::Guard, {6, 6, 1, 1, 0, 0})});
    const auto result = game.apply({bfb::ActionType::Attack, {4, 0}, {}});
    check(result.ok, "直线攻击应该命中路径上的首个敌人");
    check(game.snapshot().enemies.front().stats.hp == 2, "100% 暴击应造成双倍伤害");
    bool sawCritical = false;
    for (const auto& item : result.events) sawCritical |= item.type == bfb::EventType::Critical;
    check(sawCritical, "暴击事件应该对表现层可见");
}

void testEnemyModes() {
    {
        bfb::GameSession game(3);
        game.debugSetPlayer({3, 3}, {10, 10, 1, 5, 2, 0});
        game.debugSetEnemies({makeEnemy(2, {3, 5}, bfb::AiMode::Guard, {3, 3, 2, 3, 0, 0}, bfb::UnitKind::Turret)});
        game.debugSetPhase(bfb::TurnPhase::Enemy);
        game.advanceEnemyTurn();
        check(game.snapshot().player.stats.hp == 8, "守卫/炮台应在射程内攻击且不移动");
    }
    {
        bfb::GameSession game(4);
        game.debugSetPlayer({3, 3}, {10, 10, 1, 5, 2, 0});
        game.debugSetEnemies({makeEnemy(2, {3, 4}, bfb::AiMode::Berserk, {2, 4, 2, 1, 2, 0}, bfb::UnitKind::Berserker)});
        game.debugSetPhase(bfb::TurnPhase::Enemy);
        game.advanceEnemyTurn();
        check(game.snapshot().player.stats.hp == 6, "半血狂战士应造成双倍伤害");
    }
    {
        bfb::GameSession game(5);
        game.debugSetPlayer({3, 3}, {10, 10, 1, 5, 2, 0});
        auto wounded = makeEnemy(2, {0, 0}, bfb::AiMode::Chase, {1, 3, 1, 1, 1, 0});
        auto healer = makeEnemy(3, {0, 1}, bfb::AiMode::Support, {2, 2, 0, 3, 1, 0}, bfb::UnitKind::Healer);
        game.debugSetEnemies({wounded, healer});
        game.debugSetPhase(bfb::TurnPhase::Enemy);
        game.advanceEnemyTurn();
        const auto snapshot = game.snapshot();
        const auto healed = std::find_if(snapshot.enemies.begin(), snapshot.enemies.end(),
                                         [](const bfb::Unit& unit) { return unit.id == 2; });
        check(healed != snapshot.enemies.end() && healed->stats.hp == 2, "治疗 AI 应恢复受伤友军");
    }
    {
        bfb::GameSession game(6);
        game.debugSetPlayer({3, 3}, {10, 10, 1, 5, 2, 0});
        game.debugSetEnemies({makeEnemy(2, {3, 5}, bfb::AiMode::Flee, {1, 3, 1, 1, 1, 0}, bfb::UnitKind::Deserter)});
        game.debugSetPhase(bfb::TurnPhase::Enemy);
        const int before = std::abs(5 - 3);
        game.advanceEnemyTurn();
        const auto afterPos = game.snapshot().enemies.front().pos;
        const int after = std::abs(afterPos.x - 3) + std::abs(afterPos.y - 3);
        check(after > before, "濒死逃兵应远离玩家");
    }
}

void testTalentChoice() {
    bfb::GameSession game(7);
    game.debugSetEnemies({});
    game.debugSetPhase(bfb::TurnPhase::Enemy);
    game.advanceEnemyTurn();
    check(game.snapshot().phase == bfb::TurnPhase::LevelComplete, "清场后应进入关卡结算");
    check(game.apply({bfb::ActionType::Continue, {}, {}}).ok, "结算后应能继续");
    const auto choices = game.snapshot().talentChoices;
    check(choices.size() == 3, "每次应提供三个天赋");
    std::set<std::string> ids;
    for (const auto& choice : choices) ids.insert(choice.id);
    check(ids.size() == choices.size(), "天赋选项不能重复");
    check(game.apply({bfb::ActionType::ChooseTalent, {}, choices.front().id}).ok, "合法天赋应可选择");
    check(game.snapshot().levelNumber == 2, "选择天赋后应加载下一关");
}

void testProfileRoundTrip() {
    const auto root = std::filesystem::temp_directory_path() / "bfb-profile-test";
    std::filesystem::remove_all(root);
    bfb::ProfileStore store(root / "profile.json");
    bfb::PlayerProfile source;
    source.volume = 0.42F;
    source.muted = true;
    source.tutorialSeen = true;
    source.bestTimes[1] = 12.345;
    std::string warning;
    check(store.save(source, &warning), "存档应该可写入临时目录: " + warning);
    const auto loaded = store.load(&warning);
    check(loaded.muted && loaded.tutorialSeen, "布尔设置应该完成往返");
    check(std::abs(loaded.volume - 0.42F) < 0.01F, "音量应该完成往返");
    check(loaded.bestTimes.count(1) == 1 && std::abs(loaded.bestTimes.at(1) - 12.345) < 0.001,
          "最佳时间应该完成往返");
    std::filesystem::remove_all(root);
}

}  // namespace

int main() {
    testContent();
    testMovementAndOccupancy();
    testAttackAndCritical();
    testEnemyModes();
    testTalentChoice();
    testProfileRoundTrip();
    if (failures == 0) {
        std::cout << "All core tests passed.\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed.\n";
    return 1;
}
