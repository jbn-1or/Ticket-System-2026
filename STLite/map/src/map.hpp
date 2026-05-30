/**
 * implement a container like std::map
 */
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

// only for std::less<T>
#include <cstddef>
#include <functional>

#include "exceptions.hpp"
#include "utility.hpp"

namespace sjtu {

template <class Key, class T, class Compare = std::less<Key> >
class map {
   public:
    /**
     * the internal type of data.
     * it should have a default constructor, a copy constructor.
     * You can use sjtu::map as value_type by typedef.
     */
    typedef pair<const Key, T> value_type;
    struct Node {
        value_type value;
        Node* leftson;
        Node* rightson;
        Node* parent;
        int height; // 到最远叶子节点的节点数

        Node(const value_type& val, Node* ls = nullptr, Node* rs = nullptr, int h = 1)
            : value(val), leftson(ls), rightson(rs), height(h), parent(nullptr) {
        }
    };

   private:
    Node* root;
    size_t num_of_elem;
    Compare comp;

    inline int getHeight(Node* node) const {
        return node ? node->height : 0;
    }

    // 复制一棵树
    Node* copyTree(Node* that, Node* parent_node = nullptr) {
        if (!that) return nullptr;
        Node* new_node = new Node(that->value);
        new_node->parent = parent_node;
        new_node->leftson = copyTree(that->leftson, new_node);
        new_node->rightson = copyTree(that->rightson, new_node);
        new_node->height = that->height;
        return new_node;
    }

    // 销毁一棵树
    void cutDownTree(Node* node) {
        if (!node) return;
        cutDownTree(node->leftson);
        cutDownTree(node->rightson);
        delete node;
    }

    // 右旋操作， 处理LL
    Node* rightRotate(Node* old_root) {
        Node* new_root = old_root->leftson;
        Node* tmp = new_root->rightson;

        // 先修改子节点指针
        new_root->rightson = old_root;
        old_root->leftson = tmp;

        if (tmp) tmp->parent = old_root;
        new_root->parent = old_root->parent; 
        old_root->parent = new_root;

        // 更新节点高度
        old_root->height = std::max(getHeight(old_root->leftson), getHeight(old_root->rightson)) + 1;
        new_root->height = std::max(getHeight(new_root->leftson), getHeight(new_root->rightson)) + 1;
        
        return new_root;
    }

    // 左旋操作，处理RR
    Node* leftRotate(Node* old_root) {
        Node* new_root = old_root->rightson;
        Node* tmp = new_root->leftson;

        new_root->leftson = old_root;
        old_root->rightson = tmp;

        if (tmp) tmp->parent = old_root;
        new_root->parent = old_root->parent;
        old_root->parent = new_root;

        old_root->height = std::max(getHeight(old_root->leftson), getHeight(old_root->rightson)) + 1;
        new_root->height = std::max(getHeight(new_root->leftson), getHeight(new_root->rightson)) + 1;
        
        return new_root;
    }


    // LR，先左旋再右旋
    Node* LR(Node* old_root) {
        old_root->leftson = leftRotate(old_root->leftson);
        return rightRotate(old_root);
    }

    // RL，先右旋再左旋
    Node* RL(Node* old_root) {
        old_root->rightson = rightRotate(old_root->rightson);
        return leftRotate(old_root);
    }

    // 寻找当前节点下的最小值
    inline Node* minNode(Node* node) const {
        while (node && node->leftson) {
            node = node->leftson;
        }
        return node;
    }

    // 寻找当前节点下的最大值
    inline Node* maxNode(Node* node) const {
        while (node && node->rightson) {
            node = node->rightson;
        }
        return node;
    }

    // 用于erase，有两个儿子时，选择右子树的最小节点
    // 寻找当前节点下的最小值，并删除，返回新的子树根节点
    Node* removeMin(Node* node, Node*& minNode) {
        if (!node) return nullptr;
        if (!node->leftson) {
            minNode = node;
            return node->rightson;
        }
        node->leftson = removeMin(node->leftson, minNode);
        if (node->leftson) node->leftson->parent = node;

        // 高度与平衡调整
        int lh = getHeight(node->leftson);
        int rh = getHeight(node->rightson);
        node->height = std::max(lh, rh) + 1;
        int bf = lh - rh;

        if (bf > 1) {
            int lh_left = getHeight(node->leftson->leftson);
            int rh_left = getHeight(node->leftson->rightson);
            if (lh_left - rh_left >= 0)
                return rightRotate(node);
            else
                return LR(node);
        }
        if (bf < -1) {
            int lh_right = getHeight(node->rightson->leftson);
            int rh_right = getHeight(node->rightson->rightson);
            if (lh_right - rh_right <= 0)
                return leftRotate(node);
            else
                return RL(node);
        }
        return node;
    }

