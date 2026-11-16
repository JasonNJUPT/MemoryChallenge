/*==============================================================
  rank.c --- 成绩排行模块（任务 C）

  只依赖标准库：stdio.h（文件与打印）、string.h（字符串）、stdlib.h（测试用）

  内存结构：
      g_rank[级别][名次]  存成绩，级别用 0/1/2 对应 初/中/高
      g_count[级别]       该级别目前有几条记录（0~5）

  文件结构（ranks.txt，文本、一行一条）：
      级别 用户名 秒数
      1 alice 30
      1 bob 18
      3 carol 52

  三条规则（来自分工文档）：
      1. 同一玩家只保留较快的那次成绩；
      2. 用时相同时，先入榜的排在前面（所以插位置时用严格大于）；
      3. 每级最多 5 条，写回时最多 15 行。
==============================================================*/

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "rank.h"

/*------------------------- 内部数据 -------------------------*/
typedef struct
{
    char name[RANK_NAME];
    int  seconds;
} Score;

static Score g_rank[RANK_LEVELS][RANK_TOP];                   /* 三个级别，每级 5 个位置 */
static int   g_count[RANK_LEVELS];                            /* 每个级别现在存了几条（0~5） */

static int   g_readError = 0;  /* 文件读取失败时阻止覆盖旧成绩。 */
static int   g_loaded = 0;      /* 是否已经从文件读过一次 */

/*--------------------------------------------------------------
  级别编号 -> 中文名（只用于显示）
--------------------------------------------------------------*/
static const char *levelName(int level)
{
    if (level == 1) return "初级";
    if (level == 2) return "中级";
    if (level == 3) return "高级";
    return "未知";
}

/*--------------------------------------------------------------
  私有函数：把一个级别里的记录按用时从小到大排序。
  用插入排序（稳定的：用时相同不会交换顺序），
  主要是防止有人手工改过 ranks.txt 之后顺序乱掉。
--------------------------------------------------------------*/
static void sortLevel(int k)
{
    int i, j;
    Score t;

    for (i = 1; i < g_count[k]; i++) {
        t = g_rank[k][i];
        j = i;
        while (j > 0 && g_rank[k][j - 1].seconds > t.seconds) {  /* 严格大于：同秒不交换，保持稳定 */
            g_rank[k][j] = g_rank[k][j - 1];
            j--;
        }
        g_rank[k][j] = t;
    }
}

/*--------------------------------------------------------------
  私有函数：从 ranks.txt 读入全部成绩。
  文件不存在（第一次运行）当作空榜，不算错误。
--------------------------------------------------------------*/
static void loadRanks(void)
{
    FILE *fp;
    char line[128];
    char nm[RANK_NAME];
    int  lv, sec, k;

    g_readError = 0;
    /* 先把内存清空 */
    for (k = 0; k < RANK_LEVELS; k++) {
        g_count[k] = 0;
        memset(g_rank[k], 0, sizeof(g_rank[k]));
    }

    fp = fopen(RANK_FILE, "r");                               /* 只读打开 */
    if (fp == NULL) {
        g_readError = (errno != ENOENT);
        g_loaded = 1;           /* 只有文件不存在才按空榜处理。 */
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "%d %31s %d", &lv, nm, &sec) != 3)   /* 不是三段就说明这行不合法 */
            continue;
        if (lv < 1 || lv > RANK_LEVELS)                       /* 级别越界，丢弃 */
            continue;
        if (sec < 0)                                         /* 负用时无效，丢弃 */
            continue;

        k = lv - 1;
        if (g_count[k] >= RANK_TOP)     /* 文件里超过 5 条的，多出来的丢弃 */
            continue;

        strncpy(g_rank[k][g_count[k]].name, nm, RANK_NAME - 1);  /* 最多复制 31 字节 */
        g_rank[k][g_count[k]].name[RANK_NAME - 1] = '\0';     /* strncpy 不补结束符，手动补 */
        g_rank[k][g_count[k]].seconds = sec;
        g_count[k]++;
    }

    if (ferror(fp)) {
        g_readError = 1;
    }
    fclose(fp);

    /* 读进来后再整理一次：排序 + 截断到 5 条 */
    for (k = 0; k < RANK_LEVELS; k++) {
        if (g_count[k] > RANK_TOP)
            g_count[k] = RANK_TOP;
        sortLevel(k);
    }

    g_loaded = 1;
}

/*--------------------------------------------------------------
  私有函数：把内存里的成绩整体写回文件。
  成功返回 1，失败返回 0。
--------------------------------------------------------------*/
static int writeRanks(void)
{
    FILE *fp;
    int k, i;

    fp = fopen(RANK_FILE, "w");
    if (fp == NULL)
        return 0;

    for (k = 0; k < RANK_LEVELS; k++) {                        /* 一级一级写 */
        for (i = 0; i < g_count[k]; i++) {
            if (fprintf(fp, "%d %s %d\n", k + 1,             /* 级别写 1/2/3，方便人看 */
                        g_rank[k][i].name, g_rank[k][i].seconds) < 0) {
                fclose(fp);
                return 0;
            }
        }
    }

    if (fclose(fp) != 0)        /* 关闭失败也要当作写失败 */
        return 0;

    return 1;
}

