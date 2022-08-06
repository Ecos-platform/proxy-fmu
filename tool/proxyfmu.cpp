
#include "fixed_range_random_generator.hpp"
#include "fmu_service_handler.hpp"

#include "proxyfmu/fs_portability.hpp"
#include "proxyfmu/lib_info.hpp"

#include <boost/program_options.hpp>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <functional>
#include <ios>
#include <iostream>
#include <random>
#include <utility>

using namespace proxyfmu::thrift;
using namespace proxyfmu::server;

using namespace ::apache::thrift;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

namespace
{

class raii_path
{

public:
    explicit raii_path(proxyfmu::filesystem::path path)
        : path_(std::move(path))
    { }

    [[nodiscard]] std::string string() const
    {
        return path_.string();
    }

    ~raii_path()
    {
        proxyfmu::filesystem::remove_all(path_);
    }

private:
    proxyfmu::filesystem::path path_;
};

std::string generate_uuid()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    int i;
    std::stringstream ss;
    ss << std::hex;
    for (i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << dis(gen);
    }
    return ss.str();
}

class ServerReadyEventHandler : public TServerEventHandler
{

private:
    std::function<void()> callback_;

public:
    explicit ServerReadyEventHandler(std::function<void()> callback)
        : callback_(std::move(callback))
    { }

    void preServe() override
    {
        callback_();
    }
};

const int port_range_min = 49152;
const int port_range_max = 65535;

const int max_port_retries = 10;

const int SUCCESS = 0;
const int COMMANDLINE_ERROR = 1;
const int UNHANDLED_ERROR = 2;

int run_application(const std::string& fmu, const std::string& instanceName, bool localhost)
{
    std::unique_ptr<TSimpleServer> server;
    auto stop = [&]() {
        server->stop();
    };
    std::shared_ptr<fmu_service_handler> handler(new fmu_service_handler(fmu, instanceName, stop));
    std::shared_ptr<TProcessor> processor(new FmuServiceProcessor(handler));

    std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    if (!localhost) {
        proxyfmu::fixed_range_random_generator rng(port_range_min, port_range_max);

        int port;
        int final_port = -1;
        for (auto i = 0; i < max_port_retries; i++) {
            port = rng.next();

            std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
            server = std::make_unique<TSimpleServer>(processor, serverTransport, transportFactory, protocolFactory);
            server->setServerEventHandler(std::make_shared<ServerReadyEventHandler>([port, &final_port] {
                final_port = port;
                std::cout << "[proxyfmu] port=" << std::to_string(final_port) << std::endl;
            }));

            try {

                server->serve();
                break;

            } catch (TTransportException& ex) {
                std::cout << "[proxyfmu] " << ex.what()
                          << ". Failed to bind to port " << std::to_string(port)
                          << ". Retrying with another one. Attempt " << std::to_string(i + 1)
                          << " of " << std::to_string(max_port_retries) << ".." << std::endl;
            }
        }

        if (final_port != -1) {
            return SUCCESS;
        } else {
            std::cerr << "[proxyfmu] Unable to bind after max number of retries.." << std::endl;
            return UNHANDLED_ERROR;
        }

    } else {

        raii_path uds{"uds_" + generate_uuid() + ".binary.thrift"};
        std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(uds.string()));
        server = std::make_unique<TSimpleServer>(processor, serverTransport, transportFactory, protocolFactory);

        server->setServerEventHandler(std::make_shared<ServerReadyEventHandler>([&uds] {
            std::cout << "[proxyfmu] port=" << uds.string() << std::endl;
        }));

        try {
            server->serve();

            return SUCCESS;
        } catch (TTransportException& ex) {
            std::cerr << "[proxyfmu] " << ex.what() << std::endl;
            return UNHANDLED_ERROR;
        }
    }
}

int printHelp(boost::program_options::options_description& desc)
{
    std::cout << "proxyfmu" << '\n'
              << desc << std::endl;
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

    namespace po = boost::program_options;

    po::options_description desc("Options");
    desc.add_options()("help,h", "Print this help message and quits.");
    desc.add_options()("version,v", "Print program version.");
    desc.add_options()("fmu", po::value<std::string>()->required(), "Location of the fmu to load.");
    desc.add_options()("instanceName", po::value<std::string>()->required(), "Name of the slave instance.");
    desc.add_options()("localhost", po::value<bool>()->required(), "Running on localhost?");

    if (argc == 1) {
        return printHelp(desc);
    }

    try {

        po::variables_map vm;
        try {

            po::store(po::parse_command_line(argc, argv, desc), vm);

            if (vm.count("help")) {
                return printHelp(desc);
            } else if (vm.count("version")) {
                return printVersion();
            }

            po::notify(vm);

        } catch (const po::error& e) {
            std::cerr << "ERROR: " << e.what() << "\n"
                      << std::endl;
            std::cout << desc << std::endl;
            return COMMANDLINE_ERROR;
        }

        const auto fmu = vm["fmu"].as<std::string>();
        const auto fmuPath = proxyfmu::filesystem::path(fmu);
        if (!proxyfmu::filesystem::exists(fmuPath)) {
            std::cerr << "[proxyfmu] No such file: '" << proxyfmu::filesystem::absolute(fmuPath) << "'";
            return COMMANDLINE_ERROR;
        }

        const auto instanceName = vm["instanceName"].as<std::string>();

        return run_application(fmu, instanceName, vm["localhost"].as<bool>());

    } catch (const std::exception& e) {
        std::cerr << "[proxyfmu] Unhandled Exception reached the top of main: " << e.what() << ", application will now exit" << std::endl;
        return UNHANDLED_ERROR;
    }
}
