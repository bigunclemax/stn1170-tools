//
// Created by user on 12.11.2020.
//

#ifndef STNTOOL_SERIALPORT_H
#define STNTOOL_SERIALPORT_H

#include <string>
#include <memory>
#include "serial/serial.h"

using std::string;
using std::vector;

namespace stnlib {

struct ControllerSettings {
    string port_name;
    uint32_t baud;
    bool maximize;
};

enum CAN_PROTO {
    CAN_MS,
    CAN_HS
};

const uint32_t baud_arr[] = {
        9600,
        19200,
        38400,
        57600,
        115200,
        230400,
        460800,
        500000,
        576000,
        921600,
        1000000,
        1152000,
        1500000,
        2000000,
        2500000,
        3000000,
        3500000,
        4000000
};
const int baud_arr_sz = sizeof(baud_arr) / sizeof(baud_arr[0]);

class SerialPort {
public:

    inline static void hex2ascii(const uint8_t *bin, unsigned int binsz, char *result) {
        unsigned char hex_str[] = "0123456789ABCDEF";
        for (auto i = 0; i < binsz; ++i) {
            result[i * 2 + 0] = (char) hex_str[(bin[i] >> 4u) & 0x0Fu];
            result[i * 2 + 1] = (char) hex_str[(bin[i]) & 0x0Fu];
        }
    };

    SerialPort(string port, uint32_t baudrate = 0);

//    SerialHandler(sControllerSettings init_settings, QObject *parent = nullptr);
//    ~SerialHandler() override;
//    int transaction(int waitTimeout, const std::string &request);

    int check_baudrate(uint32_t baud);

    int detect_baudrate();

    int set_baudrate(uint32_t baud, bool save = false);

    int maximize_baudrate();

    std::string get_info();

    static void enumerate_ports();
private:

    static constexpr int transaction_timeout = 3000;
    static constexpr int check_baudrate_timeout = 1000; ///< max els response timeout in ms
    static constexpr int set_baudrate_timeout = 1000;   ///< baud rate switch timeout in ms

    std::pair<int, std::string> serial_transaction(const std::string &req);


    std::unique_ptr<serial::Serial> m_serial;
    string m_portName;
};
}

#endif //STNTOOL_SERIALPORT_H
