#ifndef LEVEL_MANAGER_H
#define LEVEL_MANAGER_H

#include <vector>
#include <string>
#include "game.h"
#include "enemy_ai.h"
#include "level_data.h"

class LevelManager {
private:
    std::vector<LevelData> levels; // 存储所有关卡数据
    int currentLevelIndex = -1;

public:
    LevelManager() {
        initializeLevels(); // 在构造函数中定义所有关卡
    }

    // 获取当前关卡数据
    const LevelData& getCurrentLevel() const {
        return levels[currentLevelIndex];
    }

    // 加载指定关卡
    bool loadLevel(int levelNumber, gamemap& gameMap, EnemyAIController& aiController) {
        // 清除旧关卡（非常重要！）
        clearMapForNewLevel(gameMap, aiController);

        // 查找关卡
        for (size_t i = 0; i < levels.size(); ++i) {
            if (levels[i].levelNumber == levelNumber) {
                currentLevelIndex = i;
                const LevelData& level = levels[i];

                std::cout << "正在加载关卡: " << level.levelName << std::endl;

                if (gameMap.getplayer()) {
                gameMap.getplayer()->setPosition(level.playerstartx, level.playerstarty);
                }

                // 根据关卡数据生成敌人
                for (const auto& spawnData : level.enemies) {
                    enemy* newEnemy = gameMap.createenemy(
                        spawnData.id,
                        spawnData.x,
                        spawnData.y,
                        spawnData.symbol,
                        spawnData.name,
                        spawnData.life,
                        spawnData.damage,
                        spawnData.attackRange,
                        spawnData.moverange
                    );
                    
                   
                    // 注册到AI控制器
                    aiController.registerEnemy(newEnemy, spawnData.aiMode);
                }
                return true;
            }
        }
        return false; // 未找到关卡
    }

    // 加载下一关
    bool loadNextLevel(gamemap& gameMap, EnemyAIController& aiController) {
        if (currentLevelIndex + 1 < levels.size()) {
            return loadLevel(levels[currentLevelIndex + 1].levelNumber, gameMap, aiController);
        }
        return false; // 已经是最后一关
    }

