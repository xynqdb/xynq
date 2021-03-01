cmake_minimum_required(VERSION 3.0)
project(xynq)

set(ROOTDIR .)
set(ROOTDIR_DEPTH 8) # Max depth to find the build root

while(ROOTDIR_DEPTH GREATER 0 AND NOT EXISTS "${ROOTDIR}/build/xynq.build")
    set(ROOTDIR "${ROOTDIR}/..")
    math(EXPR ROOTDIR_DEPTH "${ROOTDIR_DEPTH} - 1")
endwhile()

if (NOT EXISTS "${ROOTDIR}/build/xynq.build")
    message(FATAL_ERROR "Cannot find project root directory. Make sure you run this build from the repo folder.")
endif()

get_filename_component(ROOTDIR "${ROOTDIR}"
                       REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

message(STATUS "Found root directory at: ${ROOTDIR}")

set(BINDIR ${ROOTDIR}/bin)                   # Path to output binaries
set(BINTEMP ${ROOTDIR}/bin_temp)             # Temporary binaries
set(SRCDIR ${ROOTDIR}/source)                # Path to source dir
set(TESTDIR ${ROOTDIR}/tests)                # Path to tests dir
set(THIRDPARTYDIR ${ROOTDIR}/third_party)    # Path to 3rdparty
set(INCDIR ${SRCDIR})                        # Include directories


# Tests executable name
set(TEST_EXE ${PROJECT_NAME}_tests)

# Compiler.
#set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_EXE_LINKER_FLAGS "-Wl -lm -lstdc++")
set(CMAKE_CXX_FLAGS "-std=c++1z -stdlib=libc++ -fno-exceptions -fno-rtti -Wall -Wextra -pedantic -Wno-c99-extensions")

# Set Debug/release specific targets
if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Debug Build")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -DDEBUG")
else()
    message(STATUS "Release Build")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fdata-sections -ffunction-sections")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

set(PLATFORM_BINDIR ${BINDIR}/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
set(PLATFORM_BINTEMP ${BINTEMP}/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PLATFORM_BINTEMP})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PLATFORM_BINTEMP})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PLATFORM_BINDIR})
############################################################


# Platform specific
if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# Apple specific.
if (APPLE)
    set(INCDIR ${INCDIR}
               ${SRCDIR}/base/platform/unix)

    # Apple's compiler generates debug symbols with dsymutil.
    message(STATUS "Looking for dsymutil")
    find_program(DSYMUTIL dsymutil)
endif()

# Linux specific.
if (LINUX)

    # Always prefer clang. Try find it.
    message(STATUS "Looking for clang")
    find_program(CLANG clang)
    if (NOT CLANG)
        find_program(CLANG clang-12)
    endif()

    if (CLANG)
        message(STATUS "Enforcing clang as default compiler: ${CLANG}")
        set(CMAKE_CXX_COMPILER ${CLANG})
        set(CMAKE_C_COMPILER ${CLANG})
    else()
        message(STATUS "Cannot find clang. Will try compile with the default compiler")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(INCDIR ${INCDIR}
               ${SRCDIR}/event/epoll
               ${SRCDIR}/base/platform/linux
               ${SRCDIR}/base/platform/unix)

    set(EVENT_SRC
        ${SRCDIR}/event/epoll/event/eventqueue.cc
    )

    set(BASE_SRC
        ${SRCDIR}/base/platform/linux/os/utils.cc)
endif()
############################################################


# Sources.
set(BASE_SRC ${BASE_SRC}
    ${SRCDIR}/base/scratch_allocator.cc
    ${SRCDIR}/base/system_allocator.cc
    ${SRCDIR}/base/file_stream.cc
    ${SRCDIR}/base/fileutils.cc
    ${SRCDIR}/base/log.cc
    ${SRCDIR}/base/output.cc
    ${SRCDIR}/base/str_build_types.cc
    ${SRCDIR}/base/stream.cc
)

set(CONTAINERS_SRC
    ${SRCDIR}/containers/str.cc
)

set(SLANG_SRC
    ${SRCDIR}/slang/call.cc
    ${SRCDIR}/slang/compiler.cc
    ${SRCDIR}/slang/env.cc
    ${SRCDIR}/slang/lexer.cc
    ${SRCDIR}/slang/math_funcs.cc
    ${SRCDIR}/slang/slang.cc
    ${SRCDIR}/slang/program.cc
)

set(CONFIG_SRC
    ${SRCDIR}/config/config.cc
    ${SRCDIR}/config/config_detail.cc
)

set(TASK_SRC
    ${SRCDIR}/task/task.cc
    ${SRCDIR}/task/task_context.cc
    ${SRCDIR}/task/task_manager.cc
    ${SRCDIR}/task/task_semaphore.cc
    ${SRCDIR}/task/worker_thread.cc
)

