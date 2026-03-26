#include "patcher.hxx"
#include "config.hxx"
#include "embedded_scripts.hxx"

#include <windows.h>
#include <shlobj.h>
#include <commdlg.h>

#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

static std::string getSteamPathFromRegistry( ) {
    HKEY key;
    char value[ MAX_PATH ] = { 0 };
    DWORD size = sizeof( value );

    if ( RegOpenKeyExA( HKEY_LOCAL_MACHINE,
        "SOFTWARE\\WOW6432Node\\Valve\\Steam",
        0, KEY_READ, &key ) == ERROR_SUCCESS ) {
        if ( RegQueryValueExA( key, "InstallPath", NULL, NULL,
            ( LPBYTE )value, &size ) == ERROR_SUCCESS ) {
            RegCloseKey( key );
            return value;
        }
        RegCloseKey( key );
    }

    size = sizeof( value );
    if ( RegOpenKeyExA( HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Valve\\Steam",
        0, KEY_READ, &key ) == ERROR_SUCCESS ) {
        if ( RegQueryValueExA( key, "InstallPath", NULL, NULL,
            ( LPBYTE )value, &size ) == ERROR_SUCCESS ) {
            RegCloseKey( key );
            return value;
        }
        RegCloseKey( key );
    }

    size = sizeof( value );
    if ( RegOpenKeyExA( HKEY_CURRENT_USER,
        "SOFTWARE\\Valve\\Steam",
        0, KEY_READ, &key ) == ERROR_SUCCESS ) {
        if ( RegQueryValueExA( key, "SteamPath", NULL, NULL,
            ( LPBYTE )value, &size ) == ERROR_SUCCESS ) {
            RegCloseKey( key );
            std::string p( value );
            for ( auto & c : p ) if ( c == '/' ) c = '\\';
            return p;
        }
        RegCloseKey( key );
    }

    return "";
}

static std::vector<std::string> getSteamLibraryPaths( const std::string & steam_root ) {
    std::vector<std::string> paths;
    paths.push_back( steam_root );

    std::ifstream file( steam_root + "\\steamapps\\libraryfolders.vdf" );
    if ( !file.is_open( ) ) return paths;

    std::string line;
    while ( std::getline( file, line ) ) {
        auto pos = line.find( "\"path\"" );
        if ( pos == std::string::npos ) continue;
        auto q1 = line.find( '"', pos + 6 );
        auto q2 = line.find( '"', q1 + 1 );
        if ( q1 == std::string::npos || q2 == std::string::npos ) continue;

        std::string lib = line.substr( q1 + 1, q2 - q1 - 1 );
        std::string clean;
        for ( size_t i = 0; i < lib.size( ); i++ ) {
            if ( lib[ i ] == '\\' && i + 1 < lib.size( ) && lib[ i + 1 ] == '\\' ) { clean += '\\'; i++; } else
                clean += lib[ i ];
        }
        paths.push_back( clean );
    }
    return paths;
}

std::string autoDetectGameExe( ) {
    const char * rel = "\\steamapps\\common\\Buckshot Roulette"
        "\\Buckshot Roulette_windows\\Buckshot Roulette.exe";

    std::string steam = getSteamPathFromRegistry( );
    if ( !steam.empty( ) ) {
        for ( auto & lib : getSteamLibraryPaths( steam ) ) {
            std::string full = lib + rel;
            if ( fs::exists( full ) ) return full;
        }
    }

    const char * fallbacks[ ] = {
        "C:\\Program Files (x86)\\Steam", "C:\\Program Files\\Steam",
        "D:\\Steam", "D:\\SteamLibrary", "D:\\SteamGame",
        "E:\\Steam", "E:\\SteamLibrary", "F:\\Steam", "F:\\SteamLibrary",
    };
    for ( auto & base : fallbacks ) {
        std::string full = std::string( base ) + rel;
        if ( fs::exists( full ) ) return full;
    }
    return "";
}

std::string openFileDialog( ) {
    CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
    char filepath[ MAX_PATH ] = { 0 };
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof( ofn );
    ofn.lpstrFilter = "Buckshot Roulette\0Buckshot Roulette.exe\0All Files\0*.*\0";
    ofn.lpstrFile = filepath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select Buckshot Roulette.exe";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    std::string result;
    if ( GetOpenFileNameA( &ofn ) ) result = filepath;
    CoUninitialize( );
    return result;
}

static std::string findGdreTools( ) {
    char local[ MAX_PATH ];
    SHGetFolderPathA( NULL, CSIDL_LOCAL_APPDATA, NULL, 0, local );

    std::string link = std::string( local ) + "\\Microsoft\\WinGet\\Links\\gdre_tools.exe";
    if ( fs::exists( link ) ) return link;

    std::string pkg_dir = std::string( local ) + "\\Microsoft\\WinGet\\Packages";
    if ( fs::exists( pkg_dir ) ) {
        for ( auto & entry : fs::directory_iterator( pkg_dir ) ) {
            if ( entry.path( ).string( ).find( "GDRETools" ) != std::string::npos ) {
                std::string candidate = entry.path( ).string( ) + "\\gdre_tools.exe";
                if ( fs::exists( candidate ) ) return candidate;
            }
        }
    }

    std::string direct = std::string( local ) + "\\GDRETools\\gdre_tools.exe";
    if ( fs::exists( direct ) ) return direct;

    return "";
}

bool patchGame( const std::string & game_exe, uint64_t & out_size ) {
    printf( "  [1/4] Finding GDRE Tools...\n" );
    std::string gdre = findGdreTools( );

    if ( gdre.empty( ) ) {
        printf( "  GDRE Tools not found. Installing...\n" );
        int ret = system( "winget install -e --id GDRETools.gdsdecomp"
            " --accept-package-agreements --accept-source-agreements 2>&1" );
        if ( ret != 0 ) {
            printf( "  winget failed, trying direct download...\n" );
            system( "powershell -Command \""
                "$u='https://github.com/GDRETools/gdsdecomp/releases/download/v2.4.0/"
                "GDRE_tools-v2.4.0-windows.zip';"
                "$z=\\\"$env:TEMP\\\\gdre.zip\\\";"
                "$d=\\\"$env:LOCALAPPDATA\\\\GDRETools\\\";"
                "Invoke-WebRequest $u -OutFile $z;"
                "Expand-Archive $z -DestinationPath $d -Force;"
                "Remove-Item $z\" 2>nul" );
        }
        gdre = findGdreTools( );
        if ( gdre.empty( ) ) {
            printf( "  ERROR: Cannot install GDRE Tools.\n" );
            return false;
        }
    }
    printf( "  Found: %s\n", gdre.c_str( ) );

    std::string backup = game_exe + ".backup";
    if ( !fs::exists( backup ) ) {
        printf( "  [2/4] Backing up original...\n" );
        fs::copy_file( game_exe, backup );
    } else {
        printf( "  [2/4] Backup exists\n" );
    }

    printf( "  [3/4] Extracting scripts...\n" );
    std::string tmp = getConfigDir( ) + "\\temp_scripts";
    fs::create_directories( tmp );

    std::vector<std::string> args;
    for ( int i = 0; i < SCRIPT_COUNT; i++ ) {
        std::string file = tmp + "\\script_" + std::to_string( i ) + ".gd";
        std::ofstream out( file, std::ios::binary );
        out.write( reinterpret_cast< const char * >( SCRIPTS[ i ].data ), SCRIPTS[ i ].size );
        out.close( );
        args.push_back( "--patch-file=\"" + file + "=" + SCRIPTS[ i ].res_path + "\"" );
    }

    printf( "  [4/4] Patching game...\n" );
    std::string output = getConfigDir( ) + "\\patched.exe";
    std::string gdre_dir = fs::path( gdre ).parent_path( ).string( );

    std::string cmd = "cd /d \"" + gdre_dir + "\" && gdre_tools.exe --headless";
    cmd += " --pck-patch=\"" + backup + "\"";
    for ( auto & a : args ) cmd += " " + a;
    cmd += " --embed=\"" + backup + "\"";
    cmd += " --output=\"" + output + "\" >nul 2>&1";

    system( ( "cmd /c \"" + cmd + "\"" ).c_str( ) );
    if ( !fs::exists( output ) )
        system( cmd.c_str( ) );

    if ( !fs::exists( output ) ) {
        printf( "  ERROR: Patching failed.\n" );
        return false;
    }

    try {
        fs::copy_file( output, game_exe, fs::copy_options::overwrite_existing );
        out_size = fs::file_size( game_exe );
        fs::remove( output );
    } catch ( std::exception & ex ) {
        printf( "  ERROR: %s\n  Close the game first.\n", ex.what( ) );
        return false;
    }

    fs::remove_all( tmp );
    printf( "  Patched. Size: %llu bytes\n", ( unsigned long long )out_size );
    return true;
}

bool isGamePatched( const std::string & exe_path, uint64_t expected_size ) {
    if ( exe_path.empty( ) || expected_size == 0 ) return false;
    if ( !fs::exists( exe_path ) ) return false;
    return fs::file_size( exe_path ) == expected_size;
}
