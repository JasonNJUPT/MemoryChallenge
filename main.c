/*========================================================================
 * 文件名：main.c
 * 功  能：记忆力大考验的主程序。负责显示各级菜单、完成注册与登录、
 *         选择难度后调用游戏模块（B 负责）和排行榜模块（C 负责）。
 * 说  明：本文件只调用 game.h 与 rank.h 中约定的三个对外接口，函数体由
 *         B、C 两位同学分别提供：
 *             int  playGame(int level, int *seconds);
 *             int  saveScore(const char name[], int level, int seconds);
 *             void showRank(int level);
 *         难度 level 统一用 1、2、3 表示初、中、高级，只有各模块内部
 *         访问数组时才减 1。
 *========================================================================*/
#include <windows.h>    /* Sleep：显示榜单后暂停三秒 */
#include <stdio.h>      /* printf、scanf 等输入输出函数          */
#include <time.h>       /* time：初始化随机种子 */
#include <stdlib.h>     /* exit 函数                             */
#include "account.h"    /* 账号模块：User 以及注册、登录接口      */
#include "game.h"       /* 游戏模块接口：playGame                 */
#include "rank.h"       /* 排行榜模块接口：saveScore、showRank     */

#define LOGIN_EXIT     0    /* 登录菜单：退出程序     */
#define LOGIN_REGISTER 1    /* 登录菜单：注册新用户   */
#define LOGIN_SIGNIN   2    /* 登录菜单：登录         */
#define LOGIN_MAX      2    /* 登录菜单的最大编号     */

#define PLAYER_EXIT    0    /* 玩家菜单：退出程序     */
#define PLAYER_PLAY    1    /* 玩家菜单：开始游戏     */
#define PLAYER_RANK    2    /* 玩家菜单：查看排行榜   */
#define PLAYER_SWITCH  3    /* 玩家菜单：切换账号     */
#define PLAYER_MAX     3    /* 玩家菜单的最大编号     */

/* 以下是本文件内部的辅助函数，只在本文件可见 */
static int  readInt(const char *prompt, int min, int max);
static void readName(char name[]);
static void readPassword(char password[]);
static void printWelcome(void);
static void printLoginMenu(void);
static void printPlayerMenu(const char name[]);
static const char *levelName(int level);
static int  chooseLevel(void);
static void doRegister(void);
static int  doLogin(User *user);
static void doPlayGame(const User *user);
static void doShowRank(void);

/*------------------------------------------------------------------------
 * 函数：main
 * 功  能：程序入口。先停留在“注册 / 登录”菜单，登录成功后进入玩家菜单；
 *         玩家菜单里可以开始游戏、查看榜单、切换账号或退出。
 *------------------------------------------------------------------------*/
int main(void)
{
    User user;          /* 当前登录成功的用户（未登录时内容无效） */
    int choice;         /* 用户在菜单里选择的编号                 */

    srand((unsigned int)time(NULL));  /* 整个程序只设置一次随机种子。 */
    printWelcome();     /* 显示欢迎信息，只显示一次 */

    while (1)
    {
        /* ---------- 第一层：登录入口（新用户必须先注册） ---------- */
        printLoginMenu();
        choice = readInt("请选择：", LOGIN_EXIT, LOGIN_MAX);

        if (choice == LOGIN_EXIT)               /* 退出程序 */
        {
            break;
        }
        if (choice == LOGIN_REGISTER)           /* 注册后回到登录入口 */
        {
            doRegister();
            continue;
        }
        if (doLogin(&user) != 1)                /* 登录失败回到登录入口 */
        {
            continue;
        }

        /* ---------- 第二层：玩家菜单（仅在登录成功后出现） ---------- */
        while (1)
        {
            printPlayerMenu(user.name);
            choice = readInt("请选择：", PLAYER_EXIT, PLAYER_MAX);

            if (choice == PLAYER_EXIT)          /* 退出整个程序 */
            {
                printf("感谢使用，再见！\n");
                return 0;
            }
            if (choice == PLAYER_PLAY)          /* 选择难度并开始一局 */
            {
                doPlayGame(&user);
                continue;
            }
            if (choice == PLAYER_RANK)          /* 选择难度并查看榜单 */
            {
                doShowRank();
                continue;
            }
            if (choice == PLAYER_SWITCH)        /* 切换账号：回到登录入口 */
            {
                printf("已退出当前账号，请重新登录。\n");
                break;
            }
        }
    }

    printf("感谢使用，再见！\n");
    return 0;
}

