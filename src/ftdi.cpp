#include "jtag.h"
#ifdef _WIN32
#include <ftd2xx.h>
#else
#include <libftdi1/ftdi.h>
#endif
#include <iostream>
#include <unistd.h>

class FtdiAdapter : public JtagAdapter {
public:
    FtdiAdapter(uint32_t vid = 0x0403, uint32_t pid = 0x6010) 
        : vid(vid), pid(pid), ftdi(nullptr) {}
    
    ~FtdiAdapter() {
        if (ftdi) close();
    }
    
    bool open() override {
#ifndef _WIN32
        ftdi = ftdi_new();
        if (!ftdi) {
            std::cerr << "Failed to create FTDI context\n";
            return false;
        }
        
        if (ftdi_usb_open(ftdi, vid, pid) < 0) {
            std::cerr << "Failed to open FTDI device\n";
            ftdi_free(ftdi);
            ftdi = nullptr;
            return false;
        }
        
        // Configure for bitbang mode
        ftdi_set_bitmode(ftdi, 0x0f, BITMODE_BITBANG);
        ftdi_set_baudrate(ftdi, 115200);
        
        // Initial state - all outputs low
        state = 0;
        ftdi_write_data(ftdi, &state, 1);
        
        return true;
#else
        std::cerr << "Linux FTDI adapter not available on Windows\n";
        return false;
#endif
    }
    
    void close() override {
#ifndef _WIN32
        if (ftdi) {
            ftdi_usb_close(ftdi);
            ftdi_free(ftdi);
            ftdi = nullptr;
        }
#endif
    }
    
    void set_pin(JtagPin::Type pin, bool value) override {
#ifndef _WIN32
        uint8_t mask = 0;
        switch (pin) {
            case JtagPin::TCK: mask = 0x01; break;
            case JtagPin::TMS: mask = 0x02; break;
            case JtagPin::TDI: mask = 0x04; break;
            case JtagPin::TRST: mask = 0x08; break;
            default: return;
        }
        
        if (value) state |= mask;
        else state &= ~mask;
        
        ftdi_write_data(ftdi, &state, 1);
#endif
    }
    
    bool get_pin(JtagPin::Type pin) override {
#ifndef _WIN32
        if (pin != JtagPin::TDO) return false;
        
        uint8_t val;
        ftdi_read_pins(ftdi, &val);
        return val & 0x10;  // TDO on bit 4
#else
        return false;
#endif
    }
    
    void delay(unsigned us) override {
#ifndef _WIN32
        usleep(us);
#else
        Sleep(us / 1000);
#endif
    }
    
private:
#ifndef _WIN32
    ftdi_context* ftdi;
#endif
    uint32_t vid, pid;
    uint8_t state;
};
