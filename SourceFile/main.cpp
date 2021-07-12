/***********************************
 * Topic：主函数文件
 * Author: Sliverchen
 * Create file date: 2021 / 7 / 3
 * *********************************/

#include "../SourceFile/Bplus_Tree.cpp"
#include "../headFile/TextTable.h"

#include <fstream>
#include <io.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace bpt;
using namespace std;

//错误信息字段
const char *errorMessage = "> your input is invalid, print \".help\" for more information!\n";

//下一行头字段
const char *nextLineHeader = "> ";

//退出消息字段
const char *exitMesssage = "> bye!\n";

//数据库文件名
const char *dbFileName = "../Data/db.bin";

//操作时间记录
clock_t startTime, finishTime;

//B+树指针
bplus_tree *db_ptr;

void initSystem();

/* 类型转换 */
void intTokeyT(bpt::key_t *a, int *b)
{
    char key[16] = {0};
    sprintf(key, "%d", *b);
    *a = key;
}

/* 打印帮助信息 */
void printHelpMess()
{
    cout << "***************************************************************************\n"
         << "welcome to B+Tree dataBase              dbfile located in \"./Data/db.bin\"\n"
         << "***************************************************************************\n"
         << "  .help                                              print help message;   \n"
         << "  .exit                                              exit the system;      \n"
         << "  .reset                                             reset the database;   \n"
         << "  insert db {index}{name}{age}{email};               insert record;        \n"
         << "  delete from db where id = {index};                 delete record;        \n"
         << "  update db {name}{age}{email} where id = {index};   update record;        \n"
         << "  select * from db where id = {index};               search by index;      \n"
         << "  select * from db where id in ({minIndex,maxIndex}) search in range;      \n"
         << "***************************************************************************\n"
         << endl
         << nextLineHeader;
}

/* insert命令 */
int insertRecord(bplus_tree *treePtr, int *index, value_t *values)
{
    bpt::key_t key;
    intTokeyT(&key, index);
    return (*treePtr).insert(key, *values);
}

/* delete命令 */
int deleteRecord(bplus_tree *treePtr, int *index)
{
    bpt::key_t key;
    intTokeyT(&key, index);
    return (*treePtr).remove(key);
}

/* 查找命令 */
int searchRecord(bplus_tree *treePtr, int *index, value_t *return_val)
{
    bpt::key_t key;
    intTokeyT(&key, index);
    return (*treePtr).search(key, return_val);
}

/* 全局查找命令 */
int searchAll(bplus_tree *treePtr, int *start, int *end)
{
    TextTable t('-', '|', '+');
    t.add("id");
    t.add("name");
    t.add("age");
    t.add("email");
    t.endOfRow();

    bpt::key_t key;
    value_t *return_val = new value_t;
    for (int i = *start; i <= *end; ++i)
    {
        intTokeyT(&key, &i);
        int return_code = (*treePtr).search(key, return_val);
        switch (return_code)
        {
        case -1:
            break;
        case 0:
            t.add(to_string(i));
            t.add(return_val->name);
            t.add(to_string(return_val->age));
            t.add(return_val->email);
            t.endOfRow();
            break;
        case 1:
            break;
        }
    }

    cout << t << endl;
    return 0;
}

/* update 命令 */
int updateRecord(bplus_tree *treePtr, int *index, value_t *value)
{
    bpt::key_t key;
    intTokeyT(&key, index);
    return (*treePtr).update(key, *value);
}

/* 打印表 */
void printTable(int *index, value_t *values)
{
    TextTable t('-', '|', '+');
    t.add("id");
    t.add("name");
    t.add("age");
    t.add("email");
    t.endOfRow();

    t.add(to_string(*index));
    t.add(values->name);
    t.add(to_string(values->age));
    t.add(values->email);
    t.endOfRow();

    cout << t << endl;
}

/* 判断文件是否存在 */
bool is_file_exists(const char *filename)
{
    return _access(filename, 0) != -1;
}

/* 处理时间计算 */
double durationTime(clock_t *f, clock_t *s)
{
    return (double)(*f - *s) / CLOCKS_PER_SEC;
}

