#include "config.hxx"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

std::string getConfigDir( ) {
    char path[ MAX_PATH ];
    if ( SHGetFolderPathA( NULL, CSIDL_APPDATA, NULL, 0, path ) == S_OK )
        return std::string( path ) + "\\BuckshotESP";
    return "";
}

Config loadConfig( ) {
    Config config;
    std::ifstream file( getConfigDir( ) + "\\config.txt" );
    if ( file.is_open( ) ) {
        std::getline( file, config.game_exe_path );
        std::string size_line;
        std::getline( file, size_line );
        try { config.patched_size = std::stoull( size_line ); } catch ( ... ) { }
    }
    return config;
}

void saveConfig( const Config & config ) {
    fs::create_directories( getConfigDir( ) );
    std::ofstream file( getConfigDir( ) + "\\config.txt" );
    file << config.game_exe_path << "\n" << config.patched_size << "\n";
}
