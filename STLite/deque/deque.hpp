#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include <cstddef>

#include "exceptions.hpp"

namespace sjtu {

template <class T>
class deque {
   private:
    T** chunk_ptrs;
    size_t capacity;         // 当前chunk队列容量
    size_t idx_of_begin;     // 当前第一个chunk所在的位置
    size_t idx_of_end;       // 当前最后一个chunk所在编号
    size_t front_idx_;       // 第一个有效chunk中，第一个元素的下标
    size_t back_idx_;        // 最后一个有效chunk中，最后一个元素的下标
    size_t num_of_elements;  // 当前deque的总元素个数
    static const size_t CHUNK_SIZE = 16;

    // 构造与析构的辅助函数
    // 分配一个空chunk
    T* allocateChunk() {
        return static_cast<T*>(operator new(CHUNK_SIZE * sizeof(T)));
    }
    // 释放chunk的内存
    void deallocateChunk(T* chunk) {
        if (chunk != nullptr) {
            operator delete(chunk);
        }
    }
    // 销毁chunk内的元素
    void destroyChunk(T* chunk, size_t start, size_t end) {
        if (chunk == nullptr) return;
        for (size_t i = start; i <= end; ++i) {
            chunk[i].~T();
        }
    }
    // 扩容
    void expandChunkArray() {
        size_t new_capacity = (capacity == 0) ? 4 : capacity * 2;
        T** new_chunk_ptrs = static_cast<T**>(operator new[](new_capacity * sizeof(T*)));

        for (size_t i = 0; i < new_capacity; ++i) {
            new_chunk_ptrs[i] = nullptr;
        }

        size_t old_chunk_count = getNumOfChunks();
        size_t offset = (new_capacity - old_chunk_count) / 2;

        for (size_t i = 0; i < old_chunk_count; ++i) {
            new_chunk_ptrs[offset + i] = chunk_ptrs[idx_of_begin + i];
        }
        if (chunk_ptrs) {
            operator delete[](chunk_ptrs);
        }
        chunk_ptrs = new_chunk_ptrs;
        capacity = new_capacity;

        if (old_chunk_count == 0) {
            idx_of_begin = idx_of_end = offset;
        } else {
            idx_of_begin = offset;
            idx_of_end = offset + old_chunk_count - 1;
        }
    }

    size_t getNumOfChunks() const {
        if (empty()) return 0;
        return idx_of_end - idx_of_begin + 1;
    }

   public:
    class const_iterator;
    class iterator {
       private:
        /**
         * TODO add data members
         *   just add whatever you want.
         */
        deque* deque_ptr;  // 指向所属 deque 的指针
        size_t chunk_idx;  // 当前元素所在的 chunk 索引
        size_t elem_idx;   // 当前元素在 chunk 中的下标

        size_t get_position() const {
            if (!deque_ptr || deque_ptr->empty()) {
                return 0;
            }
            size_t pos = 0;
            if (chunk_idx == deque_ptr->idx_of_begin) {
                pos = elem_idx - deque_ptr->front_idx_;
            } else {
                pos += deque_ptr->CHUNK_SIZE - deque_ptr->front_idx_;
                pos += (chunk_idx - deque_ptr->idx_of_begin - 1) *
                       deque_ptr->CHUNK_SIZE;
                pos += elem_idx;
            }
            return pos;
        }

        void move(int n) {
            if (!deque_ptr) return;
            if (n == 0) return;
            if (deque_ptr->empty()) return;

            int cur = (int)get_position();
            int target = cur + n;
            size_t pos = (size_t)target;
            if (pos == deque_ptr->num_of_elements) {
                chunk_idx = deque_ptr->idx_of_end;
                elem_idx = deque_ptr->back_idx_ + 1;
                return;
            }

            size_t absolute = deque_ptr->front_idx_ + pos;
            size_t chunk_offset = absolute / deque_ptr->CHUNK_SIZE;
            chunk_idx = deque_ptr->idx_of_begin + chunk_offset;
            elem_idx = absolute % deque_ptr->CHUNK_SIZE;
        }

        friend class deque;
        friend class const_iterator;

