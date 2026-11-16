/* main.c：账号入口、玩家菜单以及三个模块之间的连接。 */
#include <stdio.h>
#include <stdlib.h>     /* srand、exit */
#include <time.h>       /* time */
#include <windows.h>    /* Sleep，排行榜显示后暂停三秒 */
#include "account.h"
#include "game.h"
#include "rank.h"

#define LOGIN_EXIT     0
#define LOGIN_REGISTER 1
#define LOGIN_SIGNIN   2

/* 工具函数：读菜单编号、账号信息和游戏难度。 */

static int readInt(const char prompt[], int min, int max);
static void readUser(User *user);
static int chooseLevel(void);
/* 流程函数：注册、登录、玩家菜单、游戏、排行榜。 */

static void doRegister(void);
static int doLogin(User *user);
static int playerMenu(User user);
static void doPlayGame(User user);
static void doShowRank(void);

/*
 * 功能：程序入口，显示注册、登录和退出菜单，登录后进入玩家菜单。
 * 输入：无参数；通过键盘读取菜单选择和账号信息。
 * 输出：显示菜单及操作提示。
 * 返回值：0，表示程序正常结束。
 */
int main(void)
{
    User user;
    int running = 1;  /* 1 继续运行，0 结束程序。 */

    srand((unsigned int)time(NULL));
    printf("\n********** 记忆力大考验 **********\n");
    printf("首次使用请先注册，登录后开始游戏。\n");
    while (running) {
        printf("\n1. 注册新用户\n2. 登录\n0. 退出程序\n");
        switch (readInt("请选择：", 0, 2)) {
            case LOGIN_REGISTER:
                doRegister();
                break;
            case LOGIN_SIGNIN:
                if (doLogin(&user) == 1) {
                    running = playerMenu(user);
                }
                break;
            case LOGIN_EXIT:
                running = 0;
                break;
        }
    }
    printf("感谢使用，再见！\n");
    return 0;
}

/*
 * 功能：读取指定范围内的整数，用于选择菜单选项。
 * 输入：prompt 为提示文字；min、max 为允许输入的最小值和最大值。
 * 输出：显示提示；超出范围时提示重新输入。
 * 返回值：用户输入的有效整数。
 * 说明：未读到整数时直接退出程序，不清理输入缓冲区。
 */
static int readInt(const char prompt[], int min, int max)
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

/*
 * 功能：读取用户名和密码，供注册和登录使用。
 * 输入：user 为用于接收账号信息的结构体地址；通过键盘输入用户名和密码。
 * 输出：将用户名写入 user->name，将密码写入 user->password。
 * 返回值：无。
 * 说明：用户名最多 31 字节，密码最多 63 字节，均不能含空格；读取失败时退出程序。
 */
static void readUser(User *user)
{
    printf("请输入用户名（不含空格，最多 31 字节）：");
    if (scanf("%31s", user->name) != 1) {
        exit(0);
    }
    printf("请输入密码（不含空格，最多 63 字节）：");
    if (scanf("%63s", user->password) != 1) {
        exit(0);
    }
}

/*
 * 功能：完成用户注册，检查密码安全等级并调用账号模块保存账号。
 * 输入：无参数；通过键盘输入账号信息，密码不安全时选择继续或取消。
 * 输出：显示密码安全等级和注册结果；注册成功时由账号模块写入账号文件。
 * 返回值：无。
 */
static void doRegister(void)
{
    User user;
    int safety;
    printf("\n---------- 注册新用户 ----------\n");
    readUser(&user);
    safety = checkPassword(user.password);
    if (safety == 0) {
        printf("密码安全等级：不安全（少于 6 位，或只含一类字符）。\n");
        if (readInt("是否仍要使用该密码注册？1=继续 0=取消：", 0, 1) == 0) {
            printf("已取消注册。\n");
            return;
        }
    } else if (safety == 1) {
        printf("密码安全等级：中度安全（包含两类字符）。\n");
    } else {
        printf("密码安全等级：安全（包含三类及以上字符）。\n");
    }
    if (registerUser(user) == 1) {
        printf("注册成功，请使用该账号登录。\n");
    } else {
        printf("注册失败：用户名已存在、账号已满或文件读写失败。\n");
    }
}

