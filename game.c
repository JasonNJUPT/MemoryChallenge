#include <stdlib.h>
#include "game.h"
#include "ui.h"

// 棋盘数据只由本模块使用，其他模块通过接口取得游戏结果
static int cards[6][6];
static int state[6][6];  // 0 盖住，1 本轮翻开，2 已配对
static int selectedRows[3], selectedCols[3];
static int rows, cols, count, groupSize;
static int currentLevel, elapsedSeconds;
static double previewEnd, startTime, hideTime;

typedef enum {
    GAME_IDLE,
    GAME_PREVIEW,
    GAME_SELECT,
    GAME_WAIT,
    GAME_FINISHED,
    GAME_EXITED
} GamePhase;
static GamePhase phase = GAME_IDLE;

static Texture2D faces[12];
static Texture2D back;
static int usePictures = 0;
// 编号只用来配对；没有图片时按编号显示原终端版字符
static const char symbols[] =
    "!@#$%^&()-_=+[]{};:,.<>/?|~`"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "1234567890";
// 每次根据窗口和棋盘计算，不再固定卡牌宽高和间距
typedef struct {
    float left, top;
    float cardWidth, cardHeight, gap;
    Rectangle backButton;
} GameLayout;

// 初始化、洗牌、配对和通关判断
static void initBoard(int level);
static void shuffleBoard(void);
static int checkMatch(void);
static int isFinished(void);
// 同一套坐标用于绘制和鼠标判断
static GameLayout getGameLayout(void);
static Rectangle cardBounds(int row, int col);
static void drawCardPicture(Texture2D picture, Rectangle bounds);

/*
 * 功能：加载 12 种正面和卡背，窗口创建后调用一次
 * 输入：无，读取 assets/cards 中 back.png 和 00.png 到 11.png
 * 输出：保存图片；完全没有卡牌资源时采用字符卡牌
 * 返回值：1 可正常游戏，0 图片不完整或加载失败，失败时释放部分资源
 */
int loadGameResources(void)
{
    int i;
    int anyPicture = FileExists("assets/cards/back.png");
    unloadGameResources();
    for (i = 0; i < 12; i++) {
        if (FileExists(TextFormat("assets/cards/%02d.png", i))) {
            anyPicture = 1;
        }
    }
    if (!anyPicture) {
        return 1;
    }

    back = LoadTexture("assets/cards/back.png");
    if (!IsTextureValid(back)) {
        unloadGameResources();
        return 0;
    }
    for (i = 0; i < 12; i++) {
        faces[i] = LoadTexture(TextFormat("assets/cards/%02d.png", i));
        if (!IsTextureValid(faces[i])) {
            unloadGameResources();
            return 0;
        }
    }
    usePictures = 1;
    return 1;
}

/*
 * 功能：开始新的一局，重置棋盘、时间和选牌记录
 * 输入：level 为 1 初级、2 中级、3 高级
 * 输出：生成并打乱棋盘，进入亮牌阶段；无效难度进入未开始状态
 * 返回值：1 成功，0 难度无效
 */
int startGame(int level)
{
    int showSeconds;
    phase = GAME_IDLE;
    if (level < 1 || level > 3) {
        return 0;
    }
    currentLevel = level;
    initBoard(level);
    shuffleBoard();
    count = 0;
    elapsedSeconds = 0;
    startTime = 0;
    hideTime = 0;
    showSeconds = level == 1 ? 4 : level == 2 ? 6 : 8;
    previewEnd = GetTime() + showSeconds;
    phase = GAME_PREVIEW;
    return 1;
}

/*
 * 功能：处理本帧的选牌、盖牌和通关，不等待输入或暂停窗口
 * 输入：seconds 是接收用时的地址，鼠标由 raylib 读取
 * 输出：更新棋盘；只有通关时才写入 *seconds，整秒用时从开局盖牌后计算
 * 返回值：0 继续，1 通关，2 退出或尚未开始
 */