       public:
        iterator() : deque_ptr(nullptr), chunk_idx(0), elem_idx(0) {}

        iterator(deque* dq, size_t c_idx, size_t e_idx)
            : deque_ptr(dq), chunk_idx(c_idx), elem_idx(e_idx) {}

        iterator(const iterator& it) : deque_ptr(it.deque_ptr),
              chunk_idx(it.chunk_idx), elem_idx(it.elem_idx) {}
        /**
         * return a new iterator which pointer n-next elements
         *   even if there are not enough elements, the behaviour is
         * **undefined**. as well as operator-
         */
        iterator operator+(const int& n) const {
            // TODO
            iterator tmp = *this;
            tmp.move(n);
            return tmp;
        }
        iterator operator-(const int& n) const {
            // TODO
            iterator tmp = *this;
            tmp.move(-n);
            return tmp;
        }

        // return the distance between two iterator,
        // if these two iterators points to different vectors, throw
        // invaild_iterator.
        int operator-(const iterator& rhs) const {
            // TODO
            if (deque_ptr != rhs.deque_ptr) throw invalid_iterator();
            return get_position() - rhs.get_position();
        }

        iterator operator+=(const int& n) {
            // TODO
            this->move(n);
            return *this;
        }

        iterator operator-=(const int& n) {
            // TODO
            this->move(-n);
            return *this;
        }
        /**
         * TODO iter++
         */
        iterator operator++(int) {
            iterator tmp = *this;
            move(1);
            return tmp;
        }
        /**
         * TODO ++iter
         */
        iterator& operator++() {
            move(1);
            return *this;
        }
        /**
         * TODO iter--
         */
        iterator operator--(int) {
            iterator tmp = *this;
            move(-1);
            return tmp;
        }
        /**
         * TODO --iter
         */
        iterator& operator--() {
            move(-1);
            return *this;
        }
        /**
         * TODO *it
         */
        T& operator*() const {
            if (!deque_ptr || !deque_ptr->is_valid_iterator(*this) ||
                *this == deque_ptr->end()) {
                throw invalid_iterator();
            }
            return deque_ptr->chunk_ptrs[chunk_idx][elem_idx];
        }
        /**
         * TODO it->field
         */
        T* operator->() const {
            if (!deque_ptr || !deque_ptr->is_valid_iterator(*this) ||
                *this == deque_ptr->end()) {
                throw invalid_iterator();
            }
            return &(deque_ptr->chunk_ptrs[chunk_idx][elem_idx]);
        }
        /**
         * a operator to check whether two iterators are same (pointing to the
         * same memory).
         */
        bool operator==(const iterator& rhs) const {
            if (deque_ptr != rhs.deque_ptr) {
                return false;
            }
            return (chunk_idx == rhs.chunk_idx) && (elem_idx == rhs.elem_idx);
        }

        bool operator==(const const_iterator& rhs) const {
            if (deque_ptr != rhs.deque_ptr) {
                return false;
            }
            return (chunk_idx == rhs.chunk_idx) && (elem_idx == rhs.elem_idx);
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
    };
    class const_iterator {
        // it should has similar member method as iterator.
        //  and it should be able to construct from an iterator.
       private:
        // data members.
        const deque* deque_ptr;
        size_t chunk_idx;
        size_t elem_idx;

        size_t get_position() const {
            if (!deque_ptr || deque_ptr->empty()) return 0;

            size_t pos = 0;
            if (chunk_idx == deque_ptr->idx_of_begin) {
                return elem_idx - deque_ptr->front_idx_;
            }

            pos += deque_ptr->CHUNK_SIZE - deque_ptr->front_idx_;
            pos += (chunk_idx - deque_ptr->idx_of_begin - 1) *
                   deque_ptr->CHUNK_SIZE;
            pos += elem_idx;

            return pos;
        }

