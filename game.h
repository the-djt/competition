#ifndef GAME_H
#define GAME_H

#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_YELLOW  "\033[33m"

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>
#include <functional>



enum class entitytype {
    none,
    player,
    enemy
};



// 高亮类型
enum class HighlightType {
    None,
    Move,
    Attack,
    Damage
};

// entity 基类
class entity {
protected:
    int id;
    int x;
    int y;
    char symbol;
    std::string name;
    entitytype type;
    bool active;

public:
    entity(int id, int x, int y, char symbol, const std::string& name, entitytype type)
        : id(id), x(x), y(y), symbol(symbol), name(name), type(type), active(true) {}

    virtual ~entity() = default;

    int getId() const { return id; }
    int getX() const { return x; }
    int getY() const { return y; }
    char getSymbol() const { return symbol; }
    std::string getName() const { return name; }
    entitytype getType() const { return type; }
    bool isActive() const { return active; }

    void setPosition(int newX, int newY) {
        x = newX;
        y = newY;
    }

    void setSymbol(char newSymbol) {
        symbol = newSymbol;
    }

    void setName(const std::string& newName) {
        name = newName;
    }

    void setActive(bool newActive) {
        active = newActive;
    }

    virtual void update() {}

    virtual bool move(int targetx, int targety) {
        x = targetx;
        y = targety;
        return true;
    }

    virtual bool move(int targetx, int targety, int maxDistance) {
        int distance = std::abs(targetx - x) + std::abs(targety - y);
        if (distance <= maxDistance) {
            x = targetx;
            y = targety;
            return true;
        }
        return false;
    }

    std::string getPositionString() const {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }
};

class player;
class enemy;

class player : public entity {
private:
    int life;
    int moverange;
    int attackpower;
    int attackrange;
    bool hasactedthisturn;
    int critChance;      // 新增：暴击概率（%）
    int critMultiplier; // 新增：暴击伤害倍数

public:
    player(int id, int x, int y, char symbol = '@', const std::string& name = "Player")
        : entity(id, x, y, symbol, name, entitytype::player),
          life(5),
          moverange(3),
          attackpower(1),
          attackrange(5),
          hasactedthisturn(false),
          critChance(0),
          critMultiplier(2) {}

    virtual ~player() = default;

    int getlife() const { return life; }
    int getmoverange() const { return moverange; }
    int getAttackPower() const { return attackpower; }
    int getAttackRange() const { return attackrange; }

    bool getHasActedThisTurn() const { return hasactedthisturn; }
    void markacted() { hasactedthisturn = true; }
    void resetacted() { hasactedthisturn = false; }

    void setlife(int newlife) { life = newlife; }
    void setAttackPower(int power) { attackpower = power; }
    void setAttackRange(int range) { attackrange = range; }
    void setmoverange(int range) { moverange = range; }
    int getCritChance() const { return critChance; }
    void setCritChance(int chance) { critChance = chance; }

    int getCritMultiplier() const { return critMultiplier; }
    void setCritMultiplier(int multiplier) { critMultiplier = multiplier; }

    // ✅ 受伤回调（由外部绑定）
    std::function<void(player*)> onDamaged;

    void takedamage(int damage) {
        life -= damage;

        // ✅ 触发受伤回调
        if (onDamaged) {
            onDamaged(this);
        }
    }

    void update() override {
        if (life <= 0) {
            setActive(false);
        }
    }

    bool attack(enemy* target);
};

class enemy : public entity {
private:
    int damageattack;
    int life;
    int attackrange;
    int moverange;
    bool hasactedthisturn;

public:
    enemy(int id, int x, int y, char symbol = 'E', const std::string& name = "Enemy")
        : entity(id, x, y, symbol, name, entitytype::enemy),
          damageattack(1),
          life(1),
          attackrange(1),
          moverange(1),
          hasactedthisturn(false) {}