/*------------------------------------------------------------------------
 * 函数：readInt
 * 功  能：显示提示并读入一个限定范围内的整数，输入不合法时重新提示。
 * 参  数：prompt —— 提示文字；min、max —— 允许的最小值、最大值。
 * 返  回：读到的合法整数；输入流结束（EOF）时显示提示后退出程序。
 *------------------------------------------------------------------------*/
/* 简单读取整数：只处理范围，非整数或输入结束时退出。 */
static int readInt(const char *prompt, int min, int max)
{
    int value;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &value) != 1) {
            printf("输入结束或格式不正确，程序退出。\n");
            exit(0);
        }
        if (value >= min && value <= max) {
            return value;
        }
        printf("请输入 %d 到 %d 之间的整数。\n", min, max);
    }
}

/* 输入用户名，最多 31 字节；输入结束时退出程序。 */
static void readName(char name[])
{
    printf("请输入用户名（不含空格，最多 31 字节）：");
    if (scanf("%31s", name) != 1) {
        exit(0);
    }
}

/* 输入密码，最多 63 字节；输入结束时退出程序。 */
static void readPassword(char password[])
{
    printf("请输入密码（不含空格，最多 63 字节）：");
    if (scanf("%63s", password) != 1) {
        exit(0);
    }
}

/*------------------------------------------------------------------------
 * 函数：printWelcome
 * 功  能：显示程序名称和欢迎语。
 *------------------------------------------------------------------------*/
static void printWelcome(void)
{
    printf("\n");
    printf("******************************************\n");
    printf("*            记 忆 力 大 考 验            *\n");
    printf("*   找出相同图案完成配对，用时越短越好   *\n");
    printf("******************************************\n");
    printf("提示：首次使用请先注册，注册时会检测密码安全等级。\n");
}

/*------------------------------------------------------------------------
 * 函数：printLoginMenu
 * 功  能：显示“注册 / 登录”菜单。
 *------------------------------------------------------------------------*/
static void printLoginMenu(void)
{
    printf("\n------------------------------------------\n");
    printf("  1. 注册新用户\n");
    printf("  2. 登录\n");
    printf("  0. 退出程序\n");
    printf("------------------------------------------\n");
}

/*------------------------------------------------------------------------
 * 函数：printPlayerMenu
 * 功  能：显示登录成功后的玩家菜单，并显示当前用户。
 * 参  数：name —— 当前登录的用户名。
 *------------------------------------------------------------------------*/
static void printPlayerMenu(const char name[])
{
    printf("\n==========================================\n");
    printf("  当前用户：%s\n", name);
    printf("==========================================\n");
    printf("  1. 开始游戏\n");
    printf("  2. 查看排行榜\n");
    printf("  3. 切换账号\n");
    printf("  0. 退出程序\n");
    printf("------------------------------------------\n");
}

/*------------------------------------------------------------------------
 * 函数：levelName
 * 功  能：把难度编号翻译成中文名称，供各个菜单和提示文字使用。
 * 参  数：level —— 难度编号 1、2、3。
 * 返  回：对应的名称字符串；编号不合法时返回“未知级别”。
 *------------------------------------------------------------------------*/
static const char *levelName(int level)
{
    if (level == 1)
    {
        return "初级";
    }
    if (level == 2)
    {
        return "中级";
    }
    if (level == 3)
    {
        return "高级";
    }
    return "未知级别";
}

/*------------------------------------------------------------------------
 * 函数：chooseLevel
 * 功  能：显示难度菜单并读入难度编号。
 * 返  回：1 初级、2 中级、3 高级；0 表示用户取消了选择。
 *------------------------------------------------------------------------*/
static int chooseLevel(void)
{
    int level;          /* 用户选择的难度编号 */

    printf("\n---------- 选择难度 ----------\n");
    printf("  1. 初级（4x4，8 对图案）\n");
    printf("  2. 中级（4x5，10 对图案）\n");
    printf("  3. 高级（6x6，12 组图案，每组 3 张）\n");
    printf("  0. 返回玩家菜单\n");
    printf("------------------------------\n");

    level = readInt("请输入难度编号：", 0, 3);
    return level;
}

/*------------------------------------------------------------------------
 * 函数：doRegister
 * 功  能：完成一次注册：读入用户名和密码、显示密码安全等级、
 *         再调用 registerUser 写入账号文件。
 *------------------------------------------------------------------------*/
