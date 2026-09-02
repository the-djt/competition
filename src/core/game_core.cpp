#include "core/game_core.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <tuple>

namespace bfb {
namespace {

int distance(GridPos a, GridPos b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

Stats stats(int hp, int attack, int range, int move, int crit = 0) {
    return Stats{hp, hp, attack, range, move, crit};
}

EnemySpawn spawn(int id, int x, int y, UnitKind kind, const char* name, AiMode ai,
                 int hp, int attack, int range, int move) {
    return EnemySpawn{id, {x, y}, kind, name, ai, stats(hp, attack, range, move)};
}

GameEvent event(EventType type, int actor, int target, GridPos from, GridPos to,
                int amount, std::string text) {
    return GameEvent{type, actor, target, from, to, amount, std::move(text)};
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

}  // namespace

const std::vector<LevelDefinition>& GameSession::levels() {
    static const std::vector<LevelDefinition> data = {
        {1, "初次交锋", {3, 0}, {
            spawn(2, 1, 7, UnitKind::Grunt, "近战兵", AiMode::Chase, 2, 1, 1, 1),
            spawn(3, 6, 7, UnitKind::Grunt, "近战兵", AiMode::Chase, 2, 1, 1, 1)}},
        {2, "影袭", {3, 0}, {
            spawn(2, 1, 1, UnitKind::Assassin, "刺客", AiMode::Chase, 1, 3, 2, 3),
            spawn(3, 2, 4, UnitKind::Grunt, "近战兵", AiMode::Chase, 2, 1, 1, 1),
            spawn(4, 5, 4, UnitKind::Grunt, "近战兵", AiMode::Chase, 2, 1, 1, 1)}},
        {3, "酒馆乱斗", {3, 0}, {
            spawn(2, 7, 1, UnitKind::Drunk, "醉汉", AiMode::Random, 1, 1, 2, 2),
            spawn(3, 1, 1, UnitKind::Drunk, "醉汉", AiMode::Random, 1, 1, 2, 2),
            spawn(4, 7, 6, UnitKind::Drunk, "醉汉", AiMode::Random, 1, 1, 2, 2),
            spawn(5, 1, 6, UnitKind::Drunk, "醉汉", AiMode::Random, 1, 1, 2, 2),
            spawn(6, 4, 0, UnitKind::Drunk, "醉汉", AiMode::Random, 1, 1, 2, 2)}},
        {4, "交叉火力", {3, 0}, {
            spawn(2, 2, 4, UnitKind::Assassin, "刺客", AiMode::Chase, 1, 3, 2, 3),
            spawn(3, 5, 4, UnitKind::Assassin, "刺客", AiMode::Chase, 1, 3, 2, 3),
            spawn(4, 7, 6, UnitKind::Sniper, "狙击手", AiMode::Sniper, 1, 3, 3, 1),
            spawn(5, 4, 7, UnitKind::Sniper, "狙击手", AiMode::Sniper, 1, 3, 3, 1)}},
        {5, "钢铁阵线", {3, 0}, {
            spawn(2, 2, 5, UnitKind::Grunt, "近战兵", AiMode::Chase, 3, 1, 1, 1),
            spawn(3, 5, 5, UnitKind::Grunt, "近战兵", AiMode::Chase, 3, 1, 1, 1),
            spawn(4, 3, 7, UnitKind::Tank, "坦克", AiMode::Chase, 6, 2, 1, 1),
            spawn(5, 4, 7, UnitKind::Tank, "坦克", AiMode::Chase, 6, 3, 1, 1)}},
        {6, "背水一战", {3, 0}, {
            spawn(2, 7, 0, UnitKind::Grunt, "近战兵", AiMode::Chase, 3, 2, 1, 2),
            spawn(3, 1, 0, UnitKind::Deserter, "逃兵", AiMode::Flee, 3, 2, 2, 2),
            spawn(4, 6, 5, UnitKind::Deserter, "逃兵", AiMode::Flee, 3, 2, 2, 2),
            spawn(5, 3, 6, UnitKind::Tank, "坦克", AiMode::Chase, 7, 2, 1, 1),
            spawn(6, 5, 7, UnitKind::Tank, "坦克", AiMode::Chase, 6, 3, 1, 1),
            spawn(7, 4, 7, UnitKind::Healer, "战斗天使", AiMode::Support, 2, 0, 3, 1)}},
        {7, "不要肉搏", {3, 0}, {
            spawn(2, 2, 5, UnitKind::Grunt, "近战兵", AiMode::Chase, 3, 1, 1, 1),
            spawn(3, 5, 5, UnitKind::Grunt, "近战兵", AiMode::Chase, 3, 1, 1, 1),
            spawn(4, 3, 7, UnitKind::Tank, "坦克", AiMode::Chase, 6, 2, 1, 1),
            spawn(5, 4, 7, UnitKind::Tank, "坦克", AiMode::Chase, 6, 3, 1, 1)}},
        {8, "撕心裂肺", {3, 0}, {
            spawn(2, 1, 5, UnitKind::Berserker, "狂战士", AiMode::Berserk, 5, 3, 2, 2),
            spawn(3, 5, 5, UnitKind::Berserker, "狂战士", AiMode::Berserk, 5, 3, 2, 2),
            spawn(4, 3, 7, UnitKind::Tank, "坦克", AiMode::Chase, 6, 2, 1, 1),
            spawn(5, 4, 7, UnitKind::Tank, "坦克", AiMode::Chase, 6, 3, 1, 1)}},
        {9, "穿越火线", {3, 0}, {
            spawn(2, 2, 6, UnitKind::Turret, "炮台", AiMode::Guard, 3, 2, 3, 0),
            spawn(3, 5, 5, UnitKind::Turret, "炮台", AiMode::Guard, 3, 2, 3, 0),
            spawn(4, 2, 4, UnitKind::Grunt, "近战兵", AiMode::Chase, 4, 2, 1, 2),
            spawn(5, 5, 3, UnitKind::Grunt, "近战兵", AiMode::Chase, 4, 2, 1, 2),
            spawn(6, 4, 7, UnitKind::Tank, "坦克", AiMode::Chase, 6, 3, 1, 1)}},
        {10, "未竟之战", {3, 4}, {
            spawn(2, 0, 0, UnitKind::Turret, "炮台", AiMode::Guard, 3, 2, 3, 0),
            spawn(3, 7, 7, UnitKind::Turret, "炮台", AiMode::Guard, 3, 2, 3, 0),
            spawn(4, 1, 7, UnitKind::Tank, "坦克", AiMode::Chase, 6, 3, 1, 1),
            spawn(5, 5, 7, UnitKind::Grunt, "近战兵", AiMode::Chase, 4, 2, 1, 2),
            spawn(6, 4, 7, UnitKind::Healer, "战斗天使", AiMode::Support, 3, 0, 3, 1),
            spawn(7, 2, 0, UnitKind::Berserker, "狂战士", AiMode::Berserk, 5, 3, 2, 2)}}
    };
    return data;
}

const std::vector<Talent>& GameSession::talents() {
    static const std::vector<Talent> data = {
        {"strong_body", "强壮体魄", "最大生命 +1", 1, 0, 0, 0, 0, {}, true},
        {"shooter_instinct", "射手直觉", "射程 +1", 0, 0, 1, 0, 0, {}, true},
        {"sharp_blade", "锋利之刃", "攻击力 +1", 0, 1, 0, 0, 0, {}, true},
        {"swift_steps", "迅捷步伐", "移动力 +1", 0, 0, 0, 1, 0, {}, true},
        {"steady_aim", "稳固瞄准", "暴击率 +10%", 0, 0, 0, 0, 10, {}, true},
        {"giant_power", "巨人之力", "最大生命 +2", 2, 0, 0, 0, 0, {"strong_body"}, true},
        {"focused_vision", "视野专注", "射程 +1", 0, 0, 1, 0, 0, {"shooter_instinct", "sharp_blade"}, false},
        {"stride", "疾步", "移动力 +1", 0, 0, 0, 1, 0, {"swift_steps"}, true},
        {"critical_hit", "致命一击", "攻击力 +2", 0, 2, 0, 0, 0, {"sharp_blade", "shooter_instinct"}, true},
        {"wild_growth", "狂野生长", "最大生命 +2", 2, 0, 0, 0, 0, {"strong_body", "swift_steps"}, true},
        {"archers_courage", "射手勇气", "暴击率 +20%", 0, 0, 0, 0, 20, {"critical_hit", "focused_vision"}, true},
        {"earth_shook", "天崩地裂", "攻击力 +2", 0, 2, 0, 0, 0, {"archers_courage", "stride"}, false},
        {"rush", "横冲直撞", "移动力 +1", 0, 0, 0, 1, 0, {"critical_hit", "stride", "wild_growth"}, false},
        {"precise_bullet", "精准弹幕", "射程 +1", 0, 0, 1, 0, 0, {"archers_courage", "giant_power"}, false},
        {"immortal", "不死之身", "最大生命 +3", 3, 0, 0, 0, 0, {"wild_growth"}, true}
    };
    return data;
}

const std::vector<EnemyArchetype>& GameSession::bestiary() {
    static const std::vector<EnemyArchetype> data = {
        {UnitKind::Grunt, "近战兵", "E", "稳步逼近的基础单位，近身后发动攻击。", AiMode::Chase},
        {UnitKind::Assassin, "刺客", "A", "生命脆弱但移动极快，擅长突然贴身。", AiMode::Chase},
        {UnitKind::Drunk, "醉汉", "Z", "行动路线不可预测，会随机游走。", AiMode::Random},
        {UnitKind::Sniper, "狙击手", "S", "进入射程后原地开火，否则寻找射击位置。", AiMode::Sniper},
        {UnitKind::Tank, "坦克", "T", "高生命、低移动，是推进中的坚固屏障。", AiMode::Chase},
        {UnitKind::Deserter, "逃兵", "F", "濒死时远离玩家，健康时参与围攻。", AiMode::Flee},
        {UnitKind::Healer, "战斗天使", "H", "优先接近并治疗受伤友军。", AiMode::Support},
        {UnitKind::Berserker, "狂战士", "B", "半血后攻击与移动能力提升。", AiMode::Berserk},
        {UnitKind::Turret, "炮台", "C", "无法移动，但控制较大的攻击范围。", AiMode::Guard}
    };
    return data;
}

GameSession::GameSession(std::uint32_t seed) : rng_(seed) {
    startNewRun();
}

void GameSession::startNewRun() {
    levelIndex_ = 0;
    phase_ = TurnPhase::Player;
    elapsedSeconds_ = 0.0;
    unlockedTalents_.clear();
    talentChoices_.clear();
    persistentPlayerStats_ = stats(5, 1, 5, 3, 0);
    loadLevel(0, false);
}

void GameSession::loadLevel(int index, bool preserveHealth) {
    levelIndex_ = std::clamp(index, 0, static_cast<int>(levels().size()) - 1);
    const auto& level = levels()[static_cast<std::size_t>(levelIndex_)];
    if (preserveHealth) {
        persistentPlayerStats_ = player_.stats;
    }
    persistentPlayerStats_.maxHp = std::max(1, persistentPlayerStats_.maxHp);
    persistentPlayerStats_.hp = std::clamp(persistentPlayerStats_.hp, 1, persistentPlayerStats_.maxHp);
    levelStartStats_ = persistentPlayerStats_;
    player_ = Unit{1, UnitKind::Player, "玩家", level.playerStart, persistentPlayerStats_, AiMode::Chase, true};
    enemies_.clear();
    for (const auto& source : level.enemies) {
        enemies_.push_back(Unit{source.id, source.kind, source.name, source.pos, source.stats, source.ai, true});
    }
    elapsedSeconds_ = 0.0;
    phase_ = TurnPhase::Player;
}

CommandResult GameSession::apply(const GameCommand& command) {
    switch (command.type) {
        case ActionType::Move: return movePlayer(command.target);
        case ActionType::Attack: return attackWithPlayer(command.target);
        case ActionType::ChooseTalent: return chooseTalent(command.talentId);
        case ActionType::Continue: return continueFlow();
        case ActionType::Restart: return restartLevel();
    }
    return {false, "未知指令", {}};
}

CommandResult GameSession::movePlayer(GridPos target) {
    if (phase_ != TurnPhase::Player) return {false, "现在不是玩家回合", {}};
    if (!inBounds(target)) return {false, "目标超出战场", {}};
    if (occupied(target)) return {false, "目标格已被占据", {}};
    if (distance(player_.pos, target) > player_.stats.move) return {false, "目标超出移动范围", {}};
    const GridPos from = player_.pos;
    player_.pos = target;
    phase_ = TurnPhase::Enemy;
    return {true, "移动完成", {
        event(EventType::Moved, player_.id, 0, from, target, 0, "玩家移动"),
        event(EventType::TurnChanged, 0, 0, {}, {}, 0, "敌方回合")}};
}

CommandResult GameSession::attackWithPlayer(GridPos target) {
    if (phase_ != TurnPhase::Player) return {false, "现在不是玩家回合", {}};
    if (!inBounds(target)) return {false, "目标超出战场", {}};
    if (distance(player_.pos, target) > player_.stats.range) return {false, "目标超出攻击范围", {}};

    Unit* victim = nullptr;
    GridPos hitPos = target;
    for (const auto pos : linePath(player_.pos, target)) {
        if (pos == player_.pos) continue;
        if (auto* found = enemyAt(pos)) {
            victim = found;
            hitPos = pos;
            break;
        }
    }
    if (!victim) return {false, "射线上没有敌人", {}};

    std::vector<GameEvent> events;
    events.push_back(event(EventType::Shot, player_.id, victim->id, player_.pos, hitPos, 0, "线性射击"));
    const bool critical = std::uniform_int_distribution<int>(0, 99)(rng_) < player_.stats.critChance;
    const int damage = player_.stats.attack * (critical ? 2 : 1);
    if (critical) {
        events.push_back(event(EventType::Critical, player_.id, victim->id, player_.pos, hitPos, damage, "暴击"));
    }
    victim->stats.hp -= damage;
    events.push_back(event(EventType::Damaged, player_.id, victim->id, player_.pos, hitPos, damage, "命中" + victim->name));
    if (victim->stats.hp <= 0) {
        victim->stats.hp = 0;
        victim->active = false;
        events.push_back(event(EventType::Defeated, player_.id, victim->id, player_.pos, hitPos, 0, victim->name + "被击破"));
    }
    phase_ = TurnPhase::Enemy;
    evaluateOutcome(events);
    if (phase_ == TurnPhase::Enemy) {
        events.push_back(event(EventType::TurnChanged, 0, 0, {}, {}, 0, "敌方回合"));
    }
    return {true, critical ? "暴击命中" : "攻击命中", std::move(events)};
}

CommandResult GameSession::advanceEnemyTurn() {
    if (phase_ != TurnPhase::Enemy) return {false, "敌方回合尚未开始", {}};
    std::vector<GameEvent> events;

    for (auto& enemy : enemies_) {
        if (!enemy.active || !player_.active) continue;

        if (enemy.ai == AiMode::Support) {
            Unit* wounded = nullptr;
            int bestDistance = std::numeric_limits<int>::max();
            for (auto& ally : enemies_) {
                if (!ally.active || ally.id == enemy.id || ally.stats.hp >= ally.stats.maxHp) continue;
                const int d = distance(enemy.pos, ally.pos);
                if (d < bestDistance) {
                    bestDistance = d;
                    wounded = &ally;
                }
            }
            if (wounded && bestDistance <= enemy.stats.range) {
                const int amount = std::min(1, wounded->stats.maxHp - wounded->stats.hp);
                wounded->stats.hp += amount;
                events.push_back(event(EventType::Healed, enemy.id, wounded->id, enemy.pos, wounded->pos, amount,
                                       enemy.name + "治疗了" + wounded->name));
                continue;
            }
            if (wounded) {
                const GridPos from = enemy.pos;
                const GridPos to = stepToward(enemy, wounded->pos, enemy.stats.move);
                enemy.pos = to;
                if (to != from) events.push_back(event(EventType::Moved, enemy.id, 0, from, to, 0, enemy.name + "寻找伤员"));
                continue;
            }
        }

        const bool berserk = enemy.ai == AiMode::Berserk && enemy.stats.hp * 2 <= enemy.stats.maxHp;
        const int attackRange = enemy.stats.range;
        if (distance(enemy.pos, player_.pos) <= attackRange && enemy.stats.attack > 0) {
            const int damage = enemy.stats.attack * (berserk ? 2 : 1);
            events.push_back(event(EventType::Shot, enemy.id, player_.id, enemy.pos, player_.pos, 0,
                                   enemy.name + (attackRange > 1 ? "开火" : "攻击")));
            player_.stats.hp -= damage;
            events.push_back(event(EventType::Damaged, enemy.id, player_.id, enemy.pos, player_.pos, damage,
                                   "玩家受到伤害"));
            if (player_.stats.hp <= 0) {
                player_.stats.hp = 0;
                player_.active = false;
                events.push_back(event(EventType::PlayerDefeated, enemy.id, player_.id, enemy.pos, player_.pos, 0,
                                       "玩家阵亡"));
                break;
            }
            continue;
        }

        const GridPos from = enemy.pos;
        GridPos to = from;
        const int moveRange = enemy.stats.move + (berserk ? 1 : 0);
        switch (enemy.ai) {
            case AiMode::Guard: break;
            case AiMode::Random:
            case AiMode::Patrol: to = randomStep(enemy, moveRange); break;
            case AiMode::Flee:
                to = enemy.stats.hp <= 1 ? stepAway(enemy, player_.pos, moveRange)
                                         : stepToward(enemy, player_.pos, moveRange);
                break;
            case AiMode::Chase:
            case AiMode::Sniper:
            case AiMode::Berserk:
            case AiMode::Support: to = stepToward(enemy, player_.pos, moveRange); break;
        }
        enemy.pos = to;
        if (to != from) {
            events.push_back(event(EventType::Moved, enemy.id, 0, from, to, 0, enemy.name + "移动"));
        }
    }

    evaluateOutcome(events);
    if (phase_ == TurnPhase::Enemy) {
        phase_ = TurnPhase::Player;
        events.push_back(event(EventType::TurnChanged, 0, 0, {}, {}, 0, "玩家回合"));
    }
    return {true, "敌方行动完成", std::move(events)};
}

void GameSession::evaluateOutcome(std::vector<GameEvent>& events) {
    if (!player_.active || player_.stats.hp <= 0) {
        phase_ = TurnPhase::Defeat;
        return;
    }
    const bool anyAlive = std::any_of(enemies_.begin(), enemies_.end(), [](const Unit& unit) { return unit.active; });
    if (!anyAlive) {
        phase_ = TurnPhase::LevelComplete;
        events.push_back(event(EventType::LevelCompleted, player_.id, 0, player_.pos, player_.pos,
                               levelIndex_ + 1, "关卡完成"));
    }
}

CommandResult GameSession::continueFlow() {
    if (phase_ != TurnPhase::LevelComplete) return {false, "当前没有可继续的结算", {}};
    if (levelIndex_ + 1 >= static_cast<int>(levels().size())) {
        phase_ = TurnPhase::Victory;
        return {true, "全部关卡完成", {event(EventType::RunCompleted, player_.id, 0, {}, {}, 0, "战线突围")}};
    }
    prepareTalentChoices();
    phase_ = TurnPhase::TalentChoice;
    return {true, "请选择一项天赋", {}};
}

void GameSession::prepareTalentChoices() {
    std::vector<Talent> eligible;
    for (const auto& talent : talents()) {
        if (!talentUnlocked(talent.id) && talentEligible(talent)) eligible.push_back(talent);
    }
    std::shuffle(eligible.begin(), eligible.end(), rng_);
    if (eligible.size() > 3) eligible.resize(3);
    talentChoices_ = std::move(eligible);
}

bool GameSession::talentUnlocked(const std::string& id) const {
    return contains(unlockedTalents_, id);
}

bool GameSession::talentEligible(const Talent& talent) const {
    if (talent.prerequisites.empty()) return true;
    if (talent.requireAll) {
        return std::all_of(talent.prerequisites.begin(), talent.prerequisites.end(),
                           [&](const std::string& id) { return talentUnlocked(id); });
    }
    return std::any_of(talent.prerequisites.begin(), talent.prerequisites.end(),
                       [&](const std::string& id) { return talentUnlocked(id); });
}

CommandResult GameSession::chooseTalent(const std::string& id) {
    if (phase_ != TurnPhase::TalentChoice) return {false, "现在不能选择天赋", {}};
    const auto it = std::find_if(talentChoices_.begin(), talentChoices_.end(),
                                 [&](const Talent& talent) { return talent.id == id; });
    if (it == talentChoices_.end()) return {false, "该天赋不在本次选项中", {}};

    player_.stats.maxHp += it->hpBonus;
    player_.stats.hp += it->hpBonus;
    player_.stats.attack += it->attackBonus;
    player_.stats.range += it->rangeBonus;
    player_.stats.move += it->moveBonus;
    player_.stats.critChance = std::min(100, player_.stats.critChance + it->critBonus);
    persistentPlayerStats_ = player_.stats;
    unlockedTalents_.push_back(it->id);
    const std::string selectedName = it->name;
    const int nextLevel = levelIndex_ + 1;
    loadLevel(nextLevel, false);
    return {true, "已获得天赋：" + selectedName,
            {event(EventType::TalentApplied, player_.id, 0, {}, {}, 0, selectedName)}};
}

CommandResult GameSession::restartLevel() {
    persistentPlayerStats_ = levelStartStats_;
    loadLevel(levelIndex_, false);
    return {true, "关卡已重新开始", {event(EventType::TurnChanged, 0, 0, {}, {}, 0, "玩家回合")}};
}

GameSnapshot GameSession::snapshot() const {
    GameSnapshot result;
    const auto& level = levels()[static_cast<std::size_t>(levelIndex_)];
    result.levelNumber = levelIndex_ + 1;
    result.levelCount = static_cast<int>(levels().size());
    result.levelName = level.name;
    result.phase = phase_;
    result.elapsedSeconds = elapsedSeconds_;
    result.player = player_;
    for (const auto& enemy : enemies_) if (enemy.active) result.enemies.push_back(enemy);
    if (phase_ == TurnPhase::Player) {
        result.moveTargets = reachable(player_.pos, player_.stats.move, false);
        result.attackTargets = reachable(player_.pos, player_.stats.range, true);
    }
    for (const auto& enemy : result.enemies) result.intents.push_back(intentFor(enemy));
    result.talentChoices = talentChoices_;
    return result;
}

void GameSession::setElapsedSeconds(double seconds) { elapsedSeconds_ = std::max(0.0, seconds); }

bool GameSession::inBounds(GridPos pos) const {
    return pos.x >= 0 && pos.x < kGridSize && pos.y >= 0 && pos.y < kGridSize;
}

bool GameSession::occupied(GridPos pos, int exceptId) const {
    if (player_.active && player_.id != exceptId && player_.pos == pos) return true;
    return std::any_of(enemies_.begin(), enemies_.end(), [&](const Unit& enemy) {
        return enemy.active && enemy.id != exceptId && enemy.pos == pos;
    });
}

Unit* GameSession::enemyAt(GridPos pos) {
    auto it = std::find_if(enemies_.begin(), enemies_.end(),
                           [&](const Unit& enemy) { return enemy.active && enemy.pos == pos; });
    return it == enemies_.end() ? nullptr : &*it;
}

const Unit* GameSession::enemyAt(GridPos pos) const {
    auto it = std::find_if(enemies_.begin(), enemies_.end(),
                           [&](const Unit& enemy) { return enemy.active && enemy.pos == pos; });
    return it == enemies_.end() ? nullptr : &*it;
}

Unit* GameSession::enemyById(int id) {
    auto it = std::find_if(enemies_.begin(), enemies_.end(), [&](const Unit& enemy) { return enemy.id == id; });
    return it == enemies_.end() ? nullptr : &*it;
}

std::vector<GridPos> GameSession::linePath(GridPos from, GridPos to) const {
    std::vector<GridPos> path;
    int dx = std::abs(to.x - from.x);
    int dy = std::abs(to.y - from.y);
    const int sx = from.x < to.x ? 1 : -1;
    const int sy = from.y < to.y ? 1 : -1;
    int error = dx - dy;
    GridPos current = from;
    while (true) {
        path.push_back(current);
        if (current == to) break;
        const int twice = error * 2;
        if (twice > -dy) { error -= dy; current.x += sx; }
        if (twice < dx) { error += dx; current.y += sy; }
    }
    return path;
}

std::vector<GridPos> GameSession::reachable(GridPos from, int range, bool attack) const {
    std::vector<GridPos> result;
    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            const GridPos pos{x, y};
            if (pos == from || distance(from, pos) > range) continue;
            if (attack || !occupied(pos)) result.push_back(pos);
        }
    }
    return result;
}

GridPos GameSession::stepToward(const Unit& unit, GridPos target, int moveRange) const {
    GridPos current = unit.pos;
    for (int step = 0; step < moveRange; ++step) {
        std::vector<GridPos> candidates = {{current.x + 1, current.y}, {current.x - 1, current.y},
                                           {current.x, current.y + 1}, {current.x, current.y - 1}};
        std::stable_sort(candidates.begin(), candidates.end(), [&](GridPos a, GridPos b) {
            return std::tie(a.y, a.x) < std::tie(b.y, b.x);
        });
        auto best = current;
        int bestDistance = distance(current, target);
        for (const auto candidate : candidates) {
            if (!inBounds(candidate) || occupied(candidate, unit.id)) continue;
            const int d = distance(candidate, target);
            if (d < bestDistance) { bestDistance = d; best = candidate; }
        }
        if (best == current) break;
        current = best;
    }
    return current;
}

GridPos GameSession::stepAway(const Unit& unit, GridPos threat, int moveRange) const {
    GridPos current = unit.pos;
    for (int step = 0; step < moveRange; ++step) {
        const std::vector<GridPos> candidates = {{current.x + 1, current.y}, {current.x - 1, current.y},
                                                 {current.x, current.y + 1}, {current.x, current.y - 1}};
        auto best = current;
        int bestDistance = distance(current, threat);
        for (const auto candidate : candidates) {
            if (!inBounds(candidate) || occupied(candidate, unit.id)) continue;
            const int d = distance(candidate, threat);
            if (d > bestDistance) { bestDistance = d; best = candidate; }
        }
        if (best == current) break;
        current = best;
    }
    return current;
}

GridPos GameSession::randomStep(const Unit& unit, int moveRange) {
    GridPos current = unit.pos;
    for (int step = 0; step < moveRange; ++step) {
        std::vector<GridPos> candidates = {{current.x + 1, current.y}, {current.x - 1, current.y},
                                           {current.x, current.y + 1}, {current.x, current.y - 1}};
        std::shuffle(candidates.begin(), candidates.end(), rng_);
        const auto it = std::find_if(candidates.begin(), candidates.end(), [&](GridPos candidate) {
            return inBounds(candidate) && !occupied(candidate, unit.id);
        });
        if (it == candidates.end()) break;
        current = *it;
    }
    return current;
}

EnemyIntent GameSession::intentFor(const Unit& enemy) const {
    EnemyIntent result{enemy.id, enemy.pos, enemy.pos, false, false};
    if (enemy.ai == AiMode::Support) {
        const Unit* wounded = nullptr;
        int best = std::numeric_limits<int>::max();
        for (const auto& ally : enemies_) {
            if (!ally.active || ally.id == enemy.id || ally.stats.hp >= ally.stats.maxHp) continue;
            const int d = distance(enemy.pos, ally.pos);
            if (d < best) { best = d; wounded = &ally; }
        }
        if (wounded) {
            result.target = wounded->pos;
            result.willHeal = best <= enemy.stats.range;
            if (!result.willHeal) result.target = stepToward(enemy, wounded->pos, enemy.stats.move);
            return result;
        }
    }
    if (distance(enemy.pos, player_.pos) <= enemy.stats.range && enemy.stats.attack > 0) {
        result.target = player_.pos;
        result.willAttack = true;
        return result;
    }
    switch (enemy.ai) {
        case AiMode::Guard: result.target = enemy.pos; break;
        case AiMode::Flee:
            result.target = enemy.stats.hp <= 1 ? stepAway(enemy, player_.pos, enemy.stats.move)
                                                : stepToward(enemy, player_.pos, enemy.stats.move);
            break;
        case AiMode::Random:
        case AiMode::Patrol: result.target = enemy.pos; break;
        default: result.target = stepToward(enemy, player_.pos, enemy.stats.move); break;
    }
    return result;
}

std::vector<std::string> GameSession::validateContent() {
    std::vector<std::string> errors;
    if (levels().size() != 10) errors.push_back("关卡数量必须为 10");
    for (const auto& level : levels()) {
        std::set<int> ids{1};
        std::set<std::pair<int, int>> positions{{level.playerStart.x, level.playerStart.y}};
        if (level.playerStart.x < 0 || level.playerStart.x >= kGridSize ||
            level.playerStart.y < 0 || level.playerStart.y >= kGridSize) {
            errors.push_back("关卡 " + std::to_string(level.number) + " 玩家坐标越界");
        }
        for (const auto& enemy : level.enemies) {
            if (!ids.insert(enemy.id).second) errors.push_back("关卡存在重复实体 ID");
            if (!positions.emplace(enemy.pos.x, enemy.pos.y).second) errors.push_back("关卡存在重叠出生点");
            if (enemy.pos.x < 0 || enemy.pos.x >= kGridSize || enemy.pos.y < 0 || enemy.pos.y >= kGridSize) {
                errors.push_back("关卡 " + std::to_string(level.number) + " 敌人坐标越界");
            }
            if (enemy.kind == UnitKind::Sniper && enemy.ai != AiMode::Sniper) errors.push_back("狙击手 AI 配置错误");
            if (enemy.kind == UnitKind::Healer && enemy.ai != AiMode::Support) errors.push_back("治疗单位 AI 配置错误");
            if (enemy.kind == UnitKind::Turret && enemy.ai != AiMode::Guard) errors.push_back("炮台 AI 配置错误");
        }
    }
    std::set<std::string> talentIds;
    for (const auto& talent : talents()) {
        if (!talentIds.insert(talent.id).second) errors.push_back("天赋 ID 重复: " + talent.id);
    }
    for (const auto& talent : talents()) {
        for (const auto& prerequisite : talent.prerequisites) {
            if (talentIds.count(prerequisite) == 0) errors.push_back("不存在的天赋前置: " + prerequisite);
        }
    }
    return errors;
}

void GameSession::debugSetPlayer(GridPos pos, Stats statsValue) {
    player_.pos = pos;
    player_.stats = statsValue;
    player_.active = statsValue.hp > 0;
}

void GameSession::debugSetEnemies(std::vector<Unit> enemies) { enemies_ = std::move(enemies); }
void GameSession::debugSetPhase(TurnPhase phase) { phase_ = phase; }

}  // namespace bfb
