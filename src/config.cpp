#include "config.h"
#include <fstream>
#include <iostream>
#include <algorithm>

Config Config::load(const std::string& file) {
    Config cfg;
    std::ifstream f(file);
    if (!f) return cfg;
    
    // Simple key=value parser
    std::string line;
    while (std::getline(f, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        
        if (key == "verbose") cfg.verbose = (val == "true");
        else if (key == "force") cfg.force = (val == "true");
        else if (key == "vid") cfg.vid = std::stoul(val, 0, 0);
        else if (key == "pid") cfg.pid = std::stoul(val, 0, 0);
        else if (key == "clock") cfg.clock_speed = std::stoi(val);
        else if (key == "adapter") cfg.adapter_type = val;
    }
    
    return cfg;
}

void Config::save(const std::string& file) const {
    std::ofstream f(file);
    f << "verbose=" << (verbose ? "true" : "false") << "\n";
    f << "force=" << (force ? "true" : "false") << "\n";
    f << "vid=0x" << std::hex << vid << "\n";
    f << "pid=0x" << std::hex << pid << "\n";
    f << "clock=" << std::dec << clock_speed << "\n";
    f << "adapter=" << adapter_type << "\n";
}

Config Config::from_args(int argc, char* argv[]) {
    Config cfg;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-v" || arg == "--verbose") {
            cfg.verbose = true;
        } else if (arg == "-f" || arg == "--force") {
            cfg.force = true;
        } else if (arg == "--vid") {
            if (i + 1 < argc) cfg.vid = std::stoul(argv[++i], 0, 0);
        } else if (arg == "--pid") {
            if (i + 1 < argc) cfg.pid = std::stoul(argv[++i], 0, 0);
        } else if (arg == "--config") {
            if (i + 1 < argc) {
                cfg.config_file = argv[++i];
                cfg = load(cfg.config_file);
            }
        }
    }
    
    return cfg;
}
