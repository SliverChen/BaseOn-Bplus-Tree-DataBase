/***************************
 * Topic: the function of B plus tree implement
 * Author: SliverChen
 * Create file date : 2021 / 7 / 2
 * ************************/

#include "../headFile/Bplus_Tree.h"
#include <algorithm>
#include <list>
#include <stdlib.h>
/*减少名称冲突的可能性，避免使用using namespace std*/
using std::binary_search;
using std::copy;
using std::copy_backward;
using std::lower_bound;
using std::swap;
using std::upper_bound;

/*
    lower_bound：
        从数组的begin到end-1位置二分查找第一个大于或等于num的数字
        返回数字的地址，不存在则返回end
        利用返回的地址减去begin即可得到数字在数组的下标
    
    upper_bound：
        从数组的begin到end-1位置二分查找第一个大于num的数字
        返回数字的地址，不存在则返回end
        利用返回的地址减去begin即可得到数字在数组的下标
*/

namespace bpt
{
    /* custom compare operator for STL algorithms */
    OPERATOR_KEYCMP(index_t)
    OPERATOR_KEYCMP(record_t)

    /*helper iterating function*/
    template <class T>
    inline typename T::child_t begin(T &node)
    {
        return node.children;
    }

    template <class T>
    inline typename T::child_t end(T &node)
    {
        return node.children + node.n;
    }

    //--------------------------------
    //返回node下元素大于等于key的第一个地址
    //--------------------------------
    inline index_t *find(internal_node_t &node, const key_t &key)
    {
        return upper_bound(begin(node), end(node) - 1, key);
    }

    //--------------------------------
    //返回node元素大于key的第一个地址
    //--------------------------------
    inline record_t *find(leaf_node_t &node, const key_t &key)
    {
        return lower_bound(begin(node), end(node), key);
    }

    //----------------------------------
    // B+树类的内部函数实现
    //----------------------------------

    //---------------------------------
    // B+树构造函数
    // 参数说明：
    //  p：存储数据的文件路径
    //  force_empty:  文件是否为空
    //----------------------------------
    bplus_tree::bplus_tree(const char *p, bool force_empty)
        : fp(NULL), fp_level(0)
    {
        memset(path, 0, sizeof(path));
        strcpy(path, p);

        if (!force_empty)
        {
            //read tree from file
            if (map(&meta, OFFSET_META) != 0)
                force_empty = true;
        }

        if (force_empty)
        {
            //创建一个用于读写的空文件
            open_file("w+");
            init_from_empty();
            close_file();
        }
    }

    //-------------------------------
    //查找函数的实现
    //参数说明：
    //   key——字符串
    //    value——字符串对应的数据（名字、年龄、邮箱）
    //-------------------------------
    int bplus_tree::search(const key_t &key, value_t *value) const
    {
        //首先定位到叶子节点的首部
        leaf_node_t leaf;
        map(&leaf, search_leaf(key));

        //然后从头开始遍历叶子节点的值是否与所要找的key相等
        record_t *record = find(leaf, key);
        if (record != leaf.children + leaf.n)
        {
            *value = record->value;
            return keycmp(record->key, key);
        }
        else
            return -1;
    }

