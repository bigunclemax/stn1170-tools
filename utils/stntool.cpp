#include <cxxopts.hpp>
#include <CanDevice.h>
#include <iostream>

using namespace stnlib;
using std::cout;

int main(int argc, char *argv[])
{
    try {
        bool verbose = false;
        bool info = false;
        bool list = false;
        bool maximize = false;
        int wish_baud = 0;
        int connect_baud = 0;

        cxxopts::Options options("stntool", "tool for STN11xx devices configuration");
        options.add_options()
                ("l,list", "list serial ports", cxxopts::value<bool>(list))
                ("s,speed", "set STN11xx UART baud", cxxopts::value<int>(wish_baud))
                ("p,port","serial port", cxxopts::value<string>())
                ("b,baud", "specify STN11xx UART baud", cxxopts::value<int>(connect_baud))
                ("m,maximize","maximize STN11xx UART baud", cxxopts::value<bool>(maximize))
                ("i,info","info about STN11xx device", cxxopts::value<bool>(info))
                ("v,verbose","Verbose output", cxxopts::value<bool>(verbose))
                ("h,help","Print help");

        options.parse_positional({"port"});

        auto result = options.parse(argc, argv);

        if (result.arguments().empty() || result.count("help")) {
            cout << options.help() << std::endl;
            return 0;
        }

        if (list) {
            CanDevice::enumerate_ports();
            return 0;
        }

        if(!result.count("port")) {
            cout << "Please, specify serial port" << std::endl;
            return 0;
        }

        if (wish_baud) {
            auto serial = CanDevice(result["port"].as<string>(), connect_baud, verbose);
            if (serial.set_baudrate(wish_baud, true)) {
                cout << "Set device baud error" << std::endl;
            } else {
                cout << "Set device baud: " << wish_baud << std::endl;
            }

            return 0;
        }

        if (maximize) {
            auto serial = CanDevice(result["port"].as<string>(), connect_baud, verbose);
            auto maximized_baud = serial.maximize_baudrate();
            if (maximized_baud > 0) {
                cout << "Device baud maximized to: " << maximized_baud << std::endl;
            } else {
                cout << "Maximize device baud error" << std::endl;
            }

            return 0;
        }

        if (info) {
            auto serial = CanDevice(result["port"].as<string>(), connect_baud, verbose);
            cout << serial.get_info() << std::endl;

            return 0;
        }

        cout << options.help() << std::endl;

    } catch (const cxxopts::OptionException& e){
        cout << "error parsing options: " << e.what() << std::endl;
    }

    return 0;
}
