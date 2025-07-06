#pragma once

#include <atomic>  // 引入原子操作

template <typename T>
class shared_ptr {
private:
    T* ptr;                                 // 指向管理的对象
    std::atomic<std::size_t>* ref_count;    // 原子引用计数

    // 释放资源
    void release() {
        // P.S. 这里使用 std::memory_order_acq_rel 内存序，保证释放资源的同步
        if (ref_count && ref_count->fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete ptr;
            delete ref_count;
        }
    }

public:
    // 默认构造函数
    shared_ptr() : ptr(nullptr), ref_count(nullptr) {}

    // 构造函数
    // P.S. 这里使用 explicit 关键字，防止隐式类型转换
    // shared_ptr<int> ptr1 = new int(10);  不允许出现
    explicit shared_ptr(T* p) : ptr(p), ref_count(p ? new std::atomic<std::size_t>(1) : nullptr) {

    }

    // 析构函数
    ~shared_ptr() {
        release();
    }

    // 拷贝构造函数
    shared_ptr(const shared_ptr<T>& other) : ptr(other.ptr), ref_count(other.ref_count) {
        if (ref_count) {
            ref_count->fetch_add(1, std::memory_order_relaxed);  // 引用计数增加，不需要强内存序
        }
    }

    // 拷贝赋值运算符
    shared_ptr<T>& operator=(const shared_ptr<T>& other) {
        if (this != &other) {
            release();  // 释放当前资源
            ptr = other.ptr;
            ref_count = other.ref_count;
            if (ref_count) {
                ref_count->fetch_add(1, std::memory_order_relaxed);  // 引用计数增加
            }
        }
        return *this;
    }

    // 移动构造函数
	// P.S. noexcept 关键字表示该函数不会抛出异常。
    // 标准库中的某些操作（如 std::swap）要求移动操作是 noexcept 的，以确保异常安全。
	// noexcept 可以帮助编译器生成更高效的代码，因为它不需要为异常处理生成额外的代码。
    shared_ptr(shared_ptr<T>&& other) noexcept : ptr(other.ptr), ref_count(other.ref_count) {
        other.ptr = nullptr;
        other.ref_count = nullptr;
    }

    // 移动赋值运算符
    shared_ptr<T>& operator=(shared_ptr<T>&& other) noexcept {
        if (this != &other) {
            release();  // 释放当前资源
            ptr = other.ptr;
            ref_count = other.ref_count;
            other.ptr = nullptr;
            other.ref_count = nullptr;
        }
        return *this;
    }

    // 解引用运算符
    // P.S. const 关键字表示该函数不会修改对象的状态。
    T& operator*() const {
        return *ptr;
    }

    // 箭头运算符
    T* operator->() const {
        return ptr;
    }

    // 获取引用计数， memory_order_acquire获取最新的数据
    std::size_t use_count() const {
        return ref_count ? ref_count->load(std::memory_order_acquire) : 0;
    }

    // 获取原始指针
    T* get() const {
        return ptr;
    }

    // 重置指针
    void reset(T* p = nullptr) {
        release();
        ptr = p;
        ref_count = p ? new std::atomic<std::size_t>(1) : nullptr;
    }
};