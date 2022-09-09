
#ifndef PROXYFMU_PROCESS_HELPER_HPP
#define PROXYFMU_PROCESS_HELPER_HPP

#include <proxyfmu/fs_portability.hpp>

#include <subprocess/subprocess.h>

#include <condition_variable>
#include <exception>
#include <iostream>
#include <mutex>
#include <string>

#ifdef WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#endif
#ifdef __linux__
#    include <climits>
#    include <cstring>
#    include <unistd.h>
#endif

namespace proxyfmu
{

std::string getLoc()
{
#ifdef __linux__
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
#else
    char result[MAX_PATH];
    return {result, GetModuleFileName(nullptr, result, MAX_PATH)};
#endif
}

void start_process(
    const proxyfmu::filesystem::path& fmuPath,
    const std::string& instanceName,
    int& port,
    std::mutex& mtx,
    std::condition_variable& cv)
{

    proxyfmu::filesystem::path executable;
#ifdef __linux__
    executable = "proxyfmu";
#else
    executable = "proxyfmu.exe";
#endif

    if (!proxyfmu::filesystem::exists(executable)) {
        const std::string loc = getLoc();
        executable = proxyfmu::filesystem::path(loc).parent_path().string() / executable;
    }

    if (!proxyfmu::filesystem::exists(executable)) {
        const auto execPath = proxyfmu::filesystem::absolute(executable).string();
        throw std::runtime_error("[proxyfmu] No proxyfmu executable found. '" + execPath + "' does not exist!");
    }

    std::string execStr = executable.string();
#ifdef __linux__
    if (!executable.is_absolute()) {
        execStr.insert(0, "./");
    }
#endif

    std::cout << "[proxyfmu] Found proxyfmu executable: " << executable << " version: ";
    std::cout.flush();
    int statusCode = system((execStr + " -v").c_str());
    if (statusCode != 0) {
        std::cerr << "ERROR - unable to invoke proxyfmu!" << std::endl;

        std::lock_guard<std::mutex> lck(mtx);
        port = -999;
        cv.notify_one();
        return;
    }
    std::cout << std::endl;
    std::cout << "[proxyfmu] Booting FMU instance '" << instanceName << "'.." << std::endl;

    const std::string fmuPathStr = fmuPath.string();
    std::vector<const char*> cmd = {execStr.c_str(), "--fmu", fmuPathStr.c_str(), "--instanceName", instanceName.c_str(), nullptr};

    // clang-format off
    struct subprocess_s process{};
    // clang-format on
    int result = subprocess_create(cmd.data(), subprocess_option_inherit_environment | subprocess_option_no_window, &process);

    bool bound = false;
    if (result == 0) {
        FILE* p_stdout = subprocess_stdout(&process);
        char buffer[256];
        while (fgets(buffer, 256, p_stdout)) {
            std::string line(buffer);
            if (!bound && line.substr(0, 16) == "[proxyfmu] port=") {
                {
                    std::lock_guard<std::mutex> lck(mtx);
                    port = std::stoi(line.substr(16));
                    std::cout << "[proxyfmu] FMU instance '" << instanceName << "' instantiated using port " << port << std::endl;
                }
                cv.notify_one();
                bound = true;
            } else if (line.substr(0, 16) == "[proxyfmu] freed") {
                break;
            } else {
                std::cerr << line;
            }
        }

        int status = 0;
        result = subprocess_join(&process, &status);

        if (result == 0 && status == 0 && bound) {
            return;
        } else {
            std::cerr << "[proxyfmu] External proxy process for instance '"
                      << instanceName << "' returned with status "
                      << std::to_string(status) << ". Unable to bind.." << std::endl;
        }
    }
    std::lock_guard<std::mutex> lck(mtx);
    port = -999;

    cv.notify_one();
}

} // namespace proxyfmu

#endif // PROXYFMU_PROCESS_HELPER_HPP
