#include <iostream>
#include <string>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <climits>
#include <limits>

const int M = 100;
// 叶子节点限制
const int MAX_KEY_LEAF = M;
const int MIN_KEY_LEAF = (M + 1) / 2;
// 内部节点数限制
const int MAX_KEY_INTERNAL = M - 1;
const int MIN_KEY_INTERNAL = M / 2 - 1;

const int KEY_LEN = 65;
const int NODE_SIZE = 8192;

struct FileHeader {
    int root_pos;  // 根节点的编号0-based
    int free_list;  // 空闲块链头
    int total_blocks; // 文件总块数
    int height;  // 高度，取根节点高度为1
};

template<typename ValueType = int>
struct Node {
    bool is_leaf;  // 是否为叶子
    int key_num;   // 实际存储的key数
    int next;  // 叶子链表
    int parent;  // -1表示无父

    char keys[MAX_KEY_LEAF][KEY_LEN];
    ValueType values[MAX_KEY_LEAF];
    int children[MAX_KEY_INTERNAL + 1];// key数+1个儿子（仅内部节点有）

    Node() {
        is_leaf = true;
        key_num = 0;
        next = -1;
        parent = -1;
        for (int i = 0; i <= MAX_KEY_INTERNAL; i++) {
            children[i] = -1;
        }
    }
};

template<typename ValueType = int>
class FileManager {
private:
    std::fstream file;
    std::string filename;  // 文件名
    FileHeader header;     // 文件头
    
public:
    FileManager(const std::string& fname) : filename(fname) {
        bool exists = std::filesystem::exists(filename);

        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            // 不存在，先创建，再读写打开
            file.open(filename, std::ios::out | std::ios::binary);
            file.close();
            file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
            // 初始化文件头
            header.root_pos = -1;
            header.free_list = -1;
            header.total_blocks = 0;
            header.height = 0;
            writeHeader();
        } else {
            // 已存在，读取
            readHeader();
        }
    }
    
    // 文件头写回磁盘
    ~FileManager() {
        writeHeader();
        file.close();
    }
    
    // 读文件头
    void readHeader() {
        file.seekg(0);
        file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
    }
    
    // 文件头写入
    void writeHeader() {
        file.seekp(0);
        file.write(reinterpret_cast<char*>(&header), sizeof(FileHeader));
    }
    
    // 分配node,优先从free取
    int allocateBlock() {
        if (header.free_list != -1) {
            int pos = header.free_list;
            Node<ValueType> temp;
            readNode(pos, temp);
            header.free_list = temp.next;
            return pos;
        }
        // 用新块
        return header.total_blocks++;
    }
    
    // 回收删除的节点
    void freeBlock(int pos) {
        Node<ValueType> temp;
        readNode(pos, temp);
        temp.next = header.free_list;
        header.free_list = pos;
        writeNode(pos, temp);
    }
    
    // 读任意node
    void readNode(int pos, Node<ValueType>& node) {
        // 计算偏移：文件头大小
        file.seekg(sizeof(FileHeader) + pos * NODE_SIZE);
        file.read(reinterpret_cast<char*>(&node), sizeof(Node<ValueType>));
    }
    
    // 将任意node写入
    void writeNode(int pos, const Node<ValueType>& node) {
        file.seekp(sizeof(FileHeader) + pos * NODE_SIZE);
        file.write(reinterpret_cast<const char*>(&node), sizeof(Node<ValueType>));
    }
    
    int getRoot() {
        return header.root_pos;
    }
    void setRoot(int pos) {
        header.root_pos = pos;
    }
    int getHeight() {
        return header.height;
    }
    void setHeight(int h) {
        header.height = h;
    }
};

template<typename ValueType = int>
class BPlusTree {
private:
    FileManager<ValueType> fm;

    int keyValueCompare(const char* a_key, ValueType a_val, const char* b_key, ValueType b_val) {
        int key_cmp = strcmp(a_key, b_key);
        if (key_cmp != 0) {
            return key_cmp;
        } else if (a_val == b_val) {
            return 0;
        } else return (a_val > b_val) ? 1 : -1;
    }

