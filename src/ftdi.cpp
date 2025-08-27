#include "jtag.h"
#include <libftdi1/ftdi.h>
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
    }
    
    void close() override {
        if (ftdi) {
            ftdi_usb_close(ftdi);
            ftdi_free(ftdi);
            ftdi = nullptr;
        }
    }
    
    void set_pin(JtagPin::Type pin, bool value) override {
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
    }
    
    bool get_pin(JtagPin::Type pin) override {
        if (pin != JtagPin::TDO) return false;
        
        uint8_t val;
        ftdi_read_pins(ftdi, &val);
        return val & 0x10;  // TDO on bit 4
    }
    
    void delay(unsigned us) override {
        usleep(us);
    }
    
private:
    uint32_t vid, pid;
    ftdi_context* ftdi;
    uint8_t state;
};
