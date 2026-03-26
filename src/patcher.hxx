#pragma once
#include <string>
#include <cstdint>

std::string autoDetectGameExe( );
std::string openFileDialog( );
bool        patchGame( const std::string & game_exe, uint64_t & out_patched_size );
bool        isGamePatched( const std::string & exe_path, uint64_t expected_size );