    // 找到目标(key,value)位置
    int findKeyValuePos(Node<ValueType>& node, const char* key, ValueType value) {
        int left = 0, right = node.key_num - 1;
        int result = node.key_num;
        while (left <= right) {
            int mid = (left + right) / 2;
            int cmp = keyValueCompare(node.keys[mid], node.values[mid], key, value);
            if (cmp < 0) {
                left = mid + 1;
            } else {
                result = mid;
                right = mid - 1;
            }
        }
        return result;
    }

    // 分裂节点(父节点,当前节点在父节点children中的索引,待分裂节点位置)
    void splitNode(int parent_pos, int child_idx, int node_pos) {
        Node<ValueType> node;
        fm.readNode(node_pos, node);
        // 分配新节点
        int new_pos = fm.allocateBlock();
        Node<ValueType> new_node;
        new_node.is_leaf = node.is_leaf;
        new_node.parent = parent_pos;

        int mid = node.key_num / 2;
        int old_key_num = node.key_num;
        // 叶子节点分裂
        if (node.is_leaf) {
            // 右半部分元素移动到新节点
            new_node.key_num = node.key_num - mid;
            for (int i = 0; i < new_node.key_num; i++) {
                strcpy(new_node.keys[i], node.keys[mid + i]);
                new_node.values[i] = node.values[mid + i];
            }
            node.key_num = mid;
            new_node.next = node.next;
            node.next = new_pos;
        } else { // 内部节点分裂,中间元素提升到父节点
            new_node.key_num = old_key_num - mid - 1;
            for (int i = 0; i < new_node.key_num; i++) {
                strcpy(new_node.keys[i], node.keys[mid + 1 + i]);
                new_node.values[i] = node.values[mid + 1 + i];
                new_node.children[i] = node.children[mid + 1 + i];
                Node<ValueType> child;
                fm.readNode(new_node.children[i], child);
                child.parent = new_pos;
                fm.writeNode(new_node.children[i], child);
            }
            // 复制最后一个子节点
            new_node.children[new_node.key_num] = node.children[mid + 1 + new_node.key_num];
            Node<ValueType> last_child;
            fm.readNode(new_node.children[new_node.key_num], last_child);
            last_child.parent = new_pos;
            fm.writeNode(new_node.children[new_node.key_num], last_child);
            
            node.key_num = mid;
        }
        // 分裂后写回
        fm.writeNode(node_pos, node);
        fm.writeNode(new_pos, new_node);
        // 提升到父节点
        const char* promote_key;
        ValueType promote_value;
        if (node.is_leaf) {
            // 叶子节点，新节点的第一个元素
            promote_key = new_node.keys[0];
            promote_value = new_node.values[0];
        } else {
            // 内部节点，中间元素
            promote_key = node.keys[mid];
            promote_value = node.values[mid];
        }
        // 原节点是根节点
        if (parent_pos == -1) {
            // 创建新的根节点
            int root_pos = fm.allocateBlock();
            Node<ValueType> root;
            root.is_leaf = false;  // 根节点现在是内部节点
            root.key_num = 1;
            root.parent = -1;
            strcpy(root.keys[0], promote_key);
            root.values[0] = promote_value;
            root.children[0] = node_pos;
            root.children[1] = new_pos;
            // 更新两个子节点的parent为新根
            node.parent = root_pos;
            new_node.parent = root_pos;
            fm.writeNode(node_pos, node);
            fm.writeNode(new_pos, new_node);
            // 更新文件头
            fm.writeNode(root_pos, root);
            fm.setRoot(root_pos);
            fm.setHeight(fm.getHeight() + 1);
        } else { // 有父节点，将提升元素插入父节点
            Node<ValueType> parent;
            fm.readNode(parent_pos, parent);
            for (int i = parent.key_num; i > child_idx; i--) {
                strcpy(parent.keys[i], parent.keys[i - 1]);
                parent.values[i] = parent.values[i - 1];
                parent.children[i + 1] = parent.children[i];
            }
            // 插入提升的元素，关联新节点作为右孩子
            strcpy(parent.keys[child_idx], promote_key);
            parent.values[child_idx] = promote_value;
            parent.children[child_idx + 1] = new_pos;
            parent.key_num++;
            
            fm.writeNode(parent_pos, parent);
            // 递归检查父节点是否也上溢
            if (parent.key_num >= MAX_KEY_INTERNAL) {
                Node<ValueType> grand_parent;
                int grand_pos = -1;
                if (parent.parent != -1) {
                    grand_pos = parent.parent;
                    fm.readNode(grand_pos, grand_parent);
                }
                int parent_idx = 0;
                if (grand_pos != -1) {
                    for (; parent_idx <= grand_parent.key_num; parent_idx++) {
                        if (grand_parent.children[parent_idx] == parent_pos) break;
                    }
                }
                splitNode(grand_pos, parent_idx, parent_pos);
            }
        }
    }