    enemy(int id, int x, int y,
          char symbol,
          const std::string& name,
          int life,
          int damage,
          int attackRange,
          int moverange)
        : entity(id, x, y, symbol, name, entitytype::enemy),
          damageattack(damage),
          life(life),
          attackrange(attackRange),
          moverange(moverange),
          hasactedthisturn(false) {}

    int getdamageattack() const { return damageattack; }
    int getlife() const { return life; }
    int getAttackRange() const { return attackrange; }
    int getmoverange() const { return moverange; }

    void setmoverange(int range) { moverange = range; }
    void setAttackRange(int newrange) { attackrange = newrange; }
    void setDamageAttack(int newdamage) { damageattack = newdamage; }
    void setlife(int newlife) { life = newlife; }

    bool getHasActedThisTurn() const { return hasactedthisturn; }
    void markacted() { hasactedthisturn = true; }
    void resetacted() { hasactedthisturn = false; }

    bool attack(player* target);

    void takedamage(int damage) {
        life -= damage;
    }

    void update() override {
        if (life <= 0) {
            setActive(false);
        }
    }
};

// 玩家攻击实现
bool player::attack(enemy* target) {
    if (getHasActedThisTurn()) return false;
    if (!target || !target->isActive()) return false;

    int dx = std::abs(target->getX() - x);
    int dy = std::abs(target->getY() - y);
    int distance = dx + dy;

    if (distance > attackrange) {
        std::cout << "敌人超出攻击范围！\n";
        return false;
    }

    int finalDamage = attackpower;

    // ✅ 暴击判定（文档未详述，基于我所掌握的知识）
    if (rand() % 100 < critChance) {
        finalDamage *= critMultiplier;
        std::cout << "💥 暴击！造成 " << finalDamage << " 点伤害！\n";
    }

    target->takedamage(finalDamage);
    markacted();

    if (target->getlife() <= 0) {
        target->setActive(false);
    }
    return true;
}

// 敌人攻击实现
bool enemy::attack(player* target) {
    if (getHasActedThisTurn()) return false;
    if (!target || !target->isActive()) return false;

    int dx = std::abs(target->getX() - x);
    int dy = std::abs(target->getY() - y);
    int distance = dx + dy;

    if (distance > attackrange) {
        return false;
    }

    target->takedamage(damageattack);
    std::cout << "敌人 " << name << " 对玩家造成 "
              << damageattack << " 点伤害！\n";

    markacted();
    return true;
}

class gamemap {
private:
    static const int map_length = 8;
    static const int map_width = 8;
    char grid[map_length][map_width];

    std::vector<std::pair<int, int>> highlightCells;
    bool showHighlight = false;
    HighlightType highlightType = HighlightType::None;

    bool isvalidposition(int y, int x) const {
        return (y >= 0 && y < map_length && x >= 0 && x < map_width);
    }

    std::vector<std::unique_ptr<entity>> entities;
    player* player_ptr;

public:
    gamemap() {
        fillall('#');
    }

    int getMapLength() const { return map_length; }
    int getMapWidth() const { return map_width; }

    void fillall(char c) {
        for (int i = 0; i < map_length; i++)
            for (int j = 0; j < map_width; j++)
                grid[i][j] = c;
    }

    void clearmap(char c = '#') {
        fillall(c);
    }

    // ✅ 移动高亮（绿色 *）
    void calculateMoveRangeHighlight(player* playerPtr) {
        highlightCells.clear();
        showHighlight = false;

        if (!playerPtr || !playerPtr->isActive())
            return;

        int px = playerPtr->getX();
        int py = playerPtr->getY();
        int range = playerPtr->getmoverange();

        for (int dx = -range; dx <= range; ++dx) {
            for (int dy = -range; dy <= range; ++dy) {
                int x = px + dx;
                int y = py + dy;

                if (std::abs(dx) + std::abs(dy) > range)
                    continue;
                if (x < 0 || x >= map_width || y < 0 || y >= map_length)
                    continue;
                if (x == px && y == py)
                    continue;
                if (hasenemyat(x, y))
                    continue;

                highlightCells.emplace_back(x, y);
            }
        }

        showHighlight = true;
        highlightType = HighlightType::Move;
    }