    //-------------------------------------
    //范围查找的实现
    //将从left到right的max个数据传到values中
    //同时利用一个变量记录后面是否存在数据
    //参数说明：
    //  left: 数据左边界（在next不为NULL下，返回该范围之后的第一个数据）
    //  right: 数据右边界
    //  values: 查找结果数组
    //  max:   查找结果个数
    //  next:  最后一个数据后面是否存在数据（若传入NULL则不记录）
    //返回: 查找数据的实际个数
    //-------------------------------------
    int bplus_tree::search_range(key_t *left, const key_t &right,
                                 value_t *values, size_t max, bool *next) const
    {
        //如果范围不合法
        if (left == NULL || keycmp(*left, right) > 0)
            return -1;

        //找到left和right所对应的叶子节点
        off_t off_left = search_leaf(*left);
        off_t off_right = search_leaf(right);
        off_t off = off_left;

        size_t i = 0;
        record_t *Begin, *End;

        leaf_node_t leaf;

        //从off开始查找值为values的节点
        while (off != off_right && off != 0 && i < max)
        {
            map(&leaf, off);

            if (off_left == off)
                Begin = find(leaf, *left); //找到left所在的叶子节点赋给leaf，再把地址赋给Begin
            else
                Begin = begin(leaf);

            End = leaf.children + leaf.n; //当前叶子节点的元素个数右边界
            for (; Begin != End && i < max; ++Begin, ++i)
                values[i] = Begin->value; //把值传到values中

            off = leaf.next;
        }

        //如果left所在的叶子节点到right所在的前一个叶子节点遍历完后还是没有达到max个数据
        //则将叶子节点right上的数据也存到结果数组中
        if (i < max)
        {
            map(&leaf, off_right);

            Begin = find(leaf, *left);
            End = upper_bound(begin(leaf), end(leaf), right);
            for (; Begin != End && i < max; ++Begin, ++i)
                values[i] = Begin->value;
        }

        //如果传入参数不为NULL，则判断在查找完left到right范围内的数据后面是否还有数据
        if (next != NULL)
        {
            if (i == max && Begin != End)
            {
                *next = true;
                *left = Begin->key;
            }
            else
                *next = false;
        }
        return i;
    }

    //------------------------------
    //删除数据
    //参数说明：
    //  key: 所要删除的数据
    //-------------------------------
    int bplus_tree::remove(const key_t &key)
    {
        internal_node_t parent;
        leaf_node_t leaf;

        //找到父结点所在位置
        off_t parent_off = search_index(key);
        map(&parent, parent_off);

        //找到key所在的模糊子节点所对应的存储地址
        index_t *where = find(parent, key);
        off_t offset = where->child;
        map(&leaf, offset);

        //判断子节点中是否存在key
        if (!binary_search(begin(leaf), end(leaf), key))
            return -1;

        size_t min_n = meta.leaf_node_num == 1 ? 0 : meta.order / 2;
        assert(leaf.n >= min_n && leaf.n <= meta.order);

        //删除key（覆盖数据的方式）
        record_t *to_delete = find(leaf, key);
        copy(to_delete + 1, end(leaf), to_delete);
        leaf.n--;

        //删除后判断是否需要合并或者借用兄弟节点
        if (leaf.n < min_n)
        {
            bool borrowed = false;

            //借用左边的兄弟节点
            if (leaf.prev != 0)
                borrowed = borrow_key(false, leaf);

            //借用右边的兄弟节点
            if (!borrowed && leaf.next != 0)
                borrowed = borrow_key(true, leaf);

            //合并节点
            if (!borrowed)
            {
                assert(leaf.next != 0 || leaf.prev != 0);
                key_t index_key;

                //如果该叶子节点是最后一个元素，则与前一个节点进行合并
                if (where == end(parent) - 1)
                {
                    assert(leaf.prev != 0);
                    leaf_node_t prev;
                    map(&prev, leaf.prev);        //读取prev的数据
                    index_key = begin(prev)->key; //得到前一节点的key

                    merge_leafs(&prev, &leaf);
                    node_remove(&prev, &leaf);
                    unmap(&prev, leaf.prev);
                }
                //如果不是，则与后一个节点进行合并
                else
                {
                    assert(leaf.next != 0);
                    leaf_node_t next;
                    map(&next, leaf.next);
                    index_key = begin(next)->key;

                    merge_leafs(&leaf, &next);
                    node_remove(&leaf, &next);
                    unmap(&leaf, offset);
                }

                //删除父结点
                remove_from_index(parent_off, parent, index_key);
            }
            else //如果无法借用兄弟节点
            {
                unmap(&leaf, offset);
            }
        }
        else //如果当前节点比最少限制要多，则直接删除即可
        {
            unmap(&leaf, offset);
        }

        return 0;
    }

