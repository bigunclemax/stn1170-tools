#include <iostream>
#include <thread>
#include "SerialPort.h"

using namespace stnlib;

using std::cout;

static inline void print_buffer(int rdlen, const unsigned char *const buf, int isWrite) {

#ifdef DEBUG
    std::vector<uint8_t> _str(rdlen+1);
    _str[rdlen] = 0;

    fprintf(stderr,"%s %d:", isWrite? "W" : "R", rdlen);
    /* first display as hex numbers then ASCII */
    for (int i =0; i < rdlen; i++) {
#ifdef DEBUG_PRINT_HEX
        fprintf(stderr," 0x%x", buf[i]);
#endif
        if (buf[i] < ' ')
            _str[i] = '.';   /* replace any control chars */
        else
            _str[i] = buf[i];
    }
    fprintf(stderr,"\n    \"%s\"\n\n", _str.data());
#endif
}

SerialPort::SerialPort(string port, uint32_t baudrate): m_portName(std::move(port)) {

    try {
        m_serial = std::make_unique<serial::Serial>(m_portName, baudrate, serial::Timeout::simpleTimeout(3000));
    } catch (serial::IOException &e) {
        throw std::runtime_error(e.what());
    }
}

void SerialPort::enumerate_ports()
{
    vector<serial::PortInfo> devices_found = serial::list_ports();

    vector<serial::PortInfo>::iterator iter = devices_found.begin();

    while( iter != devices_found.end() )
    {
        serial::PortInfo device = *iter++;

        printf( "(%s, %s, %s)\n", device.port.c_str(), device.description.c_str(),
                device.hardware_id.c_str() );
    }
}

int SerialPort::check_baudrate(uint32_t baud) {

    std::cerr << "Check baud: " << baud << std::endl;

    m_serial->setBaudrate(baud);

    /* clear */
    if(serial_transaction("?\r", check_baudrate_timeout).first) {
        return -1;
    }

    auto r = serial_transaction("ATWS\r", check_baudrate_timeout);
    if(r.first) {
        return -1;
    }

    if (r.second.find("ELM327 v1.3a") == std::string::npos) {
        return -1;
    }

    std::cerr << "Baud " << baud << " supported" << std::endl;

    return 0;
}

int SerialPort::detect_baudrate() {

    for(unsigned int i : baud_arr) {
        if(!check_baudrate(i))
            return (int)i;
    }

    return -1;
}

int SerialPort::set_baudrate(uint32_t baud, bool save) {

    auto curr_baud = m_serial->getBaudrate();

    serial_transaction("ATE0\r", 0);

    const unsigned io_buff_max_len = 1024; //alloc 1kb buffer
    char io_buff[io_buff_max_len];

    int stbr_str_sz = snprintf(io_buff, io_buff_max_len, "STBR %d\r", baud);
    if(stbr_str_sz < 0) {
        return -1;
    }

    /* Host sends STBR */
    std::string read_line;
    m_serial->write(io_buff);

    read_line="";
    auto bytes_read = m_serial->readline(read_line, 65536, "\r");
    print_buffer(bytes_read, reinterpret_cast<const unsigned char *const>(read_line.c_str()), false);
    if(read_line.find("OK") == std::string::npos) {
        std::cerr << "@DBG@ " << __PRETTY_FUNCTION__ << " [" << __LINE__ << "]" << std::endl;
        return -1;
    }

    /* Host: switch to new baud rate */
    m_serial->setBaudrate(baud);

    read_line="";
    bytes_read = m_serial->readline(read_line, 65536, "\r");
    print_buffer(read_line.size(), reinterpret_cast<const unsigned char *const>(read_line.c_str()), false);

    /* Host: received a valid STI string? */
    if(read_line.find("STN1170 v3.3.1") == std::string::npos) {
        std::cerr << "@DBG@ " << __PRETTY_FUNCTION__ << " [" << __LINE__ << "]" << std::endl;
        goto cleanup;
    }

    m_serial->write("\r");

    read_line="";
    bytes_read = m_serial->readline(read_line, 65536, ">");
    print_buffer(read_line.size(), reinterpret_cast<const unsigned char *const>(read_line.c_str()), false);

    if(read_line.find("OK") == std::string::npos) {
        std::cerr << "@DBG@ " << __PRETTY_FUNCTION__ << " [" << __LINE__ << "]" << std::endl;
        goto cleanup;
    }

    std::cerr << "@DBG@ " << __PRETTY_FUNCTION__ << " [" << __LINE__ << "]" << std::endl;
    std::cerr << "@DBG@ curr_baud " << m_serial->getBaudrate() << std::endl;

    if(save)
        serial_transaction("STWBR\r", 0);

    return 0;

cleanup:
    //TODO: clear read buffer
    std::this_thread::sleep_for(std::chrono::milliseconds(set_baudrate_timeout));
    m_serial->setBaudrate(curr_baud);
    serial_transaction("?\r", 0);

    return -1;
}

int SerialPort::maximize_baudrate() {

    /* set baudrate timeout in ms */
    std::string str = "STBRT " + std::to_string(set_baudrate_timeout) + "\r";
    if(serial_transaction(str, 0).first) {
        return -1;
    }

    //TODO: find pos in sorted arr
    auto baud = m_serial->getBaudrate();
    int i=0;
    while (baud != baud_arr[i]) {
        if(++i > baud_arr_sz) {
            return baud; //already maximized
        }
    }

    for(int j = i + 1; j < baud_arr_sz; ++j) {
        if(set_baudrate(baud_arr[j], false)) { //if ok, goes next
            std::cerr << "Baud rate " << baud_arr[j] << " not supported" << std::endl;
            continue;
        }
        std::cerr << "Baud rate " << baud_arr[j] << " supported" << std::endl;
        baud = baud_arr[j];
    }

    return baud;
}

std::pair<int, std::string>
SerialPort::serial_transaction(const std::string &req, int timeout) {

    print_buffer(req.size(), reinterpret_cast<const unsigned char *const>(req.data()), true);

    size_t bytes_wrote = m_serial->write(req);

    std::string read_line;
    auto bytes_read = m_serial->readline(read_line, 65536, ">");

    print_buffer(read_line.size(), reinterpret_cast<const unsigned char *const>(read_line.c_str()), false);

    return {0, read_line};
}

std::string SerialPort::get_info() {

    auto res = serial_transaction("STI\r", 0);
    return res.second;
}
