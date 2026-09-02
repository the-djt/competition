#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace bfb {

constexpr int kGridSize = 8;

struct GridPos {
    int x = 0;
    int y = 0;

    friend bool operator==(const GridPos& lhs, const GridPos& rhs) {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }
    friend bool operator!=(const GridPos& lhs, const GridPos& rhs) { return !(lhs == rhs); }
};

enum class UnitKind {
    Player,
    Grunt,
    Assassin,
    Drunk,
    Sniper,
    Tank,
    Deserter,
    Healer,
    Berserker,
    Turret
};

enum class AiMode { Chase, Patrol, Random, Guard, Sniper, Support, Flee, Berserk };

enum class TurnPhase { Player, Enemy, LevelComplete, TalentChoice, Defeat, Victory };

enum class ActionType { Move, Attack, ChooseTalent, Continue, Restart };

enum class EventType {
    Moved,
    Shot,
    Damaged,
    Critical,
    Healed,
    Defeated,
    TurnChanged,
    LevelCompleted,
    TalentApplied,
    RunCompleted,
    PlayerDefeated
};

struct Stats {
    int hp = 1;
    int maxHp = 1;
    int attack = 1;
    int range = 1;
    int move = 1;
    int critChance = 0;
};

struct Unit {
    int id = 0;
    UnitKind kind = UnitKind::Grunt;
    std::string name;
    GridPos pos;
    Stats stats;
    AiMode ai = AiMode::Chase;
    bool active = true;
};

struct Talent {
    std::string id;
    std::string name;
    std::string description;
    int hpBonus = 0;
    int attackBonus = 0;
    int rangeBonus = 0;
    int moveBonus = 0;
    int critBonus = 0;
    std::vector<std::string> prerequisites;
    bool requireAll = true;
};

struct EnemySpawn {
    int id = 0;
    GridPos pos;
    UnitKind kind = UnitKind::Grunt;
    std::string name;
    AiMode ai = AiMode::Chase;
    Stats stats;
};

struct LevelDefinition {
    int number = 1;
    std::string name;
    GridPos playerStart;
    std::vector<EnemySpawn> enemies;
};

struct EnemyIntent {
    int unitId = 0;
    GridPos from;
    GridPos target;
    bool willAttack = false;
    bool willHeal = false;
};

struct GameEvent {
    EventType type = EventType::Moved;
    int actorId = 0;
    int targetId = 0;
    GridPos from;
    GridPos to;
    int amount = 0;
    std::string text;
};

struct GameCommand {
    ActionType type = ActionType::Move;
    GridPos target;
    std::string talentId;
};

struct CommandResult {
    bool ok = false;
    std::string message;
    std::vector<GameEvent> events;
};

struct GameSnapshot {
    int levelNumber = 1;
    int levelCount = 0;
    std::string levelName;
    TurnPhase phase = TurnPhase::Player;
    double elapsedSeconds = 0.0;
    Unit player;
    std::vector<Unit> enemies;
    std::vector<EnemyIntent> intents;
    std::vector<Talent> talentChoices;
    std::vector<GridPos> moveTargets;
    std::vector<GridPos> attackTargets;
};

struct EnemyArchetype {
    UnitKind kind = UnitKind::Grunt;
    std::string name;
    std::string callSign;
    std::string description;
    AiMode ai = AiMode::Chase;
};

class GameSession {
public:
    explicit GameSession(std::uint32_t seed = 0xB47F2026u);

    void startNewRun();
    CommandResult apply(const GameCommand& command);
    CommandResult advanceEnemyTurn();
    GameSnapshot snapshot() const;
    void setElapsedSeconds(double seconds);

    static const std::vector<LevelDefinition>& levels();
    static const std::vector<Talent>& talents();
    static const std::vector<EnemyArchetype>& bestiary();
    static std::vector<std::string> validateContent();

    // Test seams keep the rules deterministic without exposing presentation state.
    void debugSetPlayer(GridPos pos, Stats stats);
    void debugSetEnemies(std::vector<Unit> enemies);
    void debugSetPhase(TurnPhase phase);

private:
    std::mt19937 rng_;
    int levelIndex_ = 0;
    TurnPhase phase_ = TurnPhase::Player;
    Unit player_;
    Stats persistentPlayerStats_;
    Stats levelStartStats_;
    std::vector<Unit> enemies_;
    std::vector<std::string> unlockedTalents_;
    std::vector<Talent> talentChoices_;
    double elapsedSeconds_ = 0.0;

    void loadLevel(int index, bool preserveHealth);
    void prepareTalentChoices();
    bool talentUnlocked(const std::string& id) const;
    bool talentEligible(const Talent& talent) const;
    CommandResult movePlayer(GridPos target);
    CommandResult attackWithPlayer(GridPos target);
    CommandResult chooseTalent(const std::string& id);
    CommandResult continueFlow();
    CommandResult restartLevel();
    void evaluateOutcome(std::vector<GameEvent>& events);

    bool inBounds(GridPos pos) const;
    bool occupied(GridPos pos, int exceptId = -1) const;
    Unit* enemyAt(GridPos pos);
    const Unit* enemyAt(GridPos pos) const;
    Unit* enemyById(int id);
    std::vector<GridPos> linePath(GridPos from, GridPos to) const;
    std::vector<GridPos> reachable(GridPos from, int range, bool attack) const;
    GridPos stepToward(const Unit& unit, GridPos target, int moveRange) const;
    GridPos stepAway(const Unit& unit, GridPos threat, int moveRange) const;
    GridPos randomStep(const Unit& unit, int moveRange);
    EnemyIntent intentFor(const Unit& enemy) const;
};

}  // namespace bfb