int updateGame(int *seconds)
{
    int r, c, i;
    double now = GetTime();

    if (phase == GAME_IDLE || phase == GAME_EXITED) {
        return 2;
    }
    if (phase == GAME_FINISHED) {
        *seconds = elapsedSeconds;
        return 1;
    }
    // 亮牌和等待盖回期间，仍然可以退出
    if (isButtonClicked(getGameLayout().backButton)) {
        phase = GAME_EXITED;
        return 2;
    }
    if (phase == GAME_PREVIEW) {
        if (now >= previewEnd) {
            startTime = now;
            phase = GAME_SELECT;
        }
        return 0;  // 刚盖牌这一帧不同时接收选牌
    }

    elapsedSeconds = (int)(now - startTime);
    if (phase == GAME_WAIT) {
        if (now >= hideTime) {
            for (i = 0; i < count; i++) {
                state[selectedRows[i]][selectedCols[i]] = 0;
            }
            count = 0;
            phase = GAME_SELECT;
        }
        return 0;
    }
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return 0;
    }
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            if (state[r][c] != 0 ||
                !CheckCollisionPointRec(GetMousePosition(), cardBounds(r, c))) {
                continue;
            }
            state[r][c] = 1;
            selectedRows[count] = r;
            selectedCols[count] = c;
            count++;
            if (count == groupSize) {
                if (checkMatch()) {
                    for (i = 0; i < count; i++) {
                        state[selectedRows[i]][selectedCols[i]] = 2;
                    }
                    count = 0;
                    if (isFinished()) {
                        phase = GAME_FINISHED;
                        *seconds = elapsedSeconds;
                        return 1;
                    }
                } else {
                    hideTime = now + 1.0;
                    phase = GAME_WAIT;
                }
            }
            return 0;  // 一次点击最多选择一张牌
        }
    }
    return 0;
}

/*
 * 功能：绘制棋盘、阶段提示、用时和返回按钮
 * 输入：font 为主函数提供的中文字体
 * 输出：当前画面，不改变棋盘、阶段或计时
 * 返回值：无
 */
void drawGame(Font font)
{
    int r, c;
    int remaining;
    Rectangle bounds;
    Color fill;
    const char *label;
    Vector2 size;
    GameLayout layout = getGameLayout();
    float textSize;
    DrawTextEx(font, TextFormat("Level %d - %d x %d", currentLevel, rows, cols),
               (Vector2){-getUiCamera().offset.x / getUiCamera().zoom + 10, 8}, 18, 1, DARKGRAY);
    if (phase == GAME_PREVIEW) {
        remaining = (int)(previewEnd - GetTime() + 0.999999);
        if (remaining < 0) remaining = 0;
        label = TextFormat("Remember the cards: %d s", remaining);
    } else if (phase == GAME_WAIT) {
        label = "Not a match - wait 1 second";
    } else {
        label = TextFormat("Time: %d s   Select: %d / %d", elapsedSeconds, count, groupSize);
    }
    DrawTextEx(font, label, (Vector2){-getUiCamera().offset.x / getUiCamera().zoom + 10, 32}, 16, 1, DARKGRAY);
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            bounds = cardBounds(r, c);
            fill = state[r][c] == 2 ? (Color){200, 235, 210, 255} : WHITE;
            if (phase != GAME_PREVIEW && state[r][c] == 0) {
                DrawRectangleRec(bounds, (Color){75, 125, 185, 255});
                if (usePictures) drawCardPicture(back, bounds);
            } else {
                DrawRectangleRec(bounds, fill);
                if (usePictures) {
                    drawCardPicture(faces[cards[r][c]], bounds);
                } else {
                    label = TextFormat("%c", symbols[cards[r][c]]);
                    // 字符随卡牌一起变大，并始终居中
                    textSize = (bounds.width < bounds.height ? bounds.width : bounds.height) * 0.62f;
                    size = MeasureTextEx(font, label, textSize, 1);
                    DrawTextEx(font, label,
                        (Vector2){bounds.x + (bounds.width - size.x) / 2,
                                  bounds.y + (bounds.height - size.y) / 2},
                        textSize, 1, DARKGRAY);
                }
            }
            DrawRectangleLinesEx(bounds, state[r][c] == 2 ? 3.0f : 1.0f,
                                 state[r][c] == 2 ? DARKGREEN : GRAY);
        }
    }
    drawButton(layout.backButton, "返回", font);
}

/*
 * 功能：关闭窗口前释放游戏图片，也用于清理加载失败的部分资源
 * 输入：无
 * 输出：释放并清零纹理，恢复字符卡牌模式
 * 返回值：无
 */
void unloadGameResources(void)
{
    int i;
    for (i = 0; i < 12; i++) {
        if (faces[i].id != 0) UnloadTexture(faces[i]);
        faces[i] = (Texture2D){0};
    }
    if (back.id != 0) UnloadTexture(back);
    back = (Texture2D){0};
    usePictures = 0;
}

/*
 * 功能：按难度生成有序牌面并清空状态
 * 输入：level 为已经检查有效的难度
 * 输出：更新内部数组、行列数及每组张数
 * 返回值：无
 */
static void initBoard(int level)
{
    int r, c, number = 0;
    rows = level == 3 ? 6 : 4;
    cols = level == 1 ? 4 : level == 2 ? 5 : 6;
    groupSize = level == 3 ? 3 : 2;
    for (r = 0; r < 6; r++) {
        for (c = 0; c < 6; c++) {
            cards[r][c] = 0;
            state[r][c] = 0;
        }
    }
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            cards[r][c] = number / groupSize;
            number++;
        }
    }
}

