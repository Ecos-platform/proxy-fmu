
#include <fmuproxy/thrift/server/thrift_fmu_server.hpp>

#include <boost/program_options.hpp>
#include <fmi4cpp/fmi2/fmi2.hpp>

#include <iostream>
#include <optional>
#include <unordered_map>
using fmuproxy::thrift::server::thrift_fmu_server;

using namespace std;

namespace
{

const int SUCCESS = 0;
const int COMMANDLINE_ERROR = 1;
const int UNHANDLED_ERROR = 2;

const char* THRIFT_TCP = "thrift/tcp";
const char* THRIFT_HTTP = "thrift/http";
const char* GRPC = "grpc";

void wait_for_input()
{
    do {
        cout << '\n'
             << "Press a key to continue...\n";
    } while (cin.get() != '\n');
    cout << "Done." << endl;
}

int run_application(
    const vector<shared_ptr<fmi4cpp::fmi2::cs_fmu>>& fmus,
    unordered_map<string, unsigned int> ports)
{

    unordered_map<string, shared_ptr<fmi4cpp::fmi2::cs_fmu>> fmu_map;
    vector<string> modelDescriptions;
    for (const auto& fmu : fmus) {
        fmu_map[fmu->get_model_description()->guid] = fmu;
        modelDescriptions.push_back(fmu->get_model_description_xml());
    }

    bool enable_grpc = ports.count(GRPC) == 1;
    bool enable_thrift_tcp = ports.count(THRIFT_TCP) == 1;
    bool enable_thrift_http = ports.count(THRIFT_HTTP) == 1;

#ifdef FMU_PROXY_WITH_THRIFT
    unique_ptr<thrift_fmu_server> thrift_socket_server = nullptr;
    if (enable_thrift_tcp) {
        const unsigned int port = ports[THRIFT_TCP];
        thrift_socket_server = make_unique<thrift_fmu_server>(fmu_map, port, false, true);
        thrift_socket_server->start();
        std::cout << "Thrift/tcp listening for connections on port " << std::to_string(port) << std::endl;
    }

    unique_ptr<thrift_fmu_server> thrift_http_server = nullptr;
    if (enable_thrift_http) {
        const unsigned int port = ports[THRIFT_HTTP];
        thrift_http_server = make_unique<thrift_fmu_server>(fmu_map, port, true);
        thrift_http_server->start();
        std::cout << "Thrift/http listening for connections on port " << std::to_string(port) << std::endl;
    }
#endif

#ifdef FMU_PROXY_WITH_GRPC
    unique_ptr<grpc_fmu_server> grpc_server = nullptr;
    if (enable_grpc) {
        const unsigned int port = ports[GRPC];
        grpc_server = make_unique<grpc_fmu_server>(fmu_map, port);
        grpc_server->start();
        std::cout << "gRPC/http2 listening for connections on port " << std::to_string(port) << std::endl;
    }
#endif

    wait_for_input();

#ifdef FMU_PROXY_WITH_GRPC
    if (enable_grpc) {
        grpc_server->stop();
    }
#endif
#ifdef FMU_PROXY_WITH_THRIFT
    if (enable_thrift_tcp) {
        thrift_socket_server->stop();
    }
    if (enable_thrift_http) {
        thrift_http_server->stop();
    }
#endif
    return 0;
}

} // namespace

int main(int argc, char** argv)
{

    try {

        namespace po = boost::program_options;

        po::options_description desc("Options");
        desc.add_options()("help,h", "Print this help message and quits.")("fmu,f", po::value<vector<string>>()->multitoken(), "Path to FMUs.")("remote,r", po::value<string>(), "IP address of the remote tracking server.");
        desc.add_options()(THRIFT_TCP, po::value<unsigned int>(), "Specify the network port to be used by the Thrift (TCP/IP) server.")(THRIFT_HTTP, po::value<unsigned int>(), "Specify the network port to be used by the Thrift (HTTP) server.");

        po::variables_map vm;
        try {

            po::store(po::parse_command_line(argc, argv, desc), vm);

            if (vm.count("help")) {
                cout << "FMU-proxy" << endl
                     << desc << endl;
                return SUCCESS;
            }

            po::notify(vm);

        } catch (po::error& e) {
            std::cerr << "ERROR: " << e.what() << std::endl
                      << std::endl;
            std::cerr << desc << std::endl;
            return COMMANDLINE_ERROR;
        }


        vector<shared_ptr<fmi4cpp::fmi2::cs_fmu>> fmus;
        if (vm.count("fmu")) {
            const vector<string> fmu_paths = vm["fmu"].as<vector<string>>();
            for (const auto& fmu_path : fmu_paths) {
                fmus.push_back(std::move(fmi4cpp::fmi2::fmu(fmu_path).as_cs_fmu()));
            }
        }

        auto ports = unordered_map<string, unsigned int>();

        if (vm.count(THRIFT_TCP)) {
            ports[THRIFT_TCP] = vm[THRIFT_TCP].as<unsigned int>();
        }

        return run_application(fmus, ports);

    } catch (std::exception& e) {
        std::cerr << "Unhandled Exception reached the top of main: " << e.what() << ", application will now exit" << std::endl;
        return UNHANDLED_ERROR;
    }
}
