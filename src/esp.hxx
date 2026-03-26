#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

struct EspState {
    std::mutex              lock;
    std::vector<bool>       sequence;
    bool                    is_host = false;

    int                     initial_live = 0;
    int                     initial_blank = 0;
    int                     remaining_live = 0;
    int                     remaining_blank = 0;

    std::string             chamber;
    bool                    chamber_known = false;

    std::string             phone_shell;
    int                     phone_index = 0;

    std::vector<std::string> event_log;
};

extern EspState         esp_state;
extern std::atomic<bool> app_running;

int  shellsRemaining( );
void addLogEntry( const std::string & message );

std::string              getSequenceFilePath( );
std::vector<bool>        parseShellCsv( const std::string & csv );
void                     parseSequenceFile( const std::string & path );
