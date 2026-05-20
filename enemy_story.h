#ifndef ENEMY_BESTIARY_H
#define ENEMY_BESTIARY_H

#include <vector>
#include <string>
#include <iostream>
#include <Windows.h>
#include "enemy_ai.h"

struct BestiaryEntry {
    char symbol;
    std::string name;
   
    std::string description;
};

class EnemyBestiary {
public:
    // 获取所有敌人图鉴数据
    static std::vector<BestiaryEntry> getAllEnemies() {
        return {
            {'E', "近战兵", "他们是最普通的士兵，只能近战哦"},
            {'A', "刺客", "飞檐走壁，是他们的强项，可惜只能在你身边刺杀"},
            {'S', "狙击手", "小心点！别进入他的射程范围，他可是神枪手"},
            {'H', "战斗天使", "她会为敌人疗伤哦"},
            {'F', "逃兵", "这是胆小鬼，不用逼他，他会自己逃走"},
            {'B', "狂战士", "永远不要惹怒一头雄狮，他会让你知道什么是残忍"},
            {'C',"炮台","小心别被轰到了"},
            {'Z',"醉汉","他们走起路来可没有逻辑可言"},
            {'T',"坦克","他们被称作“坦克”，是因为他们有着顽强的战斗意志和庞大的身躯"}
        };
    }

    // 显示敌人图鉴界面
    static void showEnemyBestiary() {
        SetConsoleOutputCP(CP_UTF8);
        auto enemies = getAllEnemies();

        while (true) {
            system("cls");
            std::cout << "===== 敌人图鉴 =====\n\n";

            for (const auto& e : enemies) {
                std::cout << "[" << e.symbol << "] "
                          << e.name << "\n";
                std::cout << "  说明: " << e.description << "\n\n";
            }

            std::cout << "按 r 返回...\n";

            char input;
            // ✅ 关键修复：不要 ignore，直接读取
            std::cin >> input;

            if (input == 'r' || input == 'R')
                break;
        }
    }
};

#endif