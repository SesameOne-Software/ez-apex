#include <Shobjidl.h>
#include <iostream>
#include <string>
#include <sstream>
#include <Psapi.h>
#include <thread>
#include <chrono>

#include "minhook/MinHook.h"

#include <windowsx.h>

#include <d3d9.h>
#include <d3dx9.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;
D3DPRESENT_PARAMETERS g_d3dpp {};
LPD3DXFONT  m_font = NULL;

void ResetDevice ( ) {
    if ( m_font )
        m_font->Release ( );

    HRESULT hr = g_pd3dDevice->Reset ( &g_d3dpp );

    if ( hr == D3DERR_INVALIDCALL )
        __debugbreak ( );
}

struct SD3DVertex {
    float x, y, z, rhw;
    DWORD color;
};

bool DrawMessage ( LPD3DXFONT font, unsigned int x, unsigned int y, int alpha, unsigned char r, unsigned char g, unsigned char b, LPCSTR Message )
{	// Create a colour for the text
    D3DCOLOR fontColor = D3DCOLOR_ARGB ( alpha, r, g, b );
    RECT rct; //Font
    rct.left = x;
    rct.right = 1680;
    rct.top = y;
    rct.bottom = rct.top + 200;
    font->DrawTextA ( NULL, Message, -1, &rct, 0, fontColor );
    return true;
}

void draw_filled ( LPDIRECT3DDEVICE9 device, int x, int y, int width, int height, D3DCOLOR colour ) {
    SD3DVertex pVertex [ 4 ] = { { x, y + height, 0.0f, 1.0f, colour }, { x, y, 0.0f, 1.0f, colour }, { x + width, y + height, 0.0f, 1.0f, colour }, { x + width, y, 0.0f, 1.0f, colour } };
    device->DrawPrimitiveUP ( D3DPT_TRIANGLESTRIP, 2, pVertex, sizeof ( SD3DVertex ) );
}

typedef struct _ENUM_INFO {
    DWORD dwProcID;
    HWND foundHwnd;
}ENUM_INFO;

BOOL CALLBACK EnumPopupWindowsProc ( HWND hWnd, LPARAM lParam )
{
    ENUM_INFO* pEPI = ( ENUM_INFO* ) lParam;

    DWORD dwProcID = 0;
    GetWindowThreadProcessId ( hWnd, &dwProcID );

    auto style = GetWindowStyle ( hWnd );
    auto exStyle = GetWindowExStyle ( hWnd );

    if ( dwProcID == pEPI->dwProcID && ( ( style & ( WS_POPUP ) ) ) && ( ( exStyle & ( WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED ) ) == ( WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED ) ) )
    {
        pEPI->foundHwnd = hWnd;
        return TRUE;
    }

    return TRUE;
}

HWND getWindow ( )
{
    ENUM_INFO epi = { 0 };
    memset ( &epi, 0, sizeof ( epi ) );
    epi.dwProcID = GetCurrentProcessId ( );

    EnumWindows ( EnumPopupWindowsProc, ( LPARAM ) &epi );

    int time_taken = 0;

    while ( !epi.foundHwnd && time_taken < 5000 ) { // 5 second wait time
        time_taken += 100;
        std::this_thread::sleep_for ( std::chrono::milliseconds ( 100 ) );
    }

    return epi.foundHwnd;
}

void main ( ) {
    const auto targetWindow = getWindow ( );

    if ( !targetWindow || targetWindow == INVALID_HANDLE_VALUE ) {
        MessageBoxA ( 0, "Failed to find target window!", 0, 0 );
        __debugbreak();
        return;
    }

    SetWindowPos ( targetWindow, HWND_TOPMOST, 0, 0, 1920, 1080, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER );
    SetLayeredWindowAttributes ( targetWindow, 0, 255, LWA_ALPHA );

    MARGINS Margin = { -1 };
    DwmExtendFrameIntoClientArea ( targetWindow, &Margin );

    // Initialize Direct3D
    LPDIRECT3D9 pD3D;
    if ( ( pD3D = Direct3DCreate9 ( D3D_SDK_VERSION ) ) == NULL ) {
        MessageBoxA ( targetWindow, "Failed to initialize D3D9!", 0, 0 );
        __debugbreak ();
        return;
    }

    RECT screen_rect;
    GetWindowRect ( GetDesktopWindow ( ), &screen_rect );

    g_d3dpp.BackBufferWidth = screen_rect.right;
    g_d3dpp.BackBufferHeight = screen_rect.bottom;

    g_d3dpp.hDeviceWindow = targetWindow;
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;

    if ( pD3D->CreateDevice ( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, targetWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice ) < 0 )
    {
        pD3D->Release ( );
        MessageBoxA ( 0, "Failed to create D3D9 device!", 0, 0 );
        __debugbreak();
        return;
    }

    /* initialize d3d stuff here */

    MSG msg = { 0 };

    while ( msg.message != WM_QUIT ) {
        if ( PeekMessage ( &msg, NULL, 0U, 0U, PM_REMOVE ) ) {
            TranslateMessage ( &msg );
            DispatchMessage ( &msg );
            continue;
        }

        if ( !( GetWindowLong ( targetWindow, GWL_STYLE ) & WS_VISIBLE ) )
            SetWindowLong ( targetWindow, GWL_STYLE, GetWindowLong ( targetWindow, GWL_STYLE ) | WS_VISIBLE );

        g_pd3dDevice->SetFVF ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 );

        g_pd3dDevice->SetTexture ( 0, NULL );
        g_pd3dDevice->SetPixelShader ( 0 ); //fix black color

        //alpha
        g_pd3dDevice->SetRenderState ( D3DRS_ALPHABLENDENABLE, true );
        g_pd3dDevice->SetRenderState ( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
        g_pd3dDevice->SetRenderState ( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA ( 0, 0, 0, 0 );
        g_pd3dDevice->Clear ( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.f, 0 );
        if ( g_pd3dDevice->BeginScene ( ) >= 0 )
        {
            //DrawMessage(m_font, 10, 10, 255, 255, 0, 0, ...);
            draw_filled ( g_pd3dDevice, 40, 40, 30, 30, D3DCOLOR_RGBA ( 0, 255, 0, 255 ) );
            g_pd3dDevice->EndScene ( );
        }

        HRESULT result = g_pd3dDevice->Present ( NULL, NULL, NULL, NULL );

        // Handle loss of D3D9 device
        if ( result == D3DERR_DEVICELOST ) {
            ResetDevice ( );
            //PostMessage(targetWindow, WM_CLOSE, 0, 0);
        }

        //if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
          //  ResetDevice();
    }
}

BOOL APIENTRY DllMain ( HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch ( ul_reason_for_call )
    {
    case DLL_PROCESS_ATTACH: {
        CloseHandle ( CreateThread ( 0, 0, ( LPTHREAD_START_ROUTINE ) main, 0, 0, nullptr ) );
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

