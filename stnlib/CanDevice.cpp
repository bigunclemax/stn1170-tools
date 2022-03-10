#include <iostream>
#include <thread>
#include "CanDevice.h"

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

CanDevice::CanDevice(string port, uint32_t baudrate, bool verbose): m_portName(std::move(port)), m_verbose(verbose)
{
    try {
        m_serial = std::make_unique<serial::Serial>(m_portName,
                                                    9600,
                                                    serial::Timeout::simpleTimeout(transaction_timeout));
    } catch (serial::IOException &e) {
        throw std::runtime_error(e.what());
    }

    if (baudrate) {
        if(check_baudrate(baudrate))
            throw std::runtime_error("can't connect to adapter with baud: " + std::to_string(baudrate));
    } else {
        if(detect_baudrate() < 0) {
            throw std::runtime_error("can't auto detect adapter baud");
        }
    }

    serial_transaction("ATE0\r");

    auto r = serial_transaction("STI\r");
    if(r.first) {
        throw std::runtime_error("execute STI command error");
    }
    std::getline(std::stringstream(r.second), m_sti_str, '\r');

    if (m_sti_str.find("STN") == std::string::npos) {
        m_isElm327 = true;
    }
}

void CanDevice::enumerate_ports()
{
    vector<serial::PortInfo> devices_found = serial::list_ports();

    auto iter = devices_found.begin();

    while( iter != devices_found.end() )
    {
        serial::PortInfo device = *iter++;

        printf( "(%s, %s, %s)\n", device.port.c_str(), device.description.c_str(),
                device.hardware_id.c_str() );
    }
}

int CanDevice::check_baudrate(uint32_t baud) {

    if(m_verbose) std::cerr << "Check baud: " << baud << std::endl;

    m_serial->setBaudrate(baud);

    auto tmp_timeout = m_serial->getTimeout();
    auto t = serial::Timeout::simpleTimeout(check_baudrate_timeout);
    m_serial->setTimeout(t);

    /* clear */
    m_serial->flush();
    m_serial->write("?\r");
    m_serial->readlines();

    auto r = serial_transaction("ATWS\r");
    if(r.first) {
        return -1;
    }

    m_serial->setTimeout(tmp_timeout);

    if (r.second.find("ELM327 v1.") == std::string::npos) {
        return -1;
    }

    if(m_verbose) std::cerr << "Baud " << baud << " supported" << std::endl;

    return 0;
}

int CanDevice::detect_baudrate() {

    for(unsigned int i : baud_arr) {
        if(!check_baudrate(i))
            return (int)i;
    }

    return -1;
}

int CanDevice::set_baudrate(uint32_t baud, bool save) {

    auto curr_baud = m_serial->getBaudrate();

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
    if(m_verbose) print_buffer(bytes_read, reinterpret_cast<const unsigned char *const>(read_line.c_str()), false);
    if(read_line.find("OK") == std::string::npos) {
        m_serial->readline(read_line, 65536, ">");
        return -1;
    }

    /* Host: switch to new baud rate */
    m_serial->setBaudrate(baud);

    read_line="";
    bytes_read = m_serial->readline(read_line, 65536, "\r");
    if(m_verbose) print_buffer(read_line.size(), reinterpret_cast<const unsigned char *const>(read_line.c_str()), false);

    /* Host: received a valid STI string? */
    if(read_line.find(m_sti_str) == std::string::npos) {
        goto cleanup;
    }

    m_serial->write("\r");

    read_line="";
    bytes_read = m_serial->readline(read_line, 65536, ">");
    if(m_verbose) print_buffer(read_line.size(), reinterpret_cast<const unsigned char *const>(read_line.c_str()), false);

    if(read_line.find("OK") == std::string::npos) {
        goto cleanup;
    }

    if(save)
        serial_transaction("STWBR\r");

    return 0;

cleanup:
    std::this_thread::sleep_for(std::chrono::milliseconds(set_baudrate_timeout));
    m_serial->setBaudrate(curr_baud);
    m_serial->write("?\r");
    m_serial->readline(read_line, 65536);

    return -1;
}

int CanDevice::maximize_baudrate() {

    /* set baudrate timeout in ms */
    std::string str = "STBRT " + std::to_string(set_baudrate_timeout) + "\r";
    if(serial_transaction(str).first) {
        return -1;
    }

    auto baud = m_serial->getBaudrate();
    auto it = baud_arr.lower_bound(baud);
    if(it == baud_arr.end())
        return baud; //already maximized

    for(; it != baud_arr.end(); ++it) {
        if(set_baudrate(*it, false)) { //if ok, goes next
            if(m_verbose) std::cerr << "Baud rate " << *it << " not supported" << std::endl;
            continue;
        }
        if(m_verbose) std::cerr << "Baud rate " << *it << " supported" << std::endl;
        baud = *it;
    }

    serial_transaction("STWBR\r");

    return baud;
}

std::pair<int, std::string>
CanDevice::serial_transaction(const std::string &req) {

    if(m_verbose) print_buffer(req.size(), reinterpret_cast<const unsigned char *const>(req.data()), true);

    size_t bytes_wrote = m_serial->write(req);

    std::string read_line;
    auto bytes_read = m_serial->readline(read_line, 65536, ">");

    if(m_verbose) print_buffer(read_line.size(), reinterpret_cast<const unsigned char *const>(read_line.c_str()), false);

    return {0, read_line};
}

std::string CanDevice::get_info() {

    auto f = [](const string& s){ return s.substr(0, s.find('\r'));};

    std::stringstream ss;
    ss << "Baud:\t"  << m_serial->getBaudrate() << std::endl;
    ss << "ATI:\t"   << f(serial_transaction("ATI\r").second) << std::endl;
    ss << "STDI:\t"  << f(serial_transaction("STDI\r").second) << std::endl;
    ss << "STIX:\t"  << f(serial_transaction("STIX\r").second) << std::endl;
    ss << "STMFR:\t" << f(serial_transaction("STMFR\r").second) << std::endl;
    ss << "STSN:\t"  << f(serial_transaction("STSN\r").second) << std::endl;

    return ss.str();
}

int CanDevice::set_protocol(CAN_PROTO protocol) {

    if (CAN_MS == protocol) {
        if(m_isElm327) {
            serial_transaction("ATSPB\r");   // User1 CAN (11* bit ID, 125* kbaud
        } else {
            serial_transaction("STP53\r");   //ISO 15765, 11-bit Tx, 125kbps, DLC=8
        }
    } else if (CAN_HS == protocol) {
        if(m_isElm327) {
            serial_transaction("ATSP6\r");   //ISO 15765, 11-bit Tx, 500kbps, DLC=8
            return -1;
        } else {
            serial_transaction("STP33\r");   //ISO 15765, 11-bit Tx, 500kbps, DLC=8
        }
    } else if (CAN_MM == protocol) {
        if(m_isElm327) {
            /* MM-CAN not supported by vanilla ELM327 */
            return -1;
        } else {
            //TODO: serial_transaction("STP33\r");   //ISO 15765, 11-bit Tx, 500kbps, DLC=8
            return -1;
        }
    }

    return 0;
}