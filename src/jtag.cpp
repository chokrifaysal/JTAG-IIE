#include "jtag.h"
#include <cstring>

Jtag::Jtag(JtagAdapter* a) : adapter(a), state(0) {}

Jtag::~Jtag() {
    if (adapter)
        adapter->close();
}

bool Jtag::init() {
    if (!adapter->open())
        return false;
    
    reset_tap();
    return true;
}

void Jtag::reset() {
    reset_tap();
}

void Jtag::reset_tap() {
    // 5 TMS highs to get to Test-Logic-Reset
    for (int i = 0; i < 5; i++) {
        adapter->set_pin(JtagPin::TMS, 1);
        adapter->set_pin(JtagPin::TCK, 0);
        adapter->set_pin(JtagPin::TCK, 1);
    }
    
    // Idle
    adapter->set_pin(JtagPin::TMS, 0);
    adapter->set_pin(JtagPin::TCK, 0);
    adapter->set_pin(JtagPin::TCK, 1);
}

void Jtag::pulse_clock(int n) {
    for (int i = 0; i < n; i++) {
        adapter->set_pin(JtagPin::TCK, 0);
        adapter->set_pin(JtagPin::TCK, 1);
    }
}

void Jtag::shift_ir(const uint8_t* data, int len) {
    // Select-DR-Scan
    adapter->set_pin(JtagPin::TMS, 1);
    pulse_clock();
    
    // Select-IR-Scan
    adapter->set_pin(JtagPin::TMS, 1);
    pulse_clock();
    
    // Capture-IR
    adapter->set_pin(JtagPin::TMS, 0);
    pulse_clock();
    
    // Shift-IR
    adapter->set_pin(JtagPin::TMS, 0);
    pulse_clock();
    
    // Shift bits
    for (int i = 0; i < len; i++) {
        bool bit = (data[i/8] >> (i % 8)) & 1;
        adapter->set_pin(JtagPin::TDI, bit);
        
        if (i == len - 1)
            adapter->set_pin(JtagPin::TMS, 1);  // Exit1-IR
        
        pulse_clock();
    }
    
    // Update-IR
    adapter->set_pin(JtagPin::TMS, 1);
    pulse_clock();
    
    // Run-Test/Idle
    adapter->set_pin(JtagPin::TMS, 0);
    pulse_clock();
}

void Jtag::shift_dr(const uint8_t* data, int len, uint8_t* out) {
    // Select-DR-Scan
    adapter->set_pin(JtagPin::TMS, 1);
    pulse_clock();
    
    // Capture-DR
    adapter->set_pin(JtagPin::TMS, 0);
    pulse_clock();
    
    // Shift-DR
    adapter->set_pin(JtagPin::TMS, 0);
    pulse_clock();
    
    int bytes = (len + 7) / 8;
    std::vector<uint8_t> outbuf;
    if (out) outbuf.resize(bytes, 0);
    
    // Shift bits
    for (int i = 0; i < len; i++) {
        bool bit = 0;
        if (data) bit = (data[i/8] >> (i % 8)) & 1;
        
        adapter->set_pin(JtagPin::TDI, bit);
        adapter->set_pin(JtagPin::TCK, 0);
        
        bool tdo = adapter->get_pin(JtagPin::TDO);
        if (out) {
            if (tdo) outbuf[i/8] |= (1 << (i % 8));
        }
        
        if (i == len - 1)
            adapter->set_pin(JtagPin::TMS, 1);  // Exit1-DR
        
        adapter->set_pin(JtagPin::TCK, 1);
    }
    
    // Update-DR
    adapter->set_pin(JtagPin::TMS, 1);
    pulse_clock();
    
    // Run-Test/Idle
    adapter->set_pin(JtagPin::TMS, 0);
    pulse_clock();
    
    if (out) memcpy(out, outbuf.data(), bytes);
}

uint32_t Jtag::idcode() {
    // BYPASS instruction is all 1s
    uint8_t bypass = 0xff;
    shift_ir(&bypass, 8);
    
    // Shift out IDCODE
    uint32_t id = 0;
    shift_dr(nullptr, 32, (uint8_t*)&id);
    
    return id;
}
