#pragma once

#include <atomic>

namespace xynq {

template<class T>
using LinkedStackNodePtr = T*;

// Thread-safe stack. Multi-reader, Multi-writer.
// It's up to the user to allocate/deallocate nodes and
// to avoid ABA problem.
template<class T, LinkedStackNodePtr<T> T::*NextPtr>
class LinkedStack {
public:
    using NodeType = T;
    using NodePtr = T*;

    // Enqueue node.
    void Push(T &node);

    // Dequeue the node.
    // Returns nullptr if the stack is empty.
    T *Pop();
private:
    std::atomic<T*> head_ = nullptr;
};


// LinkedStack implementation
template<class T, LinkedStackNodePtr<T> T::*NextPtr>
void LinkedStack<T, NextPtr>::Push(T &node) {
    T *head;
    do {
        head = head_.load(std::memory_order_acquire);
        node.*NextPtr = head;
    } while (!head_.compare_exchange_strong(head, &node, std::memory_order_release, std::memory_order_relaxed));
}

template<class T, LinkedStackNodePtr<T> T::*NextPtr>
T *LinkedStack<T, NextPtr>::Pop() {
    T *head, *next;
    do {
        head = head_.load(std::memory_order_acquire);
        if (head == nullptr) {
            break;
        }
        next = head->*NextPtr;
    } while (!head_.compare_exchange_strong(head, next, std::memory_order_release, std::memory_order_relaxed));

    return head;
}

} // xynq