    // 判断Key是否相等
    bool isEqualKey(const Key& lhs, const Key& rhs) const {
        return !comp(lhs, rhs) && !comp(rhs, lhs);
    }

    // 插入一个节点，
    // bool& inserted标记本次插入是否真的新建了节点
    // Node*& result 最终插入的节点指针
    Node* insertNode(Node* node, const value_type& value, bool& inserted, Node*& result) {
        if (!node) {
            result = new Node(value);
            inserted = true;
            return result;
        }
        const bool key_less = comp(value.first, node->value.first);
        if (key_less) {
            node->leftson = insertNode(node->leftson, value, inserted, result);
            node->leftson->parent = node;
        } else {
            const bool key_greater = comp(node->value.first, value.first);
            if (key_greater) {
                node->rightson = insertNode(node->rightson, value, inserted, result);
                node->rightson->parent = node;
            } else {
                result = node;
                inserted = false;
                return node;
            }
        }
        // 处理不平衡
        int lh = getHeight(node->leftson);
        int rh = getHeight(node->rightson);
        node->height = std::max(lh, rh) + 1;
        int bf = lh - rh;
        if (bf > 1) {
            if (comp(value.first, node->leftson->value.first)) {
                return rightRotate(node);
            }
            else {
                return LR(node);
            }
        }
        if (bf < -1) {
            if (comp(node->rightson->value.first, value.first)) {
                return leftRotate(node);
            }
            else {
                return RL(node);
            }
        }
        return node;
    }

    Node* eraseNode(Node* node, const Key& key, bool& erased) {
        if (!node) {
            return nullptr;
        }
        if (comp(key, node->value.first)) {
            node->leftson = eraseNode(node->leftson, key, erased);
            if (node->leftson) node->leftson->parent = node;
        } else if (comp(node->value.first, key)) {
            node->rightson = eraseNode(node->rightson, key, erased);
            if (node->rightson) node->rightson->parent = node;
        } else {
            erased = true;
            // 没有孩子或只有一个孩子
            if (!node->leftson || !node->rightson) {
                Node* child = node->leftson ? node->leftson : node->rightson;
                if (child) child->parent = node->parent;
                delete node;
                return child;
            }
            // 有两个孩子
            Node* substitute = nullptr;
            // 移除右子树最小节点
            node->rightson = removeMin(node->rightson, substitute);
            // 用后继节点替换当前被删除节点
            substitute->leftson = node->leftson;
            if (substitute->leftson) substitute->leftson->parent = substitute;
            substitute->rightson = node->rightson;
            if (substitute->rightson) substitute->rightson->parent = substitute;
            substitute->parent = node->parent;

            delete node;
            node = substitute;
        }

        if (!node) return nullptr;
        int lh = getHeight(node->leftson);
        int rh = getHeight(node->rightson);
        node->height = std::max(lh, rh) + 1;
        int bf = lh - rh;
        
        if (bf > 1) {
            int lh_left = getHeight(node->leftson->leftson);
            int rh_left = getHeight(node->leftson->rightson);
            if (lh_left - rh_left >= 0) {
                return rightRotate(node);
            }
            else {
                return LR(node);
            }
        }
        if (bf < -1) {
            int lh_right = getHeight(node->rightson->leftson);
            int rh_right = getHeight(node->rightson->rightson);
            if (lh_right - rh_right <= 0) {
                return leftRotate(node);
            }
            else {
                return RL(node);
            }
        }
        return node;
    }

    // 根据key去寻找节点
    inline Node* findNode(const Key& key) const {
        Node* cur = root;
        while (cur) {
            const bool key_less = comp(key, cur->value.first);
            if (key_less) {
                cur = cur->leftson;
            } else {
                const bool key_greater = comp(cur->value.first, key);
                if (key_greater) {
                    cur = cur->rightson;
                } else {
                    return cur;
                }
            }
        }
        return nullptr;
    }

    // 找比当前节点大的最小节点 (后继)
    inline Node* nextNode(Node* node) const {
        if (!node) return nullptr;
        // 有右子树
        if (node->rightson) {
            return minNode(node->rightson);
        }

        // 没有右子树，向上回溯， 找到第一个当前节点是左孩子的祖先
        Node* p = node->parent;
        Node* cur = node;
        while (p != nullptr && cur == p->rightson) {
            cur = p;
            p = p->parent;
        }
        return p;
    }

