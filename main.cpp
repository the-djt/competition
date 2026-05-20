#include "game.h"
#include "enemy_ai.h"
#include <Windows.h>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include "linear_attack.h"
#include "level_manager.h"
#include "rogue_system.h"
#include "enemy_story.h"
#include "timer.h"


void runGame() {
    // 设置控制台支持中文显示
    SetConsoleOutputCP(CP_UTF8);  // 使用UTF-8编码
    SetConsoleCP(CP_UTF8);
    srand(time(nullptr));

    Timer::loadRecords();

    gamemap game_map;
    EnemyAIController aicontroller;
    LevelManager levelManager;
    RogueSystem rogueSystem;

    int currentLevel = 1;
    player* player_ptr = game_map.createplayer(1, 3, 0, 'P', "玩家");

    while (true) {
        if (!levelManager.loadLevel(currentLevel, game_map, aicontroller)) {
            std::cout << "无法加载关卡 " << currentLevel << std::endl;
            break;
        }

        Timer levelTimer;
        levelTimer.start();

        player_ptr = game_map.getplayer();
        if (!player_ptr) {
            std::cout << "玩家创建失败！\n";
            break;
        }
        player_ptr->onDamaged = [&game_map](player* p) {
            game_map.triggerPlayerDamageHighlight(p);
            // 显示受伤效果后延迟清除
            Sleep(300);
            system("cls");
            game_map.printWithPlayerPosition();
            game_map.clearHighlight();
        };

        bool levelRunning = true;
        while (levelRunning) {
            system("cls");
            std::cout << "===== 第 " << currentLevel << " 关 =====\n";
            game_map.printWithPlayerPosition();
            game_map.printEntitiesInfo();
            game_map.printEnemiesPositions();
            std::cout << "玩家生命：" << player_ptr->getlife() << std::endl;

            player_ptr->resetacted();

            std::cout << "玩家回合！\n";
            std::cout << "请选择操作：\n";
            std::cout << "1 - 移动\n";
            std::cout << "2 - 直线攻击\n";

            bool actionCompleted = false;
            int choice = 0;

            while (!actionCompleted) {
                char input;
                std::cin >> input;

                if (input == '1') {
                    choice = 1;
                    actionCompleted = true;
                } else if (input == '2') {
                    choice = 2;
                    actionCompleted = true;
                }
            }

            // ========== 移动 ==========
            if (choice == 1) {
                game_map.calculateMoveRangeHighlight(player_ptr);
                system("cls");
                game_map.printWithPlayerPosition();
                game_map.printEntitiesInfo();
                game_map.printEnemiesPositions();
                std::cout << "绿色 * 为可移动区域\n";

                int tx, ty;
                std::cout << "输入目标坐标(x y)：";
                std::cin >> tx >> ty;
                std::cin.ignore(10000, '\n');

                if (!game_map.moveplayerto(tx, ty)) {
                    std::cout << "移动失败，请重新选择操作。\n";
                    actionCompleted = false;
                }
                game_map.clearHighlight();
            }

            // ========== 直线攻击 ==========
            else if (choice == 2) {
                game_map.calculateAttackRangeHighlight(player_ptr);
                system("cls");
                game_map.printWithPlayerPosition();
                game_map.printEntitiesInfo();
                game_map.printEnemiesPositions();
                std::cout << "红色 + 为可攻击区域\n";

                int tx, ty;
                std::cout << "输入攻击目标坐标(x y)：";
                std::cin >> tx >> ty;
                std::cin.ignore(10000, '\n');

                if (!game_map.playerLinearAttackAt(tx, ty)) {
                    std::cout << "攻击失败，请重新选择操作。\n";
                    actionCompleted = false;
                }
                game_map.clearHighlight();
            }

            if (player_ptr->getlife() <= 0) {
                std::cout << "玩家死亡！游戏结束！\n";
                return;
            }

            game_map.clearHighlight();

            // ========== 敌人回合 ==========
            std::cout << "敌方回合！\n";
            aicontroller.update(game_map, player_ptr);
            game_map.updateAllEntities();

            auto enemies = game_map.getenemies();
            if (enemies.empty()) {
                std::cout << "第 " << currentLevel << " 关通关！\n";
                levelTimer.reportAndCompare(currentLevel);

                if (levelManager.isGameComplete()) {
                    std::cout << "🎉 恭喜你，通关了整个游戏！\n";
                    return;
                }

                rogueSystem.chooseTalent(player_ptr);
                currentLevel++;
                std::cout << "按回车进入下一关...\n";
                std::cin.get();
                break;
            }

            std::cout << "\n按回车继续...\n";
            std::cin.get();
        }
    }

    Timer::saveRecords();
}

// ================== 首页菜单 ==================
void mainMenu() {
    while (true) {
        SetConsoleOutputCP(CP_UTF8);  // 使用UTF-8编码
        SetConsoleCP(CP_UTF8);
        system("cls");
        std::cout << "===== 游戏首页 =====\n\n";
        std::cout << "1 - 进入游戏\n";
        std::cout << "2 - 敌人图鉴\n";
        std::cout << "0 - 退出游戏\n";

        char choice;
        std::cin >> choice;

        if (choice == '1') {
            runGame();
        } else if (choice == '2') {
            EnemyBestiary::showEnemyBestiary();
            // ✅ 图鉴退出后，自动回到首页
        } else if (choice == '0') {
            std::cout << "感谢游玩，再见！\n";
            break;
        }
    }
}

// ================== 程序入口 ==================
int main() {
    // 设置控制台代码页以支持中文显示
    SetConsoleOutputCP(936);  // 简体中文GBK编码
    SetConsoleCP(936);
    
    mainMenu();
    return 0;
}