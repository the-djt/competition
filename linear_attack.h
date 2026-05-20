#ifndef LINEAR_ATTACK_H
#define LINEAR_ATTACK_H

#include "game.h"
#include <vector>
#include <utility>
#include <cmath>

inline std::vector<std::pair<int, int>> getLinePath(int x1, int y1, int x2, int y2) {
    std::vector<std::pair<int, int>> path;
    
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    int x = x1, y = y1;
    
    while (true) {
        path.emplace_back(x, y);
        
        if (x == x2 && y == y2) {
            break;
        }
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
    
    return path;
}

inline bool lineAttack(gamemap& gameMap, player* attacker, int targetX, int targetY) {
    // 基础合法性检查
    if (!attacker || !attacker->isActive()) {
        return false;
    }
    
    if (attacker->getHasActedThisTurn()) {
        std::cout << "本回合已经行动过了！\n";
        return false;
    }

    int startX = attacker->getX();
    int startY = attacker->getY();

    //  攻击范围检查（曼哈顿距离，与现有逻辑保持一致）
    int distance = std::abs(targetX - startX) + std::abs(targetY - startY);
    if (distance > attacker->getAttackRange()) {
        std::cout << "目标超出攻击范围！\n";
        return false;
    }

    // 计算直线路径
    auto path = getLinePath(startX, startY, targetX, targetY);

    //  沿路径检测第一个敌人
    for (const auto& [x, y] : path) {
        // 跳过玩家起始位置
        if (x == startX && y == startY) {
            continue;
        }

        auto enemies = gameMap.getEntitiesAt(x, y);
        for (auto e : enemies) {
            if (e->isActive()) {
                // 5. 命中敌人，执行攻击
                attacker->attack(e);
                attacker->markacted();
                gameMap.updategridfromentities();
                
                std::cout << "直线攻击命中敌人：" << e->getName() << "！\n";
                return true;
            }
        }
    }

    std::cout << "路径上没有敌人！\n";
    return false;
}

#endif