        void move(int n) {
            if (!deque_ptr) return;
            if (n == 0) return;
            if (deque_ptr->empty()) return;

            int current_pos = (int)get_position();
            int target = current_pos + n;
            size_t pos = (size_t)target;

            if (pos == deque_ptr->num_of_elements) {
                chunk_idx = deque_ptr->idx_of_end;
                elem_idx = deque_ptr->back_idx_ + 1;
                return;
            }

            size_t absolute = deque_ptr->front_idx_ + pos;
            size_t chunk_offset = absolute / deque_ptr->CHUNK_SIZE;
            chunk_idx = deque_ptr->idx_of_begin + chunk_offset;
            elem_idx = absolute % deque_ptr->CHUNK_SIZE;
        }

       public:
        friend class deque;
        const_iterator() : deque_ptr(nullptr), chunk_idx(0), elem_idx(0) {}

        const_iterator(const deque* dq, size_t c_idx, size_t e_idx,
            bool end = false) : deque_ptr(dq), chunk_idx(c_idx), elem_idx(e_idx) {}

        const_iterator(const iterator& it) : deque_ptr(it.deque_ptr),
              chunk_idx(it.chunk_idx), elem_idx(it.elem_idx) {}

        const_iterator operator+(const int& n) const {
            // TODO
            const_iterator res = *this;
            res.move(n);
            return res;
        }

        const_iterator operator-(const int& n) const {
            // TODO
            const_iterator res = *this;
            res.move(-n);
            return res;
        }

        int operator-(const const_iterator& rhs) const {
            // TODO
            if (deque_ptr != rhs.deque_ptr) throw invalid_iterator();
            return (get_position() - rhs.get_position());
        }

        const_iterator& operator+=(const int& n) {
            // TODO
            move(n);
            return *this;
        }

        const_iterator& operator-=(const int& n) {
            // TODO
            move(-n);
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            move(1);
            return tmp;
        }
        const_iterator& operator++() {
            move(1);
            return *this;
        }
        const_iterator operator--(int) {
            const_iterator tmp = *this;
            move(-1);
            return tmp;
        }
        const_iterator& operator--() {
            move(-1);
            return *this;
        }
        const T& operator*() const {
            if (!deque_ptr || !deque_ptr->is_valid_iterator(*this) ||
                *this == deque_ptr->cend()) {
                throw invalid_iterator();
            }
            return deque_ptr->chunk_ptrs[chunk_idx][elem_idx];
        }
        const T* operator->() const {
            if (!deque_ptr || !deque_ptr->is_valid_iterator(*this) ||
                *this == deque_ptr->cend()) {
                throw invalid_iterator();
            }
            return &(deque_ptr->chunk_ptrs[chunk_idx][elem_idx]);
        }
        bool operator==(const iterator& rhs) const {
            if (deque_ptr != rhs.deque_ptr) return false;
            return (chunk_idx == rhs.chunk_idx) && (elem_idx == rhs.elem_idx);
        }
        bool operator==(const const_iterator& rhs) const {
            if (deque_ptr != rhs.deque_ptr) return false;
            return (chunk_idx == rhs.chunk_idx) && (elem_idx == rhs.elem_idx);
        }
        bool operator!=(const iterator& rhs) const {
            return !(*this == rhs);
        }
        bool operator!=(const const_iterator& rhs) const {
            return !(*this == rhs);
        }
    };

    iterator begin() {
        if (empty()) return end();
        return iterator(this, idx_of_begin, front_idx_);
    }

    const_iterator cbegin() const {
        if (empty()) return cend();
        return const_iterator(this, idx_of_begin, front_idx_);
    }

    iterator end() {
        return iterator(this, idx_of_end, back_idx_ + 1);
    }

    const_iterator cend() const {
        return const_iterator(this, idx_of_end, back_idx_ + 1);
    }

