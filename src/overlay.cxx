#include "overlay.hxx"
#include "esp.hxx"
#include <cstdio>
#include <string>
#include <vector>

HWND overlay_hwnd = nullptr;

static constexpr int  OV_WIDTH = 480;
static constexpr int  OV_HEIGHT = 185;
static constexpr BYTE OV_ALPHA = 238;

static HFONT createFont( int size, bool bold ) {
    return CreateFontW( size, 0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0,
        CLEARTYPE_QUALITY, 0, L"Consolas" );
}

static LRESULT CALLBACK overlayWndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) {
    switch ( msg ) {
        case WM_CREATE:  SetTimer( hwnd, 1, 55, nullptr ); return 0;
        case WM_TIMER:   InvalidateRect( hwnd, nullptr, TRUE ); return 0;
        case WM_DESTROY: KillTimer( hwnd, 1 ); return 0;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint( hwnd, &ps );
            RECT cr;
            GetClientRect( hwnd, &cr );

            HBRUSH bg = CreateSolidBrush( RGB( 12, 12, 20 ) );
            FillRect( dc, &cr, bg );
            DeleteObject( bg );
            SetBkMode( dc, TRANSPARENT );

            HFONT title_f = createFont( 13, true );
            HFONT orig_f = ( HFONT )SelectObject( dc, title_f );
            SetTextColor( dc, RGB( 140, 140, 170 ) );
            RECT tr = { 8, 3, cr.right, 16 };
            DrawTextW( dc, L"\x2022 BUCKSHOT ESP  |  by DesirePro", -1, &tr, DT_LEFT | DT_SINGLELINE );
            DeleteObject( title_f );

            std::vector<bool> seq;
            bool host;
            std::string chamber, phone;
            int pidx, il, ib, rl, rb;
            bool ck;
            std::vector<std::string> log;
            {
                std::lock_guard<std::mutex> g( esp_state.lock );
                seq = esp_state.sequence;
                host = esp_state.is_host;
                chamber = esp_state.chamber;
                phone = esp_state.phone_shell;
                pidx = esp_state.phone_index;
                il = esp_state.initial_live;
                ib = esp_state.initial_blank;
                rl = esp_state.remaining_live;
                rb = esp_state.remaining_blank;
                ck = esp_state.chamber_known;
                log = esp_state.event_log;
            }

            int total = rl + rb;
            int pct = total > 0 ? rl * 100 / total : 0;
            int y = 20;

            if ( host && !seq.empty( ) ) {
                int x = 10, sz = 25, gap = 4;
                HFONT bf = createFont( 14, true );
                SelectObject( dc, bf );
                for ( size_t i = 0; i < seq.size( ); i++ ) {
                    bool live = seq[ i ];
                    int bs = ( i == 0 ) ? sz + 6 : sz;
                    int by = ( i == 0 ) ? y - 3 : y;
                    if ( i == 0 ) {
                        HBRUSH h = CreateSolidBrush( RGB( 255, 220, 50 ) );
                        RECT r = { x - 3, by - 3, x + bs + 3, by + bs + 3 };
                        FillRect( dc, &r, h ); DeleteObject( h );
                    }
                    HBRUSH b = CreateSolidBrush( live ? RGB( 195, 35, 35 ) : RGB( 30, 78, 188 ) );
                    RECT r = { x, by, x + bs, by + bs };
                    FillRect( dc, &r, b ); DeleteObject( b );
                    SetTextColor( dc, RGB( 255, 255, 255 ) );
                    wchar_t ch = live ? L'L' : L'B';
                    DrawTextW( dc, &ch, 1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
                    x += bs + gap;
                }
                DeleteObject( bf );
                y += sz + 8;

                HFONT nf = createFont( 14, true );
                SelectObject( dc, nf );
                SetTextColor( dc, seq[ 0 ] ? RGB( 255, 90, 90 ) : RGB( 90, 150, 255 ) );
                wchar_t txt[ 96 ];
                swprintf( txt, 96, L">> NEXT: %s", seq[ 0 ] ? L"LIVE" : L"BLANK" );
                RECT nr = { 10, y, cr.right - 10, y + 16 };
                DrawTextW( dc, txt, -1, &nr, DT_LEFT | DT_SINGLELINE );
                DeleteObject( nf );
                y += 18;

                HFONT sf = createFont( 12, false );
                SelectObject( dc, sf );
                SetTextColor( dc, RGB( 130, 130, 155 ) );
                swprintf( txt, 96, L"Live:%d  Blank:%d  |  %d left  |  %d%% Live", rl, rb, total, pct );
                RECT sr = { 10, y, cr.right - 10, y + 14 };
                DrawTextW( dc, txt, -1, &sr, DT_LEFT | DT_SINGLELINE );
                DeleteObject( sf );
                y += 16;

                int bw = cr.right - 20, bh = 10;
                int lw = total > 0 ? bw * rl / total : 0;
                HBRUSH lb = CreateSolidBrush( RGB( 190, 35, 35 ) );
                HBRUSH bb = CreateSolidBrush( RGB( 30, 75, 185 ) );
                RECT lr = { 10, y, 10 + lw, y + bh };
                RECT br = { 10 + lw, y, 10 + bw, y + bh };
                FillRect( dc, &lr, lb ); FillRect( dc, &br, bb );
                DeleteObject( lb ); DeleteObject( bb );
                y += bh + 6;
            } else if ( total > 0 || ck ) {
                if ( ck && !chamber.empty( ) ) {
                    bool live = ( chamber == "live" );
                    HBRUSH cb = CreateSolidBrush( live ? RGB( 180, 25, 25 ) : RGB( 25, 58, 168 ) );
                    RECT bx = { 10, y, cr.right - 10, y + 32 };
                    FillRect( dc, &bx, cb ); DeleteObject( cb );
                    HFONT cf = createFont( 18, true );
                    SelectObject( dc, cf );
                    SetTextColor( dc, RGB( 255, 255, 255 ) );
                    DrawTextW( dc, live ? L"\x25B6 LIVE" : L"\x25B6 BLANK", -1, &bx,
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE );
                    DeleteObject( cf );
                    y += 36;
                } else {
                    HFONT uf = createFont( 13, false );
                    SelectObject( dc, uf );
                    SetTextColor( dc, RGB( 80, 80, 100 ) );
                    RECT ur = { 10, y, cr.right - 10, y + 16 };
                    DrawTextW( dc, L"Chamber: ???  (wait for item use)", -1, &ur, DT_LEFT | DT_SINGLELINE );
                    DeleteObject( uf );
                    y += 20;
                }

                if ( total > 0 ) {
                    int bw = cr.right - 20, bh = 18;
                    int lw = total > 0 ? bw * rl / total : 0;
                    if ( lw < 3 && rl > 0 ) lw = 3;
                    HBRUSH lb = CreateSolidBrush( RGB( 190, 35, 35 ) );
                    HBRUSH bb = CreateSolidBrush( RGB( 30, 75, 185 ) );
                    RECT lr = { 10, y, 10 + lw, y + bh };
                    RECT br = { 10 + lw, y, 10 + bw, y + bh };
                    FillRect( dc, &lr, lb ); FillRect( dc, &br, bb );
                    DeleteObject( lb ); DeleteObject( bb );

                    HFONT pf = createFont( 12, true );
                    SelectObject( dc, pf );
                    SetTextColor( dc, RGB( 255, 255, 255 ) );
                    wchar_t t[ 24 ];
                    if ( rl > 0 ) { swprintf( t, 24, L"Live:%d %d%%", rl, pct ); DrawTextW( dc, t, -1, &lr, DT_CENTER | DT_VCENTER | DT_SINGLELINE ); }
                    if ( rb > 0 ) { swprintf( t, 24, L"Blank:%d %d%%", rb, 100 - pct ); DrawTextW( dc, t, -1, &br, DT_CENTER | DT_VCENTER | DT_SINGLELINE ); }
                    DeleteObject( pf );
                    y += bh + 4;

                    HFONT df = createFont( 11, false );
                    SelectObject( dc, df );
                    SetTextColor( dc, RGB( 100, 100, 120 ) );
                    wchar_t dt[ 96 ];
                    swprintf( dt, 96, L"%d left | Fired: %dL %dB | Round: %dL+%dB",
                        total, il - rl, ib - rb, il, ib );
                    RECT dr = { 10, y, cr.right - 10, y + 14 };
                    DrawTextW( dc, dt, -1, &dr, DT_LEFT | DT_SINGLELINE );
                    DeleteObject( df );
                    y += 16;
                } else {
                    HFONT ef = createFont( 13, true );
                    SelectObject( dc, ef );
                    SetTextColor( dc, RGB( 100, 100, 60 ) );
                    RECT er = { 10, y, cr.right - 10, y + 16 };
                    DrawTextW( dc, L"No shells left", -1, &er, DT_LEFT | DT_SINGLELINE );
                    DeleteObject( ef );
                    y += 18;
                }

                if ( !phone.empty( ) ) {
                    HFONT pf = createFont( 12, true );
                    SelectObject( dc, pf );
                    SetTextColor( dc, ( phone == "live" ) ? RGB( 255, 120, 120 ) : RGB( 120, 170, 255 ) );
                    wchar_t pt[ 64 ];
                    swprintf( pt, 64, L"Phone: #%d = %s", pidx, ( phone == "live" ) ? L"LIVE" : L"BLANK" );
                    RECT pr = { 10, y, cr.right - 10, y + 14 };
                    DrawTextW( dc, pt, -1, &pr, DT_LEFT | DT_SINGLELINE );
                    DeleteObject( pf );
                    y += 16;
                }
            } else {
                HFONT wf = createFont( 13, false );
                SelectObject( dc, wf );
                SetTextColor( dc, RGB( 55, 55, 75 ) );
                RECT wr = { 10, 28, cr.right - 10, 46 };
                DrawTextW( dc, L"Waiting for round...", -1, &wr, DT_LEFT | DT_SINGLELINE );
                DeleteObject( wf );
            }

            if ( !log.empty( ) ) {
                int ly = cr.bottom - 4 - ( int )log.size( ) * 12;
                if ( ly < y + 4 ) ly = y + 4;
                HFONT lf = createFont( 10, false );
                SelectObject( dc, lf );
                SetTextColor( dc, RGB( 60, 60, 80 ) );
                for ( auto & e : log ) {
                    wchar_t w[ 96 ];
                    MultiByteToWideChar( CP_UTF8, 0, e.c_str( ), -1, w, 96 );
                    RECT r = { 10, ly, cr.right - 10, ly + 12 };
                    DrawTextW( dc, w, -1, &r, DT_LEFT | DT_SINGLELINE );
                    ly += 12;
                }
                DeleteObject( lf );
            }

            SelectObject( dc, orig_f );
            EndPaint( hwnd, &ps );
            return 0;
        }
    }
    return DefWindowProcW( hwnd, msg, wp, lp );
}

void overlayThread( ) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof( wc );
    wc.lpfnWndProc = overlayWndProc;
    wc.hInstance = GetModuleHandle( nullptr );
    wc.lpszClassName = L"BuckshotEspOverlay";
    wc.hbrBackground = CreateSolidBrush( RGB( 12, 12, 20 ) );
    RegisterClassExW( &wc );

    int sx = GetSystemMetrics( SM_CXSCREEN );
    overlay_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"", WS_POPUP | WS_VISIBLE,
        sx - OV_WIDTH - 14, 14, OV_WIDTH, OV_HEIGHT,
        nullptr, nullptr, wc.hInstance, nullptr );
    SetLayeredWindowAttributes( overlay_hwnd, 0, OV_ALPHA, LWA_ALPHA );

    MSG msg;
    while ( app_running.load( ) && GetMessageW( &msg, nullptr, 0, 0 ) > 0 ) {
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }
}
