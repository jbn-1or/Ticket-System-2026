#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cmath>       // in case you need it
#include <cstddef>     // for size_t
#include <functional>  // for std::less

#include "exceptions.hpp"

namespace sjtu {

/**
 * @brief A container automatically sorting its contents, similar to
 * std::priority_queue but with extra functionalities.
 *
 * The extra functionalities are:
 * - Merge two priority queues into one (with good time complexity).
 * - Clear all elements in the queue.
 * - Limited exception safety for some operations (e.g. push, pop, top, merge)
 * when the comparator throws exceptions from `Compare` only.
 *
 * This @priority_queue does not support passing an underlying container as a template parameter.
 * Also, it does not support passing a comparator object as a constructor argument.
 *
 */
template <class T, class Compare = std::less<T>>
class priority_queue {
private:
    struct Node{
        T data;
        Node *leftson, *rightson;
        size_t distan;

        Node() : leftson(nullptr), rightson(nullptr), distan(1) {}
        Node(T num, Node *L = nullptr, Node *R = nullptr, size_t d = 1)
                : data(num), leftson(L), rightson(R), distan(d) {}
        ~Node() {};
    };

    Node * root;
    size_t num_of_elem;
    Compare comp;

    // 复制一棵树
    Node* copyTree(Node* that) {
        if (!that) return nullptr;
        Node* new_node = new Node(that->data);
        new_node->leftson = copyTree(that->leftson);
        new_node->rightson = copyTree(that->rightson);
        new_node->distan = that->distan;
        return new_node;
    }

    // 销毁一棵树
    void cutDownTree(Node* node) {
        if (!node) return;
        cutDownTree(node->leftson);
        cutDownTree(node->rightson);
        delete node;
    }

    // 合并两棵树
    Node * mergeTree(Node * root_of1, Node * root_of2) {
        if (!root_of1) return root_of2;
        if (!root_of2) return root_of1;
        if (root_of1 == root_of2) {
            return root_of1;
        }
        if (comp(root_of1->data, root_of2->data)) {
            std::swap(root_of1, root_of2);
        }
        // 递归合并
        root_of1->rightson = mergeTree(root_of1->rightson, root_of2);
        // 左右子树是否交换
        if (! root_of1->leftson || root_of1->leftson->distan < root_of1->rightson->distan) {
            std::swap(root_of1->leftson, root_of1->rightson);
        }
        // 更新distance
        root_of1->distan = ((root_of1->rightson) ? root_of1->rightson->distan + 1 : 1);
        return root_of1;
    }

public:
    priority_queue() : root(nullptr), num_of_elem(0), comp(Compare()) {};
    priority_queue(Node * node) : root(node), num_of_elem(1), comp(Compare()) {}
    priority_queue(const priority_queue& other)
            : root(copyTree(other.root)), num_of_elem(other.num_of_elem), comp(other.comp) {
    }

    ~priority_queue() {
        clear();
    }

    priority_queue& operator=(const priority_queue& other) {
        if (this == &other) {
            return *this;
        }
        cutDownTree(root);
        root = nullptr;
        num_of_elem = 0;
        comp = other.comp;
        if (other.empty()) {
            return *this;
        }
        root = copyTree(other.root);
        num_of_elem = other.num_of_elem;
        return *this;
    }

    /** Adds one element to the queue. */
    void push(const T& value) {
        Node* new_node = new Node(value);
        try {
            root = mergeTree(root, new_node);
            num_of_elem++;
        } catch (...) {
            delete new_node;
            throw;
        }
    }

    /**
     * Returns a read-only reference of the first element in the queue.
     *
     * @throws container_is_empty when the first element does not exist.
     */
    const T& top() const {
        if (empty()) {
            throw container_is_empty();
        }
        return root->data;
    }

    /**
     * Removes the first element in the queue.
     *
     * @throws container_is_empty when the first element does not exist.
     */
    void pop() {
        if (empty()) {
            throw container_is_empty();
        }
        Node * old_root = root;
        root = mergeTree(root->leftson, root->rightson);
        delete old_root;
        old_root = nullptr;
        num_of_elem--;
    }

    /** Returns the number of elements in the queue. */
    size_t size() const {
        return num_of_elem;
    }

    /** Returns whether there is any element in the queue. */
    bool empty() const {
        return (num_of_elem == 0);
    }

    /** Clears all elements in the queue. */
    void clear() {
        cutDownTree(root);
        root = nullptr;
        num_of_elem = 0;
        return;
    }

    /**
     * @brief Merges two priority queues into one.
     *
     * The merged data shall be stored in the current priority queue and the
     * other priority queue shall be cleared after merging.
     *
     * The time complexity shall be O(log n) or better.
     */
    void merge(priority_queue& other) {
        if (this == &other) return;
        root = mergeTree(root, other.root);
        num_of_elem += other.num_of_elem;
        // 清空被合并对象
        other.root = nullptr;
        other.num_of_elem = 0;
    }
};

}  // namespace sjtu

#endif