#include "execute_files.h"
#include "json_payload_handler.h"
#include "shared_deps.h"
#include "slang_env.h"
#include "version.h"

#include "base/system_allocator.h"
#include "base/dep.h"
#include "base/log.h"
#include "base/maybe.h"
#include "base/span.h"
#include "base/output.h"
#include "os/utils.h"
#include "config/config.h"
#include "main/echo_server.h"
#include "main/endpoint_handler.h"
#include "net/tcp.h"
#include "task/task_manager.h"
#include "task/task_context.h"
#include "types/type_vault.h"

#include <functional>
#include <climits>
#include <thread>

using namespace xynq;

DefineTaggedLog(Main)

namespace {

void PrintHelp() {
    XYOutput("Command line should be xynqdb --config <config_filepath>\n"
             "\tOr --key value pairs of config parameters. See documentation for available keys.");
}

// First check if there is --config arg and load config.
// Then load config overrides from command line:
// ie. command line options with slash ie. /server.port 12345.
Config::LoadResult LoadConfig(int argc, char *argv[]) {
    Config result;

    int i = 1;
    bool config_file_found = false;
    while (i < argc) {
        if (strcmp(argv[i], "--config") == 0 || !strcmp(argv[i], "-c")) { // Next arg should be config filename
            ++i;
            if (i >= argc) {
                XYOutputError("No config file specified with --config.");
                PrintHelp();
                return ConfigLoadError::InvalidArgs;
            }

            XYOutput("Loading config file '%s'", argv[i]);
            auto loaded = Config::LoadFromFile(argv[i]);
            if (loaded.IsLeft()) {
                XYOutputError("Failed to load config file at %s", argv[i]);
                return loaded.Left();
            }

            result = Config::Merge(std::move(result), std::move(loaded.Right()));
            config_file_found = true;
        } else if (argv[i][0] == '/') {
            // do nothing, will parse it later.
            i += 2;
            continue;
        } else {
            XYOutputError("Unknown argument: %s", argv[i]);
            PrintHelp();
            return ConfigLoadError::InvalidArgs;
        }

        ++i;
    }

    if (!config_file_found) { // No config was specified  - try load from default location.
        static const char *default_config_path = "./xynqdb.conf";
        auto loaded = Config::LoadFromFile(default_config_path);
        if (loaded.IsLeft()) {
            // If file not found -> continue running with defaults
            // but if file was found but failed to load (ie. parse error)
            // -> fail and exit.
            if (loaded.Left() == ConfigLoadError::FileNotFound) {
                XYOutput("No config loaded. Will use defaults. "
                         "Tried '%s' - but no file found.", default_config_path);
            } else {
                XYOutputError("Failed to load default config: %s", default_config_path);
                return loaded.Left();
            }
        }
    }

    // Try load config paramaeters from command line.
    // if some config file was loaded before - these will override them.
    auto loaded_args = Config::LoadFromArgs(argc - 1, argv + 1);
    if (loaded_args.IsLeft()) {
        XYOutputError("Failed to parse configuration arguments.");
        return loaded_args.Left();
    }

    // Merge what was loaded from the file(s) with command line args.
    result = Config::Merge(std::move(result), std::move(loaded_args.Right()));
    return std::move(result);
}

Either<LogFailure, Log> CreateLog(const Config &conf) {
    // By default, writing log to stdout, syslog is disabled.
    bool enable_stdout = conf.Get<bool>("log.stdout").RightOrDefault(true);
    bool enable_syslog = conf.Get<bool>("log.syslog").RightOrDefault(false);
    CStrSpan log_file = conf.Get<CStrSpan>("log.file").RightOrDefault("");
    CStrSpan log_level_str = conf.Get<CStrSpan>("log.level")
        .RightOrDefault("info");

    auto log_level = LogLevelFromString(log_level_str);
    if (log_level.IsLeft()) {
        XYOutputError("Invalid log level: %s. Should be error|warning|info|verbose", log_level_str.CStr());
        return log_level.Left();
    }

    unsigned flags = 0;
    if (enable_stdout) {
        flags |= LogFlags::StdOut;
    }
    if (enable_syslog) {
        flags |= LogFlags::SysLog;
    }

    auto result = Log::Create(log_level.Right(), flags, log_file.IsEmpty() ? Maybe<CStrSpan>{} : log_file);
    if (result.IsLeft()) {
        XYOutputError("Failed to open log file: %s", log_file.CStr());
    }

    if (!log_file.IsEmpty()) {
        XYOutput("Saving log to '%s'", log_file.CStr());
    }

    return result;
}

Maybe<TaskManager *> CreateTaskManager(Dep<Log> log, Dep<Config> conf) {
    size_t num_threads = kNumThreadsAutoDetect;
    auto num_threads_str = conf->Get<CStrSpan>("task.num-threads");
    if (num_threads_str.IsRight()) { // First, see if it's set to "auto".
        if (num_threads_str.Right() != "auto") {
            XYMainError(log, "Invalid number of threads in the config. Must be 'auto' or number."
                             "(task.num-threads=", num_threads_str.Right().CStr(), ").");
            return {};
        }
    } else {
        auto num_threads_cfg = conf->Get<size_t>("task.num-threads");
        if (num_threads_cfg.IsRight()) {
            XYMainError(log, "Invalid number of threads. Must be >= 0. (task.num-threads=", num_threads_cfg.Right(), ").");
            return {};
        }

        num_threads = num_threads_cfg.RightOrDefault(kNumThreadsAutoDetect);
    }

    int max_events_at_once = conf->Get<int>("events.max-events-at-once").RightOrDefault(1024);
    if (max_events_at_once <= 0) {
        XYMainError(log, "Invalid max_events_at_once limit (", max_events_at_once, ").");
        return {};
    }

    bool pin_threads = conf->Get<bool>("task.pin-threads")
        .RightOrDefault(true);

    return CreateObject<TaskManager>(SystemAllocator::Shared(),
                                     log,
                                     static_cast<size_t>(max_events_at_once),
                                     static_cast<size_t>(num_threads),
                                     pin_threads,
                                     true);
}

Maybe<TcpManager> CreateTcpManager(Dep<Log> log, Dep<Config> conf, Dep<TaskManager> tasks) {
    auto addrs_result = conf->GetList("tcp.bind")
        .RightOrDefault(ConfigList::Make("0.0.0.0:9920")).AsArray<CStrSpan>();

    if (!addrs_result.HasValue()) {
        XYMainError(log, "Tcp bind addresses list is invalid. Should be list of strings like 'ip:port'");
        return {};
    }

    TcpParameters tcp_params;
    tcp_params.listen_backlog = conf->Get<int>("tcp.listen-backlog").RightOrDefault(512);
    tcp_params.reuse_addr = conf->Get<bool>("tcp.reuse-bind-addr").RightOrDefault(false);
    tcp_params.keep_alive.enable = conf->Get<bool>("tcp.keep-alive.enable").RightOrDefault(false);
    tcp_params.keep_alive.idle_sec = conf->Get<int>("tcp.keep-alive.idle").RightOrDefault(20);
    tcp_params.keep_alive.interval_sec = conf->Get<int>("tcp.keep-alive.interval").RightOrDefault(20);
    tcp_params.keep_alive.num_probes = conf->Get<int>("tcp.keep-alive.probes").RightOrDefault(8);

    TcpNewStreamHandler stream_handler = [](TaskContext *tc, StrSpan name, InOutStream *io_stream) {
        tc->PerformSync<EndpointHandler>(name, io_stream);
    };

    return TcpManager::Create(log, tasks, tcp_params, Span<CStrSpan>{addrs_result.Value()}, stream_handler);
}

// Storage.
Storage CreateStorage(Dep<Log> log, Dep<Config> conf) {
    return Storage{};
}

bool ScheduleConfigExecs(Dep<Config> conf, Dep<TaskManager> task_manager) {
    auto exec_list = conf->GetList("exec");
    if (exec_list.IsLeft()) {
        return true;
    }

    auto files = exec_list.Right().AsArray<CStrSpan>();
    if (!files.HasValue()) {
        return false;
    }

    task_manager->AddEntryPoint<ExecuteFiles>(std::move(files.Value()));
    return true;
}

// Exit handler.
std::function<void(int)> platform_exit_handler;
std::function<void(const char *str)> platform_log_handler;
int platform_exit_code = 0;
void CreateExitHandler(Dep<Log> log, Dep<TaskManager> tasks) {
    platform_log_handler = [&](const char *str) {
        XYMainInfo(log, str);
    };

    // Will stop all tasks -> process will exit.
    platform_exit_handler = [&] (int exit_code) {
        tasks->Stop();
        platform_exit_code = exit_code;
    };

    platform::InitExitHandler([] (int exit_code) {
        platform_exit_handler(exit_code);
    },
    [](const char *str) {
        platform_log_handler(str);
    });
}

int XYEntrypoint(int argc, char *argv[]) {
    XYOutput("XynqDB v" XYNQ_VERSION_STR "-" XYNQ_BUILD_FLAVOUR);

    auto before_init = std::chrono::steady_clock::now();

    // Initializing all core subsystems.
    // Config.
    auto load_cfg = LoadConfig(argc, argv);
    if (load_cfg.IsLeft()) {
        return -1;
    }
    Dependable<Config> config = std::move(load_cfg.Right());

    // Log.
    auto create_log = CreateLog(config.Get());
    if (create_log.IsLeft()) {
        return -1;
    }
    Dependable<Log> log = std::move(create_log.Right());
    XYMainInfo(log, "Begin logging (v", XYNQ_VERSION_STR, '-', XYNQ_BUILD_FLAVOUR,
               ", pid=", platform::GetPid(),  ")");

    config->Enumerate([&log](const CStrSpan key, const CStrSpan value) {
        XYMainInfo(log, "Loaded config: ", key, " -> ", value);
    });

    // Task Manager.
    auto create_tasks = CreateTaskManager(log, config);
    if (!create_tasks.HasValue()) {
        return -1;
    }

    // Types.
    Dependable<TypeManager> type_manager{log, SystemAllocator::SharedDep(), std::initializer_list<TypeSchemaPtr>{
        XYBasicType(double),
        XYBasicType(float),
        XYBasicType(int8_t),
        XYBasicType(int16_t),
        XYBasicType(int32_t),
        XYBasicType(int64_t),
        XYBasicType(uint8_t),
        XYBasicType(uint16_t),
        XYBasicType(uint32_t),
        XYBasicType(uint64_t),
    }};

    // Storage.
    Dependable<Storage> storage = CreateStorage(log, config);

    // Slang environment.
    Dependable<JsonPayloadHandler> json_payload_handler = JsonPayloadHandler{storage};
    Dependable<slang::Env> slang_env = CreateSlangEnv(json_payload_handler);

    Dependable<TaskManager *> task_manager = create_tasks.Value();

    // Tcp.
    auto create_tcp = CreateTcpManager(log, config, task_manager);
    if (!create_tcp.HasValue()) {
        return -1;
    }

    // Exit handler.
    CreateExitHandler(log, task_manager);

    // If there are some files that need to be executes - schedule them.
    if (!ScheduleConfigExecs(config, task_manager)) {
        return -1;
    }

    // Initialize per thread user-data.
    task_manager->hooks.before_thread_start.Add([&](size_t /*thread_index*/, Dep<Log> log, ThreadUserDataStorage &store){
        SharedDeps *deps = new (&store) SharedDeps{slang_env, storage, type_manager->CreateVault(log)};
        XYAssert((void *)deps == &store);
    });
    task_manager->hooks.after_thread_stop.Add([&](size_t /*thread_index*/, ThreadUserDataStorage &store){
        ((SharedDeps *)&store)->~SharedDeps();
    });

    auto init_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - before_init).count();
    XYMainInfo(log, "Initialization complete (took ", init_time, "ms)");
    XYMainInfo(log, "Starting task manager. (", task_manager->NumThreads(), " threads)");

    // Now, launching all.
    task_manager->Run();
    return platform_exit_code;
}

} // anon namespace

int main(int argc, char *argv[]) {
    SystemAllocator::Initialize();
    int err = XYEntrypoint(argc, argv);
    SystemAllocator::Shutdown();
    return err;
}