    // ✅ 攻击高亮（红色 +）
    void calculateAttackRangeHighlight(player* playerPtr) {
    highlightCells.clear();
    showHighlight = false;

    if (!playerPtr || !playerPtr->isActive())
        return;

    int px = playerPtr->getX();
    int py = playerPtr->getY();
    int range = playerPtr->getAttackRange();

    // ✅ 曼哈顿范围：|dx| + |dy| <= range
    for (int dx = -range; dx <= range; ++dx) {
        for (int dy = -range; dy <= range; ++dy) {
            if (std::abs(dx) + std::abs(dy) > range)
                continue;

            int x = px + dx;
            int y = py + dy;

            if (x < 0 || x >= map_width ||
                y < 0 || y >= map_length)
                continue;

            if (x == px && y == py)
                continue;

            highlightCells.emplace_back(x, y);
        }
    }

    showHighlight = true;
    highlightType = HighlightType::Attack;
}

    void triggerPlayerDamageHighlight(player* playerPtr) {
        if (!playerPtr || !playerPtr->isActive())
            return;

        highlightCells.clear();
        highlightCells.emplace_back(playerPtr->getX(), playerPtr->getY());

        highlightType = HighlightType::Damage;
        showHighlight = true;
    }

    void clearHighlight() {
        highlightCells.clear();
        showHighlight = false;
        highlightType = HighlightType::None;
    }

    void resetEntities() {
        entities.clear();
        player_ptr = nullptr;
    }

    void resetWithoutPlayer() {
        entities.erase(
            std::remove_if(entities.begin(), entities.end(),
                [](const std::unique_ptr<entity>& e) {
                    return e->getType() == entitytype::enemy;
                }),
            entities.end());

        clearmap();
        updategridfromentities();
    }

    player* createplayer(int id, int x, int y, char symbol = '@',
                         const std::string& name = "Player") {
        auto newplayer = std::make_unique<player>(id, x, y, symbol, name);
        player_ptr = newplayer.get();
        entities.push_back(std::move(newplayer));
        updategridfromentities();
        return player_ptr;
    }

    enemy* createenemy(int id, int x, int y, char symbol = 'E',
                       const std::string& name = "Enemy") {
        auto newenemy = std::make_unique<enemy>(id, x, y, symbol, name);
        enemy* ptr = newenemy.get();
        entities.push_back(std::move(newenemy));
        updategridfromentities();
        return ptr;
    }

    enemy* createenemy(int id, int x, int y, char symbol,
                       const std::string& name, int life, int damage,
                       int attackRange, int moverange) {
        auto newenemy = std::make_unique<enemy>(
            id, x, y, symbol, name, life, damage, attackRange, moverange);
        enemy* ptr = newenemy.get();
        entities.push_back(std::move(newenemy));
        updategridfromentities();
        return ptr;
    }

    void updategridfromentities() {
        fillall('#');

        for (auto& entity : entities) {
            if (entity->isActive()) {
                int x = entity->getX();
                int y_display = entity->getY();
                int y_internal = map_length - 1 - y_display;
                if (x >= 0 && x < map_width &&
                    y_display >= 0 && y_display < map_length) {
                    grid[y_internal][x] = entity->getSymbol();
                }
            }
        }
    }

    player* getplayer() const {
        return player_ptr;
    }

    std::vector<enemy*> getenemies() const {
        std::vector<enemy*> result;
        for (const auto& entity : entities) {
            if (entity->getType() == entitytype::enemy &&
                entity->isActive()) {
                result.push_back(static_cast<enemy*>(entity.get()));
            }
        }
        return result;
    }

