#ifndef ENEMY_AI_H
#define ENEMY_AI_H

#include "game.h"
#include <vector>
#include <algorithm>
#include <cmath>

enum class AIMode {
    Chase,      // 追逐玩家
    Patrol,     // 巡逻
    Random,     // 随机移动
    Guard,      // 守卫
    Sniper,     // 狙击
    Support,    // 治疗
    Flee,       // 逃跑
    Berserk     //狂暴
};

class EnemyAIController {
private:
    // 为敌人添加AI模式属性
    struct EnemyAI {
        enemy* enemyPtr;
        AIMode mode;
        int patrolIndex;
        std::vector<std::pair<int, int>> patrolPoints;
    };
    
    std::vector<EnemyAI> enemyAIs;
    
public:
    // 注册敌人到AI系统
    void registerEnemy(enemy* enemyPtr, AIMode mode = AIMode::Chase) {
        if (!enemyPtr) return;

        for (auto& ai : enemyAIs) {
            if (ai.enemyPtr == enemyPtr)
                return; // 已注册
        }

        EnemyAI ai;
        ai.enemyPtr = enemyPtr;
        ai.mode = mode;
        ai.patrolIndex = 0;
        enemyAIs.push_back(ai);
    }
    
    // 移除敌人的AI
    void unregisterEnemy(enemy* enemyPtr) {
        enemyAIs.erase(
            std::remove_if(enemyAIs.begin(), enemyAIs.end(),
                [enemyPtr](const EnemyAI& ai) {
                    return ai.enemyPtr == enemyPtr;
                }),
            enemyAIs.end());
    }
    
    // 执行所有敌人的AI逻辑
    void update(gamemap& gameMap, player* playerPtr) {
        if (!playerPtr) return;
        
        for (auto& ai : enemyAIs) {
            if (!ai.enemyPtr || !ai.enemyPtr->isActive())
                continue;

            if (ai.enemyPtr->getHasActedThisTurn())
                continue;

            // 优先尝试攻击
            if (ai.enemyPtr->attack(playerPtr)) {
                continue;
            }

            int targetX = ai.enemyPtr->getX();
            int targetY = ai.enemyPtr->getY();

            switch (ai.mode) {
                case AIMode::Chase:
                    chasePlayer(ai, playerPtr, targetX, targetY);
                    break;
                case AIMode::Patrol:
                    patrol(ai, targetX, targetY);
                    break;
                case AIMode::Random:
                    randomMove(ai, targetX, targetY);
                    break;
                case AIMode::Guard:
                    break;
                case AIMode::Sniper:
                    sniper(ai, playerPtr, targetX, targetY);
                    break;
                case AIMode::Support:
                    supportBehavior(ai, gameMap, targetX, targetY);
                    break;    
                case AIMode::Flee:
                    fleeBehavior(ai, playerPtr, targetX, targetY);
                    break;   
                case AIMode::Berserk:
                    berserkBehavior(ai, playerPtr, targetX, targetY);
                    break;   
            }

            // 应用移动
            if (isValidMove(ai, gameMap, playerPtr, targetX, targetY)) {
                ai.enemyPtr->setPosition(targetX, targetY);
                ai.enemyPtr->markacted();
            }
        }
    }

    void clearAllEnemies() {
        enemyAIs.clear();
    }
    
private:
    // 追逐玩家逻辑（支持移动范围）
    void chasePlayer(EnemyAI& ai, player* playerPtr, int& targetX, int& targetY) {
        int startX = ai.enemyPtr->getX();
        int startY = ai.enemyPtr->getY();

        int dx = playerPtr->getX() - startX;
        int dy = playerPtr->getY() - startY;

        int stepX = (dx > 0) ? 1 : (dx < 0 ? -1 : 0);
        int stepY = (dy > 0) ? 1 : (dy < 0 ? -1 : 0);

        int moveRange = ai.enemyPtr->getmoverange();

        targetX = startX + stepX * (std::abs(dx) < moveRange ? std::abs(dx) : moveRange);
        targetY = startY + stepY * (std::abs(dy) < moveRange ? std::abs(dy) : moveRange);
    }
    
    // 巡逻逻辑（支持移动范围）
    void patrol(EnemyAI& ai, int& targetX, int& targetY) {
        if (ai.patrolPoints.empty()) {
            int currentX = ai.enemyPtr->getX();
            int currentY = ai.enemyPtr->getY();
            ai.patrolPoints = {
                {currentX + 1, currentY},
                {currentX, currentY + 1},
                {currentX - 1, currentY},
                {currentX, currentY - 1}
            };
        }

        int destX = ai.patrolPoints[ai.patrolIndex].first;
        int destY = ai.patrolPoints[ai.patrolIndex].second;

        int moveRange = ai.enemyPtr->getmoverange();

        targetX = ai.enemyPtr->getX();
        targetY = ai.enemyPtr->getY();

        int dx = destX - targetX;
        int dy = destY - targetY;

        int stepX = (dx > 0) ? 1 : (dx < 0 ? -1 : 0);
        int stepY = (dy > 0) ? 1 : (dy < 0 ? -1 : 0);

        targetX += stepX * (std::abs(dx) < moveRange ? std::abs(dx) : moveRange);
        targetY += stepY * (std::abs(dy) < moveRange ? std::abs(dy) : moveRange);

        ai.patrolIndex = (ai.patrolIndex + 1) % ai.patrolPoints.size();
    }
    
