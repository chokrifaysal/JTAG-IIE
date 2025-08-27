#include "flash.h"
#include "device.h"
#include "jtag.h"
#include <iostream>
#include <cstring>

Flash::Flash(Device* d, Jtag* j) : dev(d), jtag(j), driver(nullptr) {}

Flash::~Flash() {
    if (driver) delete driver;
}

bool Flash::detect() {
    const DeviceInfo* info = dev->info();
    if (!info) return false;
    
    if (info->name.find("STM32F103") != std::string::npos ||
        info->name.find("STM32F101") != std::string::npos ||
        info->name.find("STM32F102") != std::string::npos ||
        info->name.find("STM32F105") != std::string::npos ||
        info->name.find("STM32F107") != std::string::npos) {
        driver = new STM32F1Flash(dev, jtag);
        return true;
    }
    
    return false;
}

bool Flash::load_driver() {
    if (!driver) return false;
    return driver->init();
}

bool Flash::erase(uint32_t addr, uint32_t len) {
    if (!driver) return false;
    
    uint32_t end = addr + len;
    
    for (uint32_t sector = addr; sector < end; sector += driver->sector_size(sector)) {
        if (!driver->erase_sector(sector)) {
            std::cerr << "Erase failed at 0x" << std::hex << sector << std::dec << "\n";
            return false;
        }
    }
    
    return true;
}

bool Flash::program(uint32_t addr, const uint8_t* data, uint32_t len) {
    if (!driver) return false;
    
    uint32_t page_size = 1024;  // STM32F1 has 1K pages
    
    for (uint32_t offset = 0; offset < len; offset += page_size) {
        uint32_t chunk = (len - offset) < page_size ? (len - offset) : page_size;
        
        if (!driver->program_page(addr + offset, data + offset, chunk)) {
            std::cerr << "Program failed at 0x" << std::hex << (addr + offset) << std::dec << "\n";
            return false;
        }
        
        if (!driver->verify(addr + offset, data + offset, chunk)) {
            std::cerr << "Verify failed at 0x" << std::hex << (addr + offset) << std::dec << "\n";
            return false;
        }
    }
    
    return true;
}

bool Flash::read(uint32_t addr, uint8_t* data, uint32_t len) {
    return dev->read_mem(addr, data, len);
}

STM32F1Flash::STM32F1Flash(Device* d, Jtag* j) : dev(d), jtag(j) {}

bool STM32F1Flash::init() {
    return unlock();
}

FlashStatus STM32F1Flash::status() {
    uint32_t sr = 0;
    if (!dev->read_mem(0x4002200C, (uint8_t*)&sr, 4)) {
        return {true, true, false};
    }
    
    return {
        .busy = (sr & (1 << 0)) != 0,
        .error = (sr & ((1 << 2) | (1 << 4))) != 0,
        .eop = (sr & (1 << 5)) != 0
    };
}

bool STM32F1Flash::wait_ready() {
    for (int i = 0; i < 10000; i++) {
        FlashStatus st = status();
        if (!st.busy) return !st.error;
    }
    return false;
}

bool STM32F1Flash::unlock() {
    uint32_t key1 = 0x45670123;
    uint32_t key2 = 0xCDEF89AB;
    
    if (!dev->write_mem(0x40022004, (uint8_t*)&key1, 4)) return false;
    if (!dev->write_mem(0x40022004, (uint8_t*)&key2, 4)) return false;
    
    return true;
}

bool STM32F1Flash::lock() {
    uint32_t cr = 0x00000080;
    return dev->write_mem(0x40022010, (uint8_t*)&cr, 4);
}

bool STM32F1Flash::erase_sector(uint32_t addr) {
    if (!wait_ready()) return false;
    
    // Set PER bit and page address
    uint32_t ar = addr;
    if (!dev->write_mem(0x40022014, (uint8_t*)&ar, 4)) return false;
    
    uint32_t cr = 0x00000002;  // PER
    if (!dev->write_mem(0x40022010, (uint8_t*)&cr, 4)) return false;
    
    cr = 0x00000042;  // PER + STRT
    if (!dev->write_mem(0x40022010, (uint8_t*)&cr, 4)) return false;
    
    if (!wait_ready()) return false;
    
    // Clear PER
    cr = 0x00000000;
    return dev->write_mem(0x40022010, (uint8_t*)&cr, 4);
}

bool STM32F1Flash::program_page(uint32_t addr, const uint8_t* data, uint32_t len) {
    if (!wait_ready()) return false;
    
    // Set PG bit
    uint32_t cr = 0x00000001;  // PG
    if (!dev->write_mem(0x40022010, (uint8_t*)&cr, 4)) return false;
    
    // Program half-words (16-bit)
    for (uint32_t i = 0; i < len; i += 2) {
        uint16_t val = *(uint16_t*)(data + i);
        if (!dev->write_mem(addr + i, (uint8_t*)&val, 2)) return false;
        
        if (!wait_ready()) return false;
    }
    
    // Clear PG
    cr = 0x00000000;
    return dev->write_mem(0x40022010, (uint8_t*)&cr, 4);
}

bool STM32F1Flash::verify(uint32_t addr, const uint8_t* data, uint32_t len) {
    std::vector<uint8_t> readback(len);
    if (!dev->read_mem(addr, readback.data(), len)) return false;
    
    return memcmp(data, readback.data(), len) == 0;
}

uint32_t STM32F1Flash::sector_size(uint32_t addr) {
    (void)addr;  // All sectors same size on F1
    return 1024;  // 1K sectors
}
