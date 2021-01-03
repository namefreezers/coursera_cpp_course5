#include <cstddef>
#include <memory>
#include <iostream>

template<typename T>
class Data {
public:
    Data() {
        std::cerr << "Data Ctor 1: " << std::endl;
    }

    Data(size_t capacity) : data_ptr_(static_cast<T *>(operator new(capacity * sizeof(T)))), capacity_(capacity) {
        std::cerr << "Data Ctor 2: " << capacity << " ptr: " << begin() << std::endl;
    }

    ~Data() {
        std::cerr << "Data Dtor: ptr: " << data_ptr_ << std::endl;

        operator delete(data_ptr_);
    }

    Data(const Data &) = delete;

    Data(Data &&other) {
        std::cerr << "Data Ctor 3: " << other.capacity << " ptr: " << other.begin() << std::endl;

        std::swap(data_ptr_, other.data_ptr_);
        std::swap(capacity_, other.capacity_);
    }

    Data &operator=(const Data &) = delete;

    Data &operator=(Data &&other) noexcept {
        std::cerr << "Data operator=&&: " << other.capacity_ << " ptr: " << other.begin() << std::endl;

        std::swap(data_ptr_, other.data_ptr_);
        std::swap(capacity_, other.capacity_);
        return *this;
    }

    size_t Capacity() const {
        return capacity_;
    }

    void Swap(Data<T> &other) {
        std::swap(data_ptr_, other.data_ptr_);
        std::swap(capacity_, other.capacity_);
    }

    // @formatter:off
    T *begin() { return data_ptr_; }

    T *end() { return data_ptr_ + capacity_; }

    const T *begin() const { return data_ptr_; }

    const T *end() const { return data_ptr_ + capacity_; }
    // @formatter:on

private:
    T *data_ptr_ = nullptr;
    size_t capacity_ = 0;
};

template<typename T>
class Vector {

public:
//    Vector() = default;
    Vector() {
        std::cerr << "Vector Ctor 1: " << size_ << ' ' << data_.Capacity() << " ptr: " << data_.begin() << std::endl;
    }

    Vector(size_t n) : data_(n) {
        std::uninitialized_default_construct_n(data_.begin(), n);
        size_ = n;

        std::cerr << "Vector Ctor 2: " << size_ << ' ' << data_.Capacity() << " ptr: " << data_.begin() << std::endl;
    }

    Vector(const Vector &other) : data_(other.size_) {
        std::uninitialized_copy(other.begin(), other.end(), data_.begin());
        size_ = other.size_;

        std::cerr << "Vector Ctor 3: " << size_ << ' ' << data_.Capacity() << " ptr: " << data_.begin() << std::endl;
    }

    Vector(Vector &&other) {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);