    // 将右节点合并到左节点(父节点位置,左节点在父节点children中的索引)
    void mergeNode(int parent_pos, int idx) {
        Node<ValueType> parent;
        fm.readNode(parent_pos, parent);
        int left_pos = parent.children[idx]; // 左节点位置
        int right_pos = parent.children[idx + 1];// 右节点位置
        Node<ValueType> left, right;
        fm.readNode(left_pos, left);
        fm.readNode(right_pos, right);

        // 叶子节点合并
        if (left.is_leaf) {
            for (int i = 0; i < right.key_num; i++) {
                strcpy(left.keys[left.key_num + i], right.keys[i]);
                left.values[left.key_num + i] = right.values[i];
            }
            left.key_num += right.key_num;
            left.next = right.next;
        } 
        // 内部节点合并
        else {
            // 父节点的分隔key下移到左节点末尾
            strcpy(left.keys[left.key_num], parent.keys[idx]);
            left.values[left.key_num] = parent.values[idx];
            left.key_num++;
            // 元素合并
            for (int i = 0; i < right.key_num; i++) {
                strcpy(left.keys[left.key_num + i], right.keys[i]);
                left.values[left.key_num + i] = right.values[i];
                left.children[left.key_num + i] = right.children[i];

                Node<ValueType> child;
                fm.readNode(right.children[i], child);
                child.parent = left_pos;
                fm.writeNode(right.children[i], child);
            }
            // 最后一个子节点
            left.children[left.key_num + right.key_num] = right.children[right.key_num];
            Node<ValueType> last_child;
            fm.readNode(right.children[right.key_num], last_child);
            last_child.parent = left_pos;
            fm.writeNode(right.children[right.key_num], last_child);
            
            left.key_num += right.key_num;
        }

        fm.writeNode(left_pos, left);
        fm.freeBlock(right_pos);

        // 从父节点删除key和右节点指针
            for (int i = idx; i < parent.key_num - 1; i++) {
            strcpy(parent.keys[i], parent.keys[i + 1]);
            parent.values[i] = parent.values[i + 1];
            parent.children[i + 1] = parent.children[i + 2];
        }
        parent.key_num--;

        // 父节点是根且合并后为空
        if (parent_pos == fm.getRoot() && parent.key_num == 0) {
            // 左节点成为新根
            fm.setRoot(left_pos);
            left.parent = -1;
            fm.writeNode(left_pos, left);
            fm.freeBlock(parent_pos);
            fm.setHeight(fm.getHeight() - 1);
        } else {
            fm.writeNode(parent_pos, parent);
            // 递归检查父节点是否下溢
            int min_key = parent.is_leaf ? MIN_KEY_LEAF : MIN_KEY_INTERNAL;
            if (parent.key_num < min_key && parent.parent != -1) {
                handleUnderflow(parent_pos);
            }
        }
    }

