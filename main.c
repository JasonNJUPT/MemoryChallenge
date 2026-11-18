#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"
#include "ui.h"
#include "rank.h"

// 页面编号只由主函数管理，其他模块通过返回值通知主函数
typedef enum {
    PAGE_MENU,
    PAGE_LEVEL,
    PAGE_PENDING,
    PAGE_RANK
} Page;

// 三行菜单：账号、游戏、排行榜
static const Rectangle loginButton = {180, 180, 200, 48};
static const Rectangle registerButton = {420, 180, 200, 48};
static const Rectangle gameButton = {180, 260, 440, 48};
static const Rectangle rankButton = {180, 340, 440, 48};
static const Rectangle exitButton = {300, 460, 200, 48};
static const Rectangle backButton = {300, 460, 200, 48};

// 加载公共字体，缺少资源文件时尝试读取本机已有的黑体
static int loadMainFont(Font *font);
// 在窗口中居中绘制一行文字
static void drawCentered(Font font, const char text[], float y, float size);
// 绘制主菜单
static void drawMenu(Font font);
// 绘制难度选择页
static void drawLevels(Font font);

/*
 * 功能：创建图形窗口，处理菜单、难度选择和排行榜页面
 * 输入：无参数，通过鼠标选择页面
 * 输出：图形主流程；账号和游戏尚未接入时显示明确提示，不生成成绩
 * 返回值：0 正常退出，1 窗口或字体初始化失败
 */
int main(void)
{
    Font font = {0};
    Page page = PAGE_MENU;
    int running = 1;
    int level = 0;
    const char *pendingTitle = "";
    const char *pendingMessage = "";
    Rectangle levelButton;
    int i;

    srand((unsigned int)time(NULL));
    InitWindow(800, 600, "Memory Challenge - GUI main test");
    if (!IsWindowReady()) {
        return 1;
    }
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    if (!loadMainFont(&font)) {
        CloseWindow();
        return 1;
    }

    while (running && !WindowShouldClose()) {
        // 每帧只更新一个页面，切换后不会重复处理同一次点击
        switch (page) {
            case PAGE_MENU:
                if (isButtonClicked(loginButton)) {
                    pendingTitle = "登录";
                    pendingMessage = "Account GUI is not connected yet.";
                    page = PAGE_PENDING;
                } else if (isButtonClicked(registerButton)) {
                    pendingTitle = "注册";
                    pendingMessage = "Registration GUI is not connected yet.";
                    page = PAGE_PENDING;
                } else if (isButtonClicked(gameButton)) {
                    page = PAGE_LEVEL;
                } else if (isButtonClicked(rankButton)) {
                    resetRankPage();
                    page = PAGE_RANK;
                } else if (isButtonClicked(exitButton)) {
                    running = 0;
                }
                break;

            case PAGE_LEVEL:
                for (i = 0; i < 3; i++) {
                    levelButton = (Rectangle){250, 180 + i * 80.0f, 300, 48};
                    if (isButtonClicked(levelButton)) {
                        level = i + 1;
                        pendingTitle = "开始游戏";
                        pendingMessage = "Game GUI is not connected. No game or score is created.";
                        page = PAGE_PENDING;
                        break;
                    }
                }
                if (page == PAGE_LEVEL && isButtonClicked(backButton)) {
                    page = PAGE_MENU;
                }
                break;

            case PAGE_PENDING:
                if (isButtonClicked(backButton)) {
                    page = PAGE_MENU;
                    level = 0;
                }
                break;

            case PAGE_RANK:
                if (updateRankPage() == 1) {
                    page = PAGE_MENU;
                }
                break;
        }

        BeginDrawing();
        ClearBackground((Color){245, 248, 252, 255});
        switch (page) {
            case PAGE_MENU:
                drawMenu(font);
                break;
            case PAGE_LEVEL:
                drawLevels(font);
                break;
            case PAGE_PENDING:
                drawCentered(font, pendingTitle, 100, 36);
                drawCentered(font, "图形化演示", 200, 26);
                drawCentered(font, pendingMessage, 275, 16);
                if (level != 0) {
                    drawCentered(font, TextFormat("Selected level: %d", level), 325, 20);
                }
                drawButton(backButton, "返回", font);
                break;
            case PAGE_RANK:
                drawRankPage(font);
                break;
        }
        EndDrawing();
    }

    unloadUiResources(font);
    CloseWindow();
    return 0;
}