    /**
     * TODO Constructors
     */
    deque() : chunk_ptrs(nullptr), capacity(0), idx_of_begin(0), idx_of_end(0), front_idx_(0), 
        back_idx_(0), num_of_elements(0) {}
    deque(const deque& other) : chunk_ptrs(nullptr), capacity(0), idx_of_begin(0), idx_of_end(0), front_idx_(0),
        back_idx_(0), num_of_elements(0) {
        *this = other;
    }
    /**
     * TODO Deconstructor
     */
    ~deque() {
        clear();
    }
    /**
     * TODO assignment operator
     */
    deque& operator=(const deque& other) {
        if (this == &other) {
            return *this;
        }
        clear();
        // 处理空对象
        if (other.empty()) {
            capacity = 0;
            chunk_ptrs = nullptr;
            idx_of_begin = idx_of_end = 0;
            front_idx_ = back_idx_ = 0;
            num_of_elements = 0;
            return *this;
        }

        capacity = other.capacity;
        chunk_ptrs = static_cast<T**>(operator new[](capacity * sizeof(T*)));
        for (size_t i = 0; i < capacity; ++i) {
            chunk_ptrs[i] = nullptr;
        }
        size_t other_chunk_count = other.getNumOfChunks();
        // 计算当前对象的 chunk 起始位置
        idx_of_begin = (capacity - other_chunk_count) / 2;
        idx_of_end = idx_of_begin + other_chunk_count - 1;

        for (size_t i = 0; i < other_chunk_count; ++i) {
            size_t other_chunk_idx = other.idx_of_begin + i;
            size_t curr_chunk_idx = idx_of_begin + i;
            // 跳过空 chunk
            if (other.chunk_ptrs[other_chunk_idx] == nullptr) {
                continue;
            }
            // 分配新 chunk
            chunk_ptrs[curr_chunk_idx] = allocateChunk();
            T* src_chunk = other.chunk_ptrs[other_chunk_idx];
            T* dst_chunk = chunk_ptrs[curr_chunk_idx];
            size_t start = (i == 0) ? other.front_idx_ : 0;
            size_t end = (i == other_chunk_count - 1) ? other.back_idx_ : CHUNK_SIZE - 1;
            for (size_t j = start; j <= end; ++j) {
                new (dst_chunk + j) T(src_chunk[j]);
            }
        }
        front_idx_ = other.front_idx_;
        back_idx_ = other.back_idx_;
        num_of_elements = other.num_of_elements;
        return *this;
    }
    /**
     * access specified element with bounds checking
     * throw index_out_of_bound if out of bound.
     */
    T& at(const size_t& pos) {
        if (pos >= num_of_elements) {
            throw index_out_of_bound();
        }
        return operator[](pos);
    }

    const T& at(const size_t& pos) const {
        if (pos >= num_of_elements) {
            throw index_out_of_bound();
        }
        return operator[](pos);
    }

    T& operator[](const size_t& pos) {
        if (pos >= num_of_elements) {
            throw index_out_of_bound();
        }
            iterator ans;// = begin();

        size_t absolute = front_idx_ + pos;
        size_t chunk_offset = absolute / CHUNK_SIZE;
        size_t idx = absolute % CHUNK_SIZE;
        return chunk_ptrs[idx_of_begin + chunk_offset][idx];
    }

    const T& operator[](const size_t& pos) const {
        if (pos >= num_of_elements) {
            throw index_out_of_bound();
        }
        size_t absolute = front_idx_ + pos;
        size_t chunk_offset = absolute / CHUNK_SIZE;
        size_t idx = absolute % CHUNK_SIZE;
        return chunk_ptrs[idx_of_begin + chunk_offset][idx];
    }
    /**
     * access the first element
     * throw container_is_empty when the container is empty.
     */
    const T& front() const {
        if (empty()) {
            throw container_is_empty();
        }
        return chunk_ptrs[idx_of_begin][front_idx_];
    }
    /**
     * access the last element
     * throw container_is_empty when the container is empty.
     */
    const T& back() const {
        if (empty()) {
            throw container_is_empty();
        }
        return chunk_ptrs[idx_of_end][back_idx_];
    }
    /**
     * checks whether the container is empty.
     */
    bool empty() const {
        return num_of_elements == 0;
    }
    /**
     * returns the number of elements
     */
    size_t size() const {
        return num_of_elements;
    }
    /**
     * clears the contents
     */
    void clear() {
        if (chunk_ptrs != nullptr) {
            for (size_t i = idx_of_begin; i <= idx_of_end; ++i) {
                if (chunk_ptrs[i]) {
                    size_t start = (i == idx_of_begin) ? front_idx_ : 0;
                    size_t end = (i == idx_of_end) ? back_idx_ : CHUNK_SIZE - 1;
                    destroyChunk(chunk_ptrs[i], start, end);
                    deallocateChunk(chunk_ptrs[i]);
                }
            }
            operator delete[](chunk_ptrs);
        }

        chunk_ptrs = nullptr;
        capacity = 0;
        idx_of_begin = idx_of_end = 0;
        front_idx_ = back_idx_ = 0;
        num_of_elements = 0;
    }

