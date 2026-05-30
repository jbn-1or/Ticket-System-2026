#ifndef SJTU_VECTOR_HPP
#define SJTU_VECTOR_HPP

#include "exceptions.hpp"

#include <climits>
#include <cstddef>

namespace sjtu {
/**
 * a data container like std::vector
 * store data in a successive memory and support random access.
 */
template<typename T>
class vector {
private:
    T* data;          
    size_t num_of_elements;  // 当前元素数量
    size_t capacity;         // 当前容器容量

public:
	/**
	 * TODO
	 * a type for actions of the elements of a vector, and you should write
	 *   a class named const_iterator with same interfaces.
	 */
	/**
	 * you can see RandomAccessIterator at CppReference for help.
	 */
	class const_iterator;
	class iterator
	{
	// The following code is written for the C++ type_traits library.
	// Type traits is a C++ feature for describing certain properties of a type.
	// For instance, for an iterator, iterator::value_type is the type that the
	// iterator points to.
	// STL algorithms and containers may use these type_traits (e.g. the following
	// typedef) to work properly. In particular, without the following code,
	// @code{std::sort(iter, iter1);} would not compile.
	// See these websites for more information:
	// https://en.cppreference.com/w/cpp/header/type_traits
	// About value_type: https://blog.csdn.net/u014299153/article/details/72419713
	// About iterator_category: https://en.cppreference.com/w/cpp/iterator
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::output_iterator_tag;

	private:
		/**
		 * TODO add data members
		 *   just add whatever you want.
		 */
		vector<value_type> * vec;  // 迭代器所在的vector
		pointer cur_pointer;       // 迭代器当前指向的地址
	public:
		/**
		 * return a new iterator which pointer n-next elements
		 * as well as operator-
		 */
		iterator(vector<T>* v = nullptr, T* p = nullptr) : vec(v), cur_pointer(p) {}
		