    bool isGameComplete() const {
        return currentLevelIndex + 1 >= levels.size();
    }

private:
    // 定义所有关卡的内容
    void initializeLevels() {
        // --- 关卡 1 ---
        LevelData level1;
        level1.levelNumber = 1;
        level1.playerstartx = 3;
        level1.playerstarty = 0;
        level1.levelName = "新手试炼";
        level1.enemies = {
            {2, 1, 7, 'E', "近战兵", AIMode::Chase, 2, 1, 1,1},//数字分别代表敌人ID、X坐标、Y坐标、符号、名称、生命、攻击力、攻击范围,移动范围
            {3, 6, 7, 'E', "近战兵", AIMode::Chase, 2, 1, 1,1}
        };
        levels.push_back(level1);

        // --- 关卡 2 ---
        LevelData level2;
        level2.levelNumber = 2;
        level2.playerstartx = 3;
        level2.playerstarty = 0;
        level2.levelName = "小心偷袭";
        level2.enemies = {
            {2, 1, 1, 'A', "刺客", AIMode::Chase, 1, 3, 2,3},
            {3, 2, 4, 'E', "近战兵", AIMode::Chase, 2, 1, 1,1},
            {4, 5, 4, 'E', "近战兵", AIMode::Chase, 2, 1, 1,1}
        };
        levels.push_back(level2);

        LevelData level3;
        level3.levelNumber = 3;
        level3.playerstartx = 4;
        level3.playerstarty = 4;
        level3.levelName = "他们都喝醉了";
        level3.enemies = {
            {2, 7, 1, 'Z', "醉汉", AIMode::Random, 1, 1, 2,2},
            {3, 1, 1, 'Z', "醉汉", AIMode::Random, 1, 1, 2,2},
            {4, 7, 6, 'Z', "醉汉", AIMode::Random, 1, 1, 2,2},
            {5, 1, 6, 'Z', "醉汉", AIMode::Random, 1, 1, 2,2},
            {6, 4, 0, 'Z', "醉汉", AIMode::Random, 1, 1, 2,2}
        };
        levels.push_back(level3);

        LevelData level4;
        level4.levelNumber = 4;
        level4.playerstartx = 0;
        level4.playerstarty = 0;
        level4.levelName = "脆弱但残忍";
        level4.enemies = {
            {2, 2, 4, 'A', "刺客", AIMode::Chase, 1, 3, 2,3},
            {3, 5, 4, 'A', "刺客", AIMode::Chase, 1, 3, 2,3},
            {4, 7, 6, 'S', "狙击手", AIMode::Chase, 1, 3, 3,1},
            {5, 4, 7, 'S', "狙击手", AIMode::Chase, 1, 3, 3,1},
        };
        levels.push_back(level4);

        LevelData level5;
        level5.levelNumber = 5;
        level5.playerstartx = 3;
        level5.playerstarty = 0;
        level5.levelName = "不要肉搏";
        level5.enemies = {
            {2, 2, 5, 'E', "近战兵", AIMode::Chase, 3, 1, 1,1},
            {3, 5, 5, 'E', "近战兵", AIMode::Chase, 3, 1, 1,1},
            {4, 3, 7, 'T', "坦克", AIMode::Chase, 6, 2, 1,1},
            {5, 4, 7, 'T', "坦克", AIMode::Chase, 6, 3, 3,1},
        };
        levels.push_back(level5);

        LevelData level6;
        level6.levelNumber = 6;
        level6.playerstartx = 3;
        level6.playerstarty = 0;
        level6.levelName = "居然有逃兵！";
        level6.enemies ={
            {2, 7, 0, 'E', "近战兵", AIMode::Chase, 3, 2, 1,2},
            {3, 1, 0, 'F', "逃兵", AIMode::Flee, 3, 2, 2,2},
            {4, 6, 5, 'F', "逃兵", AIMode::Flee, 3, 2, 2,2},
            {5, 3, 6, 'T', "坦克", AIMode::Chase, 7, 2, 1,1},
            {6, 5, 7, 'T', "坦克", AIMode::Chase, 6, 3, 3,1},
            {7, 4, 7, 'H', "战斗天使", AIMode::Support, 2, 0, 3,1},
        };
        levels.push_back(level6);

        LevelData level7;
        level7.levelNumber = 7;
        level7.playerstartx = 3;
        level7.playerstarty = 0;
        level7.levelName = "不要肉搏";
        level7.enemies = {
            {2, 2, 5, 'E', "近战兵", AIMode::Chase, 3, 1, 1,1},
            {3, 5, 5, 'E', "近战兵", AIMode::Chase, 3, 1, 1,1},
            {4, 3, 7, 'T', "坦克", AIMode::Chase, 6, 2, 1,1},
            {5, 4, 7, 'T', "坦克", AIMode::Chase, 6, 3, 3,1},
        };
        levels.push_back(level7);

        LevelData level8;
        level8.levelNumber = 8;
        level8.playerstartx = 3;
        level8.playerstarty = 0;
        level8.levelName = "撕心裂肺";
        level8.enemies = {
            {2, 1, 5, 'B', "狂战士", AIMode::Berserk, 5, 3, 2,2},
            {3, 5, 5, 'B', "狂战士", AIMode::Berserk, 5, 3, 2,2},
            {4, 3, 7, 'T', "坦克", AIMode::Chase, 6, 2, 1,1},
            {5, 4, 7, 'T', "坦克", AIMode::Chase, 6, 3, 3,1},
        };
        levels.push_back(level8);


        LevelData level9;
        level9.levelNumber=9;
        level9.playerstartx=3;
        level9.playerstarty=0;
        level9.levelName="穿越火线";
        level9.enemies = {
            {2, 2, 6, 'C', "炮台", AIMode::Guard, 3, 2, 3,0},
            {3, 5, 5, 'C', "炮台", AIMode::Guard, 3, 2, 3,0},
            {4, 2, 4, 'E', "近战兵", AIMode::Chase, 4, 2, 1,2},
            {5, 5, 3, 'E', "近战兵", AIMode::Chase, 4, 2, 1,2},
            {6, 4, 7, 'T', "坦克", AIMode::Chase, 6, 3, 3,1},
        };
        levels.push_back(level9);

        LevelData level10;
        level10.levelNumber=10;
        level10.playerstartx=3;
        level10.playerstarty=4;
        level10.levelName="未竟";
        level10.enemies = {
            {2, 0, 0, 'C', "炮台", AIMode::Guard, 3, 2, 3,0},
            {3, 7, 7, 'C', "炮台", AIMode::Guard, 3, 2, 3,0},
            {4, 1, 7, 'T', "坦克", AIMode::Chase, 6, 3, 3,1},
            {5, 5, 7, 'E', "近战兵", AIMode::Chase, 4, 2, 1,2},
            {6, 4, 7, 'H', "战斗天使", AIMode::Support, 3, 0, 3,1},
            {7, 2, 0, 'B', "狂战士", AIMode::Berserk, 5, 3, 2,2},
        };
        levels.push_back(level10);

        


    }

    // 清理地图，为加载新关卡做准备
    void clearMapForNewLevel(gamemap& gameMap, EnemyAIController& aiController) {
        gameMap.resetWithoutPlayer();
        aiController.clearAllEnemies();
        std::cout<<"地图已清除，准备加载新关卡..."<<std::endl;
    }
};
#endif