static void doRegister(void)
{
    User user;          /* 等待注册的新用户         */
    int safety;         /* 密码安全等级：0/1/2      */
    int choice;         /* 密码不安全时用户的选择   */

    printf("\n---------- 注册新用户 ----------\n");
    readName(user.name);
    readPassword(user.password);

    /* 注册时先给出密码安全测试结果，让用户知道密码是否足够安全 */
    safety = checkPassword(user.password);
    if (safety == 0)
    {
        printf("密码安全等级：不安全（少于 6 位，或只含一类字符）。\n");
        choice = readInt("是否仍要使用该密码注册？1=继续 0=取消：", 0, 1);
        if (choice == 0)
        {
            printf("已取消注册，请重新选择注册。\n");
            return;
        }
    }
    else if (safety == 1)
    {
        printf("密码安全等级：中度安全（包含两类字符）。\n");
    }
    else
    {
        printf("密码安全等级：安全（包含三类及以上字符）。\n");
    }

    if (registerUser(user) == 1)
    {
        printf("注册成功，请回到登录入口使用该账号登录。\n");
    }
    else
    {
        printf("注册失败：用户名已存在，或账号文件读写失败。\n");
    }
}

/*------------------------------------------------------------------------
 * 函数：doLogin
 * 功  能：读入用户名和密码并验证身份。
 * 参  数：user —— 登录成功后用来保存当前用户信息的变量地址。
 * 返  回：1 登录成功；0 登录失败（用户名不存在或密码错误）。
 *------------------------------------------------------------------------*/
static int doLogin(User *user)
{
    printf("\n---------- 用户登录 ----------\n");
    readName(user->name);
    readPassword(user->password);

    if (loginUser(*user) == 1)
    {
        printf("登录成功，欢迎你，%s！\n", user->name);
        return 1;
    }

    printf("登录失败：用户名或密码不正确。\n");
    return 0;
}

/*------------------------------------------------------------------------
 * 函数：doPlayGame
 * 功  能：选好难度后开始一局游戏，通关时把成绩交给排行榜模块保存。
 * 参  数：user —— 当前登录用户，用于给成绩署名。
 * 说  明：playGame 返回 1 表示通关，此时 *seconds 是有效用时；
 *         返回 0 表示中途退出或输入结束，本局不产生可入榜成绩。
 *         saveScore 返回 1 已更新、0 未刷新、-1 读写出错。
 *------------------------------------------------------------------------*/
static void doPlayGame(const User *user)
{
    int level;          /* 本局难度：1 初级 / 2 中级 / 3 高级      */
    int seconds = 0;    /* 本局用时（秒），只有通关时才有效         */
    int result;         /* saveScore 的返回值                      */

    level = chooseLevel();
    if (level == 0)                         /* 用户取消选择 */
    {
        printf("已返回玩家菜单。\n");
        return;
    }

    printf("\n开始%s测试，看清牌面后按提示输入坐标。\n", levelName(level));

    if (playGame(level, &seconds) != 1)     /* 没有通关：不保存成绩 */
    {
        printf("本局没有完成，本次不计成绩。\n");
        return;
    }

    printf("\n恭喜通关！本局用时 %d 秒。\n", seconds);

    result = saveScore(user->name, level, seconds);   /* 成绩交给排行榜模块 */

    if (result == 1)
    {
        printf("成绩已保存到%s排行榜（%s：%d 秒）。\n",
               levelName(level), user->name, seconds);
    }
    else if (result == 0)
    {
        printf("本次未刷新个人最好成绩或未进入%s前五名，榜单未变化。\n",
               levelName(level));
    }
    else
    {
        printf("成绩文件读写出错，本次成绩未能保存。\n");
    }
}

/*------------------------------------------------------------------------
 * 函数：doShowRank
 * 功  能：选择难度后显示该级别的排行榜（前五名）。
 * 说  明：名次、姓名、用时的打印由排行榜模块的 showRank 完成。
 *------------------------------------------------------------------------*/
static void doShowRank(void)
{
    int level;
    int status = hasRankData();

    /* 全部榜单为空时，不再让玩家选择没有成绩的级别。 */
    if (status == 0) {
        printf("暂无排行榜成绩，请先完成一局游戏。\n");
        Sleep(3000);  /* 留出阅读提示的时间，再返回玩家菜单。 */
        return;
    }
    if (status == -1) {
        printf("成绩文件读取失败，暂时无法查看排行榜。\n");
        return;
    }

    printf("\n---------- 选择排行榜 ----------\n");
    printf("  1. 初级排行榜\n");
    printf("  2. 中级排行榜\n");
    printf("  3. 高级排行榜\n");
    printf("  0. 返回玩家菜单\n");
    level = readInt("请选择要查看的排行榜：", 0, 3);
    if (level == 0) {
        return;
    }
    showRank(level);
    Sleep(3000);  /* 榜单显示后暂停三秒，再显示下一次菜单。 */
}
