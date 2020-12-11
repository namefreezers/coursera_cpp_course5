#include <cstddef>
#include <memory>

template<typename T>
class Data {
public:
    Data() {}

    Data(size_t capacity) : data_ptr_(static_cast<T *>(operator new(capacity * sizeof(T)))), capacity_(capacity) {}

    ~Data() {
        operator delete(data_ptr_);
    }

    Data(const Data &) = delete;

    Data(Data &&other) {
        std::swap(data_ptr_, other.data_ptr_);
        std::swap(capacity_, other.capacity_);
    }

    Data &operator=(const Data &) = delete;

    Data &operator=(Data &&other) noexcept {
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

    T *Begin() { return data_ptr_; }

    const T *Begin() const { return data_ptr_; }

    T *End() { return data_ptr_ + capacity_; }

    const T *End() const { return data_ptr_ + capacity_; }

private:
    T *data_ptr_ = nullptr;
    size_t capacity_ = 0;
};

template<typename T>
class Vector {

public:
    Vector() = default;

    Vector(size_t n) : data_(n) {
        std::uninitialized_default_construct_n(data_.Begin(), n);
        size_ = n;
    }

    Vector(const Vector &other) : data_(other.size_) {
        std::uninitialized_copy(other.Begin(), other.End(), data_.Begin());
        size_ = other.size_;
    }

    Vector(Vector &&other) {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    ~Vector() {
        std::destroy(this->Begin(), this->End());
    }

    Vector &operator=(const Vector &other) {
        if (data_.Capacity() < other.size_) {
            Data<T> data2_(other.size_);
            std::uninitialized_copy(other.Begin(), other.End(), data2_.Begin());

            std::destroy(this->Begin(), this->End());
            data_ = std::move(data2_);
        } else if (size_ < other.size_) {
            std::copy(other.Begin(), other.Begin() + size_, data_.Begin());
            std::uninitialized_copy(other.Begin() + size_, other.End(), data_.Begin() + size_);
        } else {  // else if (size_ >= other.size_)
            std::copy(other.Begin(), other.End(), this->Begin());
            std::destroy(this->Begin() + other.size_, this->End());
        }

        size_ = other.size_;
        return *this;
    }

    Vector &operator=(Vector &&other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
        return *this;
    }

    void Reserve(size_t n) {
        if (n > data_.Capacity()) {
            Data<T> data2_(n);
            if (size_ > 0) {
                std::uninitialized_move(this->Begin(), this->End(), data2_.Begin());
                std::destroy(this->Begin(), this->End());
            }
            data_ = std::move(data2_);
        }
    }

    void Resize(size_t n) {
        if (n < size_) {
            std::destroy(this->Begin() + n, this->End());
        } else {
            Reserve(n);
            std::uninitialized_default_construct(this->End(), this->Begin() + n);
        }
        size_ = n;
    }

    void PushBack(const T &elem) {
        if (size_ == data_.Capacity()) {
            Reserve(size_ == 0 ? 1 : size_ * 2);
        }
        new(this->End()) T(elem);
        size_++;
    }

    void PushBack(T &&elem) {
        if (size_ == data_.Capacity()) {
            Reserve(size_ == 0 ? 1 : size_ * 2);
        }
        new(this->End()) T(std::move(elem));
        size_++;
    }

    template<typename ... Args>
    T &EmplaceBack(Args &&... args) {
        if (size_ == data_.Capacity()) {
            Reserve(size_ == 0 ? 1 : size_ * 2);
        }
        T &res = *(new(this->End()) T(std::forward<Args>(args) ...));
        size_++;
        return res;
    }

    void PopBack() {
        std::destroy_at(this->End() - 1);
        size_--;
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T &operator[](size_t i) const {
        return *(data_.Begin() + i);
    }

    T &operator[](size_t i) {
        return *(data_.Begin() + i);
    }

    T *Begin() { return data_.Begin(); }

    const T *Begin() const { return data_.Begin(); }

    T *End() { return data_.Begin() + size_; }

    const T *End() const { return data_.Begin() + size_; }

private:
    Data<T> data_;
    size_t size_ = 0;

};