    //----------------------------
    //插入数据
    //参数说明:
    //  key:所要插入的键
    //  value:所要插入的值
    //---------------------------
    int bplus_tree::insert(const key_t &key, value_t value)
    {
        //首先判断在数据库中是否存在key对应的数据
        off_t parent = search_index(key);
        off_t offset = search_leaf(parent, key);
        leaf_node_t leaf;
        map(&leaf, offset);

        if (binary_search(begin(leaf), end(leaf), key))
            return 1;

        //判断当前节点数是否等于阶数
        if (leaf.n == meta.order)
        {
            /* 分离节点 */

            //创建新的节点
            leaf_node_t new_leaf;
            node_create(offset, &leaf, &new_leaf);

            //找到分离的中间节点
            size_t point = leaf.n / 2;
            //如果key的值比下标point对应的key要大，则需要将point往右移一位
            bool place_right = keycmp(key, leaf.children[point].key) > 0;
            if (place_right)
                ++point;

            //分离操作（将leaf右半部分的值移到新节点的子节点下）
            copy(leaf.children + point, leaf.children + leaf.n,
                 new_leaf.children);
            new_leaf.n = leaf.n - point; //更新节点数
            leaf.n = point;

            /* 插入节点 */
            if (place_right)
                insert_record_no_split(&new_leaf, key, value);
            else
                insert_record_no_split(&leaf, key, value);

            //保存节点
            unmap(&leaf, offset);
            unmap(&new_leaf, leaf.next);
        }
        else //如果节点数小于阶数，直接插入即可
        {
            insert_record_no_split(&leaf, key, value);
            unmap(&leaf, offset);
        }

        return 0;
    }

    //---------------------------------
    //更新值的操作
    //(前提是数据已存在)
    //参数说明：
    //  key：所要修改的数据段
    //  value: 修改值
    //返回值：
    // 0表示修改成功，否则表示不存在该数据
    //--------------------------------
    int bplus_tree::update(const key_t &key, value_t value)
    {
        off_t offset = search_leaf(key);
        leaf_node_t leaf;
        map(&leaf, offset);

        record_t *record = find(leaf, key);
        if (record != leaf.children + leaf.n)
        {
            if (keycmp(key, record->key) == 0)
            {
                record->value = value;
                unmap(&leaf, offset); //保存操作
                return 0;
            }
            else
                return 1;
        }
        return -1;
    }

    //---------------------------------
    //根据索引删除节点的操作
    //参数说明：
    //  offset:偏移量（下标）
    //  node：所要删除的节点
    //  key： 所要删除的数据
    //----------------------------------
    void bplus_tree::remove_from_index(off_t offset, internal_node_t &node,
                                       const key_t &key)
    {
        size_t min_n = meta.root_offset == offset ? 1 : meta.order / 2;
        assert(node.n >= min_n && node.n <= meta.order);

        //删除数据
        key_t index_key = begin(node)->key;
        index_t *to_delete = find(node, key);
        if (to_delete != end(node))
        {
            (to_delete + 1)->child = to_delete->child;
            copy(to_delete + 1, end(node), to_delete); //覆盖操作
        }
        --node.n;

        //删除后如果只剩下一个节点
        if (node.n == 1 && meta.root_offset == offset &&
            meta.internal_node_num != 1)
        {
            unalloc(&node, meta.root_offset);
            meta.height--;
            meta.root_offset = node.children[0].child;
            unmap(&meta, OFFSET_META);
            return;
        }

        //判断删除后是否需要合并或借用兄弟节点
        if (node.n < min_n)
        {
            internal_node_t parent;
            map(&parent, node.parent);

            bool borrowed = false;
            //借用左兄弟节点（前提是左兄弟节点存在）
            if (offset != begin(parent)->child)
                borrowed = borrow_key(false, node, offset);

            //借用右兄弟节点（前提是右兄弟节点存在）
            if (!borrowed && offset != (end(parent) - 1)->child)
                borrowed = borrow_key(true, node, offset);

            //没有借用则需要合并
            if (!borrowed)
            {
                assert(node.next != 0 || node.prev != 0);
                //如果是最后一个节点
                if (offset == (end(parent) - 1)->child)
                {
                    assert(node.prev != 0);
                    internal_node_t prev;
                    map(&prev, node.prev);

                    //合并操作
                    index_t *where = find(parent, begin(prev)->key);
                    reset_index_children_parent(begin(node), end(node), node.prev);
                    merge_keys(where, prev, node);
                    unmap(&prev, node.prev);
                }
                //如果不是最后一个节点
                else
                {
                    assert(node.next != 0);
                    internal_node_t next;
                    map(&next, node.next);

                    //合并操作
                    index_t *where = find(parent, begin(next)->key);
                    reset_index_children_parent(begin(node), end(node), offset);
                    merge_keys(where, node, next);
                    unmap(&node, node.next);
                }

                //删除父结点
                remove_from_index(node.parent, parent, index_key);
            }
            //如果借用了兄弟节点，保存操作即可
            else
            {
                unmap(&node, offset);
            }
        }
        //如果当前节点个数大于最小限制，则保存操作即可
        else
        {
            unmap(&node, offset);
        }
    }