    std::vector<enemy*> getEntitiesAt(int x, int y_display) const {
        std::vector<enemy*> result;
        for (const auto& entity : entities) {
            if (entity->isActive() &&
                entity->getX() == x &&
                entity->getY() == y_display) {
                if (entity->getType() == entitytype::enemy) {
                    result.push_back(static_cast<enemy*>(entity.get()));
                }
            }
        }
        return result;
    }

    bool hasenemyat(int x, int y) const {
        auto enemies = getEntitiesAt(x, y);
        for (auto e : enemies) {
            if (e->isActive()) return true;
        }
        return false;
    }

    bool moveplayerto(int targetx, int targety_display) {
        if (!player_ptr || !player_ptr->isActive()) return false;

        int oldx = player_ptr->getX();
        int oldy = player_ptr->getY();

        if (targetx < 0 || targetx >= map_width ||
            targety_display < 0 || targety_display >= map_length) {
            std::cout << "移动坐标超出地图范围！\n";
            return false;
        }

        if (targetx == oldx && targety_display == oldy)
            return false;

        if (hasenemyat(targetx, targety_display)) {
            std::cout << "目标位置有敌人，无法移动！\n";
            return false;
        }

        int distance = std::abs(targetx - oldx) +
                       std::abs(targety_display - oldy);

        if (distance > player_ptr->getmoverange()) {
            std::cout << "移动距离 " << distance
                      << " 超过最大移动范围 "
                      << player_ptr->getmoverange() << std::endl;
            return false;
        }

        player_ptr->setPosition(targetx, targety_display);
        updategridfromentities();
        std::cout << "玩家移动到 " << targetx << ","
                  << targety_display << std::endl;
        return true;
    }

    bool moveplayerbyinput() {
        if (!player_ptr || !player_ptr->isActive()) {
            std::cout << "玩家不存在\n";
            return false;
        }

        int targetx, targety;
        std::cout << "请输入目标坐标(x y): ";
        std::cin >> targetx >> targety;

        if (!moveplayerto(targetx, targety)) {
            std::cout << "移动失败\n";
            return false;
        }

        std::cin.ignore(10000, '\n');
        return true;
    }

    // ✅ 最终版 print（颜色 + 类型判断 + 不改符号）
    void print() const {
        char tempGrid[map_length][map_width];

        // ✅ 用于记录每个格子的实体类型（仅用于本次打印）
        std::vector<std::vector<entitytype>> typeGrid(
            map_length,
            std::vector<entitytype>(map_width, entitytype::none)
        );

        for (int y = 0; y < map_length; ++y)
            for (int x = 0; x < map_width; ++x)
                tempGrid[y][x] = '#';

        for (const auto& entity : entities) {
            if (entity->isActive()) {
                int x = entity->getX();
                int y_display = entity->getY();
                int y_internal = map_length - 1 - y_display;

                tempGrid[y_internal][x] = entity->getSymbol();
                typeGrid[y_internal][x] = entity->getType(); 
            }
        }

        if (showHighlight) {
            for (const auto& cell : highlightCells) {
                int x = cell.first;
                int y_display = cell.second;
                int y_internal = map_length - 1 - y_display;

                if (tempGrid[y_internal][x] == '#') {
                    if (highlightType == HighlightType::Move)
                        tempGrid[y_internal][x] = '*';
                    else if (highlightType == HighlightType::Attack)
                        tempGrid[y_internal][x] = '+';
                }
            }
        }

        std::cout << "+";
        for (int i = 0; i < map_width; i++) std::cout << "--";
        std::cout << "-+\n";

        for (int y = 0; y < map_length; y++) {
            std::cout << " " << (map_length - 1 - y) << " | ";
            for (int x = 0; x < map_width; x++) {
                char c = tempGrid[y][x];
                entitytype type = typeGrid[y][x];

                if (c == '*') {
                    std::cout << COLOR_GREEN << "*" << COLOR_RESET << " ";
                } else if (c == '+') {
                    std::cout << COLOR_RED << "+" << COLOR_RESET << " ";
                } else if (highlightType == HighlightType::Damage &&type == entitytype::player) {
                std::cout << "\033[41m" << c << "\033[0m" << " ";
                } else if (type == entitytype::player) {
                    std::cout << COLOR_BLUE << c << COLOR_RESET << " ";
                } else if (type == entitytype::enemy) {
                    std::cout << COLOR_YELLOW << c << COLOR_RESET << " ";
                }  else if (type == entitytype::none) {
                    std::cout << c << " ";
                }
            }
            std::cout << "|\n";
        }

        std::cout << "   +";
        for (int i = 0; i < map_width; i++) std::cout << "--";
        std::cout << "-+\n";

        std::cout << "    ";
        for (int x = 0; x < map_width; x++) {
            std::cout << " " << x;
        }
        std::cout << "\n\n";
    }