    // 检查迭代器是否属于当前 deque 且有效
    bool is_valid_iterator(const iterator& it) const {
        if (it.deque_ptr != this) return false;
        if (it.chunk_idx == idx_of_end && it.elem_idx == back_idx_ + 1) {
            return true;
        }
        if (it.chunk_idx < idx_of_begin || it.chunk_idx > idx_of_end)
            return false;
        if (it.chunk_idx == idx_of_begin && it.elem_idx < front_idx_)
            return false;
        if (it.chunk_idx == idx_of_end) {
            if (it.elem_idx > back_idx_ + 1) return false;
        } else {
            if (it.elem_idx >= CHUNK_SIZE) return false;
        }
        return true;
    }
    bool is_valid_iterator(const const_iterator& it) const {
        if (it.deque_ptr != this) return false;
        if (it.chunk_idx == idx_of_end && it.elem_idx == back_idx_ + 1) {
            return true;
        }
        if (it.chunk_idx < idx_of_begin || it.chunk_idx > idx_of_end)
            return false;
        if (it.chunk_idx == idx_of_begin && it.elem_idx < front_idx_)
            return false;
        if (it.chunk_idx == idx_of_end) {
            if (it.elem_idx > back_idx_ + 1) return false;
        } else {
            if (it.elem_idx >= CHUNK_SIZE) return false;
        }
        return true;
    }