set(NET_SRC
    ${SRCDIR}/net/tcp.cc
)

set(JSON_SRC
    ${SRCDIR}/json/json_serializer.cc)

set(TYPES_SRC
    ${SRCDIR}/types/basic_types.cc
    ${SRCDIR}/types/type_vault.cc
    ${SRCDIR}/types/value_types.cc
)

set(STORAGE_SRC
    ${SRCDIR}/storage/storage.cc
    ${SRCDIR}/storage/object_writer.cc
    ${SRCDIR}/storage/object_vault.cc
)

set(MAIN_SRC
    ${SRCDIR}/main/endpoint.cc
    ${SRCDIR}/main/main.cc
    ${SRCDIR}/main/slang_env.cc
    ${SRCDIR}/main/json_payload_handler.cc
)


# Tests sources.
set(TEST_SRC
    ${TESTDIR}/tests_main.cc
    ${TESTDIR}/gtest_wrap.cc

    # Base.
    ${TESTDIR}/base/dep.cc
    ${TESTDIR}/base/defer.cc
    ${TESTDIR}/base/either.cc
    ${TESTDIR}/base/hook.cc
    ${TESTDIR}/base/maybe.cc
    ${TESTDIR}/base/fileutils.cc
    ${TESTDIR}/base/span.cc
    ${TESTDIR}/base/scratch_allocator.cc
    ${TESTDIR}/base/str_builder.cc

    # Containers.
    ${TESTDIR}/containers/linked_stack.cc
    ${TESTDIR}/containers/list.cc

    # Slang.
    ${TESTDIR}/slang/compiler.cc
    ${TESTDIR}/slang/lexer.cc
    ${TESTDIR}/slang/program.cc

    # Config.
    ${TESTDIR}/config/config.cc

    # Task manager.
    ${TESTDIR}/task/task.cc
)
############################################################


# Libraries.
add_library(base STATIC ${BASE_SRC})
target_include_directories(base PUBLIC ${INCDIR})

add_library(containers STATIC ${CONTAINERS_SRC})
target_include_directories(containers PUBLIC ${INCDIR})
target_link_libraries(containers base)

add_library(types STATIC ${TYPES_SRC})
target_include_directories(types PUBLIC ${INCDIR})
target_link_libraries(base)

add_library(slang STATIC ${SLANG_SRC})
target_include_directories(slang PUBLIC ${INCDIR})
target_link_libraries(slang base types)

add_library(storage STATIC ${STORAGE_SRC})
target_include_directories(storage PUBLIC ${INCDIR})
target_link_libraries(storage base types)

add_library(config STATIC ${CONFIG_SRC})
target_include_directories(config PUBLIC ${INCDIR})
target_link_libraries(config slang base containers)

add_library(json STATIC ${JSON_SRC})
target_include_directories(json PUBLIC ${INCDIR})
target_link_libraries(base)

# Only epoll implementation so far.
# So only for linux.
if(LINUX)
    add_library(event STATIC ${EVENT_SRC})
    target_include_directories(event PUBLIC ${INCDIR})
    target_link_libraries(event base)

    add_library(task STATIC ${TASK_SRC})
    target_include_directories(task PUBLIC ${INCDIR})
    target_link_libraries(task slang base event)

    add_library(net STATIC ${NET_SRC})
    target_include_directories(net PUBLIC ${INCDIR})
    target_link_libraries(net base event task)
endif()
#############################################################


# Targets.

# Tests.
add_executable(${TEST_EXE} ${TEST_SRC})
target_include_directories(${TEST_EXE} PRIVATE
    ${SRCDIR}
    ${TESTDIR}
    ${THIRDPARTYDIR}/googletest/googletest/include
    ${THIRDPARTYDIR}/googletest/googletest)
target_link_libraries(${TEST_EXE} PRIVATE
    base
    containers
    config
    types
    slang
    storage
    task
    event
    net)

# XynqDB
add_executable(${PROJECT_NAME} ${MAIN_SRC})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRCDIR})
target_link_libraries(${PROJECT_NAME} PRIVATE
    base
    containers
    config
    json
    types
    slang
    storage
    task
    event
    net)

if (APPLE) # Generate dSYM file on Apple platforms.
    add_custom_command(TARGET ${TEST_EXE} POST_BUILD WORKING_DIRECTORY ${BINDIR}
        COMMAND "${DSYMUTIL}" ARGS "${PLATFORM_BINDIR}/${TEST_EXE}" VERBATIM
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD WORKING_DIRECTORY ${BINDIR}
        COMMAND "${DSYMUTIL}" ARGS "${PLATFORM_BINDIR}/${PROJECT_NAME}" VERBATIM
    )
endif (APPLE)
############################################################
