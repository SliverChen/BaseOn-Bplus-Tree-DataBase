/******************************
 * Topic: 预定义头文件
 * Author: Sliverchen
 * Create file date : 2021 / 7 / 2
 * Explanation: 
 *      1、定义数据的键值类型
 *      2、重写键的比较
 *      3、一串字符串 对应 姓名 年龄 邮箱
 * ****************************/

#ifndef PREDEFINED_H
#define PREDEFINED_H

#include <cstring>

namespace bpt
{

/* predefined the order of the B plus Tree */
#define BP_ORDER 50

    /* predefined key / value type */
    struct value_t
    {
        char name[256];
        int age;
        char email[256];
    };

    struct key_t
    {
        char k[16];

        key_t(const char *str = "")
        {
            memset(k, 0, sizeof(k));
            strcpy(k, str);
        }
    };

    //compare two keys whether they are equal
    inline int keycmp(const key_t &a, const key_t &b)
    {
        int x = strlen(a.k) - strlen(b.k);
        return x == 0 ? strcmp(a.k, b.k) : x;
    }

#define OPERATOR_KEYCMP(type)                        \
    bool operator<(const key_t &t1, const type &t2)  \
    {                                                \
        return keycmp(t1, t2.key) < 0;               \
    }                                                \
    bool operator<(const type &t1, const key_t &t2)  \
    {                                                \
        return keycmp(t1.key, t2) < 0;               \
    }                                                \
    bool operator==(const type &t1, const key_t &t2) \
    {                                                \
        return keycmp(t1.key, t2) == 0;              \
    }                                                \
    bool operator==(const key_t &t1, const type &t2) \
    {                                                \
        return keycmp(t1, t2.key) == 0;              \
    }
}

#endif /* PREDEFINED_H */