    // 移动 chunk 内的元素，true后移，false前移
    void move_elements(T* chunk, size_t src, size_t dst, size_t count, bool forward) {
        if (count == 0) return;

        if (forward) {
            // 向后移动
            for (size_t i = count; i > 0; --i) {
                new (chunk + dst + i - 1) T(chunk[src + i - 1]);
                chunk[src + i - 1].~T();
            }
        } else {
            // 向前移动
            for (size_t i = 0; i < count; ++i) {
                new (chunk + dst + i) T(chunk[src + i]);
                chunk[src + i].~T();
            }
        }
    }
    /**
     * inserts elements at the specified locat on in the container.
     * inserts value before pos
     * returns an iterator pointing to the inserted value
     *     throw if the iterator is invalid or it point to a wrong place.
     */
    iterator insert(iterator pos, const T& value) {
        if (pos.deque_ptr != this || !is_valid_iterator(pos)) {
            throw invalid_iterator();
        }

        size_t idx = pos.get_position();
        // 空容器
        if (empty()) {
            push_back(value);
            return begin();
        }
        // 插入在头部或尾部
        if (idx == 0) {
            push_front(value);
            return begin();
        }
        if (idx == num_of_elements) {
            push_back(value);
            return end() - 1;
        }
        // 选择移动方向
        size_t distance_front = idx;
        size_t distance_back = num_of_elements - idx;

        if (distance_front < distance_back) {
            // 向前移动
            push_front(value);
            T tmp = *begin();
            for (size_t i = 0; i < idx; ++i) {
                (*this)[i] = (*this)[i + 1];
            }
            (*this)[idx] = tmp;
            return begin() + idx;
        } else {
            // 向后移动
            size_t old_size = num_of_elements;
            push_back(value);
            for (size_t i = old_size; i > idx; --i) {
                (*this)[i] = (*this)[i - 1];
            }
            (*this)[idx] = value;
            return begin() + idx;
        }
    }
    /**
     * removes specified element at pos.
     * removes the element at pos.
     * returns an iterator pointing to the following element, if pos pointing to
     * the last element, end() will be returned. throw if the container is
     * empty, the iterator is invalid or it points to a wrong place.
     */
    iterator erase(iterator pos) {
        if (empty()) throw container_is_empty();
        if (pos.deque_ptr != this || !is_valid_iterator(pos) || pos == end()) {
            throw invalid_iterator();
        }

        size_t idx = pos.get_position();
        // 删除头尾
        if (idx == 0) {
            pop_front();
            return begin();
        }
        if (idx == num_of_elements - 1) {
            pop_back();
            return end();
        }
        // 选择移动方向
        size_t distance_front = idx;
        size_t distance_back = num_of_elements - idx - 1;
        if (distance_front < distance_back) {
            // 向前移动
            for (size_t i = idx; i > 0; --i) {
                (*this)[i] = (*this)[i - 1];
            }
            pop_front();
            return begin() + idx;
        } else {
            // 向后移动
            for (size_t i = idx; i + 1 < num_of_elements; ++i) {
                (*this)[i] = (*this)[i + 1];
            }
            pop_back();
            return begin() + idx;
        }
    }
    /**
     * adds an element to the end
     */
    void push_back(const T& value) {
        if (empty()) {
            if (capacity == 0) expandChunkArray();
            chunk_ptrs[idx_of_begin] = allocateChunk();
            new (chunk_ptrs[idx_of_begin]) T(value);
            front_idx_ = back_idx_ = 0;
            num_of_elements = 1;
            return;
        }

        if (back_idx_ < CHUNK_SIZE - 1) {
            back_idx_++;
            new (chunk_ptrs[idx_of_end] + back_idx_) T(value);
            num_of_elements++;
            return;
        }
        if (idx_of_end + 1 >= capacity) expandChunkArray();
        idx_of_end++;
        chunk_ptrs[idx_of_end] = allocateChunk();
        back_idx_ = 0;
        new (chunk_ptrs[idx_of_end]) T(value);
        num_of_elements++;
    }
    /**
     * removes the last element
     *     throw when the container is empty.
     */
    void pop_back() {
        if (empty()) throw container_is_empty();

        chunk_ptrs[idx_of_end][back_idx_].~T();
        num_of_elements--;

        if (num_of_elements == 0) {
            deallocateChunk(chunk_ptrs[idx_of_end]);
            chunk_ptrs[idx_of_end] = nullptr;
            idx_of_begin = idx_of_end = 0;
            front_idx_ = back_idx_ = 0;
            return;
        }

        if (back_idx_ > 0) {
            back_idx_--;
        } else {
            deallocateChunk(chunk_ptrs[idx_of_end]);
            chunk_ptrs[idx_of_end] = nullptr;
            idx_of_end--;
            back_idx_ = CHUNK_SIZE - 1;
        }
    }

    /**
     * inserts an element to the beginning.
     */
    void push_front(const T& value) {
        if (empty()) {
            push_back(value);
            return;
        }

        if (front_idx_ > 0) {
            front_idx_--;
            new (chunk_ptrs[idx_of_begin] + front_idx_) T(value);
            num_of_elements++;
            return;
        }

        if (idx_of_begin == 0) expandChunkArray();
        idx_of_begin--;
        chunk_ptrs[idx_of_begin] = allocateChunk();
        front_idx_ = CHUNK_SIZE - 1;
        new (chunk_ptrs[idx_of_begin] + front_idx_) T(value);
        num_of_elements++;
    }
    /**
     * removes the first element.
     *     throw when the container is empty.
     */
    void pop_front() {
        if (empty()) {
            throw container_is_empty();
        }

        chunk_ptrs[idx_of_begin][front_idx_].~T();
        num_of_elements--;
        if (num_of_elements == 0) {
            deallocateChunk(chunk_ptrs[idx_of_begin]);
            chunk_ptrs[idx_of_begin] = nullptr;
            idx_of_begin = idx_of_end = 0;
            front_idx_ = back_idx_ = 0;
            return;
        }

        if (front_idx_ < CHUNK_SIZE - 1) {
            front_idx_++;
        } else {
            deallocateChunk(chunk_ptrs[idx_of_begin]);
            chunk_ptrs[idx_of_begin] = nullptr;
            idx_of_begin++;
            front_idx_ = 0;
        }
    }
};

}  // namespace sjtu

#endif