    // 从左/右兄弟节点借一个key（当前节点位置，当前节点在父节点children中的索引）
    bool borrowKey(int node_pos, int parent_idx) {
        Node<ValueType> node;
        fm.readNode(node_pos, node);
        int parent_pos = node.parent;
        Node<ValueType> parent;
        fm.readNode(parent_pos, parent);
        
        int min_key = node.is_leaf ? MIN_KEY_LEAF : MIN_KEY_INTERNAL;

        // 从左兄弟借
        if (parent_idx > 0) {
            int left_pos = parent.children[parent_idx - 1];
            Node<ValueType> left;
            fm.readNode(left_pos, left);
            
            // 左兄弟key数必须大于最小值
            if (left.key_num > min_key) {
                if (node.is_leaf) {
                    for (int i = node.key_num; i > 0; i--) {
                        strcpy(node.keys[i], node.keys[i - 1]);
                        node.values[i] = node.values[i - 1];
                    }
                    strcpy(node.keys[0], left.keys[left.key_num - 1]);
                    node.values[0] = left.values[left.key_num - 1];
                    node.key_num++;
                    left.key_num--;
                    // 更新父节点的分隔key
                    strcpy(parent.keys[parent_idx - 1], node.keys[0]);
                    parent.values[parent_idx - 1] = node.values[0];
                } else {
                    // 父节点分隔key下移到当前节点，左兄弟最后一个key上移到父节点
                    for (int i = node.key_num; i > 0; i--) {
                        strcpy(node.keys[i], node.keys[i - 1]);
                        node.values[i] = node.values[i - 1];
                    }
                    for (int i = node.key_num; i >= 0; i--) {
                        node.children[i + 1] = node.children[i];
                    }
                    // 父节点key下移
                    strcpy(node.keys[0], parent.keys[parent_idx - 1]);
                    node.values[0] = parent.values[parent_idx - 1];
                    node.children[0] = left.children[left.key_num];
                    node.key_num++;
                    
                    // 更新子节点parent
                    Node<ValueType> child;
                    fm.readNode(node.children[0], child);
                    child.parent = node_pos;
                    fm.writeNode(node.children[0], child);
                    
                    // 左兄弟最后一个key上移到父节点
                    strcpy(parent.keys[parent_idx - 1], left.keys[left.key_num - 1]);
                    parent.values[parent_idx - 1] = left.values[left.key_num - 1];
                    left.key_num--;
                }
                
                // 写回所有修改的节点
                fm.writeNode(left_pos, left);
                fm.writeNode(node_pos, node);
                fm.writeNode(parent_pos, parent);
                return true;
            }
        }

        // 从右兄弟借
        if (parent_idx < parent.key_num) {
            int right_pos = parent.children[parent_idx + 1];
            Node<ValueType> right;
            fm.readNode(right_pos, right);
            
            if (right.key_num > min_key) {
                if (node.is_leaf) {
                    // 叶子节点，右兄弟第一个key追加到当前节点末尾
                    strcpy(node.keys[node.key_num], right.keys[0]);
                    node.values[node.key_num] = right.values[0];
                    node.key_num++;
                    // 右节点删除第一个key
                    for (int i = 0; i < right.key_num - 1; i++) {
                        strcpy(right.keys[i], right.keys[i + 1]);
                        right.values[i] = right.values[i + 1];
                    }
                    right.key_num--;
                    // 更新父节点分隔key
                    strcpy(parent.keys[parent_idx], right.keys[0]);
                    parent.values[parent_idx] = right.values[0];
                } else {
                    // 内部节点，父节点分隔key下移到当前节点，右兄弟第一个key上移到父节点
                    strcpy(node.keys[node.key_num], parent.keys[parent_idx]);
                    node.values[node.key_num] = parent.values[parent_idx];
                    node.children[node.key_num + 1] = right.children[0];
                    node.key_num++;
                    // 更新子节点parent
                    Node<ValueType> child;
                    fm.readNode(node.children[node.key_num], child);
                    child.parent = node_pos;
                    fm.writeNode(node.children[node.key_num], child);
                    // 右兄弟第一个key上移到父节点
                    strcpy(parent.keys[parent_idx], right.keys[0]);
                    parent.values[parent_idx] = right.values[0];
                    // 右节点删除第一个key和子节点
                    for (int i = 0; i < right.key_num - 1; i++) {
                        strcpy(right.keys[i], right.keys[i + 1]);
                        right.values[i] = right.values[i + 1];
                        right.children[i] = right.children[i + 1];
                    }
                    right.children[right.key_num - 1] = right.children[right.key_num];
                    right.key_num--;
                }
                
                fm.writeNode(right_pos, right);
                fm.writeNode(node_pos, node);
                fm.writeNode(parent_pos, parent);
                return true;
            }
        }

        return false;  // 都无法借，需要合并
    }

