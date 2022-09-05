
#include "boot_service_handler.hpp"

#include <proxyfmu/lib_info.hpp>

#include <CLI/CLI.hpp>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <iostream>
#include <thread>

using namespace proxyfmu::thrift;
using namespace proxyfmu::server;

using namespace ::apache::thrift;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

namespace
{

const int SUCCESS = 0;
const int COMMANDLINE_ERROR = 1;
const int UNHANDLED_ERROR = 2;

void wait_for_input()
{
    std::cout << '\n'
              << "Press any key to quit...\n";
    // clang-format off
    while (std::cin.get() != '\n');
    //clang-format on
    std::cout << "Done." << std::endl;
}

int run_application(const int port)
{
    std::unique_ptr<TSimpleServer> server;
    std::shared_ptr<boot_service_handler> handler(new boot_service_handler());
    std::shared_ptr<TProcessor> processor(new BootServiceProcessor(handler));

    std::shared_ptr<TTransportFactory> transportFactory(new TFramedTransportFactory());
    std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    server = std::make_unique<TSimpleServer>(processor, serverTransport, transportFactory, protocolFactory);

    std::thread t([&server] { server->serve(); });

    wait_for_input();

    server->stop();
    t.join();

    return SUCCESS;
}

int printHelp(CLI::App& desc)
{
    std::cout << desc.help() << std::endl;
    return SUCCESS;
}

int printVersion()
{
    // namespace
    const auto v = proxyfmu::library_version();
    std::cout << v.major << "." << v.minor << "." << v.patch;
    return SUCCESS;
}

} // namespace


int main(int argc, char** argv)
{

CLI::App app{"proxyfmu_booter"};

    app.add_option("-v,--version", "Print program version.");
    app.add_option("--port", "Specify the network port to be used.");

    if (argc == 1) {
        return printHelp(app);
    }

    try {

        if (app.count("--version")) {
            return printVersion();
        }

        const auto port = app["--port"]->as<int>();

        return run_application(port);

    } catch (const std::exception& e) {
        std::cerr << "Unhandled Exception reached the top of main: " << e.what() << ", application will now exit" << std::endl;
        return UNHANDLED_ERROR;
    }
}
