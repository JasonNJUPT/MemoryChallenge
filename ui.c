#define _CRT_SECURE_NO_WARNINGS

/*
 * ui.c 公共界面：中文字体加载 + 按钮点击判断 + 按钮绘制
 *
 * ============================================================
 * 这个文件只做两件事
 *   1. 字体：把界面要用的中文从字体文件里"挑"出来加载好
 *   2. 按钮：统一的绘制样式 + 统一的点击判断
 * 账号输入框、结算页这些内容不在这里，由对应成员自己实现
 * ============================================================
 *
 * 先认识 raylib 里最重要的三个类型
 *
 *   Font      字体对象，里面是一张"字符图片集"（纹理）+ 每个字的坐标
 *             画字必须用 DrawTextEx(font, ...)，不能用 DrawText
 *
 *   Rectangle 矩形，四个 float 成员：x、y、width、height
 *             按钮的位置和大小都用它表示
 *
 *   Vector2   二维点，两个 float 成员：x、y
 *             画文字时的"左上角坐标"就用它
 *
 * 再理解一个概念：码点（codepoint）
 *   "码点"就是字符的编号，比如 'A' 是 65，'中' 是 20013
 *   raylib 默认字体只带了 224 个 ASCII 字形，完全没有中文，
 *   所以必须告诉它"我要哪些字的码点"，它才会照着去字体文件里取
 * ============================================================
 */

#include <stdio.h>
#include <string.h>

#include "ui.h"

#define UI_FONT_PATH      "assets/fonts/chinese.ttf"   // 组内提供的字体文件
#define UI_FONT_SIZE      32       // 字体生成尺寸（像素），画的时候可以缩放
#define UI_CODEPOINT_MAX  512      // 码点数组的容量上限

/*
 * 界面里可能出现的全部文字，用来汇总"需要哪些字形"
 *
 * 为什么要集中写在这里？
 *   只有出现在这串字符里的字，才会被加载进字体纹理；
 *   没写进来的字画出来就是空白或方块
 *
 * 以后加了新的中文提示，记得把新字补进这串字符串
 */
static const char UI_GLYPHS[] =
    "记忆力大考验排行榜初级中级高级返回名次用户名用时秒暂无成绩文件读取失败"
    "写入测试成绩查看退出开始游戏分数恭喜通过再来一局主菜单登录注册密码"
    "点击选择难度卡牌配对准成功剩余时间连击错误提示保存读取加载关闭窗口"
    "重新记录刷新进入前五自定义模式关卡继续取消确定是否玩家当前最高最低"
    "排名第本局得分已保存未保存请输入不存在安全等级图形化演示"
    "！，。：（）、？；"
    " 0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    ".:-+/()%#";

/*
 * 功能：把一串 UTF-8 文字拆成"去重后"的码点数组
 * 输入：text 为文字，out 为输出数组，max 为数组容量
 * 输出：out 中写入去重后的码点
 * 返回值：去重后的码点个数
 */
static int buildCodepoints(const char *text, int *out, int max)
{
    int total = 0;
    int *all;
    int count = 0;
    int i;
    int j;

    // LoadCodepoints 把 UTF-8 字符串转成整数码点数组
    // 注意：它返回的内存由 raylib 分配，用完必须 UnloadCodepoints 释放，
    // 否则会内存泄漏（每次调用都漏一点）
    all = LoadCodepoints(text, &total);

    if (all == NULL || total <= 0)
    {
        if (all != NULL) UnloadCodepoints(all);
        return 0;
    }

    // 逐个检查有没有重复，只保留第一次出现的
    // 重复的码点重复加载没有意义，还会让字体纹理变大
    for (i = 0; i < total; i++)
    {
        int codepoint = all[i];
        int dup = 0;

        for (j = 0; j < count; j++)
        {
            if (out[j] == codepoint)
            {
                dup = 1;
                break;
            }
        }

        // 既要不重复，也不能超过数组容量
        if (!dup && count < max) out[count++] = codepoint;
    }

    UnloadCodepoints(all);    // 必须配对释放
    return count;
}

/*
 * 功能：窗口创建后加载界面中文字体
 * 输入：font 是接收字体的变量地址
 * 输出：成功时将字体写入 *font，失败时清理本次已加载的资源
 * 返回值：1 成功，0 失败
 */
