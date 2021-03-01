#include "overlay.hpp"

#include "ck.hpp"

#include "input.hpp"
#include "renderer.hpp"

#include "../imgui/gui.hpp"
#include "../minhook/MinHook.h"
#include "../renderer/render.hpp"
#include "../menu/menu.hpp"
#include "../resources/icons.hpp"

#include "../driver_interface.hpp"

#include "../security/security.hpp"

#include <thread>
#include <iostream>
#include <dwmapi.h>
#include <string>

//#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")

namespace overlay {
    HWND window = nullptr;
    HWND game_window = nullptr;
    ImFont* default_font = nullptr;

    void draw( IDXGISwapChain* pDxgiSwapChain );

    using present_t = HRESULT( __stdcall* ) ( IDXGISwapChain* , UINT , UINT );
    present_t backup_present = nullptr;

    HRESULT __stdcall present_hk( IDXGISwapChain* pSwapChain , UINT SyncInterval , UINT Flags ) {
        draw( pSwapChain );

        return backup_present( pSwapChain, SyncInterval, Flags );
    }

    void draw( IDXGISwapChain* pDxgiSwapChain ) {
        if ( !renderer::initialized( ) ) {
            ID3D11Device* device = nullptr;

            if ( pDxgiSwapChain->GetDevice( __uuidof( ID3D11Device ) , reinterpret_cast< void** >( &device ) ) == S_OK ) {
                renderer::create( device , pDxgiSwapChain );

                ImGui::CreateContext( );
                ImGuiIO& io = ImGui::GetIO( ); ( void ) io;

                io.MouseDrawCursor = false;

                ImGui::StyleColorsSesame( );

                ImGui_ImplWin32_Init( game_window );
                ImGui_ImplDX11_Init( overlay::renderer::device , overlay::renderer::context );

                default_font = io.Fonts->AddFontFromFileTTF( _("C:\\Windows\\Fonts\\segoeui.ttf" ), 16.0f );

                ImGui::GetStyle( ).AntiAliasedFill = ImGui::GetStyle( ).AntiAliasedLines = true;

                auto& fonts = render::get_font_list( );

                fonts[ _("Default") ] = default_font;
                reinterpret_cast< ImFont* >( fonts[_( "Default" )] )->SetFallbackChar( '?' );

                fonts[ _("GUI Font" )] = io.Fonts->AddFontFromFileTTF( _("C:\\Windows\\Fonts\\segoeui.ttf" ), 16.0f );
                reinterpret_cast< ImFont* >( fonts[ _("GUI Font" )] )->SetFallbackChar( '?' );
                fonts[ _("GUI Small Font") ] = io.Fonts->AddFontFromFileTTF( _("C:\\Windows\\Fonts\\segoeui.ttf") , 14.0f );
                reinterpret_cast< ImFont* >( fonts[ _("GUI Small Font") ] )->SetFallbackChar( '?' );
                fonts[_("GUI Icons Font" )] = io.Fonts->AddFontFromMemoryTTF( sesame_icons_data , sesame_icons_size , 28.0f , nullptr , io.Fonts->GetGlyphRangesDefault( ) );
                reinterpret_cast< ImFont* >( fonts[_( "GUI Icons Font" )] )->SetFallbackChar( '?' );
            }
        }

        if ( game_window != GetForegroundWindow( ) ) {
            const float clear_clr[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
            renderer::context->ClearRenderTargetView( renderer::target_view , clear_clr );
            return;
        }

        ImGui_ImplDX11_NewFrame( );
        ImGui_ImplWin32_NewFrame( );

        ImGui::NewFrame( );

        ImGui::PushFont( default_font );

        /* invisible backbuffer to draw on */ {
            ImGuiIO& io = ImGui::GetIO( );

            ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize , 0.0f );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding , { 0.0f, 0.0f } );
            ImGui::PushStyleColor( ImGuiCol_WindowBg , { 0.0f, 0.0f, 0.0f, 0.0f } );
            ImGui::Begin( _("##Backbuffer") , nullptr , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs );

            ImGui::SetWindowPos( ImVec2( 0 , 0 ) , ImGuiCond_Always );
            ImGui::SetWindowSize( ImVec2( io.DisplaySize.x , io.DisplaySize.y ) , ImGuiCond_Always );

            /* render on screen here */
            render::gradient( 0.0f, 16.0f, 110.0f, 24.0f, rgba( 0 , 0 , 0 , 255 ), rgba( 0 , 0 , 0 , 0 ), true );
            render::text( 4.0f , 20.0f ,_( "nigga tapper v41") ,_( "Default" ), rgba( 255 , 255 , 255 , 255 ) , true );
            //const auto time = ImGui::GetTime( );
            //render::rect( 20.0f, 60.0f, (time - floor( time )) * 100.0f, 6.0f, rgba(255, 0,0 , 255) );

            /* call features like esp below */

            ImGui::GetCurrentWindow( )->DrawList->PushClipRectFullScreen( );

            ImGui::End( );
            ImGui::PopStyleColor( );
            ImGui::PopStyleVar( 2 );
        }

        ImGui::PopFont( );

        menu::draw( );

        ImGui::EndFrame( );
        ImGui::Render( );

        const float clear_clr[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
        renderer::context->OMSetRenderTargets( 1 , &renderer::target_view , nullptr );
        renderer::context->ClearRenderTargetView( renderer::target_view , clear_clr );

        ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );
    }

    void enable() {
        window = FindWindowA( nullptr, _("NVIDIA GeForce Overlay DT") );
        game_window = drv::window;

        const auto d3d_present = reinterpret_cast<void*>( find_pattern_module(_( "dxgi.dll") , _("48 89 5C 24 10 48 89 74 24 20" )) );
        
        //std::cout << "d3d_present: 0x" << std::hex << d3d_present << std::endl;

        input::enable( window, game_window );

        MH_Initialize( );
        
        MH_CreateHook( d3d_present , present_hk , reinterpret_cast< void** >( &backup_present ) );
        
        MH_EnableHook(MH_ALL_HOOKS);
    }

    void disable() {
        input::disable( window , game_window );

        MH_DisableHook(MH_ALL_HOOKS);

        renderer::destroy();

        MH_Uninitialize();

        ImGui_ImplWin32_Shutdown( );
        //ImGui_ImplDX11_Shutdown( );
        ImGui::DestroyContext( );
    }
}
