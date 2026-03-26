#include "esp.hxx"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>

EspState          esp_state;
std::atomic<bool> app_running{ true };

int shellsRemaining( ) {
    return esp_state.remaining_live + esp_state.remaining_blank;
}

void addLogEntry( const std::string & message ) {
    if ( esp_state.event_log.size( ) >= 10 )
        esp_state.event_log.erase( esp_state.event_log.begin( ) );
    esp_state.event_log.push_back( message );
}

std::string getSequenceFilePath( ) {
    char path[ MAX_PATH ];
    if ( SHGetFolderPathA( NULL, CSIDL_APPDATA, NULL, 0, path ) == S_OK )
        return std::string( path ) + "\\Godot\\app_userdata\\Buckshot Roulette\\shell_sequence.txt";
    return "";
}

std::vector<bool> parseShellCsv( const std::string & csv ) {
    std::vector<bool> result;
    if ( csv.empty( ) ) return result;
    std::stringstream stream( csv );
    std::string token;
    while ( std::getline( stream, token, ',' ) ) {
        while ( !token.empty( ) && token.front( ) == ' ' ) token.erase( 0, 1 );
        while ( !token.empty( ) && token.back( ) == ' ' ) token.pop_back( );
        if ( token == "live" ) result.push_back( true );
        else if ( token == "blank" ) result.push_back( false );
        else return {};
    }
    return result;
}

static std::string prev_raw;
static int prev_event_id = -1;
static int prev_shot_count = -1;

// Item ID: 1=Handsaw 2=Magnifier 3=Jammer 4=Cigarettes
//           5=Beer 6=Phone 8=Adrenaline 9=Inverter 10=Remote
static const char * ITEM_NAMES[ ] = {
    "?", "HANDSAW", "MAGNIFIER", "JAMMER", "CIGARETTES",
    "BEER", "PHONE", "?", "ADRENALINE", "INVERTER", "REMOTE"
};

