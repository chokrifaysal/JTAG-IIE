#include "device.h"
#include "jtag.h"
#include <iostream>
#include <cstring>

DeviceDB& DeviceDB::instance() {
    static DeviceDB db;
    return db;
}

DeviceDB::DeviceDB() {
    // STM32F103C8 (Blue Pill)
    devices.push_back({
        .idcode = 0x1BA01477,
        .name = "STM32F103C8",
        .vendor = "STMicroelectronics",
        .flash_size = 64 * 1024,
        .ram_size = 20 * 1024,
        .flash_regions = {{0x08000000, 64 * 1024, 1024}},
        .has_fpu = false,
        .has_dsp = false
    });
    
    // STM32F407VG (Discovery)
    devices.push_back({
        .idcode = 0x2BA01477,
        .name = "STM32F407VG",
        .vendor = "STMicroelectronics",
        .flash_size = 1024 * 1024,
        .ram_size = 192 * 1024,
        .flash_regions = {{0x08000000, 1024 * 1024, 16 * 1024}},
        .has_fpu = true,
        .has_dsp = true
    });
    
    // GD32F103C8 (Clone)
    devices.push_back({
        .idcode = 0x1BA01477,  // Same as STM32
        .name = "GD32F103C8",
        .vendor = "GigaDevice",
        .flash_size = 64 * 1024,
        .ram_size = 20 * 1024,
        .flash_regions = {{0x08000000, 64 * 1024, 1024}},
        .has_fpu = false,
        .has_dsp = false
    });
    
    // LPC1768
    devices.push_back({
        .idcode = 0x4BA00477,
        .name = "LPC1768",
        .vendor = "NXP",
        .flash_size = 512 * 1024,
        .ram_size = 64 * 1024,
        .flash_regions = {{0x00000000, 512 * 1024, 4096}},
        .has_fpu = false,
        .has_dsp = false
    });
}

const DeviceInfo* DeviceDB::find(uint32_t idcode) const {
    uint32_t part = idcode & 0x0fffffff;
    
    for (const auto& dev : devices) {
        if ((dev.idcode & 0x0fffffff) == part) {
            return &dev;
        }
    }
    
    return nullptr;
}

void DeviceDB::add(const DeviceInfo& dev) {
    devices.push_back(dev);
}

Device::Device(uint32_t id, Jtag* j) : id(id), jtag(j), info_(nullptr) {
    info_ = DeviceDB::instance().find(id);
    is_arm = (id & 0xf000) == 0x4000 || (id & 0xf000) == 0x3000 || (id & 0xf000) == 0x1000;
    dap_base = 0xE00FF000;  // Default for ARM
}

Device::~Device() {}

bool Device::init() {
    if (!info_) {
        std::cerr << "Unknown device ID: 0x" << std::hex << id << std::dec << "\n";
        return false;
    }
    
    if (!is_arm) {
        std::cerr << "Only ARM devices supported for now\n";
        return false;
    }
    
    std::cout << "Found " << info_->vendor << " " << info_->name << "\n";
    return true;
}

bool Device::halt() {
    // Write DHCSR to halt
    uint32_t dhcsr = 0xA05F0003;  // DBGKEY | C_HALT | C_DEBUGEN
    return write_mem(0xE000EDF0, (uint8_t*)&dhcsr, 4);
}

bool Device::resume() {
    // Clear C_HALT in DHCSR
    uint32_t dhcsr = 0xA05F0001;  // DBGKEY | C_DEBUGEN
    return write_mem(0xE000EDF0, (uint8_t*)&dhcsr, 4);
}

bool Device::reset() {
    // AIRCR reset
    uint32_t aircr = 0x05FA0004;  // VECTRESET
    return write_mem(0xE000ED0C, (uint8_t*)&aircr, 4);
}

bool Device::ap_select(uint8_t ap, uint32_t addr) {
    uint32_t select = (ap << 24) | (addr & 0xf0);
    uint8_t buf[4];
    memcpy(buf, &select, 4);
    jtag->shift_ir((uint8_t*)"\x8a", 5);  // DPACC
    jtag->shift_dr(buf, 35, buf);  // 35-bit transfer
    return true;
}

bool Device::mem_ap_transfer(uint32_t addr, uint32_t* data, bool write) {
    uint8_t ir = write ? 0x8b : 0x8b;  // APACC
    uint8_t buf[4];
    
    if (write) {
        memcpy(buf, data, 4);
        jtag->shift_ir(&ir, 3);
        jtag->shift_dr(buf, 35, nullptr);
    } else {
        jtag->shift_ir(&ir, 3);
        jtag->shift_dr(nullptr, 35, buf);
        memcpy(data, buf, 4);
    }
    
    return true;
}

bool Device::read_mem(uint32_t addr, uint8_t* buf, uint32_t len) {
    if (len % 4) return false;
    
    uint32_t* ptr = (uint32_t*)buf;
    uint32_t words = len / 4;
    
    for (uint32_t i = 0; i < words; i++) {
        if (!mem_ap_transfer(addr + i*4, &ptr[i], false))
            return false;
    }
    
    return true;
}

bool Device::write_mem(uint32_t addr, const uint8_t* buf, uint32_t len) {
    if (len % 4) return false;
    
    const uint32_t* ptr = (const uint32_t*)buf;
    uint32_t words = len / 4;
    
    for (uint32_t i = 0; i < words; i++) {
        if (!mem_ap_transfer(addr + i*4, (uint32_t*)&ptr[i], true))
            return false;
    }
    
    return true;
}
