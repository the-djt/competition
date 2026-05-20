#ifndef ROGUE_SYSTEM_H
#define ROGUE_SYSTEM_H

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include "rogue_talent.h"
#include "game.h"

class RogueSystem {
public:
    RogueSystem() {
        initTalents();
    }

    /**
     * 在关卡通关后调用
     * 从当前可解锁的天赋中选择一个
     */
    const Talent& chooseTalent(player* playerPtr) {
        auto available = getAvailableTalents();

        if (available.empty()) {
            throw std::runtime_error("没有可选择的天赋");
        }

        std::cout << "\n🎯 选择天赋：\n";
        for (size_t i = 0; i < available.size(); ++i) {
            std::cout << i + 1 << ". " << available[i]->name
                      << " - " << available[i]->description << "\n";
        }

        int choice = 0;
        while (true) {
            std::cin >> choice;
            if (choice >= 1 && choice <= static_cast<int>(available.size())) {
                break;
            }
            std::cout << "无效选择，请重新输入：";
        }

        Talent& selected = *const_cast<Talent*>(available[choice - 1]);

        // 应用天赋加成
        playerPtr->setlife(playerPtr->getlife() + selected.lifeBonus);
        playerPtr->setAttackPower(playerPtr->getAttackPower() + selected.attackBonus);
        playerPtr->setAttackRange(playerPtr->getAttackRange() + selected.attackRangeBonus);
        playerPtr->setmoverange(playerPtr->getmoverange() + selected.moveRangeBonus);
        playerPtr->setCritChance(playerPtr->getCritChance() + selected.critChance);

        // 标记为已解锁
        selected.isUnlocked = true;

        std::cout << "✅ 已获得天赋：【" << selected.name << "】\n";
        return selected;
    }

private:
    std::unordered_map<std::string, Talent> talents;

   
    void initTalents() {
        talents["strong_body"] = {
            "strong_body",
            "强壮体魄",
            "生命值+1",
            1, 0, 0, 0, 0,
            {},
            UnlockRule::All,
            false
        };

        talents["shoter_instinct"] = {
            "shoter_instinct",
            "射手直觉",
            "射程+1",
            0, 0, 1, 0, 0,
            {},
            UnlockRule::All,
            false
        };

        talents["sharp_blade"] = {
            "sharp_blade",
            "锋利之刃",
            "攻击力+1",
            0, 1, 0, 0, 0,
            {},
            UnlockRule::All,
            false
        };

        talents["giant_power"] = {
            "giant_power",
            "巨人之力",
            "生命值+2",
            2, 0, 0, 0, 0,
            {"strong_body"},
            UnlockRule::All,
            false
        };

        

        talents["critical_hit"] = {
            "critical_hit",
            "致命一击",
            "攻击力+2",
            0, 2, 0, 0, 0,
            {"sharp_blade", "shoter_instinct"},
            UnlockRule::All,
            false
        };

        talents["swift steps"]={
            "swift steps",
            "迅捷步伐",
            "移动范围+1",
            0, 0, 0, 1, 0,
            {"shoter_instinct"},
            UnlockRule::All,
            false
        };

        talents["wild growth"]={
            "wild growth",
            "狂野生长",
            "生命值+1",
            2, 0, 0, 0, 0,
            {"strong_body", "swift steps"},
            UnlockRule::All,
            false
        };

        talents["focused vision"]={
            "focused vision",
            "视野专注",
            "射程+1",
            0, 0, 1, 0, 0,
            {"shoter_instinct","sharp_blade"},
            UnlockRule::Any,
            false
        };

        talents["stride"]={
            "stride",
            "疾步",
            "移动范围+1",
            0, 0, 0, 1, 0,
            {"swift steps","sharp_blade","strong_body"},
            UnlockRule::All,
            false
        };

        talents["The Archer's Courage"]={
            "The Archer's Courage",
            "射手勇气",
            "暴击率提升20%",
            0, 0, 0, 0, 20,
            {"critical_hit","focused vision"},
            UnlockRule::All,
            false
        };

        talents["earth shook"]={
            "earth shook",
            "天崩地裂",
            "攻击力+2",
            0, 2, 0, 0, 0,
            {"The Archer's Courage","focused vision","stride"},
            UnlockRule::Any,
            false
        };

        talents["rush rush rush"]={
            "rush rush rush",
            "横冲直撞",
            "移动范围+1",
            0, 0, 0, 1, 0,
            {"critical_hit","stride","wild growth"},
            UnlockRule::All,
            false
        };

        talents["Precise Bullet"]={
            "Precise Bullet",
            "精准弹幕",
            "射程+1",
            0, 0, 1, 0, 0,
            {"The Archer's Courage","giant_power"},
            UnlockRule::All,
            false
        };

        talents["immortal"]={
            "immortal",
            "不死之身",
            "生命值+3",
            3, 0, 0, 0, 0,
            {"wild growth"},
            UnlockRule::All,
            false
        };

        talents["lucky"]={
            "lucky",
            "我真好运",
            "暴击率提升20%",
            0, 0, 0, 0, 20,
            {"The Archer's Courage","focused vision"},
            UnlockRule::All,
            false
        };



    }

    /**
     * 获取当前可解锁的天赋
     * 根据每个天赋配置的 UnlockRule 判断
     */
    std::vector<const Talent*> getAvailableTalents() const {
        std::vector<const Talent*> result;

        for (const auto& pair : talents) {
            const Talent& t = pair.second;

            if (t.isUnlocked) {
                continue;
            }

            bool canUnlock = false;

            if (t.prerequisites.empty()) {
                canUnlock = true;
            }
            else if (t.rule == UnlockRule::All) {
                // AND 逻辑：所有前置都必须解锁
                canUnlock = true;
                for (const auto& preId : t.prerequisites) {
                    auto it = talents.find(preId);
                    if (it == talents.end() || !it->second.isUnlocked) {
                        canUnlock = false;
                        break;
                    }
                }
            }
            else if (t.rule == UnlockRule::Any) {
                // OR 逻辑：任意一个前置解锁即可
                for (const auto& preId : t.prerequisites) {
                    auto it = talents.find(preId);
                    if (it != talents.end() && it->second.isUnlocked) {
                        canUnlock = true;
                        break;
                    }
                }
            }

            if (canUnlock) {
                result.push_back(&t);
            }
        }

        return result;
    }
};

#endif