        std::cerr << "Vector Ctor 4: " << size_ << ' ' << data_.Capacity() << " ptr: " << data_.begin() << std::endl;
    }

    ~Vector() {
        std::cerr << "Vector Dtor: " << size_ << ' ' << data_.Capacity() << " ptr: " << data_.begin() << std::endl;

        std::destroy(this->begin(), this->end());
    }

    Vector &operator=(const Vector &other) {
        std::cerr << "Vector operator=&: " << other.size_ << ' ' << other.Capacity() << " ptr: " << other.begin()
                  << std::endl;

        if (data_.Capacity() < other.size_) {
            Data<T> data2_(other.size_);
            std::uninitialized_copy(other.begin(), other.end(), data2_.begin());

            std::destroy(this->begin(), this->end());
            data_ = std::move(data2_);
        } else if (size_ < other.size_) {
            std::copy(other.begin(), other.begin() + size_, data_.begin());
            std::uninitialized_copy(other.begin() + size_, other.end(), data_.begin() + size_);
        } else {  // else if (size_ >= other.size_)
            std::copy(other.begin(), other.end(), this->begin());
            std::destroy(this->begin() + other.size_, this->end());
        }

        size_ = other.size_;
        return *this;
    }

    Vector &operator=(Vector &&other) noexcept {
        std::cerr << "Vector operator=&&: " << other.size_ << ' ' << other.Capacity() << " ptr: " << other.begin()
                  << std::endl;

        data_.Swap(other.data_);
        std::swap(size_, other.size_);
        return *this;
    }

    void Reserve(size_t n) {
        std::cerr << "Vector Reserve " << size_ << ' ' << data_.Capacity() << " n: " << n << std::endl;

        if (n > data_.Capacity()) {
            Data<T> data2_(n);
            if (size_ > 0) {
                std::uninitialized_move(this->begin(), this->end(), data2_.begin());
                std::destroy(this->begin(), this->end());
            }
            data_ = std::move(data2_);
        }
    }

    void Resize(size_t n) {
        std::cerr << "Vector Resize " << size_ << ' ' << data_.Capacity() << " n: " << n << std::endl;

        if (n < size_) {
            std::destroy(this->begin() + n, this->end());
        } else {
            Reserve(n);
            std::uninitialized_default_construct(this->end(), this->begin() + n);
        }
        size_ = n;
    }

    void PushBack(const T &elem) {
        std::cerr << "Vector PushBack&: " << size_ << ' ' << data_.Capacity() << " ptr: " << data_.begin() << std::endl;

        if (size_ == data_.Capacity()) {
            Reserve(size_ == 0 ? 1 : size_ * 2);
        }
        new(this->end()) T(elem);
        size_++;
    }

    void PushBack(T &&elem) {
        std::cerr << "Vector PushBack&&: " << size_ << ' ' << data_.Capacity() << " ptr: " << data_.begin() << std::endl;

        if (size_ == data_.Capacity()) {
            Reserve(size_ == 0 ? 1 : size_ * 2);
        }
        new(this->end()) T(std::move(elem));
        size_++;
    }

    template<typename ... Args>
    T &EmplaceBack(Args &&... args) {
        std::cerr << "Vector EmplaceBack&&: " << size_ << ' ' << data_.Capacity() << " ptr: " << data_.begin() << std::endl;

        if (size_ == data_.Capacity()) {
            Reserve(size_ == 0 ? 1 : size_ * 2);
        }
        T &res = *(new(this->end()) T(std::forward<Args>(args) ...));
        size_++;
        return res;
    }

    void PopBack() {
        std::cerr << "Vector PopBack: " << size_ << ' ' << data_.Capacity() << " ptr: " << data_.begin() << std::endl;

        std::destroy_at(this->end() - 1);
        size_--;
    }

    size_t Size() const noexcept {
        std::cerr << "Vector Size " << size_ << std::endl;
        return size_;
    }

    size_t Capacity() const noexcept {
        std::cerr << "Vector Capacity " << data_.Capacity() << std::endl;
        return data_.Capacity();
    }

    const T &operator[](size_t i) const {
        std::cerr << "Vector const [] " << i << std::endl;
        return *(data_.begin() + i);
    }

    T &operator[](size_t i) {
        std::cerr << "Vector [] " << i << std::endl;
        return *(data_.begin() + i);
    }


    // В данной части задачи реализуйте дополнительно эти функции:
    // @formatter: off
    using iterator = T *;
    using const_iterator = const T *;

    iterator begin() noexcept {
        std::cerr << "begin(): " << data_.begin() << std::endl;
        return data_.begin();
    }

    iterator end() noexcept {
        std::cerr << "end(): " << data_.begin() + size_ << std::endl;
        return data_.begin() + size_;
    }

    const_iterator begin() const noexcept {
        std::cerr << "const begin(): " << data_.begin() << std::endl;
        return data_.begin();
    }

    const_iterator end() const noexcept {
        std::cerr << "const end(): " << data_.begin() + size_ << std::endl;
        return data_.begin() + size_;
    }

    // Тут должна быть такая же реализация, как и для константных версий begin/end
    const_iterator cbegin() const noexcept {
        std::cerr << "const cbegin(): " << data_.begin() << std::endl;
        return data_.begin();
    }

    const_iterator cend() const noexcept {
        std::cerr << "const cend(): " << data_.begin() + size_ << std::endl;
        return data_.begin() + size_;
    }
    // @formatter: on

    // Вставляет элемент перед pos
    // Возвращает итератор на вставленный элемент
    iterator Insert(const_iterator pos, const T &elem) {
        try {
            std::cerr << "Vector Insert&: " << size_ << ' ' << data_.Capacity() << ' ' << pos - data_.begin() << std::endl;
            if (size_ == data_.Capacity()) {
                int elem_idx = pos - begin();
                Reserve(size_ == 0 ? 1 : size_ * 2);
                pos = begin() + elem_idx;
            }
            auto pos_non_const = const_cast<iterator>(pos);
            if (pos == end()) {
                auto res_it = new(end()) T(elem);
                ++size_;
                return res_it;
            }

            new(end()) T(std::move(*(end() - 1)));
            for (T *it = end() - 2; it >= pos; it--) {
                *(it + 1) = std::move(*it);
            }
            *pos_non_const = elem;
            size_++;
            return pos_non_const;
        } catch (...) {
            std::cerr << "Exception insert" << std::endl;
            return nullptr;
        }
    }

    iterator Insert(const_iterator pos, T &&elem) {
        std::cerr << "Vector Insert&&: " << size_ << ' ' << data_.Capacity() << ' ' << pos - data_.begin() << std::endl;
        if (size_ == data_.Capacity()) {
            int elem_idx = pos - begin();
            Reserve(size_ == 0 ? 1 : size_ * 2);
            pos = begin() + elem_idx;
        }
        auto pos_non_const = const_cast<iterator>(pos);
        if (pos == end()) {
            auto res_it = new(end()) T(std::move(elem));
            ++size_;
            return res_it;
        }

        new(end()) T(std::move(*(end() - 1)));
        for (T *it = end() - 2; it >= pos; it--) {
            *(it + 1) = std::move(*it);
        }
        *pos_non_const = std::move(elem);
        size_++;
        return pos_non_const;
    }

    // Конструирует элемент по заданным аргументам конструктора перед pos
    // Возвращает итератор на вставленный элемент
    template<typename ... Args>
    iterator Emplace(const_iterator pos, Args &&... args) {
        std::cerr << "Vector Emplace pos && " << size_ << ' ' << data_.Capacity() << ' ' << pos - data_.begin() << std::endl;

        if (size_ == data_.Capacity()) {
            int elem_idx = pos - begin();
            Reserve(size_ == 0 ? 1 : size_ * 2);
            pos = begin() + elem_idx;
        }
        auto pos_non_const = const_cast<iterator>(pos);
        if (pos == end()) {
            auto res_it = new(end()) T(std::forward<Args>(args)...);
            ++size_;
            return res_it;
        }

        new(end()) T(*(end() - 1));
        for (T *it = end() - 2; it >= pos; it--) {
            *(it + 1) = std::move(*it);
        }
        *pos_non_const = T(std::forward<Args>(args)...);
        size_++;
        return pos_non_const;
    }

    // Удаляет элемент на позиции pos
    // Возвращает итератор на элемент, следующий за удалённым
    iterator Erase(const_iterator pos) {
        std::cerr << "Vector Erase: " << size_ << ' ' << data_.Capacity() << ' ' << pos - data_.begin() << std::endl;
        for (auto it = const_cast<iterator>(pos); it + 1 < end(); it++) {
            *it = std::move(*(it + 1));
        }
        std::destroy_at(end() - 1);
        --size_;
        return const_cast<iterator>(pos);
    }


private:
    Data<T> data_;
    size_t size_ = 0;
};
