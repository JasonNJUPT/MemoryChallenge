// 使用 raylib 自行绘制几何卡牌，无需下载素材；不是游戏启动时调用的代码
#include <math.h>
#include "raylib.h"

// 将闭合多边形分成三角形，points 按顺时针排列
static void polygon(Vector2 center, Vector2 points[], int count, Color color)
{
    int i;
    for (i = 0; i < count; i++) DrawTriangle(center, points[(i + 1) % count], points[i], color);
}

/*
 * 功能：按编号绘制不同轮廓与颜色的几何牌面
 * 输入：id 为 0 到 11，使用 512x512 的绘制坐标
 * 输出：图案画面
 * 返回值：无
 */
static void drawFace(int id)
{
    int i;
    Vector2 points[10];
    Color colors[12] = {(Color){0, 150, 145, 255}, BLUE, ORANGE, PURPLE, GOLD, DARKGREEN,
                        MAROON, SKYBLUE, DARKPURPLE, PINK, (Color){242, 178, 0, 255}, LIME};
    Color color = colors[id];
    Vector2 center = {256, 256};
    switch (id) {
        case 0:
            DrawRectangle(126, 94, 260, 16, color);
            DrawTriangle((Vector2){130, 110}, (Vector2){256, 256}, (Vector2){382, 110}, color);
            DrawTriangle((Vector2){256, 256}, (Vector2){130, 402}, (Vector2){382, 402}, color);
            DrawRectangle(126, 402, 260, 16, color);
            break;
        case 1: DrawPoly(center, 3, 166, -90, color); break;
        case 2: DrawRectangle(124, 124, 264, 264, color); break;
        case 3: DrawPoly(center, 4, 170, 0, color); break;
        case 4:
            for (i = 0; i < 10; i++) {
                float angle = (-90.0f + i * 36.0f) * DEG2RAD;
                float radius = i % 2 == 0 ? 170 : 74;
                points[i] = (Vector2){256 + cosf(angle) * radius, 256 + sinf(angle) * radius};
            }
            polygon(center, points, 10, color);
            break;
        case 5: DrawPoly(center, 6, 155, 0, color); break;
        case 6:
            DrawRectangle(210, 105, 92, 302, color);
            DrawRectangle(105, 210, 302, 92, color);
            break;
        case 7:
            DrawCircleV(center, 150, color);
            DrawCircleV(center, 89, WHITE);
            break;
        case 8:
            DrawCircleV(center, 150, color);
            DrawCircle(320, 205, 133, WHITE);
            break;
        case 9:
            DrawCircle(185, 190, 86, color);
            DrawCircle(327, 190, 86, color);
            DrawTriangle((Vector2){99, 205}, (Vector2){256, 405}, (Vector2){413, 205}, color);
            break;
        case 10:
            points[0]=(Vector2){275,80};points[1]=(Vector2){135,280};
            points[2]=(Vector2){235,280};points[3]=(Vector2){215,435};
            points[4]=(Vector2){380,215};points[5]=(Vector2){270,215};
            // 闪电拆成两个三角形
            DrawTriangle(points[0], points[1], points[2], color);
            DrawTriangle(points[0], points[2], points[5], color);
            DrawTriangle(points[2], points[3], points[4], color);
            DrawTriangle(points[2], points[4], points[5], color);
            break;
        case 11:
            for (i = 0; i < 6; i++) {
                float angle = i * 60 * DEG2RAD;
                DrawCircle(256 + (int)(92 * cosf(angle)), 256 + (int)(92 * sinf(angle)), 66, color);
            }
            DrawCircleV(center, 54, GOLD);
            break;
    }
}

/*
 * 功能：导出 12 张正面和统一卡背，每张为 512x512 PNG
 * 输入：无，运行目录应为项目目录，assets/cards 必须已创建
 * 输出：assets/cards/00.png 到 11.png 及 back.png
 * 返回值：0 成功，1 导出失败
 */
int main(void)
{
    int id, ok = 1;
    Image image;
    Camera2D camera = {0};
    RenderTexture2D canvas;
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(64, 64, "Original card asset generator");
    canvas = LoadRenderTexture(1024, 1024);
    camera.zoom = 2;
    for (id = 0; id <= 12; id++) {
        BeginTextureMode(canvas);
        ClearBackground(WHITE);
        BeginMode2D(camera);
        if (id < 12) {
            drawFace(id);
        } else {
            DrawRectangle(0, 0, 512, 512, (Color){42, 75, 125, 255});
            DrawRectangleLinesEx((Rectangle){22, 22, 468, 468}, 5, (Color){155, 194, 225, 255});
            DrawPoly((Vector2){256, 256}, 4, 110, 0, (Color){155, 194, 225, 255});
            DrawPoly((Vector2){256, 256}, 4, 75, 0, (Color){42, 75, 125, 255});
        }
        EndMode2D();
        EndTextureMode();
        image = LoadImageFromTexture(canvas.texture);
        ImageFlipVertical(&image);
        ImageResize(&image, 512, 512);
        if (!ExportImage(image, id == 12 ? "assets/cards/back.png" : TextFormat("assets/cards/%02d.png", id))) ok = 0;
        UnloadImage(image);
    }
    UnloadRenderTexture(canvas);
    CloseWindow();
    return ok ? 0 : 1;
}
