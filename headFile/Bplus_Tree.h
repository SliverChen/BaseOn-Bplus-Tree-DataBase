/************************************************
 * Topic: B+ Tree
 * Author: Sliverchen
 * Create date file : 2021 / 7 / 1
 * *********************************************/

#ifndef BPLUS_NODE
#define BPLUS_NODE

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef PREDEFINED_H
#include "predefined.h"
#endif

/*
    说明：
    stddef——定义各种变量类型的宏
    stdio.h——提供文件流
*/

/*****************************
 * 性质1：
 *  B+树有两种节点：内节点和叶子节点。
 *  内节点存关键字和子节点指针
 *  叶子节点存关键字和数据
 * 性质2：
 *  每个关键字都会在叶子节点出现，叶子节点按照关键字大小排序，叶子节点会存有指向兄弟节点的指针
 * 性质3：
 *  每个内节点至多有M个子树
 *  每个非根内节点至少有ceil(M/2)个节点
 * 性质4：
 *  一个节点内n个关键字，对应n+1个子节点指针
 * ***************************/

namespace bpt
{
/* offsets */
#define OFFSET_META 0
#define OFFSET_BLOCK OFFSET_META + sizeof(meta_t)
#define SIZE_NO_CHILDREN sizeof(leaf_node_t) - BP_ORDER * sizeof(record_t)

    /*meta information of B+ tree */
    //主要用于记录B+树的信息
    typedef struct
    {
        size_t order;             //B+树的阶数
        size_t value_size;        //值个数
        size_t key_size;          //键个数
        size_t internal_node_num; //内节点个数
        size_t leaf_node_num;     //叶子节点个数
        size_t height;            //树的高度（除去叶子节点)
        off_t slot;               //存储新块的指针
        off_t root_offset;        //内节点的根节点
        off_t leaf_offset;        //第一个叶子节点
    } meta_t;

    /* internal nodes' index segment*/
    struct index_t
    {                //内节点的索引段
        key_t key;   //关键字
        off_t child; //子节点
    };

    /* internal node block */
    struct internal_node_t
    {
        typedef index_t *child_t;
        off_t parent;               //父结点
        off_t next;                 //后继关键字
        off_t prev;                 //前驱关键字
        size_t n;                   //子节点个数
        index_t children[BP_ORDER]; //子节点
    };

    /* the final record of value */
    struct record_t
    {
        key_t key;
        value_t value;
    };

    /* leaf node block */
    struct leaf_node_t
    {
        typedef record_t *child_t;
        off_t parent;
        off_t next;
        off_t prev;
        size_t n;
        record_t children[BP_ORDER];
    };

    /* the class of B+ tree */
    class bplus_tree
    {
    public:
        bplus_tree(const char *path, bool force_empty = false);

        int search(const key_t &key, value_t *value) const;

        int search_range(key_t *left, const key_t &right,
                         value_t *values, size_t max, bool *next = NULL) const;
        int remove(const key_t &key);
        int insert(const key_t &key, value_t value);
        int update(const key_t &key, value_t value);
        meta_t get_meta() const
        {
            return meta;
        }

    private:
        char path[512];
        meta_t meta;

        /*init empty tree*/
        void init_from_empty();

        /* find index */
        off_t search_index(const key_t &key) const;

        /* find leaf */
        off_t search_leaf(off_t index, const key_t &key) const;
        off_t search_leaf(const key_t &key) const
        {
            return search_leaf(search_index(key), key);
        }

        /* remove internal node */
        void remove_from_index(off_t offset, internal_node_t &node,
                               const key_t &key);

        /* borrow one key from other internal node */
        bool borrow_key(bool from_right, internal_node_t &borrower,
                        off_t offset);

        /*borrow one record from other leaf*/
        bool borrow_key(bool from_right, leaf_node_t &borrower);

        /* change one's parent key to another key */
        void change_parent_child(off_t parent, const key_t &o, const key_t &n);

        /* merge right leaf to left leaf */
        void merge_leafs(leaf_node_t *left, leaf_node_t *right);
        void merge_keys(index_t *where, internal_node_t &left, internal_node_t &right);

        /* insert into leaf without split */
        void insert_record_no_split(leaf_node_t *leaf, const key_t &key, const value_t &value);

        /* add key to the internal node */
        void insert_key_to_index(off_t offset, const key_t &key, off_t value,
                                 off_t after);
        void insert_key_to_index_no_split(internal_node_t &node, const key_t &key,
                                          off_t value);

        /* change children's parent */
        void reset_index_children_parent(index_t *begin, index_t *end,
                                         off_t parent);

        template <class T>
        void node_create(off_t offset, T *node, T *next);

        template <class T>
        void node_remove(T *prev, T *node);

        /* multi-level file open/close */
        mutable FILE *fp;
        mutable int fp_level;
        void open_file(const char *mode = "rb+") const
        {
            /*
                可选模式:
                    "r"：打开一个用于读取的文件
                    "w": 创建一个用于写入的空文件，如果文件名已存在会删除已有文件
                    "a": 追加到一个文件
                    "r+":打开一个用于更新的文件，可读取也可写入，文件必须存在
                    "w+":创建一个用于读写的空文件
                    "a+":打开一个用于读取和追加的文件
                    "rb+":读写一个二进制文件，只允许读写数据
                    "rt+":读写打开一个文本文件，允许读和写
            */
            if (fp_level == 0)
                fp = fopen(path, mode); //使用给定模式打开path所指向的文件

            ++fp_level;
        }

        void close_file() const
        {
            if (fp_level == 1)
                fclose(fp);
            --fp_level;
        }

        /* alloc from disk */
        off_t alloc(size_t size)
        {
            off_t slot = meta.slot;
            meta.slot += size;
            return slot;
        }

        off_t alloc(leaf_node_t *leaf)
        {
            leaf->n = 0; //初始化叶子节点的
            meta.leaf_node_num++;
            return alloc(sizeof(leaf_node_t));
        }

        off_t alloc(internal_node_t *node)
        {
            node->n = 1; //初始化内节点的子节点个数
            meta.internal_node_num++;
            return alloc(sizeof(internal_node_t));
        }

        void unalloc(leaf_node_t *leaf, off_t offset)
        {
            --meta.leaf_node_num;
        }

        void unalloc(internal_node_t *node, off_t offset)
        {
            --meta.internal_node_num;
        }

        /* read block from disk */
        /*
            参数说明：
            block：内存块(读取成功将返回到该参数中)
            offset：偏移量
            size：内存块大小
        */
        int map(void *block, off_t offset, size_t size) const
        {
            open_file();
            fseek(fp, offset, SEEK_SET);           //从头开始找到偏移量为offset的位置
            size_t rd = fread(block, size, 1, fp); //从给定流fp读取数据到ptr所指向的数组中
            close_file();
            return rd - 1;
        }

        template <class T>
        int map(T *block, off_t offset) const //将读取到的数据存到block中
        {
            return map(block, offset, sizeof(T));
        }

        /* write block to disk */
        int unmap(void *block, off_t offset, size_t size) const
        {
            open_file();
            fseek(fp, offset, SEEK_SET);
            //将block写入fp中，返回fp中的元素总数
            size_t wd = fwrite(block, size, 1, fp);
            close_file();

            return wd - 1;
        }

        template <class T>
        int unmap(T *block, off_t offset) const
        {
            return unmap(block, offset, sizeof(T));
        }
    };
}

#endif