    // 找比当前节点小的最大节点 (前驱)
    inline Node* prevNode(Node* node) const {
        if (!node) return nullptr;
        // 有左子树
        if (node->leftson) {
            return maxNode(node->leftson);
        }
        // 没有左子树
        Node* p = node->parent;
        Node* cur = node;
        while (p != nullptr && cur == p->leftson) {
            cur = p;
            p = p->parent;
        }
        return p;
    }
    /**
     * see BidirectionalIterator at CppReference for help.
     *
     * if there is anything wrong throw invalid_iterator.
     *     like it = map.begin(); --it;
     *       or it = map.end(); ++end();
     */
   public:
    class const_iterator;
    class iterator {
        friend class map;
        friend class const_iterator;

       private:
        /**
         * TODO add data members
         *   just add whatever you want.
         */
        Node* node;
        const map* container;

       public:
        iterator() : node(nullptr), container(nullptr) {
        }
        iterator(Node* n, const map* c) : node(n), container(c) {
        }
        iterator(const iterator& other) : node(other.node), container(other.container) {
        }

        /**
         * TODO iter++
         */
        iterator operator++(int) {
            iterator tmp(*this);
            ++*this;
            return tmp;
        }

        /**
         * TODO ++iter
         */
        iterator& operator++() {
            if (!container) {
                throw invalid_iterator();
            }
            if (!node) {
                throw invalid_iterator();
            }
            node = container->nextNode(node);
            return *this;
        }

        /**
         * TODO iter--
         */
        iterator operator--(int) {
            iterator tmp(*this);
            --*this;
            return tmp;
        }

        /**
         * TODO --iter
         */
        iterator& operator--() {
            if (!container) {
                throw invalid_iterator();
            }
            if (!node) {
                if (container->empty()) {
                    throw invalid_iterator();
                }
                node = container->maxNode(container->root);
                return *this;
            }
            Node* prev = container->prevNode(node);
            if (!prev) {
                throw invalid_iterator();
            }
            node = prev;
            return *this;
        }

        /**
         * a operator to check whether two iterators are same (pointing to the
         * same memory).
         */
        value_type& operator*() const {
            if (!container || !node) {
                throw invalid_iterator();
            }
            return node->value;
        }

        bool operator==(const iterator& rhs) const {
            return container == rhs.container && node == rhs.node;
        }

        bool operator==(const const_iterator& rhs) const {
            return container == rhs.container && node == rhs.node;
        }

        /**
         * some other operator for iterator.
         */
        bool operator!=(const iterator& rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const const_iterator& rhs) const {
            return !(*this == rhs);
        }

        /**
         * for the support of it->first.
         * See
         * <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/>
         * for help.
         */
        value_type* operator->() const noexcept {
            return &node->value;
        }
    };

    class const_iterator {
        // it should has similar member method as iterator.
        //  and it should be able to construct from an iterator.
        friend class map;
        friend class iterator;

       private:
        // data members.
        const Node* node;
        const map* container;

       public:
        const_iterator() : node(nullptr), container(nullptr) {
        }
        const_iterator(const Node* n, const map* c) : node(n), container(c) {
        }
        const_iterator(const const_iterator& other)
            : node(other.node), container(other.container) {
        }
        const_iterator(const iterator& other)
            : node(other.node), container(other.container) {
        }

        // And other methods in iterator.
        // And other methods in iterator.
        // And other methods in iterator.
        const_iterator operator++(int) {
            const_iterator tmp(*this);
            ++*this;
            return tmp;
        }

        const_iterator& operator++() {
            if (!container || !node) throw invalid_iterator();
            node = container->nextNode(const_cast<Node*>(node));
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator tmp(*this);
            --*this;
            return tmp;
        }

        const_iterator& operator--() {
            if (!container) {
                throw invalid_iterator();
            }
            if (!node) {
                if (container->empty()) {
                    throw invalid_iterator();
                }
                node = container->maxNode(container->root);
                return *this;
            }
            const Node* prev = container->prevNode(const_cast<Node*>(node));
            if (!prev) {
                throw invalid_iterator();
            }
            node = prev;
            return *this;
        }

        const value_type& operator*() const {
            if (!container || !node) {
                throw invalid_iterator();
            }
            return node->value;
        }

        bool operator==(const const_iterator& rhs) const {
            return container == rhs.container && node == rhs.node;
        }

        bool operator==(const iterator& rhs) const {
            return container == rhs.container && node == rhs.node;
        }

        bool operator!=(const const_iterator& rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const iterator& rhs) const {
            return !(*this == rhs);
        }

