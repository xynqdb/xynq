#include "task/task_manager.h"
#include "task/task_semaphore.h"

#include "base/defer.h"
#include "base/log.h"

#include "gtest/gtest.h"

using namespace xynq;

namespace {

// Helper struct to store data from inside tasks
// and later check them in tests.
struct TestData {
    int int_val = 0;
};

// Oneshot increment.
struct TestTask : public TaskDefaults {
    static constexpr auto exec = [](TaskContext *tc, TestData *test_data) {
        test_data->int_val++;
        tc->Exit();
    };
};

struct UserDataTest : public TaskDefaults {
    static constexpr auto exec = [](TaskContext *tc, int *outval) {
        *outval = tc->UserData<TestData>().int_val;
        tc->Exit();
    };
};

// Calculates n-th fibonacci number concurrently.
struct Fib : public TaskDefaults {
    static constexpr auto exec = [](TaskContext *tc, TestData *result, int seq_number, TaskSemaphore *sem) {
        Defer sem_signal([&]{
            if (sem == nullptr) {
                tc->Exit(); // Exiting initial task.
            }

            sem->Signal(); // Signaling parent this task is done.
        });

        if (seq_number <= 1) {
            result->int_val = seq_number;
            return;
        }

        TestData left, right;
        TaskSemaphore complete{2};
        tc->PerformAsync<Fib>(&left, seq_number - 1, &complete);
        tc->PerformAsync<Fib>(&right, seq_number - 2, &complete);

        complete.Wait(*tc);
        result->int_val = left.int_val + right.int_val;
    };
};

} // anon namespace


TEST(Task, Entrypoint) {
    Dependable<Log> log{std::move(Log::Create(LogLevel::None, 0, {}).Right())};
    TaskManager task_manager(log, 10, 2, false, true);

    TestData test_data;
    task_manager.AddEntryPoint<TestTask>(&test_data);
    task_manager.Run();

    ASSERT_EQ(test_data.int_val, 1);
}

TEST(Task, Fib) {
    Dependable<Log> log{std::move(Log::Create(LogLevel::None, 0, {}).Right())};
    TaskManager task_manager(log, 10, 2, false, true);

    TestData test_data;
    task_manager.AddEntryPoint<Fib>(&test_data, 10, nullptr);
    task_manager.Run();
    ASSERT_EQ(test_data.int_val, 55);
}

TEST(Task, UserData) {
    Dependable<Log> log{std::move(Log::Create(LogLevel::None, 0, {}).Right())};
    TaskManager task_manager(log, 10, 2, false, true);

    task_manager.hooks.before_thread_start.Add([](size_t, Dep<Log>, ThreadUserDataStorage &storage){
        TestData *td = new (&storage) TestData{};
        td->int_val = 973;
    });
    int result = 0;
    task_manager.AddEntryPoint<UserDataTest>(&result);
    task_manager.Run();
    ASSERT_EQ(result, 973);
}