    // 处理节点下溢
    void handleUnderflow(int node_pos) {
        Node<ValueType> node;
        fm.readNode(node_pos, node);
        // 根节点下溢
        if (node_pos == fm.getRoot()) {
            // 根是内部节点且只有一个子节点，则子节点升级为新根
            if (node.key_num == 0 && !node.is_leaf) {
                int child_pos = node.children[0];
                Node<ValueType> child;
                fm.readNode(child_pos, child);
                child.parent = -1;
                fm.writeNode(child_pos, child);
                fm.setRoot(child_pos);
                fm.freeBlock(node_pos);
                fm.setHeight(fm.getHeight() - 1);
            }
            return;
        }

        // 找到当前节点在父节点中的索引
        int parent_pos = node.parent;
        Node<ValueType> parent;
        fm.readNode(parent_pos, parent);
        int idx = 0;
        for (; idx <= parent.key_num; idx++) {
            if (parent.children[idx] == node_pos) break;
        }
        // 优先尝试从兄弟借key
        if (borrowKey(node_pos, idx)) {
            return;
        }
        // 借不到则合并，优先左
        if (idx > 0) {
            mergeNode(parent_pos, idx - 1);
        } else {
            mergeNode(parent_pos, idx);
        }
    }

    // 找到目标key应该所在的叶子节点
    int findLeaf(const char* key, ValueType value = std::numeric_limits<ValueType>::min()) {
        if (fm.getRoot() == -1) return -1;
        
        int current = fm.getRoot();
        Node<ValueType> node;
        fm.readNode(current, node);
        while (!node.is_leaf) {
            int pos = findKeyValuePos(node, key, value);
            current = node.children[pos];
            fm.readNode(current, node);
        }
        return current;
    }

public:
    BPlusTree(const std::string& filename) : fm(filename) {}

    bool insert(const char* key, ValueType value) {
        // 空树，创建第一个叶子节点作为根
        if (fm.getRoot() == -1) {
            int pos = fm.allocateBlock();
            Node<ValueType> node;
            node.is_leaf = true;
            node.key_num = 1;
            node.parent = -1;
            strcpy(node.keys[0], key);
            node.values[0] = value;
            fm.writeNode(pos, node);
            fm.setRoot(pos);
            fm.setHeight(1);
            return true;
        }

        int leaf_pos = findLeaf(key, value);
        Node<ValueType> leaf;
        fm.readNode(leaf_pos, leaf);
        
        // 查找插入位置
        int pos = findKeyValuePos(leaf, key, value);
        while (true) {
            // 检查是否重复插入
            if (pos < leaf.key_num && keyValueCompare(leaf.keys[pos], leaf.values[pos], key, value) == 0) {
                return false;
            }
            if (pos < leaf.key_num) 
                break;
            // pos等于节点key总数,找下一个叶子节点
            if (leaf.next != -1) {
                Node<ValueType> next;
                fm.readNode(leaf.next, next);
                if (strcmp(next.keys[0], key) == 0) {
                    // 下一个叶子开头还是相同key，继续在该叶子查找
                    leaf_pos = leaf.next;
                    leaf = next;
                    pos = findKeyValuePos(leaf, key, value);
                    continue;
                }
            }
            break;
        }

        // 插入
        for (int i = leaf.key_num; i > pos; i--) {
            strcpy(leaf.keys[i], leaf.keys[i - 1]);
            leaf.values[i] = leaf.values[i - 1];
        }
        strcpy(leaf.keys[pos], key);
        leaf.values[pos] = value;
        leaf.key_num++;
        
        fm.writeNode(leaf_pos, leaf);

        // 检查是否上溢
        if (leaf.key_num >= MAX_KEY_LEAF) {
            // 找到叶子在父节点中的索引
            Node<ValueType> parent;
            int parent_pos = -1;
            int parent_idx = 0;
            if (leaf.parent != -1) {
                parent_pos = leaf.parent;
                fm.readNode(parent_pos, parent);
                for (; parent_idx <= parent.key_num; parent_idx++) {
                    if (parent.children[parent_idx] == leaf_pos) break;
                }
            }
            splitNode(parent_pos, parent_idx, leaf_pos);
        }
        
        return true;
    }
    