    // 随机移动逻辑（支持移动范围）
    void randomMove(EnemyAI& ai, int& targetX, int& targetY) {
        int dir = rand() % 4;
        int dx[] = {1, -1, 0, 0};
        int dy[] = {0, 0, 1, -1};

        int moveRange = ai.enemyPtr->getmoverange();
        int steps = 1 + rand() % moveRange;

        targetX = ai.enemyPtr->getX() + dx[dir] * steps;
        targetY = ai.enemyPtr->getY() + dy[dir] * steps;
    }

    void sniper(EnemyAI& ai, player* playerPtr, int& targetX, int& targetY)
{
    int ex = ai.enemyPtr->getX();
    int ey = ai.enemyPtr->getY();
    int px = playerPtr->getX();
    int py = playerPtr->getY();

    int dist = std::abs(px - ex) + std::abs(py - ey);

    // 如果玩家在攻击范围内，不移动，只攻击
    if (dist <= ai.enemyPtr->getAttackRange()) {
        targetX = ex;
        targetY = ey;
        return;
    }

    // 否则，保持距离，向玩家方向移动一步
    int stepX = (px > ex) ? 1 : (px < ex ? -1 : 0);
    int stepY = (py > ey) ? 1 : (py < ey ? -1 : 0);

    targetX = ex + stepX;
    targetY = ey + stepY;
}

void supportBehavior(
    EnemyAI& ai, gamemap& gameMap, int& targetX, int& targetY)
{
    enemy* self = ai.enemyPtr;
    int sx = self->getX();
    int sy = self->getY();

    
    auto enemies = gameMap.getenemies();
    enemy* targetAlly = nullptr;
    int minDist = 999;

    for (auto e : enemies) {
        if (!e || e == self || !e->isActive())
            continue;

        int dist = std::abs(e->getX() - sx) + std::abs(e->getY() - sy);
        if (dist < minDist) {
            minDist = dist;
            targetAlly = e;
        }
    }

    // 如果没有友军，原地待命
    if (!targetAlly) {
        targetX = sx;
        targetY = sy;
        return;
    }

    int ax = targetAlly->getX();
    int ay = targetAlly->getY();

    
    if (minDist <= self->getAttackRange()) {
        targetAlly->takedamage(-1); // 治疗 1 点生命
        targetX = sx;
        targetY = sy;
        return;
    }

    
    int stepX = (ax > sx) ? 1 : (ax < sx ? -1 : 0);
    int stepY = (ay > sy) ? 1 : (ay < sy ? -1 : 0);

    targetX = sx + stepX;
    targetY = sy + stepY;
}

void fleeBehavior(
    EnemyAI& ai, player* playerPtr, int& targetX, int& targetY)
{
    enemy* self = ai.enemyPtr;
    int sx = self->getX();
    int sy = self->getY();
    int px = playerPtr->getX();
    int py = playerPtr->getY();

    // 逃跑触发条件：生命值 ≤ 1
    if (self->getlife() > 1) {
        // ✅ 未触发逃跑：使用普通 Chase 行为
        chasePlayer(ai, playerPtr, targetX, targetY);
        return;
    }

    // ✅ 触发逃跑：反向远离玩家
    int dx = sx - px;
    int dy = sy - py;

    int stepX = (dx > 0) ? 1 : (dx < 0 ? -1 : 0);
    int stepY = (dy > 0) ? 1 : (dy < 0 ? -1 : 0);

    int moveRange = self->getmoverange();

    targetX = sx + stepX * (std::abs(dx) < moveRange ? std::abs(dx) : moveRange);
    targetY = sy + stepY * (std::abs(dy) < moveRange ? std::abs(dy) : moveRange);
}

void berserkBehavior(
    EnemyAI& ai, player* playerPtr, int& targetX, int& targetY)
{
    enemy* self = ai.enemyPtr;
    int sx = self->getX();
    int sy = self->getY();
    int px = playerPtr->getX();
    int py = playerPtr->getY();

    // 1️⃣ 判断是否进入“濒死爆发”状态
    bool isBerserk = (self->getlife() <= 1);

    // 2️⃣ 爆发状态下临时强化属性
    int originalAttack = self->getdamageattack();
    int originalMoveRange = self->getmoverange();

    if (isBerserk) {
        // 攻击力 ×2
        self->setDamageAttack(originalAttack * 2);
        // 移动范围 +1
        self->setmoverange(originalMoveRange + 1);
    }

    // 3️⃣ 仍然使用追逐逻辑
    chasePlayer(ai, playerPtr, targetX, targetY);

    // 4️⃣ 恢复原始属性（防止持续叠加）
    if (isBerserk) {
        self->setDamageAttack(originalAttack);
        self->setmoverange(originalMoveRange);
    }
}
    
    // 检查移动是否有效
    bool isValidMove(EnemyAI& ai, gamemap& gameMap, player* playerPtr, int x, int y) {
        // 边界检查
        if (x < 0 || x >= gameMap.getMapWidth() ||
            y < 0 || y >= gameMap.getMapLength()) {
            return false;
        }

        // 不能移动到玩家位置
        if (playerPtr &&
            x == playerPtr->getX() &&
            y == playerPtr->getY()) {
            return false;
        }

        // 不能与其他敌人重叠
        auto enemies = gameMap.getenemies();
        for (auto e : enemies) {
            if (e && e != ai.enemyPtr &&
                e->isActive() &&
                e->getX() == x &&
                e->getY() == y) {
                return false;
            }
        }

        return true;
    }
};

#endif