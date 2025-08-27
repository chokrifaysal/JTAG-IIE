#pragma once

#include <cstdint>
#include <vector>

struct JtagPin {
    enum Type {
        TCK = 0,
        TMS,
        TDI,
        TDO,
        TRST,
        SRST
    };
};

class JtagAdapter {
public:
    virtual ~JtagAdapter() = default;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual void set_pin(JtagPin::Type pin, bool value) = 0;
    virtual bool get_pin(JtagPin::Type pin) = 0;
    virtual void delay(unsigned us) = 0;
};

class Jtag {
public:
    Jtag(JtagAdapter* adapter);
    ~Jtag();
    
    bool init();
    void reset();
    void shift_ir(const uint8_t* data, int len);
    void shift_dr(const uint8_t* data, int len, uint8_t* out = nullptr);
    uint32_t idcode();
    
private:
    void pulse_clock(int n = 1);
    void reset_tap();
    
    JtagAdapter* adapter;
    int state;
};