void parseSequenceFile( const std::string & path ) {
    std::ifstream file( path );
    if ( !file.is_open( ) ) return;
    std::string raw;
    std::getline( file, raw );
    file.close( );
    if ( raw == prev_raw ) return;
    prev_raw = raw;
    if ( raw.empty( ) ) return;

    std::vector<std::string> f;
    {
        std::stringstream ss( raw ); std::string s;
        while ( std::getline( ss, s, '|' ) ) f.push_back( s );
    }
    while ( f.size( ) < 9 ) f.push_back( "" );

    std::lock_guard<std::mutex> guard( esp_state.lock );

    auto host_seq = parseShellCsv( f[ 0 ] );
    bool is_host = !host_seq.empty( );
    auto vis_seq = parseShellCsv( f[ 5 ] );
    std::string cur = f[ 1 ], shot = f[ 2 ];
    int item_id = -1;    try { item_id = std::stoi( f[ 6 ] ); } catch ( ... ) { }
    int event_id = 0;    try { event_id = std::stoi( f[ 7 ] ); } catch ( ... ) { }
    int shot_count = 0;  try { shot_count = std::stoi( f[ 8 ] ); } catch ( ... ) { }

    bool new_round = false;
    if ( is_host && ( int )host_seq.size( ) > ( int )esp_state.sequence.size( ) )
        new_round = true;
    if ( !vis_seq.empty( ) ) {
        int vl = 0, vb = 0;
        for ( bool s : vis_seq ) { if ( s ) vl++; else vb++; }
        if ( vl + vb > 0 && ( vl != esp_state.initial_live || vb != esp_state.initial_blank ) )
            new_round = true;
        if ( shellsRemaining( ) == 0 && vl + vb > 0 )
            new_round = true;
    }

    if ( new_round ) {
        int nl = 0, nb = 0;
        if ( is_host )
            for ( bool s : host_seq ) { if ( s ) nl++; else nb++; } else if ( !vis_seq.empty( ) )
                for ( bool s : vis_seq ) { if ( s ) nl++; else nb++; }

        esp_state.initial_live = nl;
        esp_state.initial_blank = nb;
        esp_state.remaining_live = nl;
        esp_state.remaining_blank = nb;
        esp_state.chamber.clear( );
        esp_state.chamber_known = false;
        esp_state.phone_shell.clear( );
        esp_state.event_log.clear( );
        prev_event_id = event_id;
        prev_shot_count = shot_count;
        addLogEntry( "[ROUND] " + std::to_string( nl ) + "L + "
            + std::to_string( nb ) + "B = " + std::to_string( nl + nb ) );
    }

    esp_state.sequence = host_seq;
    esp_state.is_host = is_host;

    if ( is_host && !esp_state.sequence.empty( ) ) {
        int l = 0, b = 0;
        for ( bool s : esp_state.sequence ) { if ( s ) l++; else b++; }
        esp_state.remaining_live = l;
        esp_state.remaining_blank = b;
        esp_state.chamber = esp_state.sequence[ 0 ] ? "live" : "blank";
        esp_state.chamber_known = true;
    }

    if ( !is_host ) {
        if ( shot_count > prev_shot_count && prev_shot_count >= 0 ) {
            if ( shot == "live" ) esp_state.remaining_live--;
            else if ( shot == "blank" ) esp_state.remaining_blank--;
            if ( esp_state.remaining_live < 0 ) esp_state.remaining_live = 0;
            if ( esp_state.remaining_blank < 0 ) esp_state.remaining_blank = 0;
            esp_state.chamber.clear( );
            esp_state.chamber_known = false;
            addLogEntry( "[SHOT] " + std::string( shot == "live" ? "LIVE" : "BLANK" )
                + " -> " + std::to_string( shellsRemaining( ) ) + " left" );
        }

        bool item_used = ( event_id > prev_event_id
            && shot_count == prev_shot_count
            && prev_event_id >= 0 );
        if ( item_used ) {
            switch ( item_id ) {
                case 1: case 2: case 3: case 4: case 6: case 7: case 8: case 10:
                {
                    if ( !cur.empty( ) ) { esp_state.chamber = cur; esp_state.chamber_known = true; }
                    const char * name = ( item_id >= 0 && item_id <= 10 ) ? ITEM_NAMES[ item_id ] : "ITEM";
                    addLogEntry( "[" + std::string( name ) + "] Chamber: "
                        + std::string( cur == "live" ? "LIVE" : "BLANK" ) );
                    if ( item_id == 6 && f[ 3 ].find( ':' ) != std::string::npos ) {
                        auto c = f[ 3 ].find( ':' );
                        std::string ps = f[ 3 ].substr( 0, c );
                        int pi = 0; try { pi = std::stoi( f[ 3 ].substr( c + 1 ) ); } catch ( ... ) { }
                        if ( !ps.empty( ) ) {
                            esp_state.phone_shell = ps;
                            esp_state.phone_index = pi;
                            addLogEntry( "[PHONE] #" + std::to_string( pi ) + " = "
                                + std::string( ps == "live" ? "LIVE" : "BLANK" ) );
                        }
                    }
                    break;
                }
                case 5: // Beer
                {
                    if ( cur == "live" ) esp_state.remaining_live--;
                    else if ( cur == "blank" ) esp_state.remaining_blank--;
                    if ( esp_state.remaining_live < 0 ) esp_state.remaining_live = 0;
                    if ( esp_state.remaining_blank < 0 ) esp_state.remaining_blank = 0;
                    esp_state.chamber.clear( );
                    esp_state.chamber_known = false;
                    addLogEntry( "[BEER] Ejected " + std::string( cur == "live" ? "LIVE" : "BLANK" )
                        + " -> " + std::to_string( shellsRemaining( ) ) + " left" );
                    break;
                }
                case 9: // Inverter
                {
                    esp_state.chamber = ( cur == "live" ) ? "blank" : "live";
                    esp_state.chamber_known = true;
                    if ( cur == "live" ) { esp_state.remaining_live--; esp_state.remaining_blank++; } else { esp_state.remaining_live++; esp_state.remaining_blank--; }
                    if ( esp_state.remaining_live < 0 ) esp_state.remaining_live = 0;
                    if ( esp_state.remaining_blank < 0 ) esp_state.remaining_blank = 0;
                    addLogEntry( "[INVERTER] " + std::string( cur == "live" ? "LIVE->BLANK" : "BLANK->LIVE" )
                        + " | L:" + std::to_string( esp_state.remaining_live )
                        + " B:" + std::to_string( esp_state.remaining_blank ) );
                    break;
                }
                default:
                if ( !cur.empty( ) ) { esp_state.chamber = cur; esp_state.chamber_known = true; }
                addLogEntry( "[ITEM " + std::to_string( item_id ) + "] Chamber: "
                    + std::string( cur.empty( ) ? "?" : cur == "live" ? "LIVE" : "BLANK" ) );
                break;
            }
        }

        if ( f[ 3 ].find( ':' ) != std::string::npos && item_id != 6 ) {
            auto c = f[ 3 ].find( ':' );
            std::string ps = f[ 3 ].substr( 0, c );
            int pi = 0; try { pi = std::stoi( f[ 3 ].substr( c + 1 ) ); } catch ( ... ) { }
            if ( !ps.empty( ) && ( ps != esp_state.phone_shell || pi != esp_state.phone_index ) ) {
                esp_state.phone_shell = ps;
                esp_state.phone_index = pi;
                addLogEntry( "[PHONE] #" + std::to_string( pi ) + " = "
                    + std::string( ps == "live" ? "LIVE" : "BLANK" ) );
            }
        }
    }

    prev_event_id = event_id;
    prev_shot_count = shot_count;
}
