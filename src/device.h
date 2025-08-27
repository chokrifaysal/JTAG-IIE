#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

class Jtag;

struct FlashRegion {
    uint32_t addr;
    uint32_t size;
    uint32_t sector_size;
};

struct DeviceInfo {
    uint32_t idcode;
    std::string name;
    std::string vendor;
    uint32_t flash_size;
    uint32_t ram_size;
    std::vector<FlashRegion> flash_regions;
    bool has_fpu;
    bool has_dsp;
};

class DeviceDB {
public:
    static DeviceDB& instance();
    
    const DeviceInfo* find(uint32_t idcode) const;
    void add(const DeviceInfo& dev);
    
private:
    DeviceDB();
    
    std::vector<DeviceInfo> devices;
};

class Device {
public:
    Device(uint32_t id, Jtag* jtag);
    ~Device();
    
    bool init();
    const DeviceInfo* info() const { return info_; }
    
    bool halt();
    bool resume();
    bool reset();
    
    bool read_mem(uint32_t addr, uint8_t* buf, uint32_t len);
    bool write_mem(uint32_t addr, const uint8_t* buf, uint32_t len);
    
private:
    uint32_t id;
    const DeviceInfo* info_;
    Jtag* jtag;
    
    bool is_arm;
    uint32_t dap_base;
    
    bool ap_select(uint8_t ap, uint32_t addr);
    bool mem_ap_transfer(uint32_t addr, uint32_t* data, bool write);
};
