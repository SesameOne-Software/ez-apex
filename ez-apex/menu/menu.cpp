#include "menu.hpp"

#include "../overlay/overlay.hpp"
#include "../imgui/gui.hpp"
#include "../renderer/render.hpp"

#include "../security/security.hpp"

#include "options.hpp"

#include <filesystem>
#include <ShlObj.h>
#include <ShlObj_core.h>
#include <fstream>

#pragma comment(lib, "Shell32.lib")

ImFont* gui_font = nullptr;
ImFont* gui_small_font = nullptr;
ImFont* gui_icons_font = nullptr;

bool menu_key_pressed = false;

char selected_config[ 128 ] = "default";
std::vector< std::string > configs { };

void menu::load_cfg_list( ) {
	char appdata[ MAX_PATH ];

	if ( SUCCEEDED( LI_FN( SHGetFolderPathA )( nullptr , 5 , nullptr , 0 , appdata ) ) ) {
		LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex" ) ).c_str( ) , nullptr );
		LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex\\configs" ) ).c_str( ) , nullptr );
	}

	auto sanitize_name = [ ] ( const std::string& dir ) {
		const auto dot = dir.find_last_of( _( "." ) );
		return dir.substr( 0 , dot );
	};

	if ( !configs.empty( ) )
		configs.clear( );

	for ( const auto& dir : std::filesystem::recursive_directory_iterator( std::string( appdata ) + _( "\\sesame_apex\\configs" ) ) ) {
		if ( dir.exists( ) && dir.is_regular_file( ) && dir.path( ).extension( ).string( ) == _( ".xml" ) ) {
			const auto sanitized = sanitize_name( dir.path( ).filename( ).string( ) );
			configs.push_back( sanitized );
		}
	}
}