/*
 * 功能：从最后一张向前随机交换牌的位置
 * 输入：使用内部棋盘和行列数
 * 输出：打乱 cards，各种牌的数量不变
 * 返回值：无
 */
static void shuffleBoard(void)
{
    int i, j, temp;
    for (i = rows * cols - 1; i > 0; i--) {
        j = rand() % (i + 1);
        temp = cards[i / cols][i % cols];
        cards[i / cols][i % cols] = cards[j / cols][j % cols];
        cards[j / cols][j % cols] = temp;
    }
}

/*
 * 功能：判断本轮所选牌是否全部相同
 * 输入：使用已记录的选牌坐标和 count
 * 输出：不修改棋盘
 * 返回值：1 相同，0 不同
 */
static int checkMatch(void)
{
    int i;
    int first = cards[selectedRows[0]][selectedCols[0]];
    for (i = 1; i < count; i++) {
        if (cards[selectedRows[i]][selectedCols[i]] != first) return 0;
    }
    return 1;
}

/*
 * 功能：判断所有牌是否已经配对
 * 输入：使用内部 state、rows 和 cols
 * 输出：不修改棋盘
 * 返回值：全部为 2 返回 1，否则返回 0
 */
static int isFinished(void)
{
    int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            if (state[r][c] != 2) return 0;
        }
    }
    return 1;
}

/*
 * 功能：根据窗口大小和行列数分配棋盘区域
 * 输入：使用当前窗口尺寸和本局 rows、cols
 * 输出：无，不修改棋盘；上方留一条状态区，其余空间尽量分给卡牌
 * 返回值：卡牌宽高、间距、起点和右上角返回按钮的区域
 */
static GameLayout getGameLayout(void)
{
    Camera2D camera = getUiCamera();
    GameLayout layout;
    // 相机为其他页面留出的两侧空白，在游戏页也用来摆牌
    float width = GetScreenWidth() / camera.zoom;
    float height = GetScreenHeight() / camera.zoom;
    float margin = 10;
    float boardTop = 62;
    float availableHeight = height - boardTop - margin;

    layout.gap = availableHeight / 90;
    if (layout.gap < 4) layout.gap = 4;
    if (layout.gap > 10) layout.gap = 10;
    layout.left = -camera.offset.x / camera.zoom + margin;
    layout.top = boardTop;
    layout.cardWidth = (width - margin * 2 - (cols - 1) * layout.gap) / cols;
    layout.cardHeight = (availableHeight - (rows - 1) * layout.gap) / rows;
    layout.backButton = (Rectangle){layout.left + width - margin * 2 - 90, 10, 90, 36};
    // 取两个方向能容纳的较小边长，保证卡牌为正方形
    if (layout.cardWidth < layout.cardHeight) {
        layout.cardHeight = layout.cardWidth;
    } else {
        layout.cardWidth = layout.cardHeight;
    }
    // 状态栏位置不变，正方形棋盘在下方可用区域中居中
    layout.left += (width - margin * 2 -
                   (cols * layout.cardWidth + (cols - 1) * layout.gap)) / 2;
    layout.top += (availableHeight -
                  (rows * layout.cardHeight + (rows - 1) * layout.gap)) / 2;
    return layout;
}

/*
 * 功能：计算指定卡牌的实际矩形，绘制和点击判断共用此函数
 * 输入：row、col 为有效行列下标
 * 输出：无
 * 返回值：使用动态卡牌大小和间距计算的矩形
 */
static Rectangle cardBounds(int row, int col)
{
    GameLayout layout = getGameLayout();
    return (Rectangle){layout.left + col * (layout.cardWidth + layout.gap),
                       layout.top + row * (layout.cardHeight + layout.gap),
                       layout.cardWidth, layout.cardHeight};
}

/*
 * 功能：将图片等比例缩放到卡牌内部
 * 输入：picture 是有效纹理，bounds 是卡牌矩形
 * 输出：图片画面，不改变资源
 * 返回值：无
 */
static void drawCardPicture(Texture2D picture, Rectangle bounds)
{
    float padding = (bounds.width < bounds.height ? bounds.width : bounds.height) * 0.08f;
    float sx = (bounds.width - padding * 2) / picture.width;
    float sy = (bounds.height - padding * 2) / picture.height;
    float scale = sx < sy ? sx : sy;
    Rectangle dest = {bounds.x + (bounds.width - picture.width * scale) / 2,
                      bounds.y + (bounds.height - picture.height * scale) / 2,
                      picture.width * scale, picture.height * scale};
    DrawTexturePro(picture, (Rectangle){0, 0, (float)picture.width, (float)picture.height},
                   dest, (Vector2){0, 0}, 0, WHITE);
}
