
#include <fmuproxy/thrift/server/thrift_fmu_server.hpp>

#include <boost/program_options.hpp>

#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>

using fmuproxy::thrift::server::thrift_fmu_server;

using namespace std;

namespace
{

const int SUCCESS = 0;
const int COMMANDLINE_ERROR = 1;
const int UNHANDLED_ERROR = 2;

void wait_for_input()
{
    do {
        cout << '\n'
             << "Press a key to continue...\n";
    } while (cin.get() != '\n');
    cout << "Done." << endl;
}

int run_application(int port)
{
    auto thrift_socket_server = make_unique<thrift_fmu_server>(port);
    thrift_socket_server->start();

    wait_for_input();

    thrift_socket_server->stop();

    return 0;
}

} // namespace

int printHelp( boost::program_options::options_description& desc)
{
    cout << "FMU-proxy" << endl
         << desc << endl;
    return SUCCESS;
}

int main(int argc, char** argv)
{

    namespace po = boost::program_options;

    po::options_description desc("Options");
    desc.add_options()("help,h", "Print this help message and quits.");
    desc.add_options()("port", po::value<int>(), "Specify the network port to be used.");

    if (argc == 1) {
        return printHelp(desc);
    }

    try {

        po::variables_map vm;
        try {

            po::store(po::parse_command_line(argc, argv, desc), vm);

            if (vm.count("help")) {
                return printHelp(desc);
            }

            po::notify(vm);

        } catch (po::error& e) {
            std::cerr << "ERROR: " << e.what() << std::endl
                      << std::endl;
            std::cerr << desc << std::endl;
            return COMMANDLINE_ERROR;
        }

        auto port = vm["port"].as<int>();

        return run_application(port);

    } catch (std::exception& e) {
        std::cerr << "Unhandled Exception reached the top of main: " << e.what() << ", application will now exit" << std::endl;
        return UNHANDLED_ERROR;
    }

}
