#pragma once

#include <cstdint>
#include <vector>
#include <string>

class Device;
class Jtag;

struct FlashStatus {
    bool busy;
    bool error;
    bool eop;
};

class FlashDriver {
public:
    virtual ~FlashDriver() = default;
    virtual bool init() = 0;
    virtual FlashStatus status() = 0;
    virtual bool erase_sector(uint32_t addr) = 0;
    virtual bool program_page(uint32_t addr, const uint8_t* data, uint32_t len) = 0;
    virtual bool verify(uint32_t addr, const uint8_t* data, uint32_t len) = 0;
    virtual uint32_t sector_size(uint32_t addr) = 0;
};

class Flash {
public:
    Flash(Device* dev, Jtag* jtag);
    ~Flash();
    
    bool detect();
    bool load_driver();
    bool erase(uint32_t addr, uint32_t len);
    bool program(uint32_t addr, const uint8_t* data, uint32_t len);
    bool read(uint32_t addr, uint8_t* data, uint32_t len);
    
private:
    Device* dev;
    Jtag* jtag;
    FlashDriver* driver;
};

class STM32F1Flash : public FlashDriver {
public:
    STM32F1Flash(Device* dev, Jtag* jtag);
    bool init() override;
    FlashStatus status() override;
    bool erase_sector(uint32_t addr) override;
    bool program_page(uint32_t addr, const uint8_t* data, uint32_t len) override;
    bool verify(uint32_t addr, const uint8_t* data, uint32_t len) override;
    uint32_t sector_size(uint32_t addr) override;
    
private:
    bool wait_ready();
    bool unlock();
    bool lock();
    
    Device* dev;
    Jtag* jtag;
};
