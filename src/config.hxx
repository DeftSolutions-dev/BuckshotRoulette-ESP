#pragma once
#include <string>
#include <cstdint>

struct Config {
    std::string game_exe_path;
    uint64_t    patched_size = 0;
};

std::string getConfigDir( );
Config      loadConfig( );
void        saveConfig( const Config & config );