    bool remove(const char* key, ValueType value) {
        if (fm.getRoot() == -1) return false;

        int leaf_pos = findLeaf(key, value);
        Node<ValueType> leaf;
        fm.readNode(leaf_pos, leaf);

        int pos = findKeyValuePos(leaf, key, value);
        bool found = false;
        while (true) {
            if (pos < leaf.key_num && keyValueCompare(leaf.keys[pos], leaf.values[pos], key, value) == 0) {
                found = true;
                break;
            }
            if (pos < leaf.key_num) break;
            
            if (leaf.next != -1) {
                Node<ValueType> next;
                fm.readNode(leaf.next, next);
                if (strcmp(next.keys[0], key) == 0) {
                    leaf_pos = leaf.next;
                    leaf = next;
                    pos = findKeyValuePos(leaf, key, value);
                    continue;
                }
            }
            break;
        }
        if (!found) return false;

        // 删除
        for (int i = pos; i < leaf.key_num - 1; i++) {
            strcpy(leaf.keys[i], leaf.keys[i + 1]);
            leaf.values[i] = leaf.values[i + 1];
        }
        leaf.key_num--;
        fm.writeNode(leaf_pos, leaf);

        // 是否下溢
        if (leaf.key_num < MIN_KEY_LEAF) {
            handleUnderflow(leaf_pos);
        }
        
        return true;
    }
    
    // 输出指定key对应的所有value
    void find(const char* key) {
        if (fm.getRoot() == -1) {
            std::cout << "null\n";
            return;
        }
        int leaf_pos = findLeaf(key, std::numeric_limits<ValueType>::min());
        if (leaf_pos == -1) {
            std::cout << "null\n";
            return;
        }
        
        int pos = 0;
        bool has_result = false;
        bool first = true;
        int current_leaf = leaf_pos;
        bool first_leaf = true;
        
        // 遍历
        while (current_leaf != -1) {
            Node<ValueType> curr;
            fm.readNode(current_leaf, curr);
            // 第一个叶子用二分找起始位置
            pos = first_leaf ? findKeyValuePos(curr, key, std::numeric_limits<ValueType>::min()) : 0;
            first_leaf = false;
            while (pos < curr.key_num && strcmp(curr.keys[pos], key) == 0) {
                if (!first) std::cout << " ";
                std::cout << curr.values[pos];
                first = false;
                has_result = true;
                pos++;
            }
            if (pos < curr.key_num && strcmp(curr.keys[pos], key) > 0) {
                break;
            }
            if (pos < curr.key_num && strcmp(curr.keys[pos], key) != 0) {
                break;
            }
            // 继续下一个叶子节点
            current_leaf = curr.next;
        }
        
        if (!has_result) {
            std::cout << "null";
        }
        std::cout << "\n";
    }
};