int loadUiResources(Font *font)
{
    // static 是为了不放在栈上：512 个 int 大约 2 KB，
    // 而且只需要一份，没必要每次都重新分配
    static int codepoints[UI_CODEPOINT_MAX];

    int count;
    Font loaded;

    // 传进来的是指针，先确认不是空指针，否则写 *font 会崩溃
    if (font == NULL) return 0;

    // 先检查文件在不在，这样能给出"缺字体文件"这种明确的原因
    if (!FileExists(UI_FONT_PATH))
    {
        TraceLog(LOG_ERROR, "UI: font file not found: %s", UI_FONT_PATH);
        return 0;
    }

    count = buildCodepoints(UI_GLYPHS, codepoints, UI_CODEPOINT_MAX);
    if (count <= 0)
    {
        TraceLog(LOG_ERROR, "UI: failed to build codepoint set");
        return 0;
    }

    // LoadFontEx（带 Ex）才是"按码点加载"的版本：
    //   第一个参数：字体文件路径
    //   第二个参数：生成多大的字
    //   第三个参数：需要哪些字的码点
    //   第四个参数：码点个数
    // 如果用 LoadFont（不带 Ex），只会加载默认 ASCII 字形，中文画不出来
    loaded = LoadFontEx(UI_FONT_PATH, UI_FONT_SIZE, codepoints, count);

    // 判断是否真的加载成功：
    //   texture.id == 0 表示字体纹理没建出来
    //   glyphCount <= 0 表示一个字形都没有
    if (loaded.texture.id == 0 || loaded.glyphCount <= 0)
    {
        TraceLog(LOG_ERROR, "UI: failed to load font: %s", UI_FONT_PATH);

        // 失败也要把自己刚申请的资源释放掉，不能留给调用方收拾
        if (loaded.texture.id != 0) UnloadFont(loaded);
        return 0;
    }

    // 双线性过滤：字被放大缩小时边缘更平滑，不会一格一格的
    SetTextureFilter(loaded.texture, TEXTURE_FILTER_BILINEAR);

    // 检查有没有缺字
    // raylib 遇到字体里没有的码点不会报错，只会让它画成乱码，
    // 所以这里主动对一遍：requested 的每个码点，是否真的进了 font.glyphs
    // 注意不能用 GetGlyphIndex() == 0 来判断，因为 0 号槽位也可能放着真实的字
    {
        int missing = 0;
        int shown = 0;
        int a;
        int b;

        for (a = 0; a < count; a++)
        {
            int found = 0;

            for (b = 0; b < loaded.glyphCount; b++)
            {
                if (loaded.glyphs[b].value == codepoints[a])
                {
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                missing++;

                if (shown < 10)
                {
                    TraceLog(LOG_WARNING, "  missing codepoint %d (U+%04X)", codepoints[a], codepoints[a]);
                    shown++;
                }
            }
        }

        if (missing > 0)
        {
            TraceLog(LOG_WARNING, "UI: %d glyphs missing, add them to UI_GLYPHS", missing);
        }
    }

    // 只有全部成功，才把结果交给调用方
    *font = loaded;
    return 1;
}

/*
 * 功能：关闭窗口前释放界面字体
 * 输入：font 为加载成功的字体
 * 输出：释放字体资源
 * 返回值：无
 */
void unloadUiResources(Font font)
{
    // id 为 0 说明这个字体根本没加载成功，不能去释放，
    // 否则 raylib 内部会访问无效资源
    if (font.texture.id != 0) UnloadFont(font);
}

/*
 * 功能：判断本帧是否点击按钮
 * 输入：bounds 是按钮矩形区域
 * 输出：无，不绘制按钮
 * 返回值：1 本帧左键在按钮区域内按下，0 没有点击
 */
int isButtonClicked(Rectangle bounds)
{
    // IsMouseButtonPressed ：按下那一刻返回 true，只在这一帧成立
    // IsMouseButtonDown    ：按住期间每帧都返回 true
    //
    // 这里必须用 Pressed，如果用 Down，按住不放会连续触发几十次，
    // 玩家一按就可能连跳两个页面
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return 0;

    // GetMousePosition 返回当前鼠标位置（Vector2）
    // CheckCollisionPointRec 判断这个点在不在矩形里
    return CheckCollisionPointRec(GetMousePosition(), bounds) ? 1 : 0;
}

/*
 * 功能：绘制统一样式的按钮
 * 输入：bounds 为区域，text 为 UTF-8 文字，font 为字体
 * 输出：矩形按钮、边框、文字和悬停效果，不触发任何操作
 * 返回值：无
 */
void drawButton(Rectangle bounds, const char text[], Font font)
{
    Vector2 size;
    Vector2 pos;
    int hover;
    Color fill;
    Color border;

    // 鼠标在按钮上就叫"悬停"，用它来换一套颜色，做出反馈感
    hover = CheckCollisionPointRec(GetMousePosition(), bounds);

    fill = hover ? (Color){ 222, 236, 252, 255 } : (Color){ 246, 248, 252, 255 };
    border = hover ? (Color){ 70, 130, 190, 255 } : (Color){ 150, 165, 185, 255 };

    // 先铺底色，再描边
    DrawRectangleRec(bounds, fill);
    DrawRectangleLinesEx(bounds, hover ? 2.0f : 1.0f, border);

    // 没有文字或者字体没加载成功时，只画一个空按钮，不要崩溃
    if (text == NULL || font.texture.id == 0) return;

    // 文字居中：先量出文字尺寸，再用 (按钮宽 - 文字宽) / 2 算偏移
    size = MeasureTextEx(font, text, 20.0f, 1.0f);
    pos.x = bounds.x + (bounds.width - size.x) / 2.0f;
    pos.y = bounds.y + (bounds.height - size.y) / 2.0f;

    // 中文必须用 DrawTextEx（能用自定义字体），不能用 DrawText（只有 ASCII）
    DrawTextEx(font, text, pos, 20.0f, 1.0f, (Color){ 30, 40, 55, 255 });
}