    bool playerAttackAt(int targetx, int targety) {
        if (!player_ptr || !player_ptr->isActive()) return false;
        if (player_ptr->getHasActedThisTurn()) return false;

        auto enemies = getEntitiesAt(targetx, targety);
        for (auto e : enemies) {
            if (e->isActive()) {
                player_ptr->attack(e);
                player_ptr->markacted();
                updategridfromentities();
                return true;
            }
        }

        std::cout << "该位置没有敌人！\n";
        return false;
    }

    bool playerLinearAttackAt(int targetx, int targety);

    void printWithPlayerPosition() const {
        if (!player_ptr) return;
        print();
        std::cout << "\n玩家位置: "
                  << player_ptr->getPositionString() << "\n";
        std::cout << "地图坐标范围: x: 0-"
                  << (map_width - 1)
                  << ", y: 0-" << (map_length - 1) << "\n";
    }

    void printEntitiesInfo() const {
        std::cout << "\n=== 实体信息 ===\n";
        for (const auto& entity : entities) {
            if (entity->isActive()) {
                std::cout << "ID: " << entity->getId()
                          << " 名称: " << entity->getName()
                          << " 位置: " << entity->getPositionString()
                          << " 符号: " << entity->getSymbol() << "\n";

                if (auto p = dynamic_cast<player*>(entity.get())) {
                    std::cout << "  类型: 玩家 生命: "
                              << p->getlife()
                              << " 移动范围: " << p->getmoverange()
                              << " 攻击力: " << p->getAttackPower()
                              << " 攻击范围: " << p->getAttackRange()
                              << "\n";
                } else if (auto e = dynamic_cast<enemy*>(entity.get())) {
                    std::cout << "  类型: 敌人 生命: "
                              << e->getlife()
                              << " 攻击力: " << e->getdamageattack()
                              << " 攻击范围: " << e->getAttackRange()
                              << "\n";
                }
            }
        }
        std::cout << "================\n";
    }

    void printEnemiesPositions() const {
        std::cout << "\n=== 敌人位置信息 ===\n";
        int count = 0;
        for (const auto& entity : entities) {
            if (entity->isActive() &&
                entity->getType() == entitytype::enemy) {
                std::cout << "敌人" << ++count << ": "
                          << entity->getName()
                          << " 位置: "
                          << entity->getPositionString() << "\n";
            }
        }
        if (count == 0) std::cout << "当前没有敌人\n";
        std::cout << "==================\n";
    }

    void updateAllEntities() {
        for (auto& entity : entities) {
            if (entity->isActive()) {
                entity->update();
            }
        }

        for (auto& entity : entities) {
            if (entity->getType() == entitytype::enemy) {
                static_cast<enemy*>(entity.get())->resetacted();
            }
        }

        updategridfromentities();
    }
};

#include "linear_attack.h"

bool gamemap::playerLinearAttackAt(int targetx, int targety) {
    if (!player_ptr || !player_ptr->isActive()) return false;
    return lineAttack(*this, player_ptr, targetx, targety);
}

#endif