        const value_type* operator->() const noexcept {
            return &node->value;
        }
    };

    /**
     * TODO two constructors
     */
    map() : root(nullptr), num_of_elem(0), comp(Compare()) {
    }

    map(const map& other)
        : root(copyTree(other.root)), num_of_elem(other.num_of_elem), comp(other.comp) {
    }

    /**
     * TODO assignment operator
     */
    map& operator=(const map& other) {
        if (this == &other) {
            return *this;
        }
        cutDownTree(root);
        root = nullptr;
        num_of_elem = 0;
        comp = other.comp;
        if (!other.empty()) {
            root = copyTree(other.root);
            num_of_elem = other.num_of_elem;
        }
        return *this;
    }

    /**
     * TODO Destructors
     */
    ~map() {
        clear();
    }

    /**
     * TODO
     * access specified element with bounds checking
     * Returns a reference to the mapped value of the element with key
     * equivalent to key. If no such element exists, an exception of type
     * `index_out_of_bound'
     */
    T& at(const Key& key) {
        Node* node = findNode(key);
        if (!node) {
            throw index_out_of_bound();
        }
        return node->value.second;
    }

    const T& at(const Key& key) const {
        Node* node = findNode(key);
        if (!node) {
            throw index_out_of_bound();
        }
        return node->value.second;
    }

    /**
     * TODO
     * access specified element
     * Returns a reference to the value that is mapped to a key equivalent to
     * key, performing an insertion if such key does not already exist.
     */
    T& operator[](const Key& key) {
        Node* node = findNode(key);
        if (node) {
            return node->value.second;
        }
        value_type value(key, T());
        Node* result = nullptr;
        bool inserted = false;
        root = insertNode(root, value, inserted, result);
        if (inserted) {
            ++num_of_elem;
        }
        return result->value.second;
    }

    /**
     * behave like at() throw index_out_of_bound if such key does not exist.
     */
    const T& operator[](const Key& key) const {
        return at(key);
    }

    /**
     * return a iterator to the beginning
     */
    iterator begin() {
        return iterator(root ? minNode(root) : nullptr, this);
    }

    const_iterator cbegin() const {
        return const_iterator(root ? minNode(root) : nullptr, this);
    }

    /**
     * return a iterator to the end
     * in fact, it returns past-the-end.
     */
    iterator end() {
        return iterator(nullptr, this);
    }

    const_iterator cend() const {
        return const_iterator(nullptr, this);
    }

    /**
     * checks whether the container is empty
     * return true if empty, otherwise false.
     */
    bool empty() const {
        return num_of_elem == 0;
    }

    /**
     * returns the number of elements.
     */
    size_t size() const {
        return num_of_elem;
    }

    /**
     * clears the contents
     */
    void clear() {
        cutDownTree(root);
        root = nullptr;
        num_of_elem = 0;
    }

    /**
     * insert an element.
     * return a pair, the first of the pair is
     *   the iterator to the new element (or the element that prevented the
     * insertion), the second one is true if insert successfully, or false.
     */
    pair<iterator, bool> insert(const value_type& value) {
        Node* result = nullptr;
        bool inserted = false;
        root = insertNode(root, value, inserted, result);
        if (inserted) {
            ++num_of_elem;
        }
        return pair<iterator, bool>(iterator(result, this), inserted);
    }

    /**
     * erase the element at pos.
     *
     * throw if pos pointed to a bad element (pos == this->end() || pos points
     * an element out of this)
     */
    void erase(iterator pos) {
        if (!pos.container || pos.container != this || !pos.node) {
            throw invalid_iterator();
        }
        bool erased = false;
        root = eraseNode(root, pos.node->value.first, erased);
        if (!erased) {
            throw invalid_iterator();
        }
        --num_of_elem;
    }

    /**
     * Returns the number of elements with key
     *   that compares equivalent to the specified argument,
     *   which is either 1 or 0
     *     since this container does not allow duplicates.
     * The default method of check the equivalence is !(a < b || b > a)
     */
    size_t count(const Key& key) const {
        return findNode(key) ? 1 : 0;
    }

    /**
     * Finds an element with key equivalent to key.
     * key value of the element to search for.
     * Iterator to an element with key equivalent to key.
     *   If no such element is found, past-the-end (see end()) iterator is
     * returned.
     */
    iterator find(const Key& key) {
        return iterator(findNode(key), this);
    }

    const_iterator find(const Key& key) const {
        return const_iterator(findNode(key), this);
    }
};

}  // namespace sjtu

#endif