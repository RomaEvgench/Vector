#pragma once

#include <string>
#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <utility>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t capacity_to_reserve)
        : capacity_reserve(capacity_to_reserve) {
    }

    size_t GetCapacity() {
        return capacity_reserve;
    }

private:
    size_t capacity_reserve = 0;
};


template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;
    
    SimpleVector(const SimpleVector& other) {
        assert(size_ == 0);
        SimpleVector<Type> copy_(other.size_);
        std::copy(other.begin(), other.end(), copy_.begin());
        //copy_.capacity_ = other.capacity_;
        swap(copy_);
    }

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)   
        :size_(size), capacity_(size) {
        ArrayPtr<Type> elements(size);
    	std::fill(&elements[0], &elements[size], 0);
        elements_.swap(elements);
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) 
        :size_(size), 
        capacity_(size) {
        ArrayPtr<Type> elements(size);
    	std::fill(&elements[0], &elements[size], value);
        elements_.swap(elements);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) 
        :elements_(init.size()), 
        size_(init.size()), 
        capacity_(init.size()) {
        std::move(std::make_move_iterator(init.begin()), std::make_move_iterator(init.end()), elements_.Get());
    }
    
    SimpleVector(SimpleVector&& other) {
        assert(size_ == 0);
        elements_ = std::move(other.elements_);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
    }
    
    SimpleVector(ReserveProxyObj object) {
        Reserve(object.GetCapacity());
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            if (rhs.IsEmpty()) {
                Clear();
            }
            else {
                SimpleVector<Type> copy_(rhs);
                swap(copy_);
            }
        }
        return *this;
    }
    
    //SimpleVector& operator=(SimpleVector&&) = default;
    
    SimpleVector& operator=(SimpleVector&& rhs) {
        // Напишите тело конструктора самостоятельно
    	if (this != &rhs) {
           
    		elements_ = std::move(rhs.elements_);
            size_ = std::exchange(rhs.size_, 0);
            capacity_ = std::exchange(rhs.capacity_, 0);
        }
        return *this;
    }
    
    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (capacity_ == 0) {
            ArrayPtr<Type> Elements_(1);
            Elements_[0] = item;
            elements_.swap(Elements_);
            size_ = 1;
            capacity_ = 1;
        }
        else if (size_ == capacity_) {
            ArrayPtr<Type> new_elements_(size_*2);
            std::move(elements_.Get(), elements_.Get() + size_, new_elements_.Get());
            elements_.swap(new_elements_);
            capacity_ = size_*2;
            *(end()) = item;
            size_++;
        }
        else  {
            *(end()) = item;
            size_++;
        }
    }
    
     void PushBack(Type&& item) {
        if (capacity_ == 0) {
            ArrayPtr<Type> Elements_(1);
            Elements_[0] = std::move(item);
            elements_.swap(Elements_);
            size_ = 1;
            capacity_ = 1;
        }
        else if (size_ == capacity_) {
            ArrayPtr<Type> new_elements_(size_*2);
            std::move(elements_.Get(), elements_.Get() + size_, new_elements_.Get());
            elements_.swap(new_elements_);
            capacity_ = size_*2;
            *(end()) = std::move(item);
            size_++;
        }
        else  {
            *(end()) = std::move(item);
            size_++;
        }
    }
    
    
    
    
    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
	    size_t pos_idx = std::distance(cbegin(), pos);
        if (capacity_ == 0) {
            SimpleVector<Type> new_vector(1);
            swap(new_vector);
        } else if (size_ == capacity_) {
	        SimpleVector<Type> new_vector(2*size_);
	        std::copy(cbegin(), pos, new_vector.begin());
	        std::copy_backward(pos, cend(), end()+1);
	        new_vector.size_ = size_ + 1;
	        swap(new_vector);
	    } else {
	        std::move_backward(pos, cend(), end()+1);
	        ++size_;
	    }
 
        elements_[pos_idx] = value;
        return &elements_[pos_idx];
	}
 
    Iterator Insert(ConstIterator pos, Type&& value) {
    	auto* p = const_cast<Iterator>(pos);
	    size_t pos_idx = std::distance(begin(), p);
        if (capacity_ == 0) {
            SimpleVector<Type> new_vector(1);
            new_vector[pos_idx] = std::move(value);
            *this = std::move(new_vector);
        } else if (size_ == capacity_) {
	        SimpleVector<Type> new_vector(2*size_);
	        std::move(std::make_move_iterator(begin()), std::make_move_iterator(p), new_vector.begin());
	        std::move_backward(std::make_move_iterator(p), std::make_move_iterator(end()), end()+1);
	        new_vector.size_ = size_ + 1;
	        new_vector[pos_idx] = std::move(value);
	        *this = std::move(new_vector);
	    } else {
	        std::move_backward(std::make_move_iterator(p), std::make_move_iterator(end()), end()+1);
	        ++size_;
	        elements_[pos_idx] = std::move(value);
	    }
        return &elements_[pos_idx];
    }
    
     // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        size_--;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        auto position = pos - begin();
        auto* it = begin() + position;
        std::move((it + 1), end(), it);
        size_--;
        return (begin() + position);
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        elements_.swap(other.elements_);
        std::swap(other.size_, size_);
        std::swap(other.capacity_, capacity_);
    }
    
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> new_elements_(new_capacity);
            std::move(elements_.Get(), elements_.Get() + size_, new_elements_.Get());
            elements_.swap(new_elements_);
            capacity_ = new_capacity;
        }
    }
    
    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return elements_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return elements_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Выход за предел, index >= size");
        }
        return elements_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Выход за предел, index >= size");
        }
        return elements_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (size_ < new_size) {
            if (new_size <= capacity_)
                for (auto it = end(); it != &elements_[new_size]; ++it)
                    *(it) = std::move(Type{});
            else{
                ArrayPtr<Type> new_elements_(new_size*2);
                std::move(elements_.Get(), elements_.Get() + size_, new_elements_.Get());
                elements_.swap(new_elements_);
                capacity_ = new_size;
                }
        }
        size_ = new_size;
    }



    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return elements_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return   (elements_.Get() + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return  elements_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return (elements_.Get() + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return elements_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return(elements_.Get() + size_);
    }
    
private:
    ArrayPtr<Type> elements_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
} 