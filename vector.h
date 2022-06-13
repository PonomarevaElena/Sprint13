#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }


    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept
        : buffer_(Allocate(other.capacity_))
        , capacity_(other.capacity_)

    {
        buffer_ = nullptr;
        std::swap(buffer_, other.buffer_);
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept
    {
        RawMemory rhs_copy(std::move(rhs));
        Swap(rhs_copy);
        return *this;
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

///from c++20
template< class InputIt, class Size, class OutputIt>
OutputIt copy_n(InputIt first, Size count, OutputIt result)
{
    if (count > 0) {
        *result++ = *first;
        for (Size i = 1; i < count; ++i) {
            *result++ = *++first;
        }
    }
    return result;
}

template <typename T>
class Vector {
public:

    Vector() = default;


    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_(other.size_)

    {
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (size_ >= rhs.Size()) {
                size_t delta = size_ - rhs.size_;
                std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                //copy_n(rhs.data_.GetAddress(), rhs.Size(), data_.GetAddress());
                std::destroy_n(data_.GetAddress() + rhs.size_, delta);
                size_ = rhs.size_;
            }
            else {
                if (data_.Capacity() < rhs.size_) {
                    Vector rhs_copy(rhs);
                    Swap(rhs_copy);
                }
                else {

                    size_t delta = rhs.size_ - size_;
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + size_, data_.GetAddress());
                    //copy_n(rhs.data_.GetAddress(), Size(), data_.GetAddress());
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, delta, data_.GetAddress() + size_);
                    size_ = rhs.size_;
                }
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {

        if (this != &rhs) {
            size_ = 0;
            data_ = RawMemory<T>();
            Swap(rhs);
        }
        return *this;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        // Конструируем элементы в new_data, копируя их из data_
      //  std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
      // constexpr оператор if будет вычислен во время компиляции
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }

        // Разрушаем элементы в data_
        std::destroy_n(data_.GetAddress(), size_);
        // Избавляемся от старой сырой памяти, обменивая её на новую
        data_.Swap(new_data);
        // При выходе из метода старая память будет возвращена в кучу
    }

    void Resize(size_t new_size) {

        if (new_size == size_) {
            return;
        }

        if (size_ > new_size) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }
        else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        size_ = new_size;
    }

    template <typename S>
    void PushBack(S&& value) {
        if (Capacity() > size_) {
            new (data_ + size_) T(std::forward<S>(value));
        }
        else if (size_ == 0) {
            Reserve(1);
            new (data_.GetAddress()) T(std::forward<S>(value));
        }
        else {
            RawMemory<T> new_data(size_ * 2);
            new (new_data + size_) T(std::forward<S>(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        ++size_;
    }
   
    void PopBack() /* noexcept */ {
        T* for_del = data_.GetAddress() + size_ - 1;
        for_del->~T();
        size_ = size_ - 1;
    }


    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (Capacity() > size_) {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        else if (size_ == 0) {
            Reserve(1);
            new (data_.GetAddress()) T(std::forward<Args>(args)...);
        }
        else {
            RawMemory<T> new_data(size_ * 2);
            new (new_data + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        ++size_;
        return *(data_.GetAddress() + size_ - 1);
    }
        
    


    //template <typename Func, typename... T>
  //void ApplyToMany(Func& l, T&&... vs ) {
  //  (...,l(std::forward<T>(vs)));
  //}

  //  template <typename... Args>
 //   void Emplace(Args&&... args) {
 //       Reset();
 //       new(data_)T(std::forward<Args>(args)...);
 //       is_initialized_ = true;
 //   }

    ~Vector() {
        if (data_.GetAddress() != nullptr) {
            std::destroy_n(data_.GetAddress(), size_);
        }
    }

private:

    RawMemory<T> data_;
    size_t size_ = 0;

};