/*
 * 功能：读取账号信息，调用账号模块检查用户名和密码。
 * 输入：user 为用于接收本次登录账号信息的结构体地址。
 * 输出：通过 *user 保存输入的账号信息，并显示登录结果。
 * 返回值：1 表示登录成功；0 表示登录失败。
 * 说明：只有返回 1 时，主函数才进入玩家菜单。
 */
static int doLogin(User *user)
{
    printf("\n---------- 用户登录 ----------\n");
    readUser(user);
    if (loginUser(*user) == 1) {
        printf("登录成功，欢迎你，%s！\n", user->name);
        return 1;
    }
    printf("登录失败：用户名或密码不正确。\n");
    return 0;
}

/*
 * 功能：显示登录后的玩家菜单，连接游戏和排行榜功能。
 * 输入：user 为当前已登录用户的账号信息。
 * 输出：显示当前用户名、菜单和操作提示。
 * 返回值：1 表示切换账号，返回登录菜单；0 表示退出整个程序。
 * 说明：游戏或排行榜操作结束后，继续显示玩家菜单。
 */
static int playerMenu(User user)
{
    while (1) {
        printf("\n========== 当前用户：%s ==========\n", user.name);
        printf("1. 开始游戏\n2. 查看排行榜\n3. 切换账号\n0. 退出程序\n");
        switch (readInt("请选择：", 0, 3)) {
            case 1:
                doPlayGame(user);
                break;
            case 2:
                doShowRank();
                break;
            case 3:
                printf("已退出当前账号，请重新登录。\n");
                return 1;
            case 0:
                return 0;
        }
    }
}

/*
 * 功能：显示游戏难度菜单，读取玩家选择。
 * 输入：无参数；通过键盘输入难度编号。
 * 输出：显示初级、中级、高级及返回选项。
 * 返回值：1 表示初级；2 表示中级；3 表示高级；0 表示取消。
 */
static int chooseLevel(void)
{
    printf("\n---------- 选择难度 ----------\n");
    printf("1. 初级（4x4，8 对图案）\n");
    printf("2. 中级（4x5，10 对图案）\n");
    printf("3. 高级（6x6，12 组，每组 3 张）\n");
    printf("0. 返回玩家菜单\n");
    return readInt("请输入难度编号：", 0, 3);
}

/*
 * 功能：选择难度并开始一局游戏，通关后调用排行榜模块保存成绩。
 * 输入：user 为当前已登录用户的账号信息，用于记录成绩所属玩家。
 * 输出：显示游戏结果、通关用时和成绩保存结果。
 * 返回值：无。
 * 说明：取消难度选择或未通关时直接返回；只有 playGame 返回 1 时，
 *       seconds 才有效，随后调用 saveScore 保存成绩。
 */
static void doPlayGame(User user)
{
    int level = chooseLevel();
    int seconds, result;
    if (level == 0) {
        return;
    }
    if (playGame(level, &seconds) != 1) {
        printf("本局没有完成，本次不计成绩。\n");
        return;
    }
    printf("\n恭喜通关！本局用时 %d 秒。\n", seconds);
    result = saveScore(user.name, level, seconds);
    if (result == 1) {
        printf("成绩已保存到排行榜。\n");
    } else if (result == 0) {
        printf("本次未刷新个人最好成绩或未进入前五名，榜单未变化。\n");
    } else {
        printf("成绩文件读写出错，本次成绩未能保存。\n");
    }
}

/*
 * 功能：检查是否有成绩，让玩家选择并查看某个难度的排行榜。
 * 输入：无参数；有成绩时通过键盘输入排行榜难度编号。
 * 输出：显示排行榜、暂无成绩或文件读取失败的提示。
 * 返回值：无。
 * 说明：全部成绩为空时直接提示；显示排行榜或暂无成绩后停留 3 秒。
 *       读取失败或选择返回时直接结束，不等待。
 */
static void doShowRank(void)
{
    int status = hasRankData();
    int level;
    if (status == -1) {
        printf("成绩文件读取失败，暂时无法查看排行榜。\n");
        return;
    }
    if (status == 0) {
        printf("暂无排行榜成绩，请先完成一局游戏。\n");
    } else {
        printf("\n---------- 选择排行榜 ----------\n");
        printf("1. 初级排行榜\n2. 中级排行榜\n3. 高级排行榜\n0. 返回玩家菜单\n");
        level = readInt("请选择要查看的排行榜：", 0, 3);
        if (level == 0) {
            return;
        }
        showRank(level);
    }
    Sleep(3000);
}
