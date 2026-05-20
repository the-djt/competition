#ifndef ROGUE_TALENT_H
#define ROGUE_TALENT_H

#include <string>
#include <vector>

// 天赋解锁规则枚举
enum class UnlockRule {
    All,  // AND 规则：所有前置天赋都必须解锁
    Any   // OR 规则：任意一个前置天赋解锁即可
};

struct Talent {
    std::string id;
    std::string name;
    std::string description;
    int lifeBonus = 0;
    int attackBonus = 0;
    int attackRangeBonus = 0;
    int moveRangeBonus = 0;
    int critChance=0;

    std::vector<std::string> prerequisites;//前置天赋ID
    UnlockRule rule = UnlockRule::All;     //解锁规则
    bool isUnlocked = false;//是否解锁
};

#endif