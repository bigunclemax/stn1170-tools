#include <cxxopts.hpp>
#include <SerialPort.h>
#include <iostream>

using namespace stnlib;
using std::cout;

int main(int argc, char *argv[])
{
    try {
        bool verbose = false;
        bool info = false;
        bool detect = false;
        bool list = false;
        bool maximize = false;
        int wish_baud = 0;

        cxxopts::Options options("stntool", "tool for STN11xx devices configuration");
        options.add_options()
                ("l,list", "list serial ports", cxxopts::value<bool>(list))
                ("s,speed", "set serial port baud", cxxopts::value<int>(wish_baud))
                ("d,detect", "autodetect serial port baud", cxxopts::value<bool>(detect))
                ("p,port","serial port", cxxopts::value<string>())
//                ("b,baud", "force specify serial port baud")
                ("m,maximize","maximize STN11xx device baudrate", cxxopts::value<bool>(maximize))
                ("i,info","info about STN11xx device", cxxopts::value<bool>(info))
                ("v,verbose","Verbose output")
                ("h,help","Print help");

        options.parse_positional({"port"});

        auto result = options.parse(argc, argv);

        if (result.arguments().empty() || result.count("help")) {
            cout << options.help() << std::endl;
            return 0;
        }

        if (list) {
            SerialPort::enumerate_ports();
            return 0;
        }

        if(!result.count("port")) {
            cout << "Please, specify serial port" << std::endl;
            return 0;
        }

        if (detect) {
            auto serial = SerialPort(result["port"].as<string>());
            auto serial_baud = serial.detect_baudrate();
            if (serial_baud > 0) {
                cout << "Device baud is: " << serial_baud << std::endl;
            } else {
                cout << "Detect device baud error" << std::endl;
            }
            return 0;
        }

        if (wish_baud) {
            auto serial = SerialPort(result["port"].as<string>());
            auto serial_baud = serial.detect_baudrate();
            if (serial.set_baudrate(wish_baud) > 0) {
                cout << "Set device baud: " << wish_baud << std::endl;
            } else {
                cout << "Set device baud error" << std::endl;
            }

            return 0;
        }

        if (maximize) {
            auto serial = SerialPort(result["port"].as<string>());
            auto serial_baud = serial.detect_baudrate();
            if (serial_baud > 0) {
                auto maximized_baud = serial.maximize_baudrate();
                if (maximized_baud > 0) {
                    cout << "Device baud maximized to: " << maximized_baud << std::endl;
                } else {
                    cout << "Maximize device baud error" << std::endl;
                }
            } else {
                cout << "Detect device baud error" << std::endl;
            }

            return 0;
        }

        if (info) {
            auto serial = SerialPort(result["port"].as<string>());
            if (serial.detect_baudrate() > 0)
                cout << "Device: " << serial.get_info() << std::endl;

            return 0;
        }

        cout << options.help() << std::endl;

    } catch (const cxxopts::OptionException& e){
        cout << "error parsing options: " << e.what() << std::endl;
    }

    return 0;
}
