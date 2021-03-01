#include "containers/linked_stack.h"
#include "gtest/gtest.h"

#include <thread>

using namespace xynq;

struct TestNode {
    int val = 0;
    LinkedStackNodePtr<TestNode> next = nullptr;
};

TEST(LinkedStackTest, SingleThread) {
    LinkedStack<TestNode, &TestNode::next> st;
    for (int i = 1; i <= 100; ++i) {
        TestNode *n = new TestNode;
        n->val = i;
        st.Push(*n);
    }

    for (int i = 100; i > 0; --i) {
        TestNode *n = st.Pop();
        ASSERT_TRUE(n != nullptr);
        ASSERT_EQ(n->val, i);
        delete n;
    }
}

TEST(LinkedStackTest, MultiThreaded) {
    LinkedStack<TestNode, &TestNode::next> st;
    static const int num_threads = 10;
    static const int num_writers = num_threads / 2;
    static const int num_entries = 5000;

    std::atomic<int> count = 0;
    std::atomic<bool> done = false;

    std::thread threads[num_threads];
    TestNode nodes[num_threads][num_entries];

    for (int i = 0; i < num_threads; ++i) {
        if (i & 1) { // writers
            threads[i] = std::thread([&, i] {
                for (int j = 0; j < num_entries; ++j) {
                    if (done) {
                        return;
                    }

                    TestNode *n = &nodes[i][j];
                    n->val = 2;
                    st.Push(*n);
                    std::this_thread::yield();
                }
            });
        } else { // readers
            threads[i] = std::thread([&]{
                for (int j = 0; j < num_entries * num_writers; ++j) {
                    if (done) {
                        return;
                    }

                    TestNode *n = st.Pop();
                    if (n != nullptr) {
                        count += n->val;
                    }
                    std::this_thread::yield();
                }
            });
        }
    }

    int num_tries = 0;
    while (count != 2 * num_entries * num_writers && num_tries++ < 10000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    ASSERT_EQ(count, 2 * num_entries * num_writers);

    done = true; // Cleanup
    for (auto &thread : threads) {
        thread.join();
    }
}