void player_visuals_controls( const std::string& visual_name ) {
		const auto visuals_config = _( "visuals." ) + visual_name + _( "." );

	ImGui::BeginChildFrame( ImGui::GetID(_( "Player Visuals") ) , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
		ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize( _("Player Visuals") ).x * 0.5f );
		ImGui::Text( _("Player Visuals") );
		ImGui::Separator( );

		static std::vector<const char*> filters {
			"Teammates",
			"Enemies",
			"Weapons"
		};

		ImGui::MultiCombo( _( "Filters" ) , options::vars[ _( "visuals.filters" ) ].val.l , filters.data( ) , filters.size( ) );

		static std::vector<const char*> visual_options {
			"Model" ,
			"ESP Box" ,
			"Health Bar" ,
			"Ammo Bar" ,
			"Shield Bar",
			"Value Text",
			"Nametag" ,
			"Weapon Name",
			"Reloading Flag"
		};

		static std::vector<const char*> model_options {
			"Chams" ,
			"Flat Chams" ,
			"Glow Ouitline"
		};

		ImGui::MultiCombo( _( "Options" ) , options::vars[ visuals_config + _( "options" ) ].val.l , visual_options.data( ) , visual_options.size( ) );

		static std::vector<const char*> element_locations { "Left",  "Right", "Bottom", "Top" };
		ImGui::PushItemWidth( -1.0f );
		ImGui::Combo( _( "Model Type" ) , &options::vars[ visuals_config + _( "model_type" ) ].val.i , model_options.data( ) , model_options.size( ) );
		ImGui::Combo( _( "Health Bar Location" ) , &options::vars[ visuals_config + _( "health_bar_location" ) ].val.i , element_locations.data( ) , element_locations.size( ) );
		ImGui::Combo( _( "Ammo Bar Location" ) , &options::vars[ visuals_config + _( "ammo_bar_location" ) ].val.i , element_locations.data( ) , element_locations.size( ) );
		ImGui::Combo( _( "Shield Bar Location" ) , &options::vars[ visuals_config + _( "shield_bar_location" ) ].val.i , element_locations.data( ) , element_locations.size( ) );
		ImGui::Combo( _( "Value Text Location" ) , &options::vars[ visuals_config + _( "value_text_location" ) ].val.i , element_locations.data( ) , element_locations.size( ) );
		ImGui::Combo( _( "Nametag Location" ) , &options::vars[ visuals_config + _( "nametag_location" ) ].val.i , element_locations.data( ) , element_locations.size( ) );
		ImGui::Combo( _( "Weapon Name Location" ) , &options::vars[ visuals_config + _( "weapon_name_location" ) ].val.i , element_locations.data( ) , element_locations.size( ) );
		ImGui::Combo( _( "Reloading Flag Location" ) , &options::vars[ visuals_config + _( "reloading_flag_location" ) ].val.i , element_locations.data( ) , element_locations.size( ) );
		ImGui::PopItemWidth( );

		ImGui::EndChildFrame( );
	}

	ImGui::SameLine( );

	ImGui::BeginChildFrame( ImGui::GetID( _("Colors" )) , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
		ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize( _("Colors" )).x * 0.5f );
		ImGui::Text( _("Colors" ));
		ImGui::Separator( );

		ImGui::ColorEdit4( _( "Model" ) , ( float* ) &options::vars[ visuals_config + _( "model_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Box" ) , ( float* ) &options::vars[ visuals_config + _( "box_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Health Bar" ) , ( float* ) &options::vars[ visuals_config + _( "health_bar_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Ammo Bar" ) , ( float* ) &options::vars[ visuals_config + _( "ammo_bar_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Shield Bar" ) , ( float* ) &options::vars[ visuals_config + _( "shield_bar_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Value Text" ) , ( float* ) &options::vars[ visuals_config + _( "value_text_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Nametag" ) , ( float* ) &options::vars[ visuals_config + _( "nametag_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Weapon Name" ) , ( float* ) &options::vars[ visuals_config + _( "weapon_name_color" ) ].val.c );
		ImGui::ColorEdit4( _( "Reloading Flag" ) , ( float* ) &options::vars[ visuals_config + _( "reloading_flag_color" ) ].val.c );

		ImGui::EndChildFrame( );
	}
}

void weapon_controls( const std::string& weapon_name ) {
		const auto aimbot_weapon = _( "aimbot." ) + weapon_name + _( "." );

	ImGui::BeginChildFrame( ImGui::GetID( _("Weapon Settings" )) , ImVec2( ImGui::GetWindowContentRegionWidth( ) , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
		ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize( _( "General Settings" ) ).x * 0.5f );
		ImGui::Text( _( "Weapon Settings" ) );
		ImGui::Separator( );

		ImGui::Checkbox( _( "Inherit Default" ) , &options::vars[ aimbot_weapon + _( "inherit_default" ) ].val.b );
		ImGui::Checkbox( _( "Enabled" ) , &options::vars[ aimbot_weapon + _( "enabled" ) ].val.b );
		ImGui::Checkbox( _( "Visible Check" ) , &options::vars[ aimbot_weapon + _( "visible_check" ) ].val.b );
		ImGui::Checkbox( _( "Downed Check" ) , &options::vars[ aimbot_weapon + _( "downed_check" ) ].val.b );
		ImGui::Checkbox( _( "Autoshoot" ) , &options::vars[ aimbot_weapon + _( "autoshoot" ) ].val.b );
		ImGui::Checkbox( _( "Dynamic FOV" ) , &options::vars[ aimbot_weapon + _( "dynamic_fov" ) ].val.b );
		ImGui::SliderFloat( _( "Smoothing" ) , &options::vars[ aimbot_weapon + _( "smoothing" ) ].val.f , 0.0f , 1.0f , _( "x%.1f" ) );
		ImGui::SliderFloat( _( "FOV" ) , &options::vars[ aimbot_weapon + _( "fov" ) ].val.f , 0.0f , 30.0f , _( "%.1f°" ) );

		static std::vector<const char*> hitboxes {
			"Head",
			"Neck",
			"Chest",
			"Pelvis"
		};

		ImGui::MultiCombo( _( "Hitboxes" ) , options::vars[ aimbot_weapon + _( "hitboxes" ) ].val.l , hitboxes.data( ) , hitboxes.size( ) );

		ImGui::EndChildFrame( );
	}
}

void menu::draw( ) {
	ImGuiIO& io = ImGui::GetIO( );
	
	if ( io.KeysDown[ VK_INSERT ] && !menu_key_pressed )
		menu_key_pressed = true;
	else if ( !io.KeysDown[ VK_INSERT ] && menu_key_pressed )
		opened = !opened , menu_key_pressed = false;

	if ( !opened )
		return;

	auto& fonts = render::get_font_list( );

	ImGui::PushFont( reinterpret_cast< ImFont* >( fonts[ _("GUI Font") ] ) );

	if ( ImGui::custom::Begin( _("Sesame for Apex Legends v0.0.1") , &opened , reinterpret_cast< ImFont* >( fonts[ _("GUI Small Font" )] ) ) ) {
		if ( ImGui::custom::BeginTabs( &cur_tab_idx , reinterpret_cast< ImFont* >( fonts[ _("GUI Icons Font" )] ) ) ) {
			ImGui::custom::AddTab( _("A") );
			ImGui::custom::AddTab( _( "D" ) );
			ImGui::custom::AddTab( _( "E" ) );
			ImGui::custom::AddTab( _("F" ));

			ImGui::custom::EndTabs( );
		}

		/* confirm config save (overwrite) popup */
		ImGui::SetNextWindowPos( ImVec2( ImGui::GetWindowPos( ).x + ImGui::GetWindowSize( ).x * 0.5f , ImGui::GetWindowPos( ).y + ImGui::GetWindowSize( ).y * 0.5f ) , ImGuiCond_Always , ImVec2( 0.5f , 0.5f ) );

		if ( ImGui::BeginPopupModal( _( "Save Config##popup" ) , nullptr , ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings ) ) {
			ImGui::TextColored( ImVec4( 1.0f , 0.1f , 0.1f , 1.0f ) , _( "There already is a config with the same name in this location.\nAre you sure you want to overwrite the config?" ) );

			if ( ImGui::Button( _( "Confirm" ) , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) ) ) {
				char appdata[ MAX_PATH ];

				if ( SUCCEEDED( LI_FN( SHGetFolderPathA )( nullptr , 5 , nullptr , 0 , appdata ) ) ) {
					LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex" ) ).c_str( ) , nullptr );
					LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex\\configs" ) ).c_str( ) , nullptr );
				}

				const auto file = std::string( appdata ).append( _( "\\sesame_apex\\configs\\" ) ).append( selected_config ).append( _( ".xml" ) );

				options::save( options::vars , file );

				load_cfg_list( );

				//cs::i::engine->client_cmd_unrestricted( _( "play ui\\buttonclick" ) );

				ImGui::CloseCurrentPopup( );
			}

			ImGui::SameLine( );

			if ( ImGui::Button( "Cancel" , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) ) ) {
				ImGui::CloseCurrentPopup( );
			}

			ImGui::EndPopup( );
		}

		ImGui::SetNextWindowPos( ImVec2( ImGui::GetWindowPos( ).x + ImGui::GetWindowSize( ).x * 0.5f , ImGui::GetWindowPos( ).y + ImGui::GetWindowSize( ).y * 0.5f ) , ImGuiCond_Always , ImVec2( 0.5f , 0.5f ) );

		if ( ImGui::BeginPopupModal( _( "Delete Config##popup" ) , nullptr , ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings ) ) {
			ImGui::TextColored( ImVec4( 1.0f , 0.1f , 0.1f , 1.0f ) , _( "Are you sure you want to delete the config?" ) );

			if ( ImGui::Button( _( "Confirm" ) , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) ) ) {
				char appdata[ MAX_PATH ];

				if ( SUCCEEDED( LI_FN( SHGetFolderPathA )( nullptr , 5 , nullptr , 0 , appdata ) ) ) {
					LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex" ) ).c_str( ) , nullptr );
					LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex\\configs" ) ).c_str( ) , nullptr );
				}

				const auto file = std::string( appdata ).append( _( "\\sesame_apex\\configs\\" ) ).append( selected_config ).append( _( ".xml" ) );

				std::remove( ( std::string( appdata ) + _( "\\sesame_apex\\configs\\" ) + selected_config + _( ".xml" ) ).c_str( ) );

				load_cfg_list( );

				//cs::i::engine->client_cmd_unrestricted( _( "play ui\\buttonclick" ) );

				ImGui::CloseCurrentPopup( );
			}

			ImGui::SameLine( );

			if ( ImGui::Button( "Cancel" , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) ) ) {
				ImGui::CloseCurrentPopup( );
			}

			ImGui::EndPopup( );
		}

		bool open_save_modal = false;
		bool open_delete_modal = false;

		switch ( cur_tab_idx ) {
		case 0: {
				ImGui::custom::AddSubtab(_( "General") ,_( "General aimbot and accuracy settings") , [ & ] ( ) {
					ImGui::BeginChildFrame( ImGui::GetID( _( "General Settings" ) ) , ImVec2( ImGui::GetWindowContentRegionWidth( ) , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
						ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize( _( "General Settings" ) ).x * 0.5f );
						ImGui::Text( _( "General Settings" ) );
						ImGui::Separator( );

						ImGui::Checkbox( _( "Inherit Default" ) , &options::vars[ _( "aimbot.enabled" ) ].val.b );
						ImGui::SameLine( );
						ImGui::Keybind( _( "Inherit Default" ) , &options::vars[ _( "aimbot.key" ) ].val.i, &options::vars[ _( "aimbot.key_mode" ) ].val.i );

						ImGui::EndChildFrame( );
					}
					} );

			ImGui::custom::AddSubtab( _("Default") , _("Default settings used for unconfigured weapons") , [ & ] ( ) {
				weapon_controls( _("default"));
				} );

			ImGui::custom::AddSubtab(_( "Pistol") , _("Pistol class configuration" ), [ & ] ( ) {
				weapon_controls( _( "pistol" ) );
				} );
			ImGui::custom::AddSubtab( _("Shotgun" ), _("Shotgun class configuration" ), [ & ] ( ) {
				weapon_controls( _( "shotgun" ) );
				} );
			ImGui::custom::AddSubtab( _("SMG" ), _("SMG class configuration" ), [ & ] ( ) {
				weapon_controls( _( "smg" ) );
				} );
			ImGui::custom::AddSubtab( _("Rifle" ), _("Rifle class configuration") , [ & ] ( ) {
				weapon_controls( _( "rifle" ) );
				} );
			ImGui::custom::AddSubtab( _( "Sniper" ) , _( "Sniper class configuration" ) , [ & ] ( ) {
				weapon_controls( _( "sniper" ) );
				} );
			ImGui::custom::AddSubtab( _( "LMG" ) , _( "LMG class configuration" ) , [ & ] ( ) {
				weapon_controls( _( "lmg" ) );
				} );
			ImGui::custom::AddSubtab( _( "Special" ) , _( "Special weapon configuration" ) , [ & ] ( ) {
				weapon_controls( _( "special" ) );
				} );
			
		} break;
		case 1: {
			ImGui::custom::AddSubtab( _("Enemies") , _("Visuals used on filtered enemies") , [ & ] ( ) {
				player_visuals_controls( _("teammates"));
				} );
			ImGui::custom::AddSubtab(_( "Teammates") , _("Visuals used on filtered teammates" ), [ & ] ( ) {
				player_visuals_controls( _( "enemies" ) );
				} );
			ImGui::custom::AddSubtab( _( "Weapons" ) , _( "Visuals used on filtered weapons" ) , [ & ] ( ) {
				player_visuals_controls( _( "weapons" ) );
				} );
			ImGui::custom::AddSubtab( _("Other") ,_( "Other visual options" ), [ & ] ( ) {
				ImGui::BeginChildFrame( ImGui::GetID(_( "Other") ) , ImVec2( ImGui::GetWindowContentRegionWidth( ) , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
					ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize( _("Other") ).x * 0.5f );
					ImGui::Text( _("Other" ));
					ImGui::Separator( );

					static std::vector<const char*> removals {
								"IDK" 
					};

					ImGui::MultiCombo( _( "Removals" ) , options::vars[ _( "visuals.other.removals" ) ].val.l , removals.data( ) , removals.size( ) );
					ImGui::PushItemWidth( -1.0f );
					ImGui::SliderFloat( _( "FOV" ) , &options::vars[ _( "visuals.other.fov" ) ].val.f , 0.0f , 180.0f , ( char* ) _( "%.1f°" ) );
					ImGui::SliderFloat( _( "Viewmodel FOV" ) , &options::vars[ _( "visuals.other.viewmodel_fov" ) ].val.f , 0.0f , 180.0f , _( "%.1f°" ) );
					ImGui::SliderFloat( _( "Viewmodel Offset X" ) , &options::vars[ _( "visuals.other.viewmodel_offset_x" ) ].val.f , -10.0f , 10.0f , _( "%.1f units" ) );
					ImGui::SliderFloat( _( "Viewmodel Offset Y" ) , &options::vars[ _( "visuals.other.viewmodel_offset_y" ) ].val.f , -10.0f , 10.0f , _( "%.1f units" ) );
					ImGui::SliderFloat( _( "Viewmodel Offset Z" ) , &options::vars[ _( "visuals.other.viewmodel_offset_z" ) ].val.f , -10.0f , 10.0f , _( "%.1f units" ) );
					ImGui::SliderFloat( _( "Aspect Ratio" ) , &options::vars[ _( "visuals.other.aspect_ratio" ) ].val.f , 0.1f , 2.0f );
					ImGui::PopItemWidth( );

					ImGui::Checkbox( _( "Offscreen ESP" ) , &options::vars[ _( "visuals.other.offscreen_esp" ) ].val.b );
					ImGui::SameLine( );
					ImGui::ColorEdit4( _( "##Offscreen ESP Color" ) , ( float* ) &options::vars[ _( "visuals.other.offscreen_esp_color" ) ].val.c );
					ImGui::PushItemWidth( -1.0f );
					ImGui::SliderFloat( _( "Offscreen ESP Distance" ) , &options::vars[ _( "visuals.other.offscreen_esp_distance" ) ].val.f , 0.0f , 100.0f , _( "%.1f%%" ) );
					ImGui::SliderFloat( _( "Offscreen ESP Size" ) , &options::vars[ _( "visuals.other.offscreen_esp_size" ) ].val.f , 0.0f , 100.0f , ( std::to_string( static_cast< int >( options::vars[ _( "visuals.other.offscreen_esp_size" ) ].val.f ) ) + _( " px" ) ).c_str( ) );
					ImGui::PopItemWidth( );

					ImGui::Checkbox( _( "Watermark" ) , &options::vars[ _( "visuals.other.watermark" ) ].val.b );
					ImGui::Checkbox( _( "Keybind List" ) , &options::vars[ _( "visuals.other.keybind_list" ) ].val.b );

					ImGui::EndChildFrame( );
				}
				} );
		} break;
		case 2: {
				ImGui::custom::AddSubtab( _("Skin Changer" ), _("Change your gun skins! (clientsided only)" ), [ & ] ( ) {
				
					} );
		} break;
		case 3: {
				ImGui::custom::AddSubtab(_( "Movement") ,_( "Movement related cheats") , [ & ] ( ) {
				ImGui::BeginChildFrame( ImGui::GetID( _("General") ) , ImVec2( ImGui::GetWindowContentRegionWidth( ) , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
					ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize( _("General") ).x * 0.5f );
					ImGui::Text(_( "General") );
					ImGui::Separator( );

					ImGui::Checkbox( _( "Bhop" ) , &options::vars[ _( "misc.movement.bhop" ) ].val.b );
					ImGui::Checkbox( _( "Accurate Movements" ) , &options::vars[ _( "misc.movement.accurate_move" ) ].val.b );

					ImGui::EndChildFrame( );
				}
					} );
			ImGui::custom::AddSubtab(_( "Effects") , _("Miscellaneous visual effects" ), [ & ] ( ) {
				ImGui::BeginChildFrame( ImGui::GetID( _("General" )) , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
					ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize( _("General") ).x * 0.5f );
					ImGui::Text( _("General") );
					ImGui::Separator( );

					ImGui::Checkbox( _( "Third Person" ) , &options::vars[ _( "misc.effects.third_person" ) ].val.b );
					ImGui::SameLine( );
					ImGui::Keybind( _( "##Third Person Key" ) , &options::vars[ _( "misc.effects.third_person_key" ) ].val.i , &options::vars[ _( "misc.effects.third_person_key_mode" ) ].val.i , ImVec2( -1.0f , 0.0f ) );
					ImGui::PushItemWidth( -1.0f );
					ImGui::SliderFloat( _( "Third Person Range" ) , &options::vars[ _( "misc.effects.third_person_range" ) ].val.f , 0.0f , 500.0f , _( "%.1f units" ) );

					ImGui::EndChildFrame( );
				}
				} );
			ImGui::custom::AddSubtab( _("Player List") ,_( "Whitelist, clantag stealer, and bodyaim priority" ), [ & ] ( ) {
				ImGui::BeginChildFrame( ImGui::GetID( _("Players") ) , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
					ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize(_( "Players") ).x * 0.5f );
					ImGui::Text( _("Players" ));
					ImGui::Separator( );

					ImGui::EndChildFrame( );
				}

				ImGui::SameLine( );

				ImGui::BeginChildFrame( ImGui::GetID(_( "Player Actions") ) , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
					ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize( _("Player Actions" )).x * 0.5f );
					ImGui::Text( _("Player Actions" ));
					ImGui::Separator( );

					ImGui::EndChildFrame( );
				}
				} );
			ImGui::custom::AddSubtab( _("Cheat" ), _("Cheat settings and panic button") , [ & ] ( ) {
				ImGui::BeginChildFrame( ImGui::GetID(_( "Menu" )) , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
					ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize(_( "Menu") ).x * 0.5f );
					ImGui::Text( _("Menu") );
					ImGui::Separator( );

					//int dpi = ( options::vars [ _ ( "gui.dpi" ) ].val.f < 1.0f ) ? 0 : static_cast< int >( options::vars [ _ ( "gui.dpi" ) ].val.f );
					//static std::vector<const char*> dpis { "0.5",  "1.0", "2.0" , "3.0" };
					//
					//ImGui::PushItemWidth ( -1.0f );
					//ImGui::Combo ( _ ( "GUI DPI" ), &dpi, dpis.data ( ), dpis.size ( ) );
					//ImGui::PopItemWidth ( );
					//
					//switch ( dpi ) {
					//case 0: options::vars [ _ ( "gui.dpi" ) ].val.f = 0.5f; break;
					//case 1: options::vars [ _ ( "gui.dpi" ) ].val.f = 1.0f; break;
					//case 2: options::vars [ _ ( "gui.dpi" ) ].val.f = 2.0f; break;
					//case 3: options::vars [ _ ( "gui.dpi" ) ].val.f = 3.0f; break;
					//}

					ImGui::EndChildFrame( );
				}

				ImGui::SameLine( );

				ImGui::BeginChildFrame( ImGui::GetID( _("Cheat") ) , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
					ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize(_( "Cheat") ).x * 0.5f );
					ImGui::Text( _("Cheat") );
					ImGui::Separator( );

					ImGui::EndChildFrame( );
				}
				} );

			ImGui::custom::AddSubtab( _("Configuration" ),_( "Cheat configuration manager" ), [ & ] ( ) {
				ImGui::BeginChildFrame( ImGui::GetID( _("Configs" )) , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
					ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize(_( "Configs" )).x * 0.5f );
					ImGui::Text(_( "Configs") );
					ImGui::Separator( );

					for ( const auto& config : configs ) {
						if ( ImGui::Button( config.data( ) , ImVec2( -1.0f , 0.0f ) ) )
							strcpy_s( selected_config , config.c_str( ) );
					}

					ImGui::EndChildFrame( );
				}

				ImGui::SameLine( );

				ImGui::BeginChildFrame( ImGui::GetID( _("Config Actions") ) , ImVec2( ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::GetStyle( ).FramePadding.x , 0.0f ) , ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ); {
					ImGui::SetCursorPosX( ImGui::GetCursorPosX( ) + ImGui::GetWindowContentRegionWidth( ) * 0.5f - ImGui::CalcTextSize(_( "Config Actions" )).x * 0.5f );
					ImGui::Text( _("Config Actions" ));
					ImGui::Separator( );

					ImGui::PushItemWidth( -1.0f );
					ImGui::InputText( _( "Config Name" ) , selected_config , sizeof( selected_config ) );
					ImGui::PopItemWidth( );

					if ( ImGui::Button( _( "Save" ) , ImVec2( -1.0f , 0.0f ) ) ) {
						char appdata[ MAX_PATH ];

						if ( SUCCEEDED( LI_FN( SHGetFolderPathA )( nullptr , 5 , nullptr , 0 , appdata ) ) ) {
							LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex" ) ).c_str( ) , nullptr );
							LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex\\configs" ) ).c_str( ) , nullptr );
						}

						auto file_exists = [ ] ( const std::string& path ) {
							std::ifstream file( path );
							return file.good( );
						};

						const auto file = std::string( appdata ).append( _( "\\sesame_apex\\configs\\" ) ).append( selected_config ).append( _( ".xml" ) );

						if ( file_exists( file ) ) {
							open_save_modal = true;
						}
						else {
							options::save( options::vars , file );

							load_cfg_list( );

							//cs::i::engine->client_cmd_unrestricted( _( "play ui\\buttonclick" ) );
						}
					}

					if ( ImGui::Button( _( "Load" ) , ImVec2( -1.0f , 0.0f ) ) ) {
						char appdata[ MAX_PATH ];

						if ( SUCCEEDED( LI_FN( SHGetFolderPathA )( nullptr , 5 , nullptr , 0 , appdata ) ) ) {
							LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex" ) ).c_str( ) , nullptr );
							LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex\\configs" ) ).c_str( ) , nullptr );
						}

						options::load( options::vars , std::string( appdata ) + _( "\\sesame_apex\\configs\\" ) + selected_config + _( ".xml" ) );

						//cs::i::engine->client_cmd_unrestricted( _( "play ui\\buttonclick" ) );
					}

					if ( ImGui::Button( _( "Delete" ) , ImVec2( -1.0f , 0.0f ) ) ) {
						char appdata[ MAX_PATH ];

						if ( SUCCEEDED( LI_FN( SHGetFolderPathA )( nullptr , 5 , nullptr , 0 , appdata ) ) ) {
							LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex" ) ).c_str( ) , nullptr );
							LI_FN( CreateDirectoryA )( ( std::string( appdata ) + _( "\\sesame_apex\\configs" ) ).c_str( ) , nullptr );
						}

						auto file_exists = [ ] ( const std::string& path ) {
							std::ifstream file( path );
							return file.good( );
						};

						const auto file = std::string( appdata ).append( _( "\\sesame_apex\\configs\\" ) ).append( selected_config ).append( _( ".xml" ) );

						if ( file_exists( file ) ) {
							open_delete_modal = true;
						}
						else {
							std::remove( ( std::string( appdata ) + _( "\\sesame_apex\\configs\\" ) + selected_config + _( ".xml" ) ).c_str( ) );

							load_cfg_list( );

							//cs::i::engine->client_cmd_unrestricted( _( "play ui\\buttonclick" ) );
						}
					}

					if ( ImGui::Button( _( "Refresh List" ) , ImVec2( -1.0f , 0.0f ) ) ) {
						load_cfg_list( );
						//cs::i::engine->client_cmd_unrestricted( _( "play ui\\buttonclick" ) );
					}

					ImGui::EndChildFrame( );
				}
				} );
		} break;
		}

		if ( open_save_modal )
			ImGui::OpenPopup( _( "Save Config##popup" ) );

		if ( open_delete_modal )
			ImGui::OpenPopup( _( "Delete Config##popup" ) );

		ImGui::custom::End( );
	}

	ImGui::PopFont( );
}