    //---------------------------------
    //借用兄弟节点的操作（内节点）
    //参数说明：
    //  from_right:是否从右边借用
    //  borrower: 借用的节点
    //  offset: 数据在节点中的偏移量
    //返回值：
    //  true为借用成功，false为失败
    //--------------------------------
    bool bplus_tree::borrow_key(bool from_right, internal_node_t &borrower,
                                off_t offset)
    {
        typedef typename internal_node_t::child_t child_t;

        //定位兄弟节点(借之前判断兄弟节点是否在减掉一个节点的时候没有破坏结构)
        off_t lender_off = from_right ? borrower.next : borrower.prev;
        internal_node_t lender;
        map(&lender, lender_off);

        assert(lender.n >= meta.order / 2);
        if (lender.n != meta.order / 2)
        {
            child_t where_to_lend, where_to_put;
            internal_node_t parent;

            if (from_right)
            {
                where_to_lend = begin(lender);
                where_to_put = end(borrower);

                map(&parent, borrower.parent);
                child_t where = lower_bound(begin(parent), end(parent) - 1,
                                            (end(borrower) - 1)->key);

                where->key = where_to_lend->key;
                unmap(&parent, borrower.parent);
            }
            else
            {
                where_to_lend = end(lender) - 1;
                where_to_put = begin(borrower);

                map(&parent, lender.parent);
                child_t where = find(parent, begin(lender)->key);
                where_to_put->key = where->key;
                where->key = (where_to_lend - 1)->key;
                unmap(&parent, lender.parent);
            }

            //存储
            copy_backward(where_to_put, end(borrower), end(borrower) + 1);
            *where_to_put = *where_to_lend;
            borrower.n++;

            //从兄弟节点删除被借用的key
            reset_index_children_parent(where_to_lend, where_to_lend + 1, offset);
            copy(where_to_lend + 1, end(lender), where_to_lend);
            lender.n--;
            unmap(&lender, lender_off);
            return true;
        }
        return false;
    }

    //-----------------------------------
    //借用兄弟节点的操作（叶子节点）
    //参数说明：
    //  from_right:是否借用右兄弟节点
    //  borrower：想要借用的节点
    // 返回值：
    // true表示借用成功，false表示失败
    //------------------------------------
    bool bplus_tree::borrow_key(bool from_right, leaf_node_t &borrower)
    {
        off_t lender_off = from_right ? borrower.next : borrower.prev;
        leaf_node_t lender;
        map(&lender, lender_off);

        assert(lender.n >= meta.order / 2);

        if (lender.n != meta.order / 2)
        {
            typename leaf_node_t::child_t where_to_lend, where_to_put;

            //移动兄弟节点
            if (from_right)
            {
                where_to_lend = begin(lender);
                where_to_put = end(borrower);
                change_parent_child(borrower.parent, begin(borrower)->key,
                                    lender.children[1].key);
            }
            else
            {
                where_to_lend = end(lender) - 1;
                where_to_put = begin(borrower);
                change_parent_child(lender.parent, begin(lender)->key,
                                    where_to_lend->key);
            }

            //保存
            copy_backward(where_to_put, end(borrower), end(borrower) + 1);
            *where_to_put = *where_to_lend;
            borrower.n++;

            //删除
            copy(where_to_lend + 1, end(lender), where_to_lend);
            lender.n--;
            unmap(&lender, lender_off);
            return true;
        }
        return false;
    }

    //--------------------------------------
    //更新父结点的关键字为新的关键字
    //参数说明：
    //  parent：父结点的位置
    //  old：原关键字
    //  new: 新关键字
    //---------------------------------------
    void bplus_tree::change_parent_child(off_t parent, const key_t &oldKey,
                                         const key_t &newKey)
    {
        internal_node_t node;
        map(&node, parent);

        index_t *w = find(node, oldKey);
        assert(w != node.children + node.n);

        w->key = newKey;
        unmap(&node, parent);
        if (w == node.children + node.n - 1)
            change_parent_child(node.parent, oldKey, newKey);
    }

