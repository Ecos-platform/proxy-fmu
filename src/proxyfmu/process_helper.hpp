
#ifndef PROXYFMU_PROCESS_HELPER_HPP
#define PROXYFMU_PROCESS_HELPER_HPP

#include <proxyfmu/fs_portability.hpp>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process.hpp>

#include <condition_variable>
#include <exception>
#include <iostream>
#include <mutex>
#include <string>

namespace proxyfmu
{

std::string boolToStr(bool b)
{
    return b ? "true" : "false";
}

void start_process(
    const proxyfmu::filesystem::path& fmuPath,
    const std::string& instanceName,
    std::string& bind,
    std::mutex& mtx,
    std::condition_variable& cv,
    bool localhost)
{

    proxyfmu::filesystem::path executable;
#ifdef __linux__
    executable = "proxyfmu";
#else
    executable = "proxyfmu.exe";
#endif

    if (!proxyfmu::filesystem::exists(executable)) {
        auto execPath = proxyfmu::filesystem::absolute(executable).string();
        throw std::runtime_error("[proxyfmu] No proxyfmu executable found. " + execPath + " does not exist!");
    }

    std::string execStr = executable.string();
#ifdef __linux__
    if (!executable.is_absolute()) {
        execStr.insert(0, "./");
    }
#endif

    std::cout << "[proxyfmu] Found proxyfmu executable: " << executable << " version: ";
    std::cout.flush();
    system((execStr + " -v").c_str());
    std::cout << "\n";
    std::cout << "[proxyfmu] Booting FMU instance '" << instanceName << "'.." << std::endl;

    std::string cmd(execStr + " --fmu \"" + fmuPath.string() + "\" --instanceName " + instanceName + " --localhost " + boolToStr(localhost));

    boost::process::ipstream pipe_stream;
    boost::process::child c(cmd, boost::process::std_out > pipe_stream);

    bool bound = false;
    std::string line;
    while (pipe_stream && std::getline(pipe_stream, line)) {
        if (!bound && line.substr(0, 16) == "[proxyfmu] port=") {
            {
                std::lock_guard<std::mutex> lck(mtx);
                bind = line.substr(16);
                if (bind.back() == '\r' || bind.back() == '\n') {
                    bind.pop_back();
                }
                std::cout << "[proxyfmu] FMU instance '" << instanceName << "' instantiated and bound to " << bind << std::endl;
            }
            cv.notify_one();
            bound = true;
        } else if (line.substr(0, 16) == "[proxyfmu] freed") {
            break;
        } else {
            std::cerr << line << std::endl;
        }
    }

    c.wait();
    int status = c.exit_code();

    if (status == 0 && bound) {
        return;
    } else {
        std::cerr << "[proxyfmu] External proxy process for instance '"
                  << instanceName << "' returned with status "
                  << std::to_string(status) << ". Unable to bind.." << std::endl;
        std::lock_guard<std::mutex> lck(mtx);
        bind = "-";

        cv.notify_one();
    }
}

} // namespace proxyfmu

#endif // PROXYFMU_PROCESS_HELPER_HPP