/* select命令 */
void selectCommand()
{
    char *usercommand = new char[256];
    while (true)
    {
        cin.getline(usercommand, 256);
        if (strcmp(usercommand, ".exit") == 0)
        {
            cout << exitMesssage;
            break;
        }
        else if (strcmp(usercommand, ".help") == 0)
        {
            printHelpMess();
        }
        else if (strcmp(usercommand, ".reset") == 0)
        {
            if (remove(dbFileName) != 0)
                cout << "can't delete file\n"
                     << nextLineHeader;
            else
                cout << "DB file has been deleted!" << endl
                     << endl;
            initSystem();
        }
        else if (strncmp(usercommand, "insert", 6) == 0) //匹配前6个字符
        {
            int *keyIndex = new int;
            value_t *insertData = new value_t;

            //sscanf字符串格式化输入
            int okNum = sscanf(usercommand, "insert db %d %s %d %s;",
                               keyIndex, insertData->name, &(insertData->age), insertData->email);
            if (okNum < 3)
            {
                cout << errorMessage << nextLineHeader;
            }
            else
            {
                startTime = clock();

                int return_code = insertRecord(db_ptr, keyIndex, insertData);

                finishTime = clock();

                if (return_code == 0)
                {
                    cout << "> executed insert index: " << *keyIndex << ", time: " << durationTime(&finishTime, &startTime) << endl
                         << nextLineHeader;
                }
                else if (return_code == 1)
                {
                    cout << "> failed: already exist index: " << *keyIndex << "\n"
                         << nextLineHeader;
                }
                else
                {
                    cout << "> failed!\n"
                         << nextLineHeader;
                }
            }
        }
        else if (strncmp(usercommand, "delete", 6) == 0)
        {
            int *keyIndex = new int;
            int okNum = sscanf(usercommand, "delete from db where id = %d", keyIndex);
            if (okNum < 1)
            {
                cout << errorMessage << nextLineHeader;
            }
            else
            {
                startTime = clock();

                int return_code = deleteRecord(db_ptr, keyIndex);

                finishTime = clock();

                if (return_code == 0)
                {
                    cout << "> executed delete index: " << *keyIndex
                         << ", time : " << durationTime(&finishTime, &startTime)
                         << endl
                         << nextLineHeader;
                }
                else if (return_code == -1)
                {
                    cout << "> failed ! no index: " << *keyIndex << "\n"
                         << nextLineHeader;
                }
                else
                {
                    cout << "failed !\n"
                         << nextLineHeader;
                }
            }
        }

        else if (strncmp(usercommand, "select", 6) == 0)
        {
            if (!strstr(usercommand, "="))
            {
                int i_start, i_end;
                int okNum = sscanf(usercommand, "select * from db where id in (%d,%d)", &i_start, &i_end);

                if (okNum < 2)
                {
                    cout << errorMessage << nextLineHeader;
                }
                else
                {
                    startTime = clock();
                    searchAll(db_ptr, &i_start, &i_end);
                    finishTime = clock();

                    cout << "> executed search, time: " << durationTime(&finishTime, &startTime)
                         << "\n"
                         << nextLineHeader;
                }
            }
            else
            {
                int keyIndex;
                int okNum = sscanf(usercommand, "select * from db where id = %d", &keyIndex);
                if (okNum < 1)
                {
                    cout << errorMessage << nextLineHeader;
                }
                else
                {
                    value_t *return_val = new value_t;
                    startTime = clock();
                    int return_code = searchRecord(db_ptr, &keyIndex, return_val);
                    finishTime = clock();

                    if (return_code != 0)
                    {
                        cout << "> index: " << keyIndex << "doesn't exist, time: " << durationTime(&finishTime, &startTime)
                             << "\n"
                             << nextLineHeader;
                    }
                    else
                    {
                        printTable(&keyIndex, return_val);
                        cout << "> executed search, time: " << durationTime(&finishTime, &startTime)
                             << "\n"
                             << nextLineHeader;
                    }
                }
            }
        }
        else if (strncmp(usercommand, "update", 6) == 0)
        {
            int *keyIndex = new int;
            value_t *new_val = new value_t;

            int okNum = sscanf(usercommand, "update db %d %s %d where id = %d",
                               new_val->name, &new_val->age, new_val->email, keyIndex);
            if (okNum < 4)
            {
                cout << errorMessage << nextLineHeader;
            }
            else
            {
                startTime = clock();
                int return_code = updateRecord(db_ptr, keyIndex, new_val);
                finishTime = clock();
                if (return_code == 0)
                {
                    cout << "> executed update index: " << *keyIndex
                         << ", time: " << durationTime(&finishTime, &startTime)
                         << "\n"
                         << nextLineHeader;
                }
                else
                {
                    cout << "> failed ! no index: " << *keyIndex
                         << ", time: " << durationTime(&finishTime, &startTime)
                         << "\n"
                         << nextLineHeader;
                }
            }
        }
        else
        {
            cout << errorMessage << nextLineHeader;
        }
    }
}

void initSystem()
{
    //step1:显示帮助信息
    printHelpMess();

    //step2:初始化数据库
    bplus_tree db(dbFileName, !is_file_exists(dbFileName));

    db_ptr = &db;

    //step3: 输入命令
    selectCommand();
}

int main()
{
    initSystem();
    system("pause");
}