    //----------------------------------
    //将右叶子节点部分合并到左子节点的后面
    //参数说明：
    //  left：左叶子节点
    //  right：右叶子节点
    //---------------------------------
    void bplus_tree::merge_leafs(leaf_node_t *left, leaf_node_t *right)
    {
        copy(begin(*right), end(*right), end(*left));
        left->n += right->n;
    }

    //---------------------------------
    //将next的内节点部分合并到node的内节点的后面
    //参数说明：
    //  where：
    //  node：进行合并的内节点
    //  next：将要被合并的内节点
    //----------------------------------
    void bplus_tree::merge_keys(index_t *where, internal_node_t &node,
                                internal_node_t &next)
    {
        copy(begin(next), end(next), end(node));
        node.n += next.n;
        node_remove(&node, &next);
    }

    //---------------------------------
    //在不分离的情况下插入记录
    //参数说明：
    //  leaf：将要被插入数据的叶子节点
    //  key：插入的关键字
    //  value：插入的数据
    //---------------------------------
    void bplus_tree::insert_record_no_split(leaf_node_t *leaf,
                                            const key_t &key, const value_t &value)
    {
        record_t *where = upper_bound(begin(*leaf), end(*leaf), key);
        copy_backward(where, end(*leaf), end(*leaf) + 1);

        where->key = key;
        where->value = value;
        ++leaf->n;
    }

    //--------------------------------
    //往内节点插入关键字
    //参数说明：
    //  offset：内节点在内存中的偏移量
    //  key：   插入的关键字
    //  old：   左子节点
    //  after： 右子节点
    //--------------------------------
    void bplus_tree::insert_key_to_index(off_t offset, const key_t &key,
                                         off_t old, off_t after)
    {
        //如果offset为0，需要新创建根节点
        if (offset == 0)
        {
            internal_node_t root;
            root.next = 0;
            root.prev = 0;
            root.parent = 0;
            meta.root_offset = alloc(&root);
            meta.height++;

            //插入old和after
            root.n = 2;
            root.children[0].key = key;
            root.children[0].child = old;
            root.children[1].child = after;

            unmap(&meta, OFFSET_META);
            unmap(&root, meta.root_offset);

            //更新子节点的父结点
            reset_index_children_parent(begin(root), end(root),
                                        meta.root_offset);
            return;
        }

        //定位将要进行插入操作的节点
        internal_node_t node;
        map(&node, offset);
        assert(node.n <= meta.order);

        //如果满了则需要分离
        if (node.n == meta.order)
        {
            internal_node_t new_node;
            node_create(offset, &node, &new_node);

            //找到分离点
            size_t point = (node.n - 1) / 2;
            bool place_right = keycmp(key, node.children[point].key) > 0;
            if (place_right)
                ++point;

            //如果point+1后对应的关键字比key小，需要回退一步
            if (place_right && keycmp(key, node.children[point].key) < 0)
                point--;

            key_t middle_key = node.children[point].key;

            //分离操作
            copy(begin(node) + point + 1, end(node), begin(new_node));
            new_node.n = node.n - point - 1;
            node.n = point + 1;

            //插入新关键字
            if (place_right)
                insert_key_to_index_no_split(new_node, key, after);
            else
                insert_key_to_index_no_split(node, key, after);

            unmap(&node, offset);
            unmap(&new_node, node.next);

            //更新子节点对应的父结点
            reset_index_children_parent(begin(new_node), end(new_node), node.next);

            //将中间关键字放到父结点中
            insert_key_to_index(node.parent, middle_key, offset, node.next);
        }
        else
        {
            insert_key_to_index_no_split(node, key, after);
            unmap(&node, offset);
        }
    }

    //----------------------------------
    //往内节点插入（不分离）
    //参数说明：
    //  node: 想要插入的内节点
    //  key： 插入的关键字
    //  value：插入的数据
    //----------------------------------
    void bplus_tree::insert_key_to_index_no_split(internal_node_t &node,
                                                  const key_t &key, off_t value)
    {
        index_t *where = upper_bound(begin(node), end(node) - 1, key);

        //将后面的节点往后挪一个位置
        copy_backward(where, end(node), end(node) + 1);

        //插入该关键字
        where->key = key;
        where->child = (where + 1)->child;
        (where + 1)->child = value;

        node.n++;
    }

    //---------------------------
    //重置内节点或叶子节点对应的父结点和子节点下标
    //参数说明：
    //  begin: 节点起始位置
    //  end：  节点末尾位置
    //  parent: 新的父结点偏移量
    //----------------------------
    void bplus_tree::reset_index_children_parent(index_t *begin, index_t *end,
                                                 off_t parent)
    {
        internal_node_t node;
        while (begin != end)
        {
            map(&node, begin->child);
            node.parent = parent;
            unmap(&node, begin->child, SIZE_NO_CHILDREN);
            ++begin;
        }
    }

    //-----------------------------
    //获取对应关键字的下标(内节点)
    //参数说明：
    //  key： 要搜索的关键字
    //返回值：
    //  关键字在内存的偏移量
    //-----------------------------
    off_t bplus_tree::search_index(const key_t &key) const
    {
        off_t org = meta.root_offset;
        int height = meta.height;
        while (height > 1)
        {
            internal_node_t node;
            map(&node, org);

            index_t *i = upper_bound(begin(node), end(node) - 1, key);
            org = i->child;
            --height;
        }

        return org;
    }

    //-----------------------------
    //获取关键字在叶子节点的下标
    //参数说明：
    //  index：
    //  key：要搜索的关键字
    //-----------------------------
    off_t bplus_tree::search_leaf(off_t index, const key_t &key) const
    {
        internal_node_t node;
        map(&node, index);

        index_t *i = upper_bound(begin(node), end(node) - 1, key);
        return i->child;
    }

    //---------------------------
    //创建新节点
    //参数说明：
    //  offset: 创建节点的位置
    //  node：  创建的节点
    //  next：  创建成功后将新创建的节点的后继节点返回到next中
    //---------------------------

    template <class T>
    void bplus_tree::node_create(off_t offset, T *node, T *next)
    {
        //把node的后继节点赋给next
        next->parent = node->parent;
        next->next = node->next;
        next->prev = offset;

        //将next创建在node之后
        node->next = alloc(next);

        //如果next后继节点不为空，也就说明node本身的后继指向不为空
        //则修正原本后继节点的前驱指向
        if (next->next != 0)
        {
            T old_next;
            map(&old_next, next->next, SIZE_NO_CHILDREN);
            old_next.prev = node->next;
            unmap(&old_next, next->next, SIZE_NO_CHILDREN);
        }
        unmap(&meta, OFFSET_META); //保存操作
    }

    //--------------------------------
    //删除节点
    //参数说明：
    //  prev：所删除节点的前驱节点
    //  node：将要删除的节点
    //--------------------------------
    template <class T>
    void bplus_tree::node_remove(T *prev, T *node)
    {
        //改变当前节点的数量（视具体node和prev的类型而定）
        unalloc(node, prev->next);

        prev->next = node->next;
        if (node->next != 0)
        {
            T next;
            map(&next, node->next, SIZE_NO_CHILDREN);
            next.prev = node->prev;
            unmap(&next, node->next, SIZE_NO_CHILDREN);
        }
        unmap(&meta, OFFSET_META);
    }

    //-------------------------------
    //初始化空树
    //-------------------------------
    void bplus_tree::init_from_empty()
    {
        //初始化meta
        memset(&meta, 0, sizeof(meta_t));
        meta.order = BP_ORDER;
        meta.value_size = sizeof(value_t);
        meta.key_size = sizeof(key_t);
        meta.height = 1;
        meta.slot = OFFSET_BLOCK;

        //初始化根节点
        internal_node_t root;
        root.next = 0;
        root.prev = 0;
        root.parent = 0;
        meta.root_offset = alloc(&root);

        //初始化空叶子节点
        leaf_node_t leaf;
        leaf.next = 0;
        leaf.prev = 0;
        leaf.parent = meta.root_offset;
        meta.leaf_offset = alloc(&leaf);
        root.children[0].child = alloc(&leaf);

        //保存操作
        unmap(&meta, OFFSET_META);
        unmap(&root, meta.root_offset);
        unmap(&leaf, root.children[0].child);
    }
}