/*==============================================================
  对外的第一个接口：提交成绩
==============================================================*/
int saveScore(const char name[], int level, int seconds)
{
    int k, i, pos, cnt, dup;

    /* ---- 1. 检查参数 ---- */
    if (name == NULL || name[0] == '\0')
        return -1;
    if (level < 1 || level > RANK_LEVELS)
        return -1;
    if (seconds < 0)           /* 允许不足一秒的通关用时为 0，退出由返回值区分。 */
        return -1;

    if (!g_loaded)                                            /* 第一次用到才读文件 */
        loadRanks();

    if (g_readError) {
        return -1;
    }
    k = level - 1;              /* 接口是 1/2/3，数组下标才减 1 */

    /* ---- 2. 找同名玩家 ---- */
    dup = -1;                                                 /* -1 表示榜上还没有这个名字 */
    for (i = 0; i < g_count[k]; i++) {
        if (strcmp(g_rank[k][i].name, name) == 0) {
            dup = i;
            break;
        }
    }

    /* 同名：只有更快才允许替换；一样快或更慢都算没刷新 */
    if (dup >= 0 && seconds >= g_rank[k][dup].seconds)
        return 0;

    /* ---- 3. 找插入位置 ----
       第一个"用时比自己大"的位置就是该插入的地方。
       用严格大于，所以用时相同的后来者会排在后面。            */
    pos = 0;
    while (pos < g_count[k] && g_rank[k][pos].seconds <= seconds)
        pos++;

    /* 榜已满，且比第五名还慢（或一样慢）：进不了榜 */
    if (pos >= RANK_TOP)
        return 0;

    /* ---- 4. 若同名玩家已有旧记录，先删掉，避免同一个人占两行 ---- */
    if (dup >= 0) {
        for (i = dup; i < g_count[k] - 1; i++)                /* 后面的往前盖一格，等于删掉 dup */
            g_rank[k][i] = g_rank[k][i + 1];
        g_count[k]--;                                         /* 删掉一条，条数减一 */
        if (pos > dup)          /* 删掉前面一条之后，插入位置要往前挪一格 */
            pos--;
    }

    /* ---- 5. 插入新记录：从后往前挪出空位 ---- */
    cnt = g_count[k];                                         /* 当前条数 */
    if (cnt < RANK_TOP)                                       /* 没满就多一条；满了让最后一名被顶掉 */
        cnt++;

    for (i = cnt - 1; i > pos; i--)                           /* 从末尾往前挪，空出 pos 这一格 */
        g_rank[k][i] = g_rank[k][i - 1];

    strncpy(g_rank[k][pos].name, name, RANK_NAME - 1);         /* 最多复制 31 字节 */
    g_rank[k][pos].name[RANK_NAME - 1] = '\0';                /* 手动补结束符 */
    g_rank[k][pos].seconds = seconds;                         /* 新成绩就位 */
    g_count[k] = cnt;                                         /* 更新条数 */

    /* ---- 6. 写回文件 ---- */
    if (!writeRanks())                                        /* 内存改了不算数，落盘成功才算 */
        return -1;

    return 1;
}

/*==============================================================
  对外的第二个接口：显示某个级别的排行榜
==============================================================*/
/*
 * 功能：供菜单检查三个难度是否至少有一条成绩。
 * 输入：无。输出：不修改榜单。
 * 返回：1 有成绩，0 全部为空，-1 文件读取失败。
 */
int hasRankData(void)
{
    int k;
    if (!g_loaded) {
        loadRanks();
    }
    if (g_readError) {
        return -1;
    }
    for (k = 0; k < RANK_LEVELS; k++) {
        if (g_count[k] > 0) {
            return 1;
        }
    }
    return 0;
}

void showRank(int level)
{
    int k, i;

    if (level < 1 || level > RANK_LEVELS) {
        printf("难度参数错误（只能是 1、2、3）。\n");
        return;
    }

    if (!g_loaded)                                            /* 第一次用到才读文件 */
        loadRanks();

    if (g_readError) {
        printf("成绩文件读取失败。\n");
        return;
    }
    k = level - 1;

    printf("\n===== %s 排行榜（前五名）=====\n", levelName(level));

    if (g_count[k] == 0) {
        printf("  暂无成绩。\n");
        return;
    }

    printf("  名次   姓名                 用时\n");
    printf("  --------------------------------\n");
    for (i = 0; i < g_count[k]; i++) {
        printf("  %2d    %-16s %4d 秒\n",                      /* %-16s 左对齐；中文按字节补空格，会略偏 */
               i + 1, g_rank[k][i].name, g_rank[k][i].seconds);
    }
}

/*==============================================================
  自测入口（只在编译时加 /DRANK_TEST 才参与编译）

  编译方法：
      cl /DRANK_TEST /Fe:rank_test.exe rank.c
  或
      gcc -DRANK_TEST -o rank_test.exe rank.c

  测试用例对应分工文档里的要求：
      1. 初级依次传入 30、18、25、40、22、35 → 前五应为 18、22、25、30、35
      2. 原本 30 秒的玩家再传 16 秒 → 升到第一，且只出现一次
      3. 同名玩家传更慢的成绩 → 返回 0，榜单不变
      4. 两人同分 → 先入榜的在前
      5. 中级单独写入 → 不影响初级
      6. 文件不存在 → 空榜
      7. 文件里有坏行 → 跳过，不影响其它记录
      8. 非法参数 → 返回 -1
      9. 超长用户名 → 截断且不越界
==============================================================*/
