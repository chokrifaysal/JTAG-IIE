#pragma once

#include <string>
#include <map>

struct Config {
    bool verbose = false;
    bool force = false;
    uint32_t vid = 0x0403;
    uint32_t pid = 0x6010;
    uint32_t clock_speed = 1000;  // kHz
    std::string adapter_type = "ftdi";
    std::string config_file;
    
    static Config load(const std::string& file);
    static Config from_args(int argc, char* argv[]);
    void save(const std::string& file) const;
};

class Args {
public:
    static void parse(int argc, char* argv[]);
    static bool has(const std::string& key);
    static std::string get(const std::string& key, const std::string& def = "");
    static uint32_t get_uint(const std::string& key, uint32_t def = 0);
};
