#include "containers/list.h"
#include "gtest/gtest.h"

#include <thread>

using namespace xynq;

struct TestNode {
    int val = 0;
    ListNodePtr<TestNode> next;
};

TEST(ListTest, Nop) {
    List<TestNode, &TestNode::next> l;
    ASSERT_TRUE(l.IsEmpty());
    ASSERT_EQ(l.Head(), nullptr);
    ASSERT_EQ(l.Last(), nullptr);
}

TEST(ListTest, PushFront) {
    List<TestNode, &TestNode::next> l;

    TestNode nodes[10];

    int i = 0;
    for (TestNode &n : nodes) {
        n.val = i++;
        l.PushFront(&n);
    }

    ASSERT_FALSE(l.IsEmpty());
    ASSERT_EQ(l.Head(), &nodes[9]);
    ASSERT_EQ(l.Head()->val, 9);
}

TEST(ListTest, PushListFront) {
    List<TestNode, &TestNode::next> l1;
    List<TestNode, &TestNode::next> l2;

    TestNode nodes1[10];
    TestNode nodes2[5];

    int i = 0;
    for (TestNode &n : nodes1) {
        n.val = i++;
        l1.PushBack(&n);
    }

    for (TestNode &n : nodes2) {
        n.val = i++;
        l2.PushBack(&n);
    }

    l2.PushFront(std::move(l1));

    int j = 0;
    for (TestNode *n = l2.Head(); n != l2.End(); n = n->next) {
        ASSERT_EQ(n->val, j);
        ++j;
    }
    ASSERT_EQ(j, i);
}

TEST(ListTest, PushBack) {
    List<TestNode, &TestNode::next> l;

    TestNode nodes[10];

    int i = 0;
    for (TestNode &n : nodes) {
        n.val = i++;
        l.PushBack(&n);
    }

    ASSERT_FALSE(l.IsEmpty());
    ASSERT_EQ(l.Last(), &nodes[9]);
    ASSERT_EQ(l.Last()->val, 9);
}

TEST(ListTest, PopFront) {
    List<TestNode, &TestNode::next> l;

    static const size_t num_nodes = 10;
    TestNode nodes[num_nodes];

    int i = 0;
    for (TestNode &n : nodes) {
        n.val = i++;
        l.PushBack(&n);
    }

    i = 0;
    while (TestNode *node = l.PopFront()) {
        ASSERT_EQ(node, &nodes[i]);
        ASSERT_EQ(node->val, i);

        ++i;
    }

    ASSERT_TRUE(l.IsEmpty());
    ASSERT_EQ(i, (int)num_nodes);
    ASSERT_EQ(l.PopFront(), nullptr);
    ASSERT_EQ(l.Head(), nullptr);
    ASSERT_EQ(l.Last(), nullptr);
}

TEST(ListTest, PopFrontEmpty) {
    List<TestNode, &TestNode::next> l;
    ASSERT_EQ(l.PopFront(), nullptr);
}