/*========================================================================
 * 文件名：account.c
 * 功  能：账号模块的实现。负责密码安全检测、用户注册和用户登录，
 *         账号数据保存在文本文件 users.txt 中，一行一个账号。
 * 说  明：账号用结构体数组保存，不使用链表，只使用标准 C 库。
 *========================================================================*/
#include <errno.h>      /* 区分首次运行无文件与读取错误 */
#include <stdio.h>      /* printf、fopen、fprintf 等输入输出函数 */
#include <string.h>     /* strcmp、strlen 等字符串函数           */
#include <ctype.h>      /* islower、isupper、isdigit 字符判断    */
#include "account.h"    /* 本模块的结构体与函数声明              */

#define USERS_FILE "users.txt"   /* 账号文件的文件名，与 main.c 在同一目录 */

/* 以下是本文件内部使用的辅助函数，只在本文件可见 */
static int loadUsers(User users[], int maxCount);
static int findUser(const User users[], int count, const char name[]);
static int isNameUsed(const char name[]);

/*------------------------------------------------------------------------
 * 函数：checkPassword
 * 功  能：检测密码安全等级（接口说明见 account.h）。
 * 思  路：先判断长度，再统计“四类字符”中出现了几类，最后按类数定等级。
 *------------------------------------------------------------------------*/
int checkPassword(char password[])
{
    int length;         /* 密码的字符个数（不含结束符）              */
    int hasLower = 0;   /* 是否含小写字母：1 有，0 无                */
    int hasUpper = 0;   /* 是否含大写字母：1 有，0 无                */
    int hasDigit = 0;   /* 是否含数字：1 有，0 无                    */
    int hasOther = 0;   /* 是否含其它符号：1 有，0 无                */
    int kinds;          /* 密码中实际出现的字符种类数（0～4）        */
    int i;              /* 扫描密码用的下标                          */
    unsigned char ch;   /* 当前字符，用无符号类型可避免 ctype 越界   */

    if (password == NULL)                       /* 防御性检查：空指针直接判不安全 */
    {
        return 0;
    }

    length = (int)strlen(password);             /* 取密码长度 */
    if (length < 6)                             /* 长度不足 6 位一律不安全 */
    {
        return 0;
    }

    for (i = 0; i < length; i++)                /* 逐个字符统计所属类别 */
    {
        ch = (unsigned char)password[i];
        if (islower(ch))
        {
            hasLower = 1;
        }
        else if (isupper(ch))
        {
            hasUpper = 1;
        }
        else if (isdigit(ch))
        {
            hasDigit = 1;
        }
        else
        {
            hasOther = 1;                       /* 其它符号也算一类 */
        }
    }

    kinds = hasLower + hasUpper + hasDigit + hasOther;  /* 出现过的类别数 */
    if (kinds <= 1)                             /* 只有一类：不安全 */
    {
        return 0;
    }
    if (kinds == 2)                             /* 两类：中度安全 */
    {
        return 1;
    }
    return 2;                                   /* 三类及以上：安全 */
}

/*------------------------------------------------------------------------
 * 函数：registerUser
 * 功  能：把新用户追加写入 users.txt（接口说明见 account.h）。
 *------------------------------------------------------------------------*/
int registerUser(User user)
{
    FILE *fp;       /* 指向 users.txt 的文件指针                 */
    int used;       /* 用户名是否已被占用：1 已占用，0 未占用    */

    if (user.name[0] == '\0' || user.password[0] == '\0')   /* 参数为空不作处理 */
    {
        return 0;
    }

    used = isNameUsed(user.name);       /* 先查重，用户名不能重复 */
    if (used != 0)
    {
        return 0;                       /* 1 重名，-1 读取失败，都按失败处理 */
    }

    fp = fopen(USERS_FILE, "a");        /* 追加方式打开，文件不存在时自动创建 */
    if (fp == NULL)
    {
        return 0;                       /* 文件建立失败 */
    }

    /* 每条记录写成“用户名 密码”一行，两个字段之间用空格分隔 */
    if (fprintf(fp, "%s %s\n", user.name, user.password) < 0)
    {
        fclose(fp);
        return 0;                       /* 写入失败 */
    }

    if (fclose(fp) != 0)                /* 关闭失败说明数据可能没有完整落盘 */
    {
        return 0;
    }
    return 1;                           /* 注册成功 */
}

/*------------------------------------------------------------------------
 * 函数：loginUser
 * 功  能：验证用户名和密码是否与账号文件中的某一记录完全一致。
 * 说  明：把文件读入结构体数组后逐个比较，不使用链表。
 *------------------------------------------------------------------------*/
int loginUser(User user)
{
    User users[USER_MAX];   /* 从文件读入的账号数组            */
    int count;              /* 实际读入的账号个数              */
    int i;                  /* 遍历账号数组用的下标            */

    count = loadUsers(users, USER_MAX);     /* 读入全部账号 */
    for (i = 0; i < count; i++)
    {
        /* 用户名和密码必须同时相同才算登录成功 */
        if (strcmp(users[i].name, user.name) == 0 &&
            strcmp(users[i].password, user.password) == 0)
        {
            return 1;
        }
    }
    return 0;               /* 用户名不存在或密码错误 */
}

/*------------------------------------------------------------------------
 * 函数：loadUsers
 * 功  能：把 users.txt 中的账号读入结构体数组。
 * 参  数：users    —— 用来接收账号的数组，由调用者提供；
 *         maxCount —— 数组最多能装下的账号个数。
 * 返  回：实际读入的账号个数；文件不存在时返回 0（首次运行视为空表）。
 * 说  明："%31s %63s" 中的宽度正好对应 name 的 31 字节和 password 的
 *         63 字节，可以防止读入超长字符串把数组写坏。
 *------------------------------------------------------------------------*/
static int loadUsers(User users[], int maxCount)
{
    FILE *fp;           /* 指向 users.txt 的文件指针   */
    int count = 0;      /* 已经读入的账号个数          */

    fp = fopen(USERS_FILE, "r");
    if (fp == NULL)     /* 文件还不存在：还没有任何账号 */
    {
        return errno == ENOENT ? 0 : -1;
    }

    /* 每次读两个字段，读到文件末尾或数组装满就停止 */
    while (count < maxCount &&
           fscanf(fp, "%31s %63s", users[count].name, users[count].password) == 2)
    {
        count++;
    }

    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return count;
}

/*------------------------------------------------------------------------
 * 函数：findUser
 * 功  能：在已读入的账号数组中按用户名查找。
 * 返  回：找到时返回该账号在数组中的下标（从 0 开始），找不到返回 -1。
 *------------------------------------------------------------------------*/
static int findUser(const User users[], int count, const char name[])
{
    int i;              /* 遍历数组用的下标 */

    for (i = 0; i < count; i++)
    {
        if (strcmp(users[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

/*------------------------------------------------------------------------
 * 函数：isNameUsed
 * 功  能：判断用户名是否已经在 users.txt 中出现过。
 * 返  回：1 表示已被占用，0 表示可以使用。
 *------------------------------------------------------------------------*/
static int isNameUsed(const char name[])
{
    User users[USER_MAX];   /* 临时存放读入的账号 */
    int count;              /* 读入的账号个数     */

    count = loadUsers(users, USER_MAX);
    if (count < 0 || count >= USER_MAX) {
        return -1;  /* 文件读取失败或账号数组已满，不再追加。 */
    }
    if (findUser(users, count, name) >= 0)
    {
        return 1;
    }
    return 0;
}
