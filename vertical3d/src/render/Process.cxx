/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Process.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <chrono>
#include <csignal>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include <boost/filesystem/path.hpp>

namespace v3d::editor {

namespace {

/**
 * How long the wait for the program sleeps between looks at the stop.
 **/
constexpr std::chrono::milliseconds POLL{ 50 };

};  // namespace

#ifdef _WIN32

/**
 **/
int runProgram(const boost::filesystem::path& program, const std::vector<std::string>& arguments,
    const std::stop_token& stop) {
    // each argument is quoted, so a path with spaces in it stays one argument. The arguments
    // passed here are paths and flags, which hold no quote of their own
    std::wstring command = L"\"" + program.wstring() + L"\"";
    for (const std::string& argument : arguments) {
        command += L" \"" + boost::filesystem::path(argument).wstring() + L"\"";
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup,
        &process) == 0) {
        return -1;
    }
    CloseHandle(process.hThread);

    int code = -1;
    while (true) {
        if (stop.stop_requested()) {
            TerminateProcess(process.hProcess, 1);
            WaitForSingleObject(process.hProcess, INFINITE);
            break;
        }
        if (WaitForSingleObject(process.hProcess, static_cast<DWORD>(POLL.count())) == WAIT_OBJECT_0) {
            DWORD exit = 0;
            if (GetExitCodeProcess(process.hProcess, &exit) != 0) {
                code = static_cast<int>(exit);
            }
            break;
        }
    }
    CloseHandle(process.hProcess);
    return code;
}

#else

/**
 **/
int runProgram(const boost::filesystem::path& program, const std::vector<std::string>& arguments,
    const std::stop_token& stop) {
    std::vector<std::string> owned{ program.string() };
    owned.insert(owned.end(), arguments.begin(), arguments.end());
    std::vector<char*> argv;
    for (std::string& each : owned) {
        argv.push_back(each.data());
    }
    argv.push_back(nullptr);

    pid_t child = 0;
    if (posix_spawn(&child, owned.front().c_str(), nullptr, nullptr, argv.data(), environ) != 0) {
        return -1;
    }
    while (true) {
        int state = 0;
        const pid_t ended = waitpid(child, &state, WNOHANG);
        if (ended == child) {
            return WIFEXITED(state) ? WEXITSTATUS(state) : -1;
        }
        if (ended < 0) {
            return -1;
        }
        if (stop.stop_requested()) {
            kill(child, SIGTERM);
            waitpid(child, &state, 0);
            return -1;
        }
        std::this_thread::sleep_for(POLL);
    }
}

#endif

};  // namespace v3d::editor