/*
 * 功能：优先使用公共 UI 字体，没有文件时读取 Windows 自带字体
 * 输入：font 是接收字体的地址，调用前必须创建窗口
 * 输出：成功时写入 *font，不复制或下载字体文件
 * 返回值：1 成功，0 失败
 */
static int loadMainFont(Font *font)
{
    const char *fontPath = "C:/Windows/Fonts/simhei.ttf";
    const char *glyphs =
        "记忆力大考验排行榜初级中级高级返回名次用户名用时秒暂无成绩文件读取失败"
        "写入测试成绩查看退出开始游戏分数恭喜通过再来一局主菜单登录注册密码"
        "点击选择难度卡牌配对准成功剩余时间连击错误提示保存读取加载关闭窗口"
        "重新记录刷新进入前五自定义模式关卡继续取消确定是否玩家当前最高最低"
        "排名第本局得分已保存未保存请输入不存在安全等级图形化演示"
        "！，。：（）、？； 0123456789abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ.:-+/()%#";
    int count;
    int *codepoints;
    Font loaded;

    if (FileExists("assets/fonts/chinese.ttf")) {
        return loadUiResources(font);
    }
    if (!FileExists(fontPath)) {
        TraceLog(LOG_ERROR, "Place a Chinese font at assets/fonts/chinese.ttf");
        return 0;
    }

    codepoints = LoadCodepoints(glyphs, &count);
    if (codepoints == NULL) {
        return 0;
    }
    loaded = LoadFontEx(fontPath, 32, codepoints, count);
    UnloadCodepoints(codepoints);
    if (!IsFontValid(loaded) || loaded.texture.id == GetFontDefault().texture.id) {
        return 0;
    }
    SetTextureFilter(loaded.texture, TEXTURE_FILTER_BILINEAR);
    *font = loaded;
    return 1;
}

/*
 * 功能：居中绘制一行文字
 * 输入：font 为字体，text 为文字，y 为纵坐标，size 为字号
 * 输出：屏幕中的文字，不修改页面状态
 * 返回值：无
 */
static void drawCentered(Font font, const char text[], float y, float size)
{
    Vector2 width = MeasureTextEx(font, text, size, 1);
    DrawTextEx(font, text, (Vector2){(800 - width.x) / 2, y}, size, 1, DARKGRAY);
}

/*
 * 功能：绘制三行主菜单和退出按钮
 * 输入：font 为中文字体
 * 输出：主菜单及当前开发状态提示
 * 返回值：无
 */
static void drawMenu(Font font)
{
    drawCentered(font, "记忆力大考验", 70, 40);
    drawButton(loginButton, "登录", font);
    drawButton(registerButton, "注册", font);
    drawButton(gameButton, "开始游戏", font);
    drawButton(rankButton, "排行榜", font);
    drawButton(exitButton, "退出", font);
    drawCentered(font, "GUI preview - account and game integration pending", 550, 16);
}

/*
 * 功能：绘制三个难度和返回按钮
 * 输入：font 为中文字体
 * 输出：难度选择页面，不开始游戏或保存成绩
 * 返回值：无
 */
static void drawLevels(Font font)
{
    drawCentered(font, "选择难度", 80, 36);
    drawButton((Rectangle){250, 180, 300, 48}, "初级 4x4", font);
    drawButton((Rectangle){250, 260, 300, 48}, "中级 4x5", font);
    drawButton((Rectangle){250, 340, 300, 48}, "高级 6x6", font);
    drawButton(backButton, "返回", font);
}
