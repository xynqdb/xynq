#pragma once

namespace xynq {

template<class T>
using ListNodePtr = T*;

// Non-thread-safe singly linked list.
template<class T, ListNodePtr<T> T::*NextPtr>
class List {
public:
    using NodeType = T;
    using NodePtr = T*;

    // First node in the list.
    inline NodePtr Head() { return head_; }
    inline const NodePtr Head() const { return head_; }

    // Last node in the list.
    inline NodePtr Last() { return last_; }
    inline const NodePtr Last() const { return last_; }

    // List end. Should not be dereferenced.
    inline const NodePtr End() { return nullptr; }

    // Append node to the end of the list.
    void PushBack(NodePtr node);

    // Insert node at the beginning of the list.
    void PushFront(NodePtr node);

    // Appends another list to begginging of the list.
    void PushFront(List &&another);

    // Returns node from the beginning of the list
    // and removes it from the list.
    // Returns nullptr if the list is empty.
    NodePtr PopFront();

    // Returns true if the list has no nodes, false otherwise.
    inline bool IsEmpty() const;
private:
    NodePtr head_ = nullptr;
    NodePtr last_ = nullptr;
};


// Implementation.
template<class T, ListNodePtr<T> T::*NextPtr>
bool List<T, NextPtr>::IsEmpty() const {
    return head_ == nullptr;
}

template<class T, ListNodePtr<T> T::*NextPtr>
void List<T, NextPtr>::PushBack(NodePtr node) {
    if (last_ == nullptr) {
        head_ = last_ = node;
    } else {
        last_->*NextPtr = node;
        last_ = node;
    }
    node->*NextPtr = nullptr;
}

template<class T, ListNodePtr<T> T::*NextPtr>
void List<T, NextPtr>::PushFront(NodePtr node) {
    if (head_ == nullptr) {
        head_ = last_ = node;
    } else {
        node->*NextPtr = head_;
        head_ = node;
    }
}

template<class T, ListNodePtr<T> T::*NextPtr>
void List<T, NextPtr>::PushFront(List<T, NextPtr> &&another) {
    if (another.IsEmpty()) {
        return;
    }

    another.last_->*NextPtr = head_;
    head_ = another.head_;
    if (last_ == nullptr) {
        last_ = another.last_;
    }
}

template<class T, ListNodePtr<T> T::*NextPtr>
typename List<T, NextPtr>::NodePtr List<T, NextPtr>::PopFront() {
    if (head_ == last_) { // empty or single element
        NodePtr node = head_;
        head_ = last_ = nullptr;
        return node;
    }

    NodePtr node = head_;
    head_ = node->*NextPtr;
    return node;
}

} // xynq
