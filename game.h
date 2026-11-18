#ifndef GAME_H
#define GAME_H

#include "raylib.h"

// 加载卡牌资源；没有图片时使用字符卡牌，成功返回 1，资源损坏返回 0
int loadGameResources(void);
// 开始指定难度的一局游戏，成功返回 1，难度无效返回 0
int startGame(int level);
// 每帧更新；0 继续，1 通关并写入 *seconds，2 退出且不写用时
int updateGame(int *seconds);
// 查询本局配对错误次数，新一局清零，不参与排行榜排序
int getGameMistakes(void);
// 查询本局手动亮牌次数，开局亮牌不计入，新一局清零
int getGameCheats(void);
// 只绘制当前画面，font 是主函数提供的中文字体
void drawGame(Font font);
// 在关闭窗口前释放已加载的图片
void unloadGameResources(void);

#endif