		iterator operator+(const int &n) const {
			//TODO
            return iterator(vec, cur_pointer + n);
		}
		iterator operator-(const int &n) const {
			//TODO
			return iterator(vec, cur_pointer - n);
		}
		// return the distance between two iterators,
		// if these two iterators point to different vectors, throw invaild_iterator.
		int operator-(const iterator &rhs) const {
			//TODO
			if (vec != rhs.vec) throw invalid_iterator();
            return static_cast<int>(cur_pointer - rhs.cur_pointer);
		}
		iterator& operator+=(const int &n) {
			//TODO
			cur_pointer += n;
            return *this;
		}
		iterator& operator-=(const int &n) {
			//TODO
			cur_pointer -= n;
            return *this;
		}
		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
			iterator temp = *this;
            ++cur_pointer;
            return temp;
		}
		/**
		 * TODO ++iter
		 */
		iterator& operator++() {
			++cur_pointer;
            return *this;
		}
		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
			iterator temp = *this;
            --cur_pointer;
            return temp;
		}
		/**
		 * TODO --iter
		 */
		iterator& operator--() {
			--cur_pointer;
            return *this;
		}
		/**
		 * TODO *it
		 */
		T& operator*() const{
			if (vec == nullptr || cur_pointer < vec->data || cur_pointer >= vec->data + vec->num_of_elements) {
                throw invalid_iterator();
            }
            return *cur_pointer;
		}
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory address).
		 */
		bool operator==(const iterator &rhs) const {
			return vec == rhs.vec && cur_pointer == rhs.cur_pointer;
		}
		bool operator==(const const_iterator &rhs) const {
			return vec == rhs.vec && cur_pointer == rhs.cur_pointer;
		}
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
	};
	/**
	 * TODO
	 * has same function as iterator, just for a const object.
	 */
	class const_iterator {
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::output_iterator_tag;

	private:
		/*TODO*/
        const vector<T>* vec;        // 指向所属的const vector
        const T* cur_pointer;        // 指向当前元素的const指针

    public:
        // 构造函数
        const_iterator(const vector<T>* v = nullptr, const T* p = nullptr) : vec(v), cur_pointer(p) {}

        // 从普通转换为常量迭代器
        const_iterator(const iterator& other) : vec(other.vec), cur_pointer(other.cur_pointer) {}

        // 重载 + 运算符
        const_iterator operator+(const int &n) const {
            return const_iterator(vec, cur_pointer + n);
        }

        // 重载 - 运算符（int）
        const_iterator operator-(const int &n) const {
            return const_iterator(vec, cur_pointer - n);
        }

        // 重载 - 运算符（迭代器）
        int operator-(const const_iterator &rhs) const {
            if (vec != rhs.vec) throw invalid_iterator();
            return static_cast<int>(cur_pointer - rhs.cur_pointer);
        }

        // 重载 += 运算符
        const_iterator& operator+=(const int &n) {
            cur_pointer += n;
            return *this;
        }

        // 重载 -= 运算符
        const_iterator& operator-=(const int &n) {
            cur_pointer -= n;
            return *this;
        }

        // 后置 ++
        const_iterator operator++(int) {
            const_iterator temp = *this;
            ++cur_pointer;
            return temp;
        }

        // 前置 ++
        const_iterator& operator++() {
            ++cur_pointer;
            return *this;
        }

        // 后置 --
        const_iterator operator--(int) {
            const_iterator temp = *this;
            --cur_pointer;
            return temp;
        }

        // 前置 --
        const_iterator& operator--() {
            --cur_pointer;
            return *this;
        }

        // 解引用：返回const引用
        const T& operator*() const {
            if (vec == nullptr || cur_pointer < vec->data || cur_pointer >= vec->data + vec->num_of_elements) {
                throw invalid_iterator();
            }
            return *cur_pointer;
        }

        // 判断相等
        bool operator==(const iterator &rhs) const {
            return vec == rhs.vec && cur_pointer == rhs.cur_pointer;
        }

        bool operator==(const const_iterator &rhs) const {
            return vec == rhs.vec && cur_pointer == rhs.cur_pointer;
        }

        // 判断不等
        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };
	/**
	 * TODO Constructs
	 * At least two: default constructor, copy constructor
	 */
	vector() : data(nullptr), num_of_elements(0), capacity(0) {}

	vector(const vector &other) {
		num_of_elements = other.num_of_elements;
		capacity = other.capacity;
		data = static_cast<T*>(operator new(capacity * sizeof(T)));
		for (size_t i = 0; i < num_of_elements; ++i) {
            new (data + i) T(other.data[i]);
        }
	}
	/**
	 * TODO Destructor
	 */
	~vector() {
		// 析构构造的元素
		for (size_t i = 0; i < num_of_elements; ++i) {
            data[i].~T();
        }
		// 释放原内存
		operator delete(data);
		data = nullptr;
		num_of_elements = 0;
		capacity = 0;
	}
	/**
	 * TODO Assignment operator
	 */
	vector &operator=(const vector &other) {
		if (this == &other) return *this;

		for (size_t i = 0; i < num_of_elements; ++i) {
            data[i].~T();
        }
		operator delete(data);

		num_of_elements = other.num_of_elements;
        capacity = other.capacity;
        data = static_cast<T*>(operator new(capacity * sizeof(T)));
        for (size_t i = 0; i < num_of_elements; ++i) {
            new (data + i) T(other.data[i]);
        }
        return *this;
	}
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 */
	T & at(const size_t &pos) {
		if (pos >= num_of_elements) {
			throw index_out_of_bound();
		} else {
			return data[pos];
		}
	}
	const T & at(const size_t &pos) const {
		if (pos >= num_of_elements) {
			throw index_out_of_bound();
		} else {
			return data[pos];
		}
	}
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 * !!! Pay attentions
	 *   In STL this operator does not check the boundary but I want you to do.
	 */
	T & operator[](const size_t &pos) {
		if (pos >= num_of_elements) {
			throw index_out_of_bound();
		} else {
			return data[pos];
		}
	}

	const T & operator[](const size_t &pos) const {
		if (pos >= num_of_elements) {
			throw index_out_of_bound();
		} else {
			return data[pos];
		}
	}
	/**
	 * access the first element.
	 * throw container_is_empty if size == 0
	 */
	const T & front() const {
		if (num_of_elements == 0) throw container_is_empty();
        return data[0];
	}
	/**
	 * access the last element.
	 * throw container_is_empty if size == 0
	 */
	const T & back() const {
		if (num_of_elements == 0) throw container_is_empty();
        return data[num_of_elements - 1];
	}
	/**
	 * returns an iterator to the beginning.
	 */
	iterator begin() {
		return iterator(this, data);
	}
	const_iterator begin() const {
		return const_iterator(this, data);
	}
	const_iterator cbegin() const {
		return const_iterator(this, data);
	}
	/**
	 * returns an iterator to the end.
	 */
	iterator end() {
		return iterator(this, data + num_of_elements);
	}
	const_iterator end() const {
		return const_iterator(this, data + num_of_elements);
	}
	const_iterator cend() const {
		return const_iterator(this, data + num_of_elements);
	}
	/**
	 * checks whether the container is empty
	 */
	bool empty() const {
		return num_of_elements == 0;
	}
	/**
	 * returns the num_of_elementsber of elements
	 */
	size_t size() const {
		return num_of_elements;
	}
	/**
	 * clears the contents
	 */
	void clear() {
		// 析构所有元素，不释放内存
		for (size_t i = 0; i < num_of_elements; ++i) {
            data[i].~T();
        }
		num_of_elements = 0;
		// 清空容量
		// operator delete(data);
		// data = nullptr;
		// capacity = 0;
	}

	// 容量不足时扩容（翻倍）
    void expand() {
        size_t new_capacity = (capacity == 0) ? 1 : capacity * 2;
        // 分配内存
        T* new_data = static_cast<T*>(operator new(new_capacity * sizeof(T)));
        // 复制
        for (size_t i = 0; i < num_of_elements; ++i) {
            new (new_data + i) T(std::move(data[i]));  // 移动构造
            data[i].~T();  // 析构旧元素
        }
        // 释放旧内存
        operator delete(data);
        data = new_data;
        capacity = new_capacity;
    }
	/**
	 * inserts value before pos
	 * returns an iterator pointing to the inserted value.
	 */
	iterator insert(iterator pos, const T &value) {
		const size_t ind = pos - begin();
		if (num_of_elements == capacity) {
			expand();
		}
		// 先在末尾构造一个临时元素。避免覆盖未析构的元素
		new (data + num_of_elements) T();
		// 后移元素
		for (size_t i = num_of_elements; i > ind; --i) {
            data[i] = std::move(data[i - 1]);
		}
		// 析构临时元素，再构造
		data[ind].~T();
		new (data + ind) T(value);
		num_of_elements++;
		return iterator(this, data + ind);
	}
	/**
	 * inserts value at index ind.
	 * after inserting, this->at(ind) == value
	 * returns an iterator pointing to the inserted value.
	 * throw index_out_of_bound if ind > size (in this situation ind can be size because after inserting the size will increase 1.)
	 */
	iterator insert(const size_t &ind, const T &value) {
		if (ind > num_of_elements) {
			throw index_out_of_bound();
		}
		if (num_of_elements == capacity) {
			expand();
		}
		for (size_t i = num_of_elements; i > ind; --i) {
			// 在 i 位置构造：移动 data[i-1] 的资源
			new (data + i) T(std::move(data[i - 1]));
			// 析构 i-1 位置的旧元素
			data[i - 1].~T();
		}
		new (data + ind) T(value);
		num_of_elements++;
		return iterator(this, data + ind);
	}
	/**
	 * removes the element at pos.
	 * return an iterator pointing to the following element.
	 * If the iterator pos refers the last element, the end() iterator is returned.
	 */
	iterator erase(iterator pos) {
		size_t ind = pos - begin();
		if (ind >= num_of_elements) throw index_out_of_bound();
		// 析构要删除的元素
		data[ind].~T();
		// 前移后续元素
		for (size_t i = ind; i < num_of_elements - 1; ++i) {
			data[i] = std::move(data[i + 1]);
			data[i + 1].~T();  // 析构被移动的旧元素
		}
		num_of_elements--;
		return iterator(this, data + ind);
	}
	/**
	 * removes the element with index ind.
	 * return an iterator pointing to the following element.
	 * throw index_out_of_bound if ind >= size
	 */
	iterator erase(const size_t &ind) {
		if (ind >= num_of_elements) {
			throw index_out_of_bound();
		}
		// 析构目标元素
		data[ind].~T();
		// 前移元素
		for (size_t i = ind; i < num_of_elements - 1; ++i) {
			data[i] = std::move(data[i + 1]);
			data[i + 1].~T();
		}
		num_of_elements--;
		return iterator(this, data + ind);
	}
	/**
	 * adds an element to the end.
	 */
	void push_back(const T &value) {
		insert(num_of_elements, value);
		return;
	}
	/**
	 * remove the last element from the end.
	 * throw container_is_empty if size() == 0
	 */
	void pop_back() {
		if (num_of_elements == 0) {
			throw container_is_empty();
		}
		erase(num_of_elements - 1);
		return;
	}
};


}

#endif