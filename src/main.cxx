/*
 * BuckshotESP - Shell ESP for Buckshot Roulette
 * by DesirePro | free
 */
#include "config.hxx"
#include "patcher.hxx"
#include "esp.hxx"
#include "overlay.hxx"

#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <thread>
#include <filesystem>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

static HANDLE console;

static void color( WORD c ) { SetConsoleTextAttribute( console, c ); }

static DWORD findGameProcess( ) {
    HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if ( snap == INVALID_HANDLE_VALUE ) return 0;
    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof( pe );
    DWORD pid = 0;
    if ( Process32FirstW( snap, &pe ) ) {
        do {
            if ( _wcsicmp( pe.szExeFile, L"Buckshot Roulette.exe" ) == 0 ) { pid = pe.th32ProcessID; break; }
        }
        while ( Process32NextW( snap, &pe ) );
    }
    CloseHandle( snap );
    return pid;
}

int main( int argc, char * argv[ ] ) {
    console = GetStdHandle( STD_OUTPUT_HANDLE );
    SetConsoleTitleW( L"BuckshotESP | by DesirePro | free" );
    CONSOLE_CURSOR_INFO ci = { 1, FALSE };
    SetConsoleCursorInfo( console, &ci );

    color( 0x0B );
    printf( "  BUCKSHOT ROULETTE - SHELL ESP\n" );
    printf( "  by DesirePro | free\n\n" );

    Config config = loadConfig( );

    if ( config.game_exe_path.empty( ) || !fs::exists( config.game_exe_path ) ) {
        color( 0x0E );
        printf( "  Searching for Buckshot Roulette...\n" );

        std::string path = autoDetectGameExe( );
        if ( !path.empty( ) ) {
            color( 0x0A );
            printf( "  Found: %s\n", path.c_str( ) );
        } else {
            color( 0x0E );
            printf( "  Not found. Opening file dialog...\n" );
            path = openFileDialog( );
        }

        if ( path.empty( ) || !fs::exists( path ) ) {
            color( 0x0C );
            printf( "  Game not found.\n" );
            printf( "  Press Enter to exit...\n" );
            getchar( );
            return 1;
        }

        config.game_exe_path = path;
        config.patched_size = 0;
        saveConfig( config );
    }

    color( 0x07 );
    printf( "  Game: %s\n\n", config.game_exe_path.c_str( ) );

    if ( isGamePatched( config.game_exe_path, config.patched_size ) ) {
        color( 0x0A );
        printf( "  [OK] Game is patched\n\n" );
    } else {
        color( 0x0E );
        printf( "  Patching required...\n" );

        if ( findGameProcess( ) ) {
            color( 0x0C );
            printf( "  Game is running. Close it first.\n" );
            printf( "  Press Enter after closing...\n" );
            getchar( );
            if ( findGameProcess( ) ) {
                printf( "  Still running.\n" );
                getchar( );
                return 1;
            }
        }

        if ( !patchGame( config.game_exe_path, config.patched_size ) ) {
            color( 0x0C );
            printf( "\n  Patching failed.\n" );
            getchar( );
            return 1;
        }

        saveConfig( config );
        color( 0x0A );
        printf( "\n  [OK] Patched successfully.\n\n" );
    }

    color( 0x0F );
    printf( "  HOST = full sequence | CLIENT = chamber + odds\n" );
    color( 0x08 );
    printf( "  [ESC] Exit\n\n" );
    color( 0x0E );
    printf( "  Waiting for Buckshot Roulette...\n" );

    std::thread( overlayThread ).detach( );

    std::string seq_path = getSequenceFilePath( );
    FILETIME last_ft = {};

    while ( app_running ) {
        if ( GetAsyncKeyState( VK_ESCAPE ) & 1 )
            break;

        WIN32_FILE_ATTRIBUTE_DATA fa;
        if ( GetFileAttributesExA( seq_path.c_str( ), GetFileExInfoStandard, &fa )
            && CompareFileTime( &fa.ftLastWriteTime, &last_ft ) != 0 ) {
            last_ft = fa.ftLastWriteTime;
            parseSequenceFile( seq_path );
        }

        Sleep( 40 );
    }

    if ( overlay_hwnd )
        SendMessageW( overlay_hwnd, WM_CLOSE, 0, 0 );

    return 0;
}
