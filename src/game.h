/**
 *    Name:        game.h
 *    Description: prototypes of the game's functions and the addresses of the code it keeps in memory slots
 *    Version:     0.1.0
 *    Created:     12-10-2026
 *    Modified:    23-10-2026
 *    Author:      Sebastian Toyos
 *
 *    SegaSonic The Hedgehog (arcade, Sega System 32): the game's V60 program decompiled into C by
 *    tools/decompile.py, with the names of symbols.txt.
 */
#ifndef SONIC_GAME_H
#define SONIC_GAME_H

#include "game/support.h"
#include "ram.h"

/* registers the whole program shares */
extern uint32_t r14, current_player, current_object, free_objects, first_object, object_cursor, last_object, fill_value, stack_pointer;

/* where the code the program keeps in memory slots (objects' handlers, hooks) is, as the game's own
   addresses: what it stores and compares, and what call_code() runs */
#define cleared_banner_wait_ADDRESS 0x001664u
#define stage_clear_banner_reveal_ADDRESS 0x001674u
#define stage_clear_banner_hold_ADDRESS 0x0016B6u
#define banner_sparkle_animate_ADDRESS 0x0017B6u
#define banner_letter_spin_ADDRESS 0x0017D1u
#define player_1_start_ADDRESS 0x0027F2u
#define player_2_start_ADDRESS 0x0027FAu
#define player_3_start_ADDRESS 0x002802u
#define player_resume_ADDRESS 0x0028D1u
#define player_join_ADDRESS 0x0028DBu
#define player_start_stage_ADDRESS 0x00291Fu
#define player_respawn_ADDRESS 0x00294Au
#define player_reset_joined_ADDRESS 0x00299Cu
#define player_place_ADDRESS 0x002BC5u
#define player_update_ADDRESS 0x002CECu
#define blown_away_effect_update_ADDRESS 0x006B96u
#define stuck_effect_update_ADDRESS 0x007A1Eu
#define trap_flash_start_ADDRESS 0x0083F7u
#define trap_flash_update_ADDRESS 0x008407u
#define trap_start_ADDRESS 0x008426u
#define trap_update_ADDRESS 0x008436u
#define climb_hand_update_ADDRESS 0x008C69u
#define player_effect_start_ADDRESS 0x009A62u
#define player_effect_update_ADDRESS 0x009A6Fu
#define struggle_marker_update_ADDRESS 0x009ADCu
#define scene_object_start_ADDRESS 0x009B4Du
#define scene_object_update_ADDRESS 0x009B8Eu
#define sinking_effect_update_ADDRESS 0x00A3F8u
#define skid_dust_update_ADDRESS 0x00A6D5u
#define slip_effect_update_ADDRESS 0x00B28Fu
#define shadow_1_start_ADDRESS 0x00BE20u
#define shadow_2_start_ADDRESS 0x00BE28u
#define shadow_3_start_ADDRESS 0x00BE30u
#define shadow_update_ADDRESS 0x00BED5u
#define tumble_effect_update_ADDRESS 0x00D22Eu
#define slip_piece_update_ADDRESS 0x00DB69u
#define slip_pieces_update_ADDRESS 0x00DC69u
#define climb_zone_effect_start_ADDRESS 0x00E2C3u
#define climb_zone_effect_update_ADDRESS 0x00E2D2u
#define intro_marker_update_ADDRESS 0x00E36Du
#define vine_init_ADDRESS 0x00EC9Eu
#define vine_swing_ADDRESS 0x00EDA9u
#define vine_bar_update_ADDRESS 0x00EED2u
#define vine_scroll_stop_init_ADDRESS 0x00EFCCu
#define vine_scroll_stop_wait_ADDRESS 0x00EFD8u
#define vine_scroll_stop_hold_ADDRESS 0x00EFFAu
#define vine_scroll_stop_end_ADDRESS 0x00F054u
#define spike_panel_init_ADDRESS 0x00F087u
#define spike_panel_hold_ADDRESS 0x00F1A9u
#define spike_panel_update_ADDRESS 0x00F1F1u
#define spike_panel_base_draw_ADDRESS 0x00F3F2u
#define spike_panel_hitbox_update_ADDRESS 0x00F412u
#define spike_panel_part_idle_ADDRESS 0x00F46Au
#define electric_pole_init_ADDRESS 0x00F53Au
#define electric_pole_wait_ADDRESS 0x00F59Eu
#define electric_pole_rise_ADDRESS 0x00F5B4u
#define electric_pole_update_ADDRESS 0x00F5F1u
#define metal_wall_init_ADDRESS 0x00F77Du
#define metal_wall_wait_ADDRESS 0x00F878u
#define metal_wall_rise_ADDRESS 0x00F8B2u
#define metal_wall_update_ADDRESS 0x00F8E1u
#define metal_wall_broken_ADDRESS 0x00F97Au
#define flame_machine_init_ADDRESS 0x00FBE4u
#define flame_machine_update_ADDRESS 0x00FE46u
#define flame_machine_remove_ADDRESS 0x00FFA0u
#define flame_machine_explosion_start_ADDRESS 0x01000Bu
#define flame_machine_back_wall_update_ADDRESS 0x01004Du
#define flame_machine_arm_update_ADDRESS 0x0101C2u
#define flame_machine_arm_rest_ADDRESS 0x010240u
#define flame_machine_cannon_update_ADDRESS 0x0103F0u
#define flame_machine_flame_burn_ADDRESS 0x0104C6u
#define spiked_disc_hold_ADDRESS 0x01069Du
#define spiked_disc_drop_ADDRESS 0x0106D4u
#define spiked_disc_roll_ADDRESS 0x010705u
#define flame_machine_scroll_stop_wait_ADDRESS 0x0107EBu
#define flame_machine_scroll_stop_hold_ADDRESS 0x010802u
#define object_type_9_idle_ADDRESS 0x010814u
#define flame_machine_wreck_draw_ADDRESS 0x010870u
#define treadmill_init_ADDRESS 0x0108D4u
#define treadmill_update_ADDRESS 0x010AD7u
#define treadmill_roller_update_ADDRESS 0x010BDCu
#define treadmill_gate_update_ADDRESS 0x010C53u
#define treadmill_gate_half_update_ADDRESS 0x010D74u
#define flame_jet_init_ADDRESS 0x010DD3u
#define flame_jet_nozzle_update_ADDRESS 0x010F59u
#define flame_jet_pilot_ADDRESS 0x010FE2u
#define flame_jet_ignite_ADDRESS 0x011031u
#define flame_jet_burn_ADDRESS 0x011049u
#define push_crate_init_ADDRESS 0x0111AFu
#define push_crate_update_ADDRESS 0x011259u
#define push_crate_break_ADDRESS 0x0112D8u
#define push_crate_move_ADDRESS 0x01130Eu
#define push_crate_stop_post_update_ADDRESS 0x011378u
#define sign_start_from_type_ADDRESS 0x01140Fu
#define handle_rod_init_ADDRESS 0x011437u
#define handle_rod_handle_init_ADDRESS 0x0115FCu
#define handle_rod_wait_ADDRESS 0x011629u
#define handle_rod_start_swing_ADDRESS 0x011662u
#define handle_rod_swing_ADDRESS 0x011675u
#define handle_rod_hang_ADDRESS 0x011767u
#define handle_rod_remove_ADDRESS 0x0117B2u
#define handle_rod_handle_update_ADDRESS 0x0117B9u
#define handle_rod_post_init_ADDRESS 0x0118D0u
#define handle_rod_post_update_ADDRESS 0x01191Du
#define handle_rod_post_fall_ADDRESS 0x011989u
#define spear_trap_init_100_ADDRESS 0x0119A7u
#define spear_trap_init_300_ADDRESS 0x0119B0u
#define spear_trap_wait_ADDRESS 0x011AF3u
#define spear_trap_rise_ADDRESS 0x011B1Eu
#define spear_trap_stay_up_ADDRESS 0x011B54u
#define spear_trap_retract_ADDRESS 0x011B61u
#define ice_launcher_init_ADDRESS 0x011CB5u
#define ice_launcher_update_ADDRESS 0x011CE7u
#define ice_cube_slide_ADDRESS 0x011EABu
#define object_type_17_idle_ADDRESS 0x011F1Fu
#define padded_wall_init_ADDRESS 0x011F20u
#define padded_wall_update_ADDRESS 0x011FDEu
#define wall_pad_update_ADDRESS 0x011FFCu
#define cactus_init_ADDRESS 0x012086u
#define cactus_update_ADDRESS 0x0120C2u
#define whirlwind_aim_ADDRESS 0x0121C9u
#define whirlwind_move_ADDRESS 0x012220u
#define whirlwind_vanish_ADDRESS 0x01224Au
#define centipede_init_ADDRESS 0x012309u
#define centipede_chase_ADDRESS 0x0123CCu
#define centipede_leave_ADDRESS 0x0123E6u
#define centipede_segment_wait_ADDRESS 0x01254Eu
#define centipede_segment_follow_ADDRESS 0x012561u
#define centipede_tail_wait_ADDRESS 0x012595u
#define centipede_tail_follow_ADDRESS 0x0125A8u
#define sand_puff_start_ADDRESS 0x0126CDu
#define sand_puff_animate_ADDRESS 0x0126E1u
#define crawler_init_ADDRESS 0x01274Cu
#define crawler_head_update_ADDRESS 0x0128EAu
#define crawler_segment_animate_ADDRESS 0x01298Cu
#define crawler_segment_update_ADDRESS 0x0129CEu
#define eggman_booth_slide_in_ADDRESS 0x012B79u
#define eggman_booth_animate_ADDRESS 0x012B9Eu
#define object_type_4_idle_ADDRESS 0x012C80u
#define wall_piece_init_ADDRESS 0x012C81u
#define wall_piece_draw_ADDRESS 0x012CC6u
#define shaking_block_init_ADDRESS 0x012D2Cu
#define shaking_block_animate_ADDRESS 0x012D9Cu
#define sand_rock_init_ADDRESS 0x012E54u
#define sand_rock_update_ADDRESS 0x012EA9u
#define snow_mound_init_ADDRESS 0x012FB9u
#define snow_mound_update_ADDRESS 0x013056u
#define rock_wall_init_ADDRESS 0x013207u
#define rock_wall_update_ADDRESS 0x013263u
#define eggman_teleport_init_ADDRESS 0x01343Cu
#define eggman_teleport_animate_ADDRESS 0x013464u
#define sand_arch_init_ADDRESS 0x013637u
#define sand_arch_update_ADDRESS 0x01370Eu
#define sand_arch_ball_init_ADDRESS 0x01374Au
#define sand_arch_ball_delay_ADDRESS 0x013782u
#define sand_arch_ball_move_ADDRESS 0x0137A9u
#define sand_arch_ball_gone_ADDRESS 0x013899u
#define fence_post_init_ADDRESS 0x0138C6u
#define fence_post_start_ADDRESS 0x01399Du
#define fence_post_update_ADDRESS 0x0139B1u
#define fence_post_draw_ADDRESS 0x013A35u
#define bubble_column_init_ADDRESS 0x013ADDu
#define bubble_column_animate_ADDRESS 0x013AF9u
#define mine_init_ADDRESS 0x013B8Fu
#define mine_float_ADDRESS 0x013C2Au
#define mine_fall_ADDRESS 0x013C66u
#define mine_sink_ADDRESS 0x013CDDu
#define mine_explode_ADDRESS 0x013D26u
#define mine_splash_animate_ADDRESS 0x013F7Du
#define eggman_scene_init_ADDRESS 0x014054u
#define eggman_scene_wait_ADDRESS 0x0140B8u
#define eggman_scene_pause_ADDRESS 0x0140D6u
#define eggman_scene_talk_ADDRESS 0x0140F0u
#define eggman_scene_press_button_ADDRESS 0x014177u
#define eggman_scene_run_off_ADDRESS 0x0141C9u
#define button_box_init_ADDRESS 0x0142A0u
#define button_box_wait_ADDRESS 0x0142BEu
#define button_box_rise_ADDRESS 0x0142D4u
#define button_box_press_ADDRESS 0x0142F3u
#define draw_button_box_ADDRESS 0x01432Bu
#define sliding_door_init_ADDRESS 0x0143CCu
#define sliding_door_wait_ADDRESS 0x01444Eu
#define sliding_door_open_ADDRESS 0x014490u
#define sliding_door_draw_ADDRESS 0x0144CDu
#define lid_trap_init_ADDRESS 0x01453Eu
#define lid_trap_update_ADDRESS 0x01459Eu
#define turtle_init_ADDRESS 0x014651u
#define turtle_wait_ADDRESS 0x0146C9u
#define turtle_surface_ADDRESS 0x014780u
#define turtle_drop_rings_ADDRESS 0x0147B5u
#define turtle_swim_ADDRESS 0x0147F2u
#define eel_init_ADDRESS 0x014971u
#define eel_anchor_update_ADDRESS 0x014B46u
#define eel_anchor_wait_explode_ADDRESS 0x014B59u
#define eel_head_move_ADDRESS 0x014C5Au
#define eel_segment_move_ADDRESS 0x014D64u
#define eel_head_explode_delay_ADDRESS 0x014ED5u
#define eel_segment_explode_delay_ADDRESS 0x014EF8u
#define eel_piece_explode_ADDRESS 0x014F5Du
#define eel_piece_gone_ADDRESS 0x014FB7u
#define breakable_panel_init_ADDRESS 0x01503Cu
#define breakable_panel_wait_ADDRESS 0x01508Du
#define breakable_panel_break_ADDRESS 0x0150C7u
#define draw_breakable_panel_ADDRESS 0x0150F4u
#define white_puff_init_ADDRESS 0x0151DDu
#define white_puff_animate_ADDRESS 0x0151F3u
#define checker_pad_init_ADDRESS 0x01524Du
#define checker_pad_animate_ADDRESS 0x01525Cu
#define static_triangle_init_ADDRESS 0x0152AAu
#define static_triangle_animate_ADDRESS 0x0152CBu
#define railing_init_ADDRESS 0x015341u
#define railing_draw_ADDRESS 0x0153C6u
#define railing_side_update_ADDRESS 0x0153E5u
#define spike_gate_init_ADDRESS 0x0153F2u
#define spike_gate_pillar_update_ADDRESS 0x015544u
#define spike_gate_beam_rise_ADDRESS 0x01559Cu
#define spike_gate_beam_hold_ADDRESS 0x0155CBu
#define spike_gate_beam_drop_ADDRESS 0x0155D8u
#define spike_gate_beam_wait_ADDRESS 0x015623u
#define eel_den_init_ADDRESS 0x015706u
#define eel_den_wait_ADDRESS 0x015756u
#define eel_den_collapse_ADDRESS 0x0157C0u
#define eel_den_draw_ADDRESS 0x0157EDu
#define stepped_wall_init_ADDRESS 0x01586Eu
#define stepped_wall_draw_ADDRESS 0x0158BBu
#define debug_sprite_mover_update_ADDRESS 0x016C3Du
#define escape_alarm_flash_ADDRESS 0x01A6E1u
#define ending_tower_animate_ADDRESS 0x01BF2Du
#define ending_debris_1_fly_ADDRESS 0x01BFC4u
#define ending_debris_2_fly_ADDRESS 0x01BFF2u
#define ending_debris_3_fly_ADDRESS 0x01C020u
#define ending_tower_blast_1_ADDRESS 0x01C04Eu
#define ending_tower_blast_2_ADDRESS 0x01C072u
#define ending_tower_blast_3_ADDRESS 0x01C096u
#define ending_horizon_blast_1_ADDRESS 0x01C0BAu
#define ending_horizon_blast_2_ADDRESS 0x01C0DEu
#define ending_horizon_blast_3_ADDRESS 0x01C102u
#define ending_horizon_blast_4_ADDRESS 0x01C126u
#define ending_horizon_blast_5_ADDRESS 0x01C14Au
#define ending_horizon_blast_6_ADDRESS 0x01C16Eu
#define ending_horizon_blast_7_ADDRESS 0x01C192u
#define ending_horizon_blast_8_ADDRESS 0x01C1B6u
#define ending_horizon_blast_9_ADDRESS 0x01C1DAu
#define ending_big_explosion_ADDRESS 0x01C20Eu
#define ending_flash_to_white_ADDRESS 0x01C2F3u
#define ending_horizon_show_ADDRESS 0x01C53Au
#define ending_dust_cloud_rise_ADDRESS 0x01C5BBu
#define ending_sonic_run_forward_ADDRESS 0x01C68Au
#define ending_ray_run_forward_ADDRESS 0x01C6E8u
#define ending_mighty_run_forward_ADDRESS 0x01C74Cu
#define ending_road_scroll_ADDRESS 0x01C7B0u
#define ending_credits_fade_in_ADDRESS 0x01C82Fu
#define ending_credits_spawner_run_ADDRESS 0x01C91Du
#define ending_music_play_ADDRESS 0x01C968u
#define ending_portrait_show_ADDRESS 0x01C9D2u
#define ending_name_plate_show_ADDRESS 0x01CAACu
#define ending_credits_team_logo_scroll_ADDRESS 0x01CB98u
#define ending_credits_director_scroll_ADDRESS 0x01CC4Bu
#define ending_credits_program_scroll_ADDRESS 0x01CD17u
#define ending_credits_design_scroll_ADDRESS 0x01CE15u
#define ending_credits_sound_scroll_ADDRESS 0x01CF2Cu
#define ending_credits_voice_actor_scroll_ADDRESS 0x01D02Au
#define ending_credits_special_thanks_scroll_ADDRESS 0x01D141u
#define ending_window_open_ADDRESS 0x01D3E7u
#define ending_credits_fade_out_ADDRESS 0x01D49Au
#define ending_eggman_scene_fade_in_ADDRESS 0x01D4F3u
#define ending_eggman_fly_in_ADDRESS 0x01D59Au
#define ending_eggman_turn_ADDRESS 0x01D8F8u
#define ending_eggman_scene_fade_out_ADDRESS 0x01D96Fu
#define ending_close_up_fade_in_ADDRESS 0x01D9DCu
#define ending_eggman_fly_away_ADDRESS 0x01DA21u
#define ending_sonic_run_across_ADDRESS 0x01DDF4u
#define ending_ray_run_across_ADDRESS 0x01DE86u
#define ending_mighty_run_across_ADDRESS 0x01DF00u
#define ring_spin_ADDRESS 0x01E4EDu
#define still_ring_wait_ADDRESS 0x01E5ADu
#define carried_ring_wait_ADDRESS 0x01E5FAu
#define carried_ring_spin_ADDRESS 0x01E61Fu
#define scattered_ring_wait_ADDRESS 0x01E85Bu
#define scattered_ring_live_ADDRESS 0x01E86Eu
#define scattered_ring_vanish_ADDRESS 0x01E9C1u
#define sparkle_start_ADDRESS 0x01EB04u
#define sparkle_animate_ADDRESS 0x01EB11u
#define gauge_ring_update_ADDRESS 0x01EC15u
#define spawn_objects_by_handler_ADDRESS 0x080000u
#define stage_03_floor_block_start_ADDRESS 0x080072u
#define stage_03_floor_block_update_ADDRESS 0x0800FBu
#define spiked_roller_start_ADDRESS 0x080160u
#define spiked_roller_update_ADDRESS 0x080372u
#define roller_fixed_zone_update_ADDRESS 0x080E59u
#define catapult_x_start_ADDRESS 0x080E75u
#define catapult_y_start_ADDRESS 0x080E8Eu
#define catapult_wait_for_platform_ADDRESS 0x080EA8u
#define catapult_update_ADDRESS 0x08104Eu
#define catapult_base_update_ADDRESS 0x0811DDu
#define catapult_dummy_update_ADDRESS 0x081216u
#define goal_unlocker_start_ADDRESS 0x081217u
#define arrow_board_start_ADDRESS 0x08239Cu
#define arrow_board_wait_for_platform_ADDRESS 0x0823CAu
#define arrow_board_ride_ADDRESS 0x0823E0u
#define pebble_start_ADDRESS 0x082434u
#define pebble_update_ADDRESS 0x08246Eu
#define propped_board_start_ADDRESS 0x082493u
#define propped_board_update_ADDRESS 0x0825A8u
#define board_prop_start_ADDRESS 0x082669u
#define board_prop_update_ADDRESS 0x082679u
#define boulder_start_ADDRESS 0x082712u
#define boulder_wait_ADDRESS 0x0827A9u
#define boulder_roll_ADDRESS 0x0827D3u
#define door_wall_start_ADDRESS 0x08288Au
#define door_wall_wait_to_burst_ADDRESS 0x0828E4u
#define door_roller_update_ADDRESS 0x082A94u
#define fireball_emitter_start_ADDRESS 0x082B7Du
#define fireball_emitter_update_ADDRESS 0x082BC7u
#define bouncing_fireball_update_ADDRESS 0x082D36u
#define knocked_object_fly_ADDRESS 0x082E37u
#define chaser_back_zone_check_parent_ADDRESS 0x082EBEu
#define chaser_back_zone_update_ADDRESS 0x082ED3u
#define roller_back_zone_update_ADDRESS 0x082FEDu
#define machine_start_ADDRESS 0x08343Eu
#define machine_run_ADDRESS 0x083536u
#define machine_fire_update_ADDRESS 0x08357Fu
#define fireball_start_ADDRESS 0x0839F7u
#define fireball_fly_ADDRESS 0x083A99u
#define flame_start_ADDRESS 0x083B35u
#define flame_burn_ADDRESS 0x083B80u
#define rising_flame_update_ADDRESS 0x083BABu
#define how_to_play_button_hand_start_ADDRESS 0x083CE8u
#define how_to_play_button_hand_update_ADDRESS 0x083D3Bu
#define how_to_play_trackball_hand_start_ADDRESS 0x083D95u
#define how_to_play_trackball_hand_update_ADDRESS 0x083E39u
#define how_to_play_panel_start_ADDRESS 0x083F99u
#define how_to_play_panel_show_ADDRESS 0x083FC3u
#define opening_ground_start_ADDRESS 0x083FDCu
#define opening_ground_run_ADDRESS 0x084028u
#define opening_machine_start_ADDRESS 0x08406Bu
#define opening_machine_update_ADDRESS 0x08407Bu
#define player_pacer_start_ADDRESS 0x0843C0u
#define player_pacer_update_ADDRESS 0x0843D5u
#define exit_door_start_ADDRESS 0x084558u
#define exit_door_wait_for_switch_ADDRESS 0x08458Bu
#define exit_door_show_ADDRESS 0x0845C2u
#define stage_05_floor_patch_start_ADDRESS 0x0845DDu
#define stage_05_floor_patch_show_ADDRESS 0x084610u
#define bubble_start_ADDRESS 0x08462Au
#define bubble_update_ADDRESS 0x0846B5u
#define bubble_near_parent_update_ADDRESS 0x0846E7u
#define spiked_press_start_ADDRESS 0x084778u
#define spiked_press_update_ADDRESS 0x08481Du
#define cage_start_ADDRESS 0x084A02u
#define cage_bar_start_ADDRESS 0x084ACDu
#define cage_bar_update_ADDRESS 0x084ADCu
#define sign_start_ADDRESS 0x084B91u
#define sign_show_unless_mode_32_ADDRESS 0x084C43u
#define sign_show_while_player_busy_ADDRESS 0x084C52u
#define sign_show_until_goal_unlocked_ADDRESS 0x084C82u
#define sign_show_while_parent_still_ADDRESS 0x084C8Eu
#define sign_show_until_gate_closed_ADDRESS 0x084CA0u
#define sign_show_until_cage_bar_hit_ADDRESS 0x084CACu
#define sign_show_unless_stage_0_ADDRESS 0x084CB8u
#define sign_show_ADDRESS 0x084CC6u
#define sign_follow_parent_ADDRESS 0x084CE6u
#define sign_follow_player_ADDRESS 0x084D2Bu
#define screen_sign_show_ADDRESS 0x084E0Bu
#define floor_sign_update_ADDRESS 0x084E67u
#define spreading_flame_start_ADDRESS 0x084E83u
#define spreading_flame_burn_ADDRESS 0x084ECEu
#define stage_03_end_pillar_show_ADDRESS 0x08539Fu
#define exit_gate_update_ADDRESS 0x085428u
#define ram_target_update_ADDRESS 0x08551Du
#define ram_target_part_update_ADDRESS 0x0856CEu
#define eggman_speech_start_ADDRESS 0x08574Au
#define eggman_speech_run_ADDRESS 0x08576Fu
#define scenery_sprite_start_ADDRESS 0x0857E5u
#define scenery_sprite_draw_until_you_failed_ADDRESS 0x08584Cu
#define scenery_sprite_draw_ADDRESS 0x08585Fu
#define scenery_sprite_draw_until_offscreen_ADDRESS 0x085880u
#define scenery_sprite_draw_wide_margin_ADDRESS 0x0858A5u
#define scenery_sprite_cycle_palette_start_ADDRESS 0x0858D1u
#define scenery_sprite_cycle_palette_ADDRESS 0x0858FEu
#define fire_vent_spawner_start_ADDRESS 0x085925u
#define fire_vent_spawner_update_ADDRESS 0x085936u
#define fire_vent_start_ADDRESS 0x0859FBu
#define fire_vent_update_ADDRESS 0x085A0Au
#define fire_vent_burst_animate_ADDRESS 0x085B2Eu
#define group_leader_hold_start_ADDRESS 0x085B75u
#define group_leader_hold_wait_ADDRESS 0x085BC2u
#define collapsing_bridge_plank_update_ADDRESS 0x085F80u
#define burning_boulder_start_ADDRESS 0x08626Fu
#define burning_boulder_update_ADDRESS 0x0863EAu
#define burning_boulder_flame_start_ADDRESS 0x086839u
#define burning_boulder_flame_flicker_ADDRESS 0x086881u
#define ember_emitter_start_ADDRESS 0x0868F8u
#define ember_emitter_move_ADDRESS 0x08692Bu
#define falling_ember_update_ADDRESS 0x086A9Au
#define scene_carrier_start_ADDRESS 0x086B6Eu
#define scene_carrier_move_ADDRESS 0x086BD3u
#define seesaw_start_ADDRESS 0x086C07u
#define seesaw_tilt_ADDRESS 0x086C71u
#define metal_platform_base_start_ADDRESS 0x086CD3u
#define metal_platform_base_draw_ADDRESS 0x086D11u
#define metal_platform_start_ADDRESS 0x086D47u
#define metal_platform_update_ADDRESS 0x086DEBu
#define fire_wall_start_ADDRESS 0x086F3Au
#define fire_wall_rise_ADDRESS 0x086FF5u
#define fire_trench_start_ADDRESS 0x087048u
#define fire_trench_update_ADDRESS 0x08708Cu
#define fire_trench_flame_burn_ADDRESS 0x08718Bu
#define ridden_object_start_ADDRESS 0x0871C4u
#define ridden_object_fall_ADDRESS 0x0871E5u
#define ridden_object_follow_ADDRESS 0x0871FBu
#define bridge_fire_start_ADDRESS 0x08721Fu
#define bridge_fire_rise_ADDRESS 0x08730Cu
#define lava_flow_start_ADDRESS 0x08750Eu
#define lava_flow_wait_ADDRESS 0x0875C9u
#define lava_flow_advance_ADDRESS 0x0875E2u
#define lava_flow_segment_follow_ADDRESS 0x087803u
#define lava_flame_burn_ADDRESS 0x087969u
#define lava_steam_puff_update_ADDRESS 0x087A0Eu
#define ice_block_row_start_ADDRESS 0x087D88u
#define ice_block_update_ADDRESS 0x087E22u
#define ice_platform_leader_move_ADDRESS 0x087F5Bu
#define ice_platform_piece_update_ADDRESS 0x087FA5u
#define ice_floor_start_ADDRESS 0x08807Fu
#define ice_floor_block_update_ADDRESS 0x08815Eu
#define icicle_dropper_start_ADDRESS 0x088294u
#define icicle_dropper_move_ADDRESS 0x088321u
#define icicle_dropper_finish_ADDRESS 0x088624u
#define icicle_update_ADDRESS 0x088832u
#define icicle_buried_part_draw_ADDRESS 0x088CA6u
#define icicle_cracks_spread_ADDRESS 0x088D5Du
#define icicle_shadow_follow_ADDRESS 0x088DD2u
#define tornado_start_ADDRESS 0x0890DCu
#define tornado_move_ADDRESS 0x08924Eu
#define tornado_column_circle_ADDRESS 0x089685u
#define tornado_hazard_update_ADDRESS 0x0897EFu
#define tornado_rear_zone_follow_ADDRESS 0x089868u
#define quicksand_start_ADDRESS 0x089927u
#define quicksand_swirl_ADDRESS 0x0899EEu
#define sinkhole_start_ADDRESS 0x089AB4u
#define sinkhole_open_ADDRESS 0x089B72u
#define sand_patch_start_ADDRESS 0x089C0Bu
#define sand_patch_draw_ADDRESS 0x089C51u
#define rimmed_sinkhole_start_ADDRESS 0x089C84u
#define rimmed_sinkhole_open_ADDRESS 0x089DBBu
#define rimmed_sinkhole_edge_zone_update_ADDRESS 0x089E88u
#define sand_block_leader_wait_ADDRESS 0x089F3Bu
#define sand_block_leader_move_ADDRESS 0x089F55u
#define sand_block_update_ADDRESS 0x08A16Eu
#define sand_face_update_ADDRESS 0x08A412u
#define sand_streams_update_ADDRESS 0x08A492u
#define sink_swirl_start_ADDRESS 0x08A593u
#define sink_swirl_animate_ADDRESS 0x08A5A2u
#define brick_wall_init_ADDRESS 0x08A85Eu
#define brick_wall_wait_ADDRESS 0x08A89Eu
#define brick_dust_animate_ADDRESS 0x08A9B9u
#define biting_head_init_ADDRESS 0x08A9DCu
#define biting_head_wait_ADDRESS 0x08AAFEu
#define biting_head_chase_ADDRESS 0x08AB1Fu
#define fire_front_init_ADDRESS 0x08AEDDu
#define fire_front_chase_ADDRESS 0x08AFAAu
#define falling_grate_update_ADDRESS 0x08B216u
#define rising_saw_init_ADDRESS 0x08B3A7u
#define rising_saw_rise_ADDRESS 0x08B560u
#define saw_blade_update_ADDRESS 0x08B6EDu
#define saw_sparks_update_ADDRESS 0x08B8F6u
#define saw_shaft_update_ADDRESS 0x08B94Au
#define grate_steps_init_ADDRESS 0x08B9B9u
#define build_grate_steps_ADDRESS 0x08BA1Bu
#define grate_step_update_ADDRESS 0x08BB09u
#define grate_step_debris_fly_ADDRESS 0x08BC4Du
#define build_saw_shaft_base_ADDRESS 0x08BC91u
#define saw_shaft_base_draw_ADDRESS 0x08BD80u
#define river_raft_spawner_init_ADDRESS 0x08C148u
#define river_raft_spawner_update_ADDRESS 0x08C1A4u
#define river_raft_update_ADDRESS 0x08C332u
#define river_raft_foam_update_ADDRESS 0x08C547u
#define clipped_copy_sprite_update_ADDRESS 0x08C61Eu
#define geyser_front_init_ADDRESS 0x08C6E1u
#define geyser_front_wait_ADDRESS 0x08C7ACu
#define geyser_front_rise_ADDRESS 0x08C7D5u
#define geyser_wait_ADDRESS 0x08CB40u
#define geyser_erupt_ADDRESS 0x08CBC4u
#define waterfall_move_ADDRESS 0x08CD1Au
#define river_water_update_ADDRESS 0x08CEB3u
#define stone_block_init_ADDRESS 0x08D04Cu
#define stone_block_draw_ADDRESS 0x08D0ACu
#define geyser_line_move_ADDRESS 0x08D13Du
#define geyser_burst_start_ADDRESS 0x08D298u
#define geyser_burst_erupt_ADDRESS 0x08D2A8u
#define propeller_machine_init_ADDRESS 0x08D32Du
#define propeller_machine_chase_ADDRESS 0x08D3E1u
#define barred_gate_init_ADDRESS 0x08D5CEu
#define barred_gate_update_ADDRESS 0x08D630u
#define scene_anchor_init_ADDRESS 0x08D67Fu
#define scene_anchor_update_ADDRESS 0x08D6C0u
#define railing_spawner_init_ADDRESS 0x08D707u
#define railing_spawner_update_ADDRESS 0x08D720u
#define railing_segment_start_ADDRESS 0x08D7C7u
#define railing_segment_draw_ADDRESS 0x08D7D7u
#define delete_object_and_children_ADDRESS 0x08E0CEu
#define explosion_animate_ADDRESS 0x08E49Eu
#define released_ring_wait_ADDRESS 0x08E61Bu
#define released_ring_start_ADDRESS 0x08E639u
#define released_ring_bounce_ADDRESS 0x08E650u
#define debris_fly_ADDRESS 0x08E7FDu
#define build_ranking_display_ADDRESS 0x08F263u
#define ranking_title_spin_in_ADDRESS 0x08F4FDu
#define ranking_title_pause_ADDRESS 0x08F551u
#define ranking_title_rise_ADDRESS 0x08F55Eu
#define draw_ranking_title_ADDRESS 0x08F578u
#define ranking_display_scroll_in_ADDRESS 0x08F649u
#define ranking_display_hold_ADDRESS 0x08F662u
#define ranking_display_end_ADDRESS 0x08F6F8u
#define ranking_row_update_ADDRESS 0x08F704u
#define character_chooser_wait_ADDRESS 0x08FBB4u
#define character_chooser_turn_ADDRESS 0x08FDBAu
#define character_chooser_chosen_ADDRESS 0x08FF9Au
#define character_select_countdown_update_ADDRESS 0x08FFE3u
#define join_character_chooser_update_ADDRESS 0x090161u
#define wall_update_ADDRESS 0x090E49u
#define crumbling_floor_update_ADDRESS 0x0914D2u
#define group_leader_wait_ADDRESS 0x09177Du
#define group_leader_move_ADDRESS 0x09179Au
#define lava_rock_update_ADDRESS 0x091939u
#define lava_column_start_ADDRESS 0x091D6Bu
#define lava_column_rise_ADDRESS 0x091D7Au
#define lava_column_toss_start_ADDRESS 0x091E30u
#define lava_column_toss_ADDRESS 0x091E3Fu
#define lava_column_land_ADDRESS 0x091EB5u
#define group_slot_trigger_update_ADDRESS 0x091F63u
#define rope_bridge_plank_update_ADDRESS 0x093ADEu
#define falling_rock_bounce_ADDRESS 0x0947D6u
#define stage_00_platform_start_ADDRESS 0x094C3Fu
#define stage_00_platform_stand_ADDRESS 0x094C93u
#define stage_00_platform_break_ADDRESS 0x094D06u
#define stage_00_geyser_rise_ADDRESS 0x094D36u
#define stage_00_geyser_column_start_ADDRESS 0x094E1Eu
#define stage_00_geyser_column_rise_ADDRESS 0x094E5Au
#define rock_debris_fly_ADDRESS 0x094E92u
#define stage_06_lightning_animate_ADDRESS 0x0953A0u
#define message_box_show_ADDRESS 0x09542Cu
#define map_intro_update_ADDRESS 0x095A53u
#define map_screen_update_ADDRESS 0x095DB2u
#define map_eggman_update_ADDRESS 0x09617Cu
#define map_eggman_hands_update_ADDRESS 0x09628Du
#define title_emblem_animate_ADDRESS 0x096F9Fu
#define title_land_slide_ADDRESS 0x09734Au
#define timed_spawner_run_ADDRESS 0x0973B4u
#define title_flying_craft_move_ADDRESS 0x097435u
#define title_smoke_puff_animate_ADDRESS 0x0974BAu
#define title_clouds_1_drift_ADDRESS 0x09756Eu
#define title_clouds_2_drift_ADDRESS 0x097976u
#define title_clouds_3_drift_ADDRESS 0x097BDAu
#define title_sky_wave_ADDRESS 0x097E95u
#define ring_tally_percent_update_ADDRESS 0x098670u
#define ring_tally_end_sound_wait_ADDRESS 0x0988D0u
#define ring_tally_ticker_update_ADDRESS 0x09891Cu
#define ring_tally_player_1_update_ADDRESS 0x098954u
#define ring_tally_player_2_update_ADDRESS 0x098981u
#define ring_tally_player_3_update_ADDRESS 0x098A00u
#define empty_vblank_hook_ADDRESS 0x099530u
#define apply_video_registers_ADDRESS 0x099531u
#define swap_sprite_buffers_ADDRESS 0x09A1E9u
#define swap_sprite_buffers_now_ADDRESS 0x09A1FBu
#define cancel_sprite_buffer_swap_ADDRESS 0x09A229u
#define irq_vblank_start_ADDRESS 0x0FF500u
#define irq_vblank_end_ADDRESS 0x0FF560u
#define irq_sound_ADDRESS 0x0FF578u
#define irq_timer0_ADDRESS 0x0FF584u
#define irq_timer1_ADDRESS 0x0FF59Cu

/* engine/contacts.c: contacts between objects and players: the zones, the hitter, touchable, rider, platform and wall lists, and the checks that pair them */
uint32_t check_zones(uint32_t box_edge);
uint32_t add_to_zone_list(uint32_t old_head);
void check_hits(uint32_t *p_box_edge, uint32_t *p_distance, uint32_t *p_dy, uint32_t *p_edge_or_centre_y, uint32_t *p_radius, uint32_t *p_centre_offset, uint32_t *p_previous);
void check_hit(uint32_t other, uint32_t hitter, uint32_t *p_box_edge, uint32_t *p_distance, uint32_t *p_dy, uint32_t *p_edge_or_centre_y, uint32_t *p_radius, uint32_t *p_centre_offset, uint32_t *p_previous);
void add_to_hitter_list(void);
uint32_t add_to_touchable_list(uint32_t old_head);
void add_to_rider_list(void);
void add_to_platform_list(void);
void check_platforms(uint32_t *p_middle, uint32_t *p_plank, uint32_t *p_span, uint32_t *p_sag, uint32_t *p_sag_step, uint32_t *p_load_count, uint32_t *p_passes_left, uint32_t *p_platform, uint32_t *p_load_list);
void compute_bridge_sag(uint32_t load_list, uint16_t plank_count, uint32_t *p_middle, uint32_t *p_plank, uint32_t *p_span, uint32_t *p_sag, uint32_t *p_sag_step, uint32_t *p_count, uint32_t *p_passes_left, uint32_t *p_load_entry, uint32_t *p_sag_curve);
void add_to_wall_list(void);
uint32_t check_walls(uint32_t hitbox_offset);

/* engine/geometry.c: between the map and the screen: projecting objects, the floor under a point, unprojecting a screen point; square roots, angles and BCD */
void side_of_box(uint32_t other, uint32_t *p_hitbox_offset_y, uint32_t *p_sides);
uint32_t set_speed_from_direction(void);
void polar_to_xy(uint32_t *p_cos_part, uint32_t *p_angle, uint32_t *p_length);
uint32_t set_speed_from_player_speed(void);
void project_object(uint32_t *p_screen_y, uint32_t *p_depth);
void update_screen_position(uint32_t *p_screen_y, uint32_t *p_depth);
void project_point(uint32_t z, uint32_t *p_projected_y, uint32_t *p_point_x, uint32_t *p_point_y);
void point_to_screen(uint16_t projected_x, uint32_t z, uint32_t *p_projected_y, uint32_t *p_screen_x, uint32_t *p_screen_y);
uint32_t project_object_2(void);
void find_nearby_floor_blocks(uint32_t *p_corner_y, uint32_t *p_floor_block, uint32_t *p_bounds_offset, uint32_t *p_nearby_count, uint32_t *p_corners_left, uint32_t *p_left, uint32_t *p_right);
void find_floor(uint32_t *p_x, uint32_t *p_y, uint32_t *p_block_offset, uint32_t *p_slope_height, uint32_t *p_cursor);
void find_floor_no_block(uint32_t *p_x, uint32_t *p_slope_height, uint32_t *p_cursor);
void find_floor_set_floor_z(uint32_t block, uint32_t *p_x, uint32_t *p_slope_height, uint32_t *p_cursor);
void get_floor_z_at_point(uint32_t x, uint32_t y, uint32_t *p_floor_z, uint32_t *p_block);
uint32_t find_floor_block_at_point(uint32_t x, uint32_t y);
void screen_point_to_ground(uint32_t z, uint32_t *p_point_x, uint32_t *p_point_y);
void unproject_point(uint32_t *p_point_x, uint32_t *p_point_y);
void square_root(uint32_t *p_value, uint32_t *p_steps_left);
void angle_of_vector(uint32_t dy, uint32_t *p_dx, uint32_t *p_octant);
uint32_t convert_from_bcd(uint32_t value);
void convert_to_bcd(uint32_t *p_value, uint32_t *p_digit);
void convert_64_to_bcd(uint32_t *p_number_low, uint32_t *p_number_high, uint32_t *p_bcd_high);

/* engine/objects.c: the object system: the object list and its two pools, allocating, linking and deleting objects, spawning them from the stage lists (by type, by handler, in groups led by a leader), and what the stage objects share */
void move_object_to_next_stage_slot(void);
void next_random(uint32_t *p_random_value, uint32_t *p_words_left, uint32_t *p_random_offset);
void create_child_shadow_unscaled(uint32_t shadow);
void create_child_shadow_9ffe(uint16_t scale, uint32_t shadow);
void create_child_shadow(uint16_t scale, uint32_t shadow);
void draw_child_shadow(void);
uint32_t is_floor_block_off_screen(void);
uint32_t is_off_screen(void);
uint32_t is_off_screen_margin(uint16_t margin_x, uint16_t margin_y);
uint32_t check_left_screen(void);
uint32_t check_left_screen_margin(uint16_t margin);
uint32_t check_left_screen_margins(uint16_t margin, uint16_t unseen_margin);
void allocate_object_outside_loop(uint32_t *p_free_count, uint32_t *p_object);
void delete_object_outside_loop(uint32_t object, uint32_t *p_free_count, uint32_t *p_previous);
void spawn_objects_by_handler(uint32_t *p_list_offset, uint32_t *p_record, uint32_t *p_object);
void reset_linked_objects_flag(void);
void start_spawns_by_type(void);
void spawn_objects_by_type(uint32_t *p_player_object, uint32_t *p_player_index, uint32_t *p_first, uint32_t *p_projected_x, uint32_t *p_projected_y, uint32_t *p_object_type, uint32_t *p_object);
void count_active_players(uint32_t *p_player_object, uint32_t *p_player_index);
void spawn_object_with_pieces(uint32_t return_address, uint32_t *p_spawned, uint32_t *p_piece_count, uint32_t *p_free_count, uint32_t *p_first, uint32_t *p_block);
void add_pieces_to_current_object(uint32_t *p_spawned, uint32_t *p_free_count, uint32_t *p_first, uint32_t *p_block);
void add_object_pieces(uint32_t *p_spawned, uint32_t *p_piece_count, uint32_t *p_free_count, uint32_t *p_first, uint32_t *p_block);
void spawn_objects(uint32_t return_address, uint32_t *p_spawned, uint32_t *p_first, uint32_t *p_block);
void unlink_child_chain(uint32_t object);
void init_object(uint32_t object);
void init_object_2(uint32_t object);
void init_object_fields(uint32_t object, uint8_t mode);
void clear_object_speeds(void);
void init_object_fields_tail(uint32_t object, uint8_t mode);
void move_object_projected(uint32_t *p_screen_y, uint32_t *p_block_offset, uint32_t *p_slope_height, uint32_t *p_cursor);
void unproject_object(uint32_t *p_screen_y, uint32_t *p_block_offset, uint32_t *p_slope_height, uint32_t *p_cursor);
uint32_t update_screen_position_no_offset(void);
void move_object_projected_no_floor(void);
uint32_t is_on_screen(void);
void delete_if_off_screen(uint32_t *p_deleted, uint32_t *p_block_offset, uint32_t *p_slope_height);
void delete_object_and_children(uint32_t *p_deleted, uint32_t *p_block_offset, uint32_t *p_slope_height);
void copy_position_to(uint32_t object);
void place_sprite_near_cursor(uint16_t offset_x, uint16_t offset_y);
void copy_position_to_cursor(void);
void copy_position_from_cursor(void);
void copy_screen_position_from_cursor(void);
uint32_t face_nearest_player(uint32_t direction, uint32_t octant);
void find_nearest_player_x(void);
uint32_t face_object(void);
uint32_t distance_to_object(void);
uint32_t direction_to_object(void);
void distance_xy_to_object(uint32_t *p_distance_x, uint32_t *p_distance_y);
void fall_to_floor(uint32_t *p_height, uint32_t *p_over_pit, uint32_t *p_z);
uint32_t place_sprite_at_cursor_floor(void);
void direction_to_speed(uint32_t speed, uint32_t *p_direction, uint32_t *p_speed_x, uint32_t *p_speed_y);
uint32_t cursor_floor_screen_y(uint32_t floor_screen_y);
void explode_object_medium(void);
void explode_object(void);
void explosion_animate(void);
void drop_rings(uint32_t shadow, uint32_t *p_allocated, uint32_t *p_random_value, uint32_t *p_ring);
void released_ring_wait(void);
void released_ring_start(void);
void released_ring_bounce(void);
void released_ring_move(uint32_t cursor);
void spawn_debris(uint32_t spawned, uint16_t count, uint32_t block);
void debris_fly(void);
void spawn_object_groups(uint32_t *p_list_offset, uint32_t *p_free_stack, uint32_t *p_slope_z, uint32_t *p_count, uint32_t *p_object);
void spawn_object_groups_mark_silent(uint32_t record, uint32_t *p_list_offset, uint32_t *p_slope_z, uint32_t *p_count, uint32_t *p_object);
void spawn_object_groups_no_depth_step(uint32_t record, uint32_t *p_list_offset, uint32_t *p_slope_z, uint32_t *p_count, uint32_t *p_object);
void spawn_object_groups_build_group(uint32_t record, uint32_t *p_list_offset, uint32_t *p_slope_z, uint32_t *p_count, uint32_t *p_object);
void crumbling_floor_update(void);
void crumbling_floor_crack(void);
void crumbling_floor_play_crack_steps(void);
void crumbling_floor_start_falling(void);
void crumbling_floor_draw(void);
void spawn_group_leader(uint32_t record, uint32_t *p_spawned, uint32_t *p_leader);
void group_leader_wait(void);
void group_leader_move(void);
void group_leader_read_script(void);
void group_leader_move_step(void);
void spawn_object_groups_2(uint32_t *p_list_offset, uint32_t *p_count, uint32_t *p_object);
void spawn_object_groups_2_pick_bridge(uint32_t record, uint32_t *p_list_offset, uint32_t *p_count, uint32_t *p_object);
void spawn_object_groups_2_build_group(uint32_t record, uint32_t plank_count_var, uint32_t *p_list_offset, uint32_t *p_count, uint32_t *p_object);
void reset_objects(uint32_t *p_block, uint32_t *p_stack_slot);
void reset_objects_alt(void);
void run_objects(void);
void reset_object_list(void);
void take_free_object(uint32_t *p_free_count, uint32_t *p_object);
uint32_t allocate_object_or_start_list(uint32_t object);
void allocate_object(uint32_t *p_free_count, uint32_t *p_object);
void link_object(uint32_t object);
void allocate_object_after_current(void);
void delete_current_object(void);
void push_free_object(void);
void delete_next_object(void);
void delete_object(uint32_t object, uint32_t *p_free_count, uint32_t *p_previous);
void reset_object_pool_2(uint32_t *p_block, uint32_t *p_stack_slot);
void reset_object_pool_2_alt(void);
void allocate_object_2(uint32_t *p_free_count, uint32_t *p_free_stack, uint32_t *p_object);
uint32_t free_object_2(uint16_t object);

/* engine/sprites.c: animation and the sprite list: animation steps (frames, flips, moves, offsets, multi-part sprites), part palette cycles, the depth buckets, build_sprite_list and the buffer swap at VBLANK */
void cycle_part_palettes(void);
void animate_with_flags(uint32_t *p_frame, uint32_t *p_at_last_step);
void animate_step_with_flags(uint32_t *p_frame, uint32_t *p_at_last_step);
void animate(uint32_t *p_frame, uint32_t *p_wrapped, uint32_t *p_at_last_step);
void animate_step(uint32_t *p_frame, uint32_t *p_wrapped, uint32_t *p_at_last_step);
uint32_t animate_and_move(uint32_t at_last_step);
uint32_t animate_around_anchor(void);
void set_step_around_anchor(uint32_t frame, uint32_t steps);
uint32_t animate_with_screen_offset(uint32_t at_last_step);
void apply_step_screen_offset(void);
void add_step_screen_offset(uint32_t step_offset);
void animate_part_list(void);
void animate_part_list_step(void);
void animate_all_parts(uint32_t *p_at_last_step, uint32_t *p_part);
void init_sprites_without_swaps(void);
void init_sprites_swapping_every_frame(void);
void init_sprites(uint32_t *p_end_mark, uint32_t *p_source);
void init_sprites_from_frame_wait(uint32_t *p_end_mark, uint32_t *p_source);
void init_sprite_hardware(uint32_t *p_end_mark, uint32_t *p_source);
uint32_t clear_sprite_buckets(void);
uint32_t queue_sprite(uint32_t depth);
void queue_sprite_at_depth(uint32_t *p_bucket_head, uint32_t *p_depth);
void queue_sprite_in_layer(void);
void queue_sprite_on_top(void);
void queue_sprite_in_bucket(uint32_t bucket);
uint32_t queue_sprite_at_floor(void);
uint32_t queue_sprite_at_previous_floor(void);
uint32_t queue_sprite_at_base(uint32_t depth);
uint32_t depth_to_bucket(uint32_t bucket, uint16_t depth);
void queue_sprite_at_depth_offset(uint16_t depth, uint8_t bucket_offset);
void queue_sprite_at_previous_floor_2(void);
void get_previous_floor_bucket(uint32_t top_bucket, uint32_t *p_floor_depth, uint32_t *p_bucket);
void swap_sprite_buffers(void);
void swap_sprite_buffers_now(void);
void cancel_sprite_buffer_swap(void);
void load_sized_frame_address_at_row(uint16_t row);
void load_sized_frame_address(void);
void load_frame_size(void);
void build_sprite_list(uint32_t *p_height, uint32_t *p_scale_x, uint32_t *p_scale_y, uint32_t *p_sprite_flags, uint32_t *p_sprite_entry);

/* objects/eggman.c: Eggman: his booth that slides in during his speeches, the speech scripts (stages 3, 8, A), his scene with the button box in stage B and the sliding door, his teleport (never spawned) */
void spawn_eggman_booth(uint32_t spawned, uint32_t piece_count, uint32_t booth, uint32_t *p_free_count, uint32_t *p_block);
void eggman_booth_slide_in(void);
void eggman_booth_animate(void);
void draw_eggman_booth(void);
void spawn_eggman_teleport(uint32_t free_count, uint32_t teleport, uint32_t block);
void eggman_teleport_init(void);
void eggman_teleport_animate(void);
void eggman_scene_init(void);
void eggman_scene_wait(void);
void eggman_scene_pause(void);
void eggman_scene_talk(void);
void eggman_scene_press_button(void);
void eggman_scene_run_off(void);
void eggman_scene_move(void);
void button_box_init(void);
void button_box_wait(void);
void button_box_rise(void);
void button_box_press(void);
void draw_button_box(void);
void sliding_door_init(void);
void sliding_door_wait(void);
void sliding_door_open(void);
void sliding_door_draw(void);
void eggman_speech_start(void);
void eggman_speech_run(void);

/* objects/intro.c: the scenes that start a game: stage 6's opening (the rushing ground, the giant machine head) and stage 7's how to play (the panel, the hands on the button and the trackball, the metal wall that rises and breaks) */
void metal_wall_init(void);
void metal_wall_wait(void);
void metal_wall_rise(uint32_t octant);
void metal_wall_update(uint32_t octant);
void metal_wall_broken(void);
void metal_wall_draw(void);
void how_to_play_button_hand_start(void);
void how_to_play_button_hand_update(void);
void CONSOLE_ORIGINAL(how_to_play_button_hand_update)(void);
void how_to_play_trackball_hand_start(void);
void how_to_play_trackball_hand_update(void);
void CONSOLE_ORIGINAL(how_to_play_trackball_hand_update)(void);
void how_to_play_panel_start(void);
void how_to_play_panel_show(void);
void opening_ground_start(void);
void opening_ground_run(void);
void opening_machine_start(void);
void opening_machine_update(void);
void opening_machine_leap_in(void);
void opening_machine_approach(void);
void opening_machine_lunge(void);
void opening_machine_show_mouth(void);
void opening_machine_engulf(void);
void opening_machine_hold(void);
void draw_opening_machine_shadow(void);
void draw_opening_machine_shadow_spare_return(void);

/* objects/rings.c: rings: the stage rows, spinning, carried and scattered rings, sparkles; the unused gauge ring and template spawner */
void spawn_ring_rows(uint32_t *p_stage_list_offset, uint32_t *p_free_stack, uint32_t *p_slope_height, uint32_t *p_object);
void ring_spin(void);
void ring_spin_keep_spinning(uint32_t sparkle);
void still_ring_wait(void);
void carried_ring_wait(void);
void carried_ring_spin(void);
void carried_ring_keep_spinning(uint32_t sparkle);
void carried_ring_spin_spare_return(void);
void scatter_rings_fixed(uint32_t count, uint32_t *p_random_value, uint32_t *p_allocated);
void scatter_rings_random(uint32_t random_value, uint32_t count, uint32_t *p_allocated, uint32_t *p_ring);
void scatter_rings(uint32_t count, uint32_t *p_random_value, uint32_t *p_allocated, uint32_t *p_ring);
void scattered_ring_wait(void);
void scattered_ring_live(void);
void move_scattered_ring(uint32_t cursor);
void scattered_ring_vanish(void);
void spawn_ring_sparkle(uint32_t sparkle);
void spawn_ring_glint(uint32_t sparkle);
void init_sparkle_object(uint32_t sparkle);
void sparkle_start(void);
void sparkle_animate(void);
void spawn_gauge_ring(uint32_t ring);
void gauge_ring_update(void);
void gauge_ring_rise(void);
void gauge_ring_fall_into_gauge(void);
void spawn_stage_template_0(uint16_t x, uint16_t y, uint16_t z, uint32_t object);
void spawn_stage_template(uint16_t template_index, uint16_t x, uint16_t y, uint16_t z, uint32_t object);

/* objects/shared.c: objects several stages use: the signs, the scenery sprites, the walls players and objects bounce off, the zones behind the chasers, the pacer that keeps two players together, the hold on a group leader; object types 4 and 13; the crawler, which nothing spawns */
void animate_sign(void);
void sign_start_from_type(void);
void crawler_init(void);
void crawler_head_update(void);
void crawler_segment_animate(void);
void crawler_segment_update(void);
void crawler_step(void);
void object_type_4_idle(void);
uint32_t spawn_chaser_back_zone(uint32_t zone_index, uint32_t zone);
void chaser_back_zone_check_parent(void);
void chaser_back_zone_update(void);
void spawn_roller_back_zone(uint32_t zone_index, uint32_t zone);
void roller_back_zone_update(void);
void player_pacer_start(void);
void player_pacer_update(void);
void get_pacing_axis(uint32_t player, uint32_t *p_axis_field, uint32_t *p_decreasing);
void sign_start(void);
void spawn_sign(uint16_t x, uint16_t y, uint16_t kind, uint16_t z, uint32_t *p_depth, uint32_t *p_sign);
uint32_t spawn_sign_2(uint16_t x, uint16_t y, uint16_t kind, uint16_t z, uint32_t sign);
void place_sign(uint16_t x, uint16_t y, uint32_t kind, uint16_t z, uint32_t *p_depth, uint32_t *p_sign);
void init_sign_from_kind(uint32_t record, uint32_t *p_depth, uint32_t *p_sign);
void sign_show_unless_mode_32(void);
void sign_show_while_player_busy(void);
void sign_show_until_goal_unlocked(void);
void sign_show_while_parent_still(void);
void sign_show_until_gate_closed(void);
void sign_show_until_cage_bar_hit(void);
void sign_show_unless_stage_0(void);
void sign_show(void);
void sign_follow_parent(void);
void sign_follow_player(void);
void screen_sign_start(void);
void spawn_screen_sign(uint16_t screen_x, uint16_t screen_y, uint16_t kind, uint32_t sign);
void spawn_screen_sign_2(uint16_t screen_x, uint16_t screen_y, uint16_t kind, uint32_t sign);
void place_screen_sign(uint16_t screen_x, uint16_t screen_y, uint32_t kind, uint32_t sign);
void screen_sign_show(void);
void floor_sign_start(void);
void floor_sign_update(void);
void scenery_sprite_start(void);
void scenery_sprite_draw_until_you_failed(void);
void scenery_sprite_draw(void);
void scenery_sprite_draw_until_offscreen(void);
void scenery_sprite_draw_wide_margin(void);
void scenery_sprite_cycle_palette_start(void);
void scenery_sprite_cycle_palette(void);
void group_leader_hold_start(void);
void group_leader_hold_wait(void);
void spawn_walls(uint32_t *p_spawns_offset, uint32_t *p_part_size, uint32_t *p_second_sprite, uint32_t *p_point_y, uint32_t *p_z, uint32_t *p_wall);
void wall_update(void);
void wall_update_list_and_draw(void);
void wall_update_spare_return(void);
uint32_t check_wall_left_screen(void);
void wall_editor_set_side_label(void);
void wall_editor_draw_arrow(uint32_t marker_x, uint32_t marker_y);
void wall_editor_draw_arrow_at_z(uint32_t marker_x, uint32_t marker_y, uint32_t marker_z);
void wall_editor_draw_pattern_number(void);
void wall_editor_rebuild_parts(uint32_t part);
void build_wall_parts(uint32_t *p_part_size, uint32_t *p_second_sprite, uint32_t *p_part);

/* objects/stage_03.c: stage 3's objects: the spiked panels and the electric poles, the crate pushed to a post, the breakable panel, the spiked roller that chases the players (also in stage E) and the floor that crumbles at the finale, the machine at 8300 with its fire and fireballs, the fire vents, the end pillar */
void spike_panel_init(void);
void spike_panel_hold(void);
void spike_panel_update(void);
void spike_panel_base_draw(void);
void spike_panel_hitbox_update(void);
void spike_panel_part_idle(void);
void electric_pole_init(void);
void electric_pole_wait(void);
void electric_pole_rise(void);
void electric_pole_update(void);
void electric_pole_draw(void);
void push_crate_init(void);
void push_crate_update(void);
void push_crate_break(void);
void push_crate_move(uint32_t cursor, uint32_t random_value, uint32_t ring);
void push_crate_stop_post_update(void);
void push_crate_release_all_rings(uint32_t *p_allocated, uint32_t *p_random_value, uint32_t *p_ring);
void push_crate_release_rings(uint32_t seed, uint32_t *p_allocated, uint32_t *p_random_value, uint32_t *p_ring);
void breakable_panel_init(void);
void breakable_panel_wait(void);
void breakable_panel_break(void);
void draw_breakable_panel(void);
void stage_03_floor_block_start(void);
void stage_03_floor_block_update(void);
void spiked_roller_start(void);
void spiked_roller_update(void);
void spiked_roller_chase(void);
void spiked_roller_chase_read_script(void);
void spiked_roller_chase_move_step(void);
void spiked_roller_chase_rumble(void);
void spiked_roller_chase_check_event(void);
void spiked_roller_fall_off_ledge(void);
void spiked_roller_fall(void);
void spiked_roller_follow_leader(void);
void spiked_roller_wait_hidden(void);
void spiked_roller_drop_in(void);
void spiked_roller_ram(void);
void spiked_roller_ram_charge(void);
void spiked_roller_ram_pause(void);
void spiked_roller_ram_back_off(void);
void spiked_roller_ram_wait(void);
void spiked_roller_remove(void);
void spiked_roller_crash_into_machine(void);
void spiked_roller_sink_and_jump(void);
void spiked_roller_burst_out(void);
void spiked_roller_smash_wall(void);
void spiked_roller_wait_for_turn(void);
void spiked_roller_wait_to_burst_out(void);
void spiked_roller_shift_box(void);
void spiked_roller_spawn_fixed_zone(void);
void roller_fixed_zone_update(void);
void machine_start(void);
void machine_run(void);
void machine_fire_update(void);
void machine_fire_read_script(void);
void machine_fire_climb_step(void);
void start_fireball_flipped(void);
void fireball_start(void);
void init_fireball_flight(void);
void fireball_fly(void);
void flame_start(void);
void flame_burn(void);
void rising_flame_update(void);
void rising_flame_init(void);
void rising_flame_rise(void);
void rising_flame_spawn_next(void);
void rising_flame_wait_offscreen(void);
uint32_t spawn_stage_03_end_pillar(uint32_t pillar);
void stage_03_end_pillar_show(void);
void exit_gate_start(uint32_t sign);
void exit_gate_update(void);
void ram_target_start(void);
void ram_target_update(void);
void ram_target_check_hits(void);
void ram_target_draw(void);
void ram_target_burst(uint32_t piece);
void ram_target_part_start(void);
void ram_target_part_update(void);
void fire_vent_spawner_start(void);
void fire_vent_spawner_update(void);
void fire_vent_start(void);
void fire_vent_update(void);
void fire_vent_blast(void);
void fire_vent_draw(void);
void fire_vent_burst_animate(void);

/* objects/stage_04.c: stage 4's objects: the vines, the catapults, arrow boards, pebbles, the propped board, boulders, the door wall at the start and the rollers behind it, the fireball emitter and the bouncing fireballs, the electric fence and the flame machine at the end */
void vine_init(void);
void vine_swing(void);
void vine_bar_update(void);
void vine_scroll_stop_init(void);
void vine_scroll_stop_wait(void);
void vine_scroll_stop_hold(void);
void vine_scroll_stop_end(void);
void flame_machine_init(void);
void flame_machine_update(void);
void flame_machine_draw(void);
void flame_machine_destroyed(uint32_t free_count, uint32_t first, uint32_t block);
void flame_machine_update_spare_return(void);
void flame_machine_remove(void);
void flame_machine_explosion_start(void);
void flame_machine_back_wall_update(void);
void flame_machine_arm_update(void);
void flame_machine_arm_rest(void);
void flame_machine_arm_draw(void);
void flame_machine_cannon_update(void);
void flame_machine_flame_burn(void);
void flame_machine_arm_take_disc(uint32_t free_count, uint32_t disc, uint32_t block);
void spiked_disc_hold(void);
void spiked_disc_drop(void);
void spiked_disc_draw_held(void);
void spiked_disc_roll(void);
void spiked_disc_draw_shadow(void);
void spiked_disc_spin(void);
void flame_machine_scroll_stop_wait(void);
void flame_machine_scroll_stop_hold(void);
void object_type_9_idle(void);
void flame_machine_wreck_draw(void);
void fence_post_init(void);
void fence_post_start(void);
void fence_post_update(void);
void fence_post_draw(void);
void catapult_x_start(void);
void catapult_y_start(void);
void catapult_wait_for_platform(void);
void catapult_y_start_on_floor(uint32_t object);
void catapult_x_start_on_floor(uint32_t object);
void catapult_wait_for_platform_setup(uint32_t template_offset, uint32_t object);
void catapult_update(void);
void catapult_wait_for_rider(void);
void catapult_launch(void);
void catapult_swing_up(void);
void catapult_hold_up(void);
void catapult_swing_down(void);
void catapult_wait_for_scroll(void);
void catapult_base_update(void);
void catapult_dummy_update(void);
void goal_unlocker_start(void);
void arrow_board_start(void);
void arrow_board_wait_for_platform(void);
void arrow_board_ride(void);
void pebble_start(void);
void pebble_update(void);
void propped_board_start(void);
void propped_board_update(void);
void board_prop_start(void);
void board_prop_update(void);
void release_board_rings(void);
void boulder_start(void);
void boulder_wait(void);
void boulder_roll(void);
void door_wall_start(void);
void door_wall_wait_to_burst(void);
void spawn_door_rollers(uint32_t *p_block_offset, uint32_t *p_slope_height, uint32_t *p_object);
void door_roller_update(void);
void door_roller_wait(void);
void door_roller_drop(void);
void door_roller_roll_out(void);
void door_roller_ride(void);
void fireball_emitter_start(void);
void fireball_emitter_update(void);
void bouncing_fireball_update(void);
void bouncing_fireball_update_spare_return(void);
void bouncing_fireball_knocked_away(void);
void knocked_object_fly(void);

/* objects/stage_05.c: stage 5's objects: the exit door, the floor patch, the bubbles, the spiked press, the cage */
void exit_door_start(void);
void exit_door_wait_for_switch(void);
void exit_door_show(void);
void stage_05_floor_patch_start(void);
void stage_05_floor_patch_show(void);
void bubble_start(void);
void bubble_update(void);
void bubble_near_parent_update(void);
void bubble_pop(void);
void spiked_press_start(void);
void spiked_press_update(void);
void cage_start(void);
void cage_bar_start(void);
void cage_bar_update(void);

/* objects/stage_08.c: stage 8's objects, the lava: the treadmill that raises a gate, the flame jets, the rock wall, the collapsing log bridge, the burning boulders and embers, the carriers and the seesaw, the metal platform and the fire wall, the fire trenches, the lava flow under the hanging bridge, the rock slabs and the lava columns */
void treadmill_init(void);
void treadmill_update(void);
void treadmill_roller_update(void);
void treadmill_gate_update(void);
void treadmill_gate_half_update(void);
void flame_jet_init(void);
void flame_jet_nozzle_update(void);
void flame_jet_pilot(void);
void flame_jet_ignite(void);
void flame_jet_burn(void);
void rock_wall_init(void);
void rock_wall_update(void);
void collapsing_bridge_plank_update(void);
void burning_boulder_start(void);
void burning_boulder_update(void);
void burning_boulder_flame_start(void);
void burning_boulder_flame_flicker(void);
void ember_emitter_start(void);
void ember_emitter_move(void);
void falling_ember_update(void);
void scene_carrier_start(void);
void scene_carrier_move(void);
void seesaw_start(void);
void seesaw_tilt(void);
void metal_platform_base_start(void);
void metal_platform_base_draw(void);
void metal_platform_start(void);
void metal_platform_update(void);
void fire_wall_start(void);
void fire_wall_rise(void);
void fire_trench_start(void);
void fire_trench_update(void);
void spawn_fire_trench_flames(void);
void fire_trench_flame_burn(void);
void ridden_object_start(void);
void ridden_object_fall(void);
void ridden_object_follow(void);
void bridge_fire_start(void);
void bridge_fire_rise(void);
void bridge_fire_read_script(void);
void bridge_fire_climb_step(void);
void lava_flow_start(void);
void lava_flow_wait(void);
void lava_flow_advance(void);
void lava_flow_read_script(void);
void lava_flow_move_step(void);
void extend_lava_flow(uint32_t block);
void lava_flow_segment_follow(void);
uint32_t spawn_lava_flame(uint32_t random_value, uint32_t flame);
void lava_flame_burn(void);
void spawn_lava_steam(void);
void lava_steam_puff_update(void);
void lava_rock_update(void);
void lava_rock_erupt(uint32_t block);
void lava_rock_draw_at_previous_floor(void);
uint32_t spawn_lava_column(uint32_t block);
void lava_column_start(void);
void lava_column_rise(void);
void lava_column_toss_start(void);
void lava_column_toss(void);
void lava_column_land(void);
void lava_column_land_in_place(void);
void lava_column_become_bubble(void);
void spawn_group_slot_trigger(uint32_t record, uint32_t *p_spawned, uint32_t *p_count, uint32_t *p_trigger);
void group_slot_trigger_update(void);

/* objects/stage_09.c: stage 9's objects, the ice: the handle rod between two posts, the spear traps, the ice cube launcher, the padded wall, the snow mound, the ice blocks and the moving ice platform, the crumbling ice floor, the icicles */
void handle_rod_init(void);
void handle_rod_handle_init(void);
void handle_rod_wait(void);
void handle_rod_start_swing(void);
void handle_rod_swing(void);
void handle_rod_hang(void);
void handle_rod_remove(void);
void handle_rod_handle_update(void);
void handle_rod_post_init(void);
void handle_rod_post_update(void);
void handle_rod_post_fall(void);
void spear_trap_init_100(void);
void spear_trap_init_300(void);
void spear_trap_init(void);
void spear_trap_wait(void);
void spear_trap_rise(void);
void spear_trap_stay_up(void);
void spear_trap_retract(void);
void spear_trap_draw_touchable(void);
void spear_trap_draw(void);
void ice_launcher_init(void);
void ice_launcher_update(void);
void ice_cube_slide(void);
void object_type_17_idle(void);
void padded_wall_init(void);
void padded_wall_update(void);
void wall_pad_update(void);
void snow_mound_init(void);
void snow_mound_update(void);
void snow_mound_burst(void);
void snow_mound_draw(void);
void ice_block_row_start(void);
void ice_block_update(void);
void spawn_ice_platform_leader(uint32_t record, uint32_t *p_spawned, uint32_t *p_travel, uint32_t *p_leader);
void ice_platform_leader_move(void);
void ice_platform_piece_update(void);
void ice_floor_start(void);
void ice_floor_block_update(void);
void icicle_dropper_start(void);
void icicle_dropper_move(void);
void icicle_dropper_coordinate_reached(uint32_t path_cursor);
void icicle_dropper_check_scroll_position(uint32_t path_cursor);
void icicle_dropper_load_record(uint32_t path_cursor);
void icicle_dropper_move_step(void);
void icicle_dropper_choose_speed(uint32_t slope_height, uint32_t cursor);
void icicle_dropper_catch_up_speed(uint32_t slope_height, uint32_t cursor);
void icicle_dropper_move_at_speed(uint16_t move_speed, uint32_t slope_height, uint32_t cursor);
void icicle_dropper_finish(void);
void drop_icicle_row(uint32_t random_value, uint32_t block);
void icicle_update(void);
void icicle_buried_part_draw(void);
void icicle_cracks_spread(void);
void icicle_shadow_follow(void);
void set_icicle_wall_box(void);

/* objects/stage_0a.c: stage A's objects, the sand: the cactus, the centipede, the wall pieces, the shaking blocks, the sand rock, the sand arch, the stepped wall, the tornado and the whirlwinds it throws, quicksand, sinkholes, the collapsing sand blocks */
void cactus_init(void);
void cactus_update(void);
uint32_t spawn_whirlwind(uint32_t spawned, uint32_t free_count, uint32_t whirlwind, uint32_t object);
void whirlwind_aim(void);
void whirlwind_move(void);
void whirlwind_vanish(void);
void centipede_init(void);
void centipede_chase(void);
void centipede_leave(void);
void centipede_move(uint32_t block);
void centipede_segment_wait(void);
void centipede_segment_follow(void);
void centipede_tail_wait(void);
void centipede_tail_follow(void);
uint32_t centipede_follow_path(void);
void centipede_hop(uint32_t *p_height, uint32_t *p_z);
void draw_centipede_piece(void);
void spawn_centipede_sand_puff(uint32_t spawned, uint32_t free_count, uint32_t puff, uint32_t block);
void sand_puff_start(void);
void sand_puff_animate(void);
void wall_piece_init(void);
void wall_piece_draw(void);
void shaking_block_init(void);
void shaking_block_animate(void);
void sand_rock_init(void);
void sand_rock_update(void);
void sand_rock_crumble(void);
void sand_rock_draw(void);
void sand_rock_draw_piece(void);
void sand_arch_init(void);
void sand_arch_update(void);
void sand_arch_ball_init(void);
void sand_arch_ball_delay(void);
void sand_arch_ball_move(void);
void sand_arch_ball_gone(void);
void sand_arch_ball_bob(void);
void stepped_wall_init(void);
void stepped_wall_draw(void);
void tornado_start(void);
void tornado_move(void);
void tornado_load_record(uint32_t path_cursor);
void tornado_remove_with_columns(void);
void tornado_move_step(void);
void tornado_choose_speed(void);
void tornado_catch_up_speed(void);
void tornado_move_at_speed(uint16_t move_speed);
void tornado_column_circle(void);
void spawn_tornado_hazard(uint32_t x, uint32_t y, uint32_t z, uint32_t hazard);
void tornado_hazard_update(void);
uint32_t tornado_throw_whirlwinds(uint8_t spawned, uint32_t free_count, uint32_t whirlwind, uint32_t object);
uint32_t spawn_tornado_rear_zone(uint32_t zone);
void tornado_rear_zone_follow(void);
void quicksand_start(void);
void quicksand_swirl(void);
void sinkhole_start(void);
void sinkhole_open(void);
void sand_patch_start(void);
void sand_patch_draw(void);
void rimmed_sinkhole_start(void);
void rimmed_sinkhole_open(void);
void rimmed_sinkhole_edge_zone_update(void);
void spawn_sand_block_leader(uint32_t record, uint32_t *p_spawned, uint32_t *p_leader);
void sand_block_leader_wait(void);
void sand_block_leader_move(void);
void sand_block_leader_read_script(void);
void sand_block_leader_move_step(void);
void sand_block_update(void);
void sand_face_update(void);
void sand_streams_update(void);
uint32_t spawn_sink_swirl(uint32_t allocated);
void sink_swirl_start(void);
void sink_swirl_animate(void);

/* objects/stage_10.c: stage 10's objects, the river: the mines and the bubble column, the lid trap, the turtle, the eel and its den, the rafts, the geysers, the waterfall and the river water, the propeller machine that chases the players, the barred gate, the rope bridge */
void spawn_bubble_column(uint32_t spawned, uint32_t free_count, uint32_t column, uint32_t block);
void bubble_column_init(void);
void bubble_column_animate(void);
void mine_init(void);
void mine_float(void);
void mine_fall(void);
void mine_sink(void);
void mine_explode(void);
void animate_sinking_mine(void);
void draw_mine_with_bubbles(void);
void flash_mine_palette(uint8_t sinking);
void spawn_mine_splash(uint32_t spawned, uint32_t free_count, uint32_t splash, uint32_t block);
void mine_splash_animate(void);
void lid_trap_init(void);
void lid_trap_update(void);
void turtle_init(void);
void turtle_wait(void);
void turtle_surface(void);
void turtle_drop_rings(void);
void turtle_swim(void);
void eel_init(void);
void eel_anchor_update(void);
void eel_anchor_wait_explode(void);
void eel_anchor_stretch(void);
void eel_anchor_draw_shadow(void);
void eel_anchor_wait_explode_spare_return(void);
void eel_head_move(void);
void eel_segment_move(void);
void draw_eel_piece_parts(void);
void place_eel_piece(void);
void eel_head_explode_delay(void);
void eel_segment_explode_delay(void);
void eel_piece_start_explosion(void);
void eel_piece_explode(void);
void eel_piece_gone(void);
void spawn_eel_den(uint32_t spawned, uint32_t free_count, uint32_t den, uint32_t block);
void eel_den_init(void);
void eel_den_wait(void);
void eel_den_collapse(void);
void eel_den_draw(void);
void river_raft_spawner_init(void);
void river_raft_spawner_update(void);
void river_raft_update(void);
void river_raft_foam_update(void);
void clipped_copy_sprite_start(uint32_t copy);
void clipped_copy_sprite_update(void);
void geyser_front_init(void);
void geyser_front_wait(void);
void geyser_front_rise(void);
void geyser_front_read_script(void);
void geyser_front_rise_step(void);
void geyser_front_spawn_geyser_row(void);
void geyser_front_rise_sound(void);
uint32_t spawn_listed_geyser(uint32_t geyser);
void spawn_geyser(uint32_t x, uint32_t y, uint16_t wait, uint32_t *p_spawned, uint32_t *p_geyser);
void geyser_wait(void);
void geyser_erupt(void);
void spawn_waterfall(uint32_t record, uint32_t *p_spawned, uint32_t *p_free_stack, uint32_t *p_block);
void waterfall_move(void);
void waterfall_read_script(void);
void waterfall_move_step(void);
void river_water_update(void);
void stone_block_init(void);
void stone_block_draw(void);
void spawn_geyser_line(uint32_t line);
void geyser_line_move(void);
void spawn_geyser_burst(void);
void geyser_burst_start(void);
void geyser_burst_erupt(void);
void propeller_machine_init(void);
void propeller_machine_chase(void);
void propeller_machine_read_script(void);
void propeller_machine_chase_step(void);
void barred_gate_init(void);
void barred_gate_update(void);
void scene_anchor_init(void);
void scene_anchor_update(void);
void railing_spawner_init(void);
void railing_spawner_update(void);
void railing_segment_start(void);
void railing_segment_draw(void);
void rope_bridge_plank_update(void);
void rope_bridge_plank_start_falling(void);
void rope_bridge_plank_check_rider_and_draw(void);
void draw_rope_bridge_ropes(void);
void event_19_check_rope_bridge(uint32_t position, uint32_t in_play_count, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void event_12_stage_frame(uint32_t pressed, uint32_t position, uint32_t in_play_count, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);

/* objects/stages_0b_0f.c: the objects of stages B to F: the chasers (the brick wall that bursts, the biting head, the fire front) and the flames that spread ahead of it, the spike gate, the falling grates, the rising saws and the steps they cut, the puffs, the checkered pad, the static triangle, the railing */
void white_puff_init(void);
void white_puff_animate(void);
void checker_pad_init(void);
void checker_pad_animate(void);
void static_triangle_init(void);
void static_triangle_animate(void);
void railing_init(void);
void railing_draw(void);
void railing_side_update(void);
void spike_gate_init(void);
void spike_gate_pillar_update(void);
void spike_gate_beam_rise(void);
void spike_gate_beam_hold(void);
void spike_gate_beam_drop(void);
void spike_gate_beam_wait(void);
void spike_gate_beam_update(void);
void spreading_flame_start(void);
void spreading_flame_burn(void);
void brick_wall_init(void);
void brick_wall_wait(void);
void brick_dust_animate(void);
void biting_head_init(void);
void biting_head_wait(void);
void biting_head_chase(void);
void biting_head_read_script(void);
void biting_head_load_record(uint32_t path_cursor);
void biting_head_chase_step(void);
void biting_head_choose_speed(void);
void biting_head_catch_up_speed(void);
void biting_head_move_at_speed(uint16_t move_speed);
void fire_front_init(void);
void fire_front_chase(void);
void fire_front_read_script(void);
void fire_front_load_record(uint32_t path_cursor);
void fire_front_chase_step(void);
void falling_grate_update(void);
void rising_saw_init(void);
void rising_saw_rise(void);
void saw_blade_update(void);
void saw_sparks_update(void);
void saw_shaft_update(void);
void grate_steps_init(void);
void build_grate_steps(void);
void grate_step_update(void);
void spawn_grate_step_debris(void);
void grate_step_debris_fly(void);
void build_saw_shaft_base(void);
void saw_shaft_base_draw(void);

/* player/carried_states.c: the player's states while the stage carries or holds them: thrown, carried off, trapped, riding, the final stage, tripped, swept away, climbing an object, pulled, held still, stuck on an object, caught, sinking, knocked off */
void player_state_scene_thrown(uint32_t depth, uint32_t cursor);
void player_state_carried_off(uint32_t depth, uint32_t cursor);
void player_state_trapped(uint32_t depth, uint32_t cursor);
void trap_flash_start(void);
void trap_flash_update(void);
void trap_start(void);
void trap_update(void);
void player_state_riding(void);
void player_state_final_stage(void);
void player_state_tripped(uint32_t cursor);
void player_state_swept_away(void);
void player_state_climbing_object(uint32_t cursor);
void player_object_climb_begin(void);
void player_object_climb_push(uint32_t cursor);
void player_object_climb_move(void);
void spawn_climb_hand(uint32_t hand);
void climb_hand_update(void);
void player_state_pulled(uint32_t cursor);
void player_state_floor_8_sequence(uint32_t cursor);
void player_floor_8_sequence_struggle(void);
void player_move_with_scroll(uint32_t *p_depth, uint32_t *p_cursor);
uint32_t spawn_player_effect(uint32_t effect);
void player_effect_start(void);
void player_effect_update(void);
uint32_t spawn_struggle_marker(uint32_t marker);
void struggle_marker_update(void);
void scene_object_start(void);
void scene_object_update(void);
void player_state_held_still(void);
void player_state_stuck_on_object(uint32_t depth, uint32_t cursor);
void player_stuck_on_object_begin(uint32_t sign);
void player_stuck_on_object_wait(void);
void player_stuck_on_object_shake(void);
void player_stuck_on_object_jump_out(void);
void player_state_caught(uint32_t cursor);
void player_state_sinking(uint32_t cursor);
void sinking_effect_update(void);
void player_state_knocked_off(uint32_t depth, uint32_t cursor);
void player_knocked_off_start(void);
void player_knocked_off_fly(void);

/* player/effects.c: around the players: the skid dust, the HUD (gauge, portrait, ring count, start prompt, messages), the ride of game mode 0x1C, joining with a character select, the result bonus, the climb zone splash, the intro marker */
void spawn_skid_dust(uint32_t dust);
void skid_dust_update(void);
void player_hud_init(void);
void player_hud_start(void);
void player_hud_update(uint32_t *p_digit_attribute, uint32_t *p_text_position);
uint32_t draw_player_start_prompt(void);
uint32_t CONSOLE_ORIGINAL(draw_player_start_prompt)(void);
void animate_hud_palette(uint32_t *p_list_index, uint32_t *p_wrapped);
void animate_hud_tiles(uint32_t *p_list_index, uint32_t *p_wrapped);
uint32_t blink_player_hud_message(uint32_t text, uint32_t text_position);
void draw_player_stage_rings(void);
uint32_t spawn_player_ride(uint32_t object);
void player_select_character(uint32_t player_number);
void give_result_bonus(uint32_t player);
void spawn_climb_zone_effect(uint32_t object);
void climb_zone_effect_start(void);
void climb_zone_effect_update(void);
void spawn_intro_marker(uint32_t object);
void intro_marker_update(void);
void update_hurt_output(void);
void check_player_stalled(void);

/* player/player.c: the player: starting, joining, resetting and placing a player, player_update, the movement (screen edges, trackball, turning, jumps, walls), the shadow, the floor checks, the restart point, the checks of every frame, and check_player_contacts (what touching each kind of object does) */
void player_1_start(void);
void player_2_start(void);
void player_3_start(void);
void player_start(void);
void player_resume(void);
void player_join(void);
void player_start_stage(void);
void player_respawn(void);
void player_restart(void);
void player_reset_joined(void);
void player_reset(void);
void player_place(void);
void player_update(void);
void player_dispatch_state(uint32_t depth, uint32_t cursor);
uint32_t keep_player_on_screen(uint32_t depth);
uint32_t push_player_from_screen_edge(void);
void compute_scroll_drift(uint32_t block, uint8_t drift_byte);
void slip_effect_update(void);
uint32_t set_speed_from_trackball(void);
uint32_t set_climb_speed_from_trackball(void);
uint32_t update_ledge_pullback(void);
void get_ledge_side(uint32_t *p_side, uint32_t *p_block);
void check_player_floor_edge(uint32_t *p_depth, uint32_t *p_block);
void check_fall_into_pit(void);
void add_player_to_lists(void);
void remember_platform(void);
void update_player_blink(void);
void toggle_hidden(void);
void check_player_jump(void);
void start_jump_if_pressed(void);
void push_player_out_of_wall(uint32_t *p_angle, uint32_t *p_push_direction, uint32_t *p_record_index);
void push_player_out_of_wall_deflect(uint32_t *p_angle, uint32_t *p_push_direction, uint32_t *p_record_index);
void player_steer_in_air(void);
void player_steer(uint32_t wanted_speed);
void player_steer_speed_from_trackball(void);
void player_steer_approach_speed(uint32_t wanted_speed);
void player_steer_turn_and_ground_speed(void);
void set_target_heading_from_trackball(void);
void turn_player(void);
void set_heading_direction(void);
uint32_t set_facing(uint32_t facing);
uint32_t face_movement_direction(uint32_t facing);
uint32_t compute_ground_speed(void);
void read_player_trackball(void);
void shadow_1_start(void);
void shadow_2_start(void);
void shadow_3_start(void);
void shadow_start(void);
void shadow_update(void);
void draw_attack_effect(uint32_t player);
void check_player_step(uint32_t *p_height_diff, uint32_t *p_cursor);
void check_player_step_against_floor(uint32_t *p_height_diff, uint32_t *p_cursor);
void bounce_off_wall_in_air(void);
uint32_t get_floor_block_side(uint32_t floor_block, uint32_t x, uint32_t y);
void get_floor_block_side_or_corner(uint32_t block, uint32_t x, uint32_t y, uint32_t *p_outcode, uint32_t *p_side_code);
uint32_t place_player_at_restart_point(void);
void aim_at_screen_centre(uint32_t *p_cos_part, uint32_t *p_dx, uint32_t *p_dy);
void flash_hurt_palette(void);
void set_floor_sprite_priority(void);
uint32_t update_quicksand_object(uint32_t sink_offset);
uint32_t update_sinking_floor(uint32_t sink_offset);
void apply_moving_floor(void);
void check_zone_contact(void);
void check_stage_entry_path(void);
void load_stage_goal(void);
void check_player_reached_goal(void);
void spawn_tumble_effect(uint32_t effect);
void tumble_effect_update(void);
uint32_t apply_speed_scale(uint32_t target_speed);
void set_player_entry_by_scene(void);
uint32_t try_continue(uint8_t cost);
uint32_t try_join(void);
uint32_t pay_and_join(uint8_t cost, uint32_t joined);
uint32_t check_scene_states(uint32_t object);
void set_fall_sprite_priority(void);
void start_climb_at_spawn(uint32_t climbed_height);
void start_climb(void);
void add_push_to_player_motion(void);
void apply_floor_push(void);
uint32_t spawn_slip_piece(uint32_t piece);
void slip_piece_update(void);
void spawn_slip_pieces(uint32_t piece);
void slip_pieces_update(void);
uint32_t follow_player_path(uint32_t target_speed);
uint32_t follow_player_path_start(void);
uint32_t follow_player_path_run(void);
uint32_t follow_player_path_next_record(void);
uint32_t follow_player_path_load_record(void);
uint32_t follow_player_path_aim_at_point(void);
uint32_t follow_player_path_target_speed(void);
void check_player_contacts(uint32_t *p_health_limit, uint32_t *p_cursor);
void check_player_contacts_clear_state(void);
void check_player_contacts_hurt_unless_invulnerable(uint32_t toucher);
void check_player_contacts_hurt(uint32_t toucher);
void check_player_contacts_attack_or_hurt(uint8_t new_substate, uint32_t toucher);
uint32_t check_player_contacts_solid(uint32_t health_limit, uint32_t toucher);
void check_player_contacts_attack(uint32_t toucher);

/* player/scene_states.c: the player's states in scenes and stage events: out of the game, continue, entering and leaving a stage, tumbling, blown away, the intro and opening scenes, stunned, choosing a character, YOU FAILED, held, stuck */
void player_state_out_of_game(void);
void player_state_continue(void);
void player_state_in_zone(uint32_t depth, uint32_t cursor);
void player_in_zone_steer(uint32_t depth, uint32_t cursor);
void player_state_entering_stage(uint32_t depth, uint32_t cursor);
void player_state_stage_exit(uint32_t depth, uint32_t cursor);
void player_state_tumbling(uint32_t depth, uint32_t cursor);
void player_tumbling_start(uint32_t cursor);
void player_tumbling_knock_with_toucher_speed(uint32_t toucher, uint32_t effect);
void player_tumbling_set_launch_speed(uint32_t toucher, uint32_t effect);
void player_tumbling_launch(uint32_t toucher, uint32_t effect);
void player_tumbling_fly(void);
void player_state_blown_away(uint32_t cursor);
void blown_away_effect_update(void);
void player_state_intro_turn(uint32_t depth, uint32_t cursor);
void player_state_opening_scene(void);
void player_state_opening_scene_spare_return(void);
void shadow_update_spare_return(void);
void player_state_stage_scene(uint32_t depth, uint32_t cursor);
void player_state_stunned(uint32_t depth, uint32_t cursor);
void player_state_choosing_character(uint32_t cursor);
void player_state_you_failed(void);
void player_state_held(uint32_t depth, uint32_t cursor);
void player_state_stuck(uint32_t cursor);
void player_stuck_begin(uint32_t object);
void player_stuck_wait_release(void);
void player_stuck_shake(void);
void player_stuck_jump_out(void);
void stuck_effect_update(void);
void player_state_stage_b_sequence(uint32_t depth, uint32_t cursor);

/* player/states.c: the player's states in play: standing, walking, running, falling and jumping (player_move, player_animate), teetering on a ledge and going over it, dropping, falling into a pit, climbing a wall, skidding, hopping, dying, launched, hurt, bouncing */
void player_move(uint32_t depth, uint32_t cursor);
void player_move_check_edge(uint32_t depth, uint32_t cursor);
void player_animate(uint32_t depth, uint32_t cursor);
void player_animate_standing(void);
void player_animate_walking(void);
void player_animate_running_fast(void);
void player_animate_running(void);
void player_animate_run_cycle(uint32_t animation_table);
void player_animate_run_cycle_alternate(void);
void player_animate_run_from_table(uint32_t animation_table);
void player_animate_falling(void);
void player_state_inactive(void);
void player_animate_jumping(uint32_t sign);
void player_animate_jumping_land(void);
void player_draw(void);
void player_state_teetering(uint32_t cursor);
void player_state_falling_off_edge(void);
void player_state_falling_off_edge_spare_return(void);
void player_state_over_edge(uint32_t jump_offset, uint32_t cursor);
void player_over_edge_check_pull_up(uint32_t cursor);
void player_over_edge_drop(uint32_t cursor);
void player_over_edge_hang_finish(uint32_t cursor);
void player_state_dropping(uint32_t cursor);
void player_state_dropping_spare_return(void);
void player_state_falling_into_pit(void);
void player_state_falling_into_pit_spare_return(void);
void player_state_climbing(uint32_t depth, uint32_t cursor);
void player_climbing_start(uint32_t cursor, uint32_t attribute_record);
void player_climbing_grab_wall(uint32_t cursor, uint32_t attribute_record);
void player_climbing_descend_over_edge(uint32_t cursor, uint32_t attribute_record);
void player_climbing_begin_climb(uint32_t cursor, uint32_t attribute_record);
void player_climbing_move(uint32_t cursor, uint32_t attribute_record);
void player_climbing_side_step(uint32_t cos_speed, uint32_t cursor, uint32_t attribute_record);
void player_climbing_check_vertical_step(uint32_t cursor, uint32_t attribute_record);
void player_climbing_start_vertical_step(uint32_t cursor, uint32_t attribute_record);
void player_climbing_animate_vertical_step(uint32_t cursor, uint32_t attribute_record);
void player_climbing_update_position(uint32_t cursor, uint32_t attribute_record);
void player_climbing_pull_up(uint32_t attribute_record);
void player_state_not_playing(void);
void player_animate_skidding(uint32_t depth, uint32_t cursor);
void player_animate_skidding_slow_down(uint32_t cursor);
void player_state_game_over(uint32_t jump_offset);
void player_state_short_animation(void);
void player_state_inactive_2(void);
void player_animate_hopping(void);
void player_state_dying(void);
void player_state_launched(void);
void player_state_launched_spare_return(void);
void player_launch_angle_200(void);
void player_launch_angle_100(void);
void player_launch(void);
void player_launched_fly(void);
void player_launched_land(void);
void player_state_hurt(uint32_t depth, uint32_t cursor);
void player_hurt_knockback(uint32_t cursor);
void player_hurt_set_animation(uint32_t cursor);
void player_hurt_fly(uint32_t depth, uint32_t cursor);
void player_hurt_land_after_hop(void);
void player_hurt_get_up(void);
void player_state_hurt_spare_return(void);
void player_state_bouncing(void);

/* scenes/character_select.c: choosing a character: the PLAYER SELECT chooser at a new game and the chooser of a player joining */
void reset_character_select(void);
void start_character_chooser(uint32_t block);
void character_chooser_wait(void);
void CONSOLE_ORIGINAL(character_chooser_wait)(void);
void character_chooser_confirm(void);
void character_chooser_idle(void);
void draw_character_chooser_labels(void);
void character_chooser_turn(void);
void character_chooser_chosen(void);
void character_select_countdown_update(void);
uint32_t start_join_character_chooser(void);
void join_character_chooser_update(uint32_t text_x);
void CONSOLE_ORIGINAL(join_character_chooser_update)(uint32_t text_x);
void join_character_chooser_take_character(void);
void count_free_characters(uint32_t *p_characters_left, uint32_t *p_character, uint32_t *p_taken_bit);

/* scenes/ending.c: stage 0x11, the ending: the runners, the tower and the explosions, the flash to white, the credits (portraits, name plates, scrolling blocks), Eggman flying away and the three running across */
void build_ending_scene(uint32_t *p_write_index_address, uint32_t *p_tile_bank_bits, uint32_t *p_color, uint32_t *p_row_pairs_left, uint32_t *p_blocks_per_row, uint32_t *p_scene_actor);
void init_ending_scene_object(uint32_t object);
void ending_tower_animate(void);
void ending_debris_1_fly(void);
void ending_debris_2_fly(void);
void ending_debris_3_fly(void);
void ending_tower_blast_1(void);
void ending_tower_blast_2(void);
void ending_tower_blast_3(void);
void ending_horizon_blast_1(void);
void ending_horizon_blast_2(void);
void ending_horizon_blast_3(void);
void ending_horizon_blast_4(void);
void ending_horizon_blast_5(void);
void ending_horizon_blast_6(void);
void ending_horizon_blast_7(void);
void ending_horizon_blast_8(void);
void ending_horizon_blast_9(void);
void start_ending_blast(void);
void ending_big_explosion(void);
void ending_flash_to_white(void);
void animate_horizon_blast(void);
void draw_ending_blast_frame_copy(uint32_t frame_entry);
void animate_tower_blast(void);
void draw_ending_blast_frame(uint32_t frame_entry);
void animate_ending_debris(uint32_t path);
void run_timed_spawns_2(uint32_t object);
void ending_horizon_show(void);
void ending_dust_cloud_rise(void);
void ending_sonic_run_forward(void);
void ending_ray_run_forward(void);
void ending_mighty_run_forward(void);
void ending_road_scroll(void);
void ending_credits_fade_in(void);
void ending_credits_spawner_run(void);
uint32_t load_credits_palettes(void);
void ending_music_play(void);
void ending_portrait_show(void);
void ending_name_plate_show(void);
void ending_credits_team_logo_scroll(void);
void ending_credits_director_scroll(void);
void scroll_credits_block(void);
void ending_credits_program_scroll(void);
void ending_credits_design_scroll(void);
void ending_credits_sound_scroll(void);
void ending_credits_voice_actor_scroll(void);
void ending_credits_special_thanks_scroll(void);
void scroll_credits_sprite(void);
void wrap_credits_scroll_timer(void);
void init_credits_sprite(void);
void run_timed_spawns_3(void);
void animate_ending_runner(uint32_t frame_entry);
void ending_window_open(void);
void ending_credits_fade_out(void);
void ending_eggman_scene_fade_in(void);
void ending_eggman_fly_in(void);
void init_eggman_smoke_puff(uint32_t piece);
void ending_eggman_turn(void);
void ending_eggman_scene_fade_out(void);
void ending_close_up_fade_in(void);
void ending_eggman_fly_away(void);
void ending_eggman_fly_away_spare_return(void);
void ending_sonic_run_across(void);
void ending_ray_run_across(void);
void ending_mighty_run_across(void);
void init_ending_runner_across(void);
void animate_ending_runner_across(void);

/* scenes/map.c: the map screen between stages (game mode 4): the wipe, the island map, the trail, the markers, Eggman */
uint32_t run_map_screen(uint32_t bottom_speed, uint32_t object);
void run_map_screen_wait_for_wipe(uint32_t edges_done);
void map_intro_update(void);
void move_map_stage_icon(void);
void draw_map_markers_and_trail(void);
void map_screen_draw_flipping_icon(void);
void map_screen_update(void);
void map_eggman_update(void);
void map_eggman_hands_update(void);
void map_eggman_draw_plain(void);
void map_eggman_draw_plain_2(void);
void draw_map_eggman(void);
void start_map_trail(void);
void next_map_trail_segment(uint32_t trail_point);
void draw_map_trail_dot(uint32_t object);
uint32_t draw_map_stage_markers(uint32_t badge_offset);
void blink_current_map_marker(void);
uint32_t start_window_wipe(uint32_t record);
void update_screen_wipe(uint32_t *p_clip_right, uint32_t *p_clip_top, uint32_t *p_clip_bottom, uint32_t *p_edges_done);

/* scenes/ranking.c: the ranking: entering initials at game over, loading and resetting the table, the TOP15 display */
void start_ranking_entry(void);
void enter_ranking_initials(uint32_t jump_offset);
void enter_ranking_initials_count_down(uint32_t button_bit);
void enter_ranking_initials_save_record(void);
uint32_t load_ranking(void);
uint32_t reset_ranking(void);
uint32_t copy_ranking_records(uint32_t source, uint32_t dest, uint32_t count);
uint32_t ascii_to_ranking_letter(uint32_t letter);
void start_ranking_display(uint32_t display, uint32_t *p_rows, uint32_t *p_source);
void build_ranking_display(void);
void ranking_title_spin_in(void);
void ranking_title_pause(void);
void ranking_title_rise(void);
void draw_ranking_title(void);
void ranking_display_scroll_in(void);
void ranking_display_hold(void);
void drift_ranking_background(void);
void ranking_display_end(void);
void ranking_row_update(void);

/* scenes/ring_tally.c: the ring tally after a stage (game mode 0x1D): the total and the players' panels */
void start_ring_tally(uint32_t *p_stage_rings_3, uint32_t *p_player_3_out_flags, uint32_t *p_ring_total, uint32_t *p_p2_object, uint32_t *p_p3_object, uint32_t *p_object);
void pick_ring_bonus_players(uint32_t *p_player_1_rings, uint32_t *p_player_2_rings, uint32_t *p_player_3_rings);
void ring_tally_percent_update(void);
void ring_tally_percent_set_digits(uint8_t shown_percent);
void ring_tally_percent_draw_digits(void);
void update_ring_tally_percent(void);
void ring_tally_end_sound_wait(void);
void ring_tally_ticker_update(void);
void ring_tally_player_1_update(void);
void ring_tally_player_2_update(void);
void ring_tally_player_3_update(void);
void ring_tally_player_3_update_spare_return(void);
void build_ring_tally_panel(uint32_t character);
void update_ring_tally_panel(void);
void update_ring_tally_panel_count_up(void);
void update_ring_tally_panel_tail(void);
void cycle_ring_tally_backdrop_palette(void);

/* scenes/title.c: the title screen: the SEGA logo, the emblem, the land, the craft, Sonic's loop, the clouds and the sky */
void run_title_screen(uint32_t text_position, uint32_t *p_color, uint32_t *p_row_pairs_left, uint32_t *p_blocks_per_row, uint32_t *p_text);
void title_emblem_animate(void);
void title_emblem_animate_spare_return(void);
void draw_title_emblem_part(void);
void title_land_slide(void);
void timed_spawner_run(void);
void title_flying_craft_move(void);
void title_smoke_puff_animate(void);
void init_title_puff_sprite(uint32_t puff);
void title_clouds_1_drift(void);
void title_clouds_2_drift(void);
void title_clouds_3_drift(void);
void init_title_cloud_block(uint32_t cloud);
void drift_title_cloud(void);
void drift_title_cloud_2(void);
void title_sky_wave(void);
void run_sega_logo(uint32_t logo_block, uint32_t *p_color, uint32_t *p_row_pairs_left, uint32_t *p_blocks_per_row, uint32_t *p_lower_row_cursor);
void set_sega_logo_sprite(void);

/* stage/camera.c: the stage view, the scroll and the camera: the view set-up and the tile sets around it, the scroll modes and the screen edges the players push, the camera's speeds, shake and path along the sections, the fades between stages, the start section of the demos and the unused scroll skip panel */
uint32_t compute_camera_ground_speed(uint32_t octant);
uint32_t compute_camera_slope_speed(uint32_t octant, uint32_t slope_factor);
void init_stage_view(uint32_t *p_direction_y, uint32_t *p_direction_x, uint32_t *p_count, uint32_t *p_stage_offset);
void update_stage_tile_layers(uint32_t *p_shifted_tile_set, uint32_t *p_copy_entry, uint32_t *p_columns_left, uint32_t *p_palette_count, uint32_t *p_stage_number, uint32_t *p_row_skip, uint32_t *p_cycle, uint32_t *p_block_cursor, uint32_t *p_dest);
void reload_changed_tile_set(uint32_t *p_shifted_tile_set, uint32_t *p_slots_left, uint32_t *p_set_shift, uint32_t *p_slot_tile_set, uint32_t *p_loaded_shift, uint32_t *p_script, uint32_t *p_dest);
void load_tile_set(uint32_t tile_set, uint32_t *p_count, uint32_t *p_stage_offset, uint32_t *p_script, uint32_t *p_dest);
uint32_t get_tile_set_script_bank(uint32_t script);
uint32_t get_tile_set_list_bank(uint32_t script_list);
void get_tile_set_list(uint32_t tile_set, uint32_t *p_stage_offset, uint32_t *p_script_list);
void clamp_camera_pixels(void);
void update_scroll(uint32_t *p_position, uint32_t *p_speed, uint32_t *p_speed_value, uint32_t *p_next_scroll_position, uint32_t *p_second_player_object, uint32_t *p_third_player_object, uint32_t *p_end_script, uint32_t *p_dest);
uint32_t is_player_pushing_edge(uint32_t pushing, uint32_t object);
uint32_t is_player_at_diagonal_edge_30(uint32_t at_edge, uint32_t object, uint16_t edge_offset);
uint32_t is_player_at_diagonal_edge_f0(uint32_t at_edge, uint32_t object, uint16_t edge_offset);
uint32_t is_player_at_diagonal_edge_c0(uint32_t at_edge, uint32_t object, uint16_t edge_offset);
uint32_t is_player_at_diagonal_edge_60(uint32_t at_edge, uint32_t object, uint16_t edge_offset);
uint32_t is_player_at_right_edge(uint32_t at_edge, uint32_t object);
uint32_t is_player_at_left_edge(uint32_t at_edge, uint32_t object);
uint32_t is_player_at_top_edge(uint32_t at_edge, uint32_t object);
uint32_t is_player_at_bottom_edge(uint32_t at_edge, uint32_t object);
void set_screen_push_edges(uint32_t quadrant_bits, uint32_t *p_count, uint32_t *p_sine_entry);
uint32_t is_player_facing_scroll(uint32_t facing, uint16_t heading, uint16_t sine_entry);
void is_direction_in_quadrant(uint32_t count, uint32_t *p_found, uint32_t *p_sine_entry);
void scroll_slowing_down(uint32_t *p_other_list, uint32_t *p_speed, uint32_t *p_speed_value, uint32_t *p_next_scroll_position, uint32_t *p_range, uint32_t *p_quadrant_cursor);
void scroll_speeding_up(uint32_t *p_other_list, uint32_t *p_speed, uint32_t *p_speed_value, uint32_t *p_next_scroll_position, uint32_t *p_range, uint32_t *p_quadrant_cursor);
void scroll_at_scroll_speed(uint32_t *p_other_list, uint32_t *p_speed, uint32_t *p_speed_value, uint32_t *p_next_scroll_position, uint32_t *p_range, uint32_t *p_quadrant_cursor);
void advance_scroll_unless_ended(uint32_t *p_other_list, uint32_t *p_speed, uint32_t *p_speed_value, uint32_t *p_next_scroll_position, uint32_t *p_range, uint32_t *p_quadrant_cursor);
void advance_scroll(uint32_t *p_other_list, uint32_t *p_speed, uint32_t *p_speed_value, uint32_t *p_next_scroll_position, uint32_t *p_range, uint32_t *p_quadrant_cursor);
void set_camera_velocity(uint32_t *p_scroll_y, uint32_t *p_speed, uint32_t *p_speed_value, uint32_t *p_next_scroll_position, uint32_t *p_range, uint32_t *p_quadrant_cursor);
void update_background_scroll(uint32_t *p_fast_scroll_start, uint32_t *p_scroll_y, uint32_t *p_speed, uint32_t *p_speed_value, uint32_t *p_next_scroll_position, uint32_t *p_range, uint32_t *p_quadrant_cursor);
void update_scroll_at_end(uint32_t *p_position, uint32_t *p_speed, uint32_t *p_speed_value, uint32_t *p_next_scroll_position, uint32_t *p_end_script, uint32_t *p_dest);
void spawn_debug_sprite_mover(uint32_t object);
void debug_sprite_mover_update(void);
void start_stage_view(uint32_t *p_direction_y, uint32_t *p_direction_x, uint32_t *p_count, uint32_t *p_stage_offset);
void call_run_map_screen(uint32_t object);
void run_stage_05_switch(uint32_t speed, uint32_t *p_scroll_y, uint32_t *p_speed_value, uint32_t *p_dest);
uint32_t update_fade(uint32_t delta);
void update_stage_00_exit_scene(uint32_t dest, uint32_t *p_speed_value, uint32_t *p_script);
void clamp_colour_offsets(void);
uint32_t update_camera_with_shake(void);
uint32_t set_nbg0_scroll_from_camera(uint32_t scroll_x);
uint32_t CONSOLE_ORIGINAL(set_nbg0_scroll_from_camera)(uint32_t scroll_x);
uint32_t update_camera_view(void);
uint32_t update_camera_view_y(uint32_t y);
uint32_t move_camera_along_path(uint32_t speed);
void advance_camera_scroll_x_y(uint32_t speed);
void advance_camera_x_y(uint32_t speed);
void advance_camera_y_scroll(uint32_t speed);
void advance_camera_y(uint32_t speed);
void advance_camera_x_scroll(uint32_t speed);
void advance_camera_x(uint32_t speed);
void advance_scroll_position(uint16_t speed);
void snap_camera_to_section_end(void);
void run_scroll_skip_mode(void);
void scroll_skip_next_point(void);
void scroll_skip_previous_point(void);
void scroll_skip_check_exit(void);
void scroll_skip_erase_panel(void);
void scroll_skip_check_call(void);
void move_camera_to_start_section(uint32_t *p_y, uint32_t *p_section_offset, uint32_t *p_stage_index);
void load_camera_start_section(uint32_t *p_section_offset, uint32_t *p_stage_index);
void index_stage_list_pair(uint32_t section_offset, uint32_t stage_offset, uint32_t *p_first_entry, uint32_t *p_second_entry);
uint32_t scroll_skip_x_position_cell(void);
uint32_t scroll_skip_y_position_cell(void);
uint32_t scroll_skip_total_cell(void);
void scroll_skip_draw_values(void);
void scroll_skip_print_panel(void);
void clear_bitmap_lower_half(void);

/* stage/events.c: the scripted game modes run_game_mode jumps into and what they spawn: platforms, rocks, lightning, message boxes, the escape alarm and countdown */
void escape_alarm_flash(void);
void event_0f_start(void);
void event_0f_stop_scroll(void);
void event_0f_show_messages(uint32_t *p_frames, uint32_t *p_box);
void spawn_falling_rocks(uint32_t object);
void falling_rock_bounce(void);
void event_0c_start(uint32_t *p_block_offset, uint32_t *p_slope_height, uint32_t *p_object);
uint32_t event_0c_first_quake(uint32_t write_index_address);
void event_0c_second_quake(uint32_t *p_part_size, uint32_t *p_point_y, uint32_t *p_z, uint32_t *p_wall);
void event_0c_finish(void);
void update_stage_05_event(uint32_t position, uint32_t speed, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void stage_05_event_show_message_11(uint32_t part_size, uint32_t box);
void stage_05_event_show_message_12(uint32_t part_size, uint32_t box);
void stage_05_event_show_message(uint32_t part_size, uint16_t screen_y, uint16_t kind, uint32_t box, uint32_t *p_sag, uint32_t *p_frames, uint32_t *p_z, uint32_t *p_platform);
void stage_05_event_step_3_frame(uint32_t part_size, uint32_t wall, uint32_t *p_sag, uint32_t *p_point_y, uint32_t *p_z, uint32_t *p_platform);
void stage_00_platform_start(void);
void stage_00_platform_stand(void);
void stage_00_platform_break(void);
void stage_00_geyser_rise(void);
void stage_00_geyser_column_start(void);
void stage_00_geyser_column_rise(void);
void rock_debris_fly(void);
void event_1c_follow(void);
void event_1c_stop(void);
void event_1c_scroll(void);
void update_ending(uint32_t position, uint32_t speed, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_platform);
void spawn_stage_06_lightning(uint32_t *p_record, uint32_t *p_bolt);
void stage_06_lightning_animate(void);
uint32_t spawn_message_box(uint16_t screen_x, uint16_t screen_y, uint16_t kind, uint16_t frames, uint32_t box);
void spawn_message_box_2(uint16_t screen_x, uint16_t screen_y, uint16_t kind, uint16_t frames, uint32_t box);
void place_message_box(uint16_t screen_x, uint16_t screen_y, uint32_t kind, uint16_t frames, uint32_t box);
void CONSOLE_ORIGINAL(place_message_box)(uint16_t screen_x, uint16_t screen_y, uint32_t kind, uint16_t frames, uint32_t box);
void message_box_show(void);
void start_escape_countdown(uint32_t *p_free_stack, uint32_t *p_count, uint32_t *p_digit);
void update_escape_countdown(void);
void start_escape_alarm_flash(uint32_t flash);

/* stage/game_modes.c: the game modes (run_game_mode) and what a stage frame runs: starting a stage, layer blending, the stage clear check, the player summary, stage slots and events, restart points, the player objects, the GAME OVER banner and the demo prompt */
void run_game_mode(uint32_t pressed, uint32_t position, uint32_t speed, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void CONSOLE_ORIGINAL(run_game_mode)(uint32_t pressed, uint32_t position, uint32_t speed, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_start_game(uint32_t *p_count, uint32_t *p_list_cursor);
void start_stage(uint32_t stage_offset, uint32_t *p_write_index_address, uint32_t *p_floor_block, uint32_t *p_bounds_offset, uint32_t *p_corners_left, uint32_t *p_palette_record, uint32_t *p_source, uint32_t *p_object);
void game_mode_play_stage(uint32_t pressed, uint32_t position, uint32_t in_play_count, uint32_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_intro(uint32_t position, uint32_t speed, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_continue(uint32_t position, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
uint32_t game_mode_stage_end_fade(uint32_t bottom_speed, uint32_t object);
void game_mode_screen_wipe(uint32_t position, uint32_t speed, uint32_t edges_done, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_stage_clear(uint32_t position, uint32_t speed, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_game_over(uint32_t box_edge, uint32_t position, uint16_t scale, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_character_select(uint32_t position, uint32_t speed, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_stage_results(uint32_t position, uint32_t speed, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_stage_change_fade(uint32_t delta, uint32_t position, uint32_t in_play_count, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_you_failed(uint32_t box_edge, uint32_t position, uint32_t in_play_count, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void check_stage_select_buttons(void);
void game_mode_stage_select(uint32_t *p_count, uint32_t *p_list_cursor);
void CONSOLE_ORIGINAL(game_mode_stage_select)(uint32_t *p_count, uint32_t *p_list_cursor);
void game_mode_event_09(uint32_t speed, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_0A(uint32_t pressed, uint32_t position, uint32_t speed, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_0C(uint32_t position, uint32_t speed, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_0D(uint32_t position, uint32_t speed, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_1A(uint32_t pressed, uint32_t position, uint32_t in_play_count, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_0F(uint32_t position, uint32_t speed, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_10(uint32_t random_value, uint32_t object, uint32_t *p_rolls_left, uint32_t *p_words_left, uint32_t *p_list_cursor);
void game_mode_event_12(uint32_t pressed, uint32_t position, uint32_t done_count, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_13(uint32_t pressed, uint32_t position, uint32_t in_play_count, uint32_t edges_done, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_14(uint32_t pressed, uint32_t position, uint32_t unbounced_count, uint16_t x, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_15(uint32_t pressed, uint32_t position, uint32_t speed, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_16(uint32_t box_edge, uint32_t position, uint32_t in_play_count, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_18(uint32_t position, uint32_t speed, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_19(uint32_t pressed, uint32_t position, uint32_t in_play_count, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_1C(uint32_t pressed, uint32_t position, uint32_t in_play_count, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void game_mode_event_1F(uint32_t pressed, uint32_t position, uint32_t in_play_count, uint16_t next_scroll_position, uint32_t object, uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor);
void cleared_banner_wait(void);
void stage_clear_banner_reveal(void);
void stage_clear_banner_hold(void);
void spawn_banner_sparkle(uint16_t offset_x);
void banner_sparkle_animate(void);
void banner_letter_spin(void);
void apply_stage_layer_blending(uint32_t *p_layer_bit, uint32_t *p_count);
void check_stage_clear(uint32_t *p_count, uint32_t *p_finished_flag, uint32_t *p_status);
uint32_t clear_player_blocks(void);
uint32_t update_grey_screen(void);
void update_player_summary(uint32_t *p_in_play_count, uint32_t *p_count, uint32_t *p_cursor, uint32_t *p_player_object);
void continue_to_next_stage_slot(uint32_t *p_table_offset, uint32_t *p_link_record, uint32_t *p_block);
void load_stage_map_pointers(uint32_t *p_table_offset, uint32_t *p_bounds_list);
void run_stage_events(uint32_t *p_table_offset, uint32_t *p_event);
void reset_trackball_step(uint32_t player_number);
void read_trackball_step(uint32_t *p_player_number, uint32_t *p_trackball);
uint32_t update_restart_point(uint32_t position);
uint32_t start_player_objects_alt(uint32_t object);
uint32_t start_player_objects(uint32_t object);
void reset_objects_from_list(uint32_t object, uint32_t *p_player_number, uint32_t *p_handler_cursor);
void draw_game_over_banner(void);
uint32_t blink_demo_prompt(uint32_t prompt_frame, uint32_t credit_block);

/* stage/layers.c: the stage's background: the palettes animated per stage, the palette sets and the backdrop, and the tilemap layers NBG0-NBG3 (pictures, mixing, per-stage scroll) */
void update_stage_06_background(uint32_t random_value, uint32_t dest, uint32_t *p_rolls_left, uint32_t *p_words_left, uint32_t *p_source);
void animate_stage_palette(uint32_t *p_copy_offset, uint32_t *p_copy_entry, uint32_t *p_palette_count, uint32_t *p_cycle, uint32_t *p_source, uint32_t *p_dest);
void skip_stage_palette_animation(void);
void animate_palette_stage_10(uint32_t *p_copy_offset, uint32_t *p_copy_entry, uint32_t *p_palette_count, uint32_t *p_cycle, uint32_t *p_source, uint32_t *p_dest);
void animate_palette_stage_0c(uint32_t *p_copy_offset, uint32_t *p_copy_entry, uint32_t *p_palette_count, uint32_t *p_cycle, uint32_t *p_source, uint32_t *p_dest);
void animate_palette_stage_0d_0e(uint32_t *p_copy_offset, uint32_t *p_copy_entry, uint32_t *p_palette_count, uint32_t *p_cycle, uint32_t *p_source, uint32_t *p_dest);
void animate_palette_stage_0f(uint32_t *p_copy_offset, uint32_t *p_copy_entry, uint32_t *p_palette_count, uint32_t *p_cycle, uint32_t *p_source, uint32_t *p_dest);
void animate_palette_stage_0a(uint32_t *p_copy_offset, uint32_t *p_copy_entry, uint32_t *p_palette_count, uint32_t *p_cycle, uint32_t *p_source, uint32_t *p_dest);
void animate_palette_stage_09(uint32_t *p_copy_offset, uint32_t *p_copy_entry, uint32_t *p_palette_count, uint32_t *p_cycle, uint32_t *p_source, uint32_t *p_dest);
void animate_palette_stage_04(uint32_t *p_copy_offset, uint32_t *p_copy_entry, uint32_t *p_palette_count, uint32_t *p_cycle, uint32_t *p_source, uint32_t *p_dest);
void cycle_palette(uint32_t cycle, uint16_t halfwords, uint32_t *p_copy_offset, uint32_t *p_copy_entry, uint32_t *p_palette_count, uint32_t *p_source, uint32_t *p_dest);
uint32_t cycle_palette_set_0(uint32_t source);
uint32_t cycle_palette_set_1(uint32_t source);
uint32_t cycle_palette_set(uint32_t source);
void load_stage_palette_set(uint32_t *p_tile_bank_bits, uint32_t *p_dest);
void load_palette_set_and_backdrop(uint32_t stage_index, uint32_t *p_tile_bank_bits, uint32_t *p_source, uint32_t *p_dest);
void set_backdrop_color_19aa(void);
uint32_t init_nbg0_picture_screen(void);
void show_winners_dont_use_drugs_screen(void);
void show_recycle_it_screen(void);
void show_nbg0_picture(uint32_t *p_copy_entry, uint32_t *p_rows, uint32_t *p_source, uint32_t *p_dest);
uint32_t set_stage_layer_mixing(void);
uint32_t init_nbg1_layer(uint32_t dest);
void update_nbg1_scroll(void);
void skip_nbg1_scroll(void);
void set_nbg1_scroll_to_camera(void);
uint32_t init_nbg2_layer(uint32_t dest);
uint32_t update_nbg2_scroll(uint32_t position);
void set_nbg2_scroll_to_camera(void);
void skip_nbg2_scroll(void);
void set_nbg2_scroll_to_camera_2(void);
uint32_t set_nbg2_scroll_stages_03_07(void);
void set_nbg2_scroll_stage_04(void);
void init_nbg3_layer(uint32_t dest);
void init_nbg3_layer_cleared_pages(uint32_t dest);
void set_nbg3_pages_5_6(void);
void set_nbg3_page_5_flags(uint32_t *p_count, uint32_t *p_dest);
void update_nbg3_scroll(uint32_t *p_scroll_y, uint32_t *p_quadrants, uint32_t *p_speed_value, uint32_t *p_next_scroll_position, uint32_t *p_quadrant_cursor);
void select_nbg3_pages(uint32_t *p_block_pages, uint32_t *p_quadrants, uint32_t *p_byte_mask, uint32_t *p_shift, uint32_t *p_quadrant_cursor);
void get_nbg3_block(void);
void load_stage_palette(uint32_t *p_palette_record, uint32_t *p_source, uint32_t *p_dest);

/* system/coins.c: coins and credits: the coin inputs and counters, the credits and start prompts drawn outside a game */
void apply_settings(void);
void update_coins(void);
void pulse_coin_counter(uint32_t counter_bit, uint32_t counter_state, uint16_t service_credits, uint16_t coin_credits);
uint32_t add_service_credit(uint32_t service_credits, uint32_t credit_block, uint8_t max_credits);
uint32_t add_coin(uint32_t chute_offset, uint32_t pending_offset, uint32_t bookkeeping_chute, uint32_t coin_credits, uint32_t credit_block, uint8_t max_credits);
void save_credits(void);
void copy_credit_counts(uint32_t dest, uint32_t credit_block);
void draw_attract_credits(uint32_t *p_credits, uint32_t *p_blink_bit, uint32_t *p_message_text, uint32_t *p_text_position);
void erase_attract_credits(void);
void blink_start_prompt(uint32_t credit_block, uint32_t prompt_texts, uint32_t *p_blink_bit, uint32_t *p_text, uint32_t *p_text_position);
void CONSOLE_ORIGINAL(blink_start_prompt)(uint32_t credit_block, uint32_t prompt_texts, uint32_t *p_blink_bit, uint32_t *p_text, uint32_t *p_text_position);
void erase_start_prompt(uint32_t text_position, uint32_t prompt_texts);
void draw_credit_count(uint32_t credit_block, uint32_t *p_credits, uint32_t *p_coins_per_credit, uint32_t *p_text, uint32_t *p_text_position);
void draw_credit_count_now(uint32_t credit_block, uint32_t *p_credits, uint32_t *p_coins_per_credit, uint32_t *p_text, uint32_t *p_text_position);
void CONSOLE_ORIGINAL(draw_credit_count_now)(uint32_t credit_block, uint32_t *p_credits, uint32_t *p_coins_per_credit, uint32_t *p_text, uint32_t *p_text_position);
void build_fraction_glyph(uint32_t numerator, uint32_t denominator, uint32_t *p_numerator_top_rows, uint32_t *p_numerator_bottom_rows, uint32_t *p_denominator_top_rows, uint32_t *p_denominator_bottom_rows);
void erase_credit_count(uint32_t text_position);
uint32_t add_to_credit_pool(uint32_t credits_to_add);
void take_from_credit_pool(uint8_t credits_to_take);

/* system/main.c: the machine's main program: power-on and boot, the frame loop, the attract mode and the system modes, the frame wait and the controls, the interrupt handlers, the reset vector */
void boot(void);
void run_frame(uint32_t pressed, uint32_t released, uint32_t input_cursor, uint32_t count, uint32_t trackball_port, uint32_t trackball_cursor, uint32_t *p_scale, uint32_t *p_cursor, uint32_t *p_height);
void run_frame_check_p2_start(uint32_t credits, uint32_t blink_bit, uint32_t credits_needed, uint32_t count, uint32_t message_text, uint32_t text_position, uint32_t *p_scale, uint32_t *p_cursor, uint32_t *p_height);
void run_frame_start_game(uint32_t credits, uint32_t blink_bit, uint32_t credits_needed, uint32_t count, uint32_t message_text, uint32_t text_position, uint32_t *p_scale, uint32_t *p_cursor, uint32_t *p_height);
void CONSOLE_ORIGINAL(run_frame_start_game)(uint32_t credits, uint32_t blink_bit, uint32_t credits_needed, uint32_t count, uint32_t message_text, uint32_t text_position, uint32_t *p_scale, uint32_t *p_cursor, uint32_t *p_height);
void attract_title(uint32_t *p_color, uint32_t *p_row_pairs_left, uint32_t *p_blocks_per_row, uint32_t *p_text);
void CONSOLE_ORIGINAL(attract_title)(uint32_t *p_color, uint32_t *p_row_pairs_left, uint32_t *p_blocks_per_row, uint32_t *p_text);
void attract_ranking(uint32_t *p_rows, uint32_t *p_source);
void attract_demo_1(uint32_t pressed, uint32_t released, uint32_t input_cursor, uint16_t cursor, uint32_t trackball_cursor, uint32_t *p_scale, uint32_t *p_count, uint32_t *p_height, uint32_t *p_trackball_port);
void attract_demo_2(uint32_t pressed, uint32_t released, uint32_t input_cursor, uint16_t cursor, uint32_t trackball_cursor, uint32_t *p_scale, uint32_t *p_count, uint32_t *p_height, uint32_t *p_trackball_port);
void advance_attract_screen(void);
void CONSOLE_ORIGINAL(advance_attract_screen)(void);
void init_system(uint32_t *p_polynomial, uint32_t *p_words_left);
void init_mixer(void);
void init_io_ports(void);
void clear_work_ram(void);
uint32_t init_game_settings(void);
void delay_after_reset(void);
uint32_t wait_for_frames(void);
void wait_for_frame(uint32_t *p_pressed, uint32_t *p_released, uint32_t *p_input_cursor, uint32_t *p_count, uint32_t *p_trackball_port, uint32_t *p_trackball_cursor);
void read_controls(uint32_t *p_inputs, uint32_t *p_pressed, uint32_t *p_released, uint32_t *p_input_cursor, uint32_t *p_count, uint32_t *p_trackball_port, uint32_t *p_trackball_cursor);
uint32_t play_demo_buttons(uint32_t inputs);
void play_demo_trackballs(uint32_t *p_count, uint32_t *p_demo_cursor, uint32_t *p_trackball_cursor);
void pause_on_start_3(void);
void split_frame_count(uint32_t frames, uint32_t frames_high, uint32_t *p_seconds, uint32_t *p_minutes, uint32_t *p_hours, uint32_t *p_days);
void attract_warning(uint32_t *p_pixels, uint32_t *p_text);
void CONSOLE_ORIGINAL(attract_warning)(uint32_t *p_pixels, uint32_t *p_text);
void system_mode_idle(void);
void CONSOLE_ORIGINAL(system_mode_idle)(void);
void wait_attract_timer(void);
void irq_vblank_start(void);
void irq_vblank_end(void);
void irq_sound(void);
void irq_timer0(void);
void irq_timer1(void);
void int_default(uint32_t *p_source, uint32_t *p_dest, uint32_t *p_copies_left, uint32_t *p_copy_entry);
void int_vector_02(uint32_t *p_source, uint32_t *p_dest, uint32_t *p_copies_left, uint32_t *p_copy_entry);
void run_vblank_copies(uint32_t *p_source, uint32_t *p_dest, uint32_t *p_copies_left, uint32_t *p_copy_entry);
void send_sound_command(void);
void read_service_inputs(void);
void count_frames_and_check_test_switch(void);
void reset(void);

/* system/settings.c: what the board keeps when it is off: the settings in the 93C46 EEPROM and the bookkeeping in the backup RAM, with their checksums */
uint32_t compute_crc16_strided(uint32_t crc, uint32_t count, uint32_t data, uint32_t byte_gap);
void eeprom_lock(void);
void eeprom_unlock(void);
uint32_t eeprom_read_word_at(uint32_t data, uint8_t command);
void load_settings(uint32_t crc);
void eeprom_read_all_and_enable_interrupts(uint32_t data);
void eeprom_read_all(uint32_t data);
void eeprom_write_word_at(uint16_t data, uint8_t command);
void eeprom_save_and_lock(uint32_t *p_command, uint32_t *p_words_left);
void eeprom_save(uint32_t *p_command, uint32_t *p_words_left);
void eeprom_read_word(uint8_t command, uint32_t port_f, uint32_t port_d, uint32_t *p_data, uint32_t *p_lines);
uint32_t eeprom_write_word(uint32_t data, uint32_t lines, uint8_t command, uint32_t port_f, uint32_t port_d);
uint32_t eeprom_send_command(uint32_t lines, uint8_t command, uint32_t port_d);
void eeprom_select(uint32_t *p_lines, uint32_t *p_port_f, uint32_t *p_port_d);
uint32_t compute_eeprom_backup_checksum(void);
void compute_eeprom_checksum(uint32_t *p_crc, uint32_t *p_polynomial);
void compute_eeprom_crc(uint32_t *p_crc, uint32_t *p_polynomial);
uint32_t eeprom_deselect(uint32_t lines, uint32_t port_d);
uint32_t eeprom_set_lines(uint32_t data, uint8_t lines, uint32_t port_d);
void load_bookkeeping(void);
void reset_bookkeeping(void);
uint32_t update_bookkeeping_checksum(void);
void compute_bookkeeping_crc(uint32_t *p_crc, uint32_t *p_data);
void compute_crc16(uint32_t count, uint32_t *p_crc, uint32_t *p_data, uint32_t *p_polynomial);
uint32_t compute_crc16_alternate_bytes(uint32_t data, uint32_t count);
void add_play_time(void);
void record_player_game(void);

/* system/sound.c: the sound the game asks the Z80 for: init_sound, play_sound and the sound queue */
uint32_t init_sound(void);
void set_port_d_bit_3_if_sound_allowed(void);
void queue_coin_sound(void);
void play_sound(uint8_t sound_id, uint32_t *p_write_index, uint32_t *p_write_index_address);
void CONSOLE_ORIGINAL(play_sound)(uint8_t sound_id, uint32_t *p_write_index, uint32_t *p_write_index_address);
void queue_sound(uint8_t sound_id, uint32_t *p_write_index, uint32_t *p_write_index_address);

/* system/test_mode.c: the test mode: the menu, the memory, input and sound tests, the CRT test, the game and coin assignments, the bookkeeping screen, the backup clear, the output test, the exit */
void start_test_mode(uint32_t *p_color, uint32_t *p_blocks_per_row);
void run_test_mode(uint32_t pressed, uint16_t count, uint32_t *p_color, uint32_t *p_blocks_per_row);
void CONSOLE_ORIGINAL(run_test_mode)(uint32_t pressed, uint16_t count, uint32_t *p_color, uint32_t *p_blocks_per_row);
void test_menu(void);
void draw_test_menu(void);
void toggle_test_continue_mode(void);
void test_memory(uint32_t *p_results_left, uint32_t *p_blocks_per_row);
void check_rom_checksums(void);
void BOARD_ORIGINAL(check_rom_checksums)(void);
void test_work_ram(void);
void test_work_ram_verify(void);
void test_video_ram(void);
void test_video_ram_verify(void);
void test_sprite_ram(void);
void test_sprite_ram_verify(void);
void test_palette_ram(void);
void test_palette_ram_verify_first_half(void);
void test_palette_ram_fill_second_half(void);
void test_palette_ram_verify_second_half(void);
void test_sound_ram(void);
void test_sound_ram_verify(void);
void test_inputs(uint16_t count, uint32_t *p_panel_color, uint32_t *p_blocks_per_row);
void move_input_test_cursors(uint32_t count, uint32_t text, uint32_t *p_cursors_left, uint32_t *p_height);
uint32_t get_trackball_quadrant(uint8_t dx, uint8_t dy, uint32_t quadrant);
void test_sound(void);
void wrap_sound_test_number(void);
void wrap_sound_test_number_down(void);
void test_crt(uint32_t start_y, uint32_t *p_color, uint32_t *p_blocks_per_row);
void test_game_assignments(uint32_t *p_color, uint32_t *p_blocks_per_row);
void draw_vitality_setting(uint32_t *p_color, uint32_t *p_blocks_per_row);
void draw_difficulty_setting(uint32_t *p_color, uint32_t *p_blocks_per_row);
uint32_t test_coin_assignments(uint32_t color);
void test_coin_assignments_draw_main_page(void);
void test_coin_assignments_spare_return(void);
void apply_coin_setting(void);
void draw_coin_setting_number(uint32_t setting_number);
uint32_t test_coin_assignments_manual_page(uint32_t color);
void test_coin_assignments_spare_return_2(void);
void clamp_coin_setting(uint32_t setting);
uint32_t draw_coin_credit_table(void);
uint32_t draw_coin_credit_cell(uint16_t multiplier, uint16_t coins, uint32_t cell_position);
void draw_test_hex_4(uint32_t value, uint32_t text_position);
void draw_test_hex_2(uint32_t value, uint32_t text_position);
void test_bookkeeping(void);
uint32_t test_backup_clear(uint32_t polynomial);
void test_outputs(void);
void exit_test_mode(void);
void flip_test_coordinates(uint32_t *p_left, uint32_t *p_top, uint32_t *p_right, uint32_t *p_bottom);
void blink_test_cursor(void);
void draw_blinking_test_cursor(uint32_t cursor_char);
void erase_test_cursor(uint32_t cursor_positions);
void set_test_cursor(void);
uint32_t check_moving_seat_dip(uint32_t words_left);

/* video/bitmap.c: the bitmap layer: clearing it, its address, scroll and window, pixels, lines, spans and rectangles; an unused packed picture decoder and LZ unpacker */
void draw_packed_picture(uint32_t picture);
void init_picture_unpacker(uint32_t *p_colour_cache, uint32_t *p_newest_slot, uint32_t *p_done_bits);
void init_picture_colour_cache(uint32_t colour_cache);
uint32_t read_picture_size(uint32_t stream);
void unpack_picture_pixels(uint32_t stream, uint32_t width, uint32_t colour_cache, uint32_t newest_slot, uint32_t done_bits);
void read_picture_colour(uint32_t stream, uint32_t colour_cache, uint32_t *p_colour, uint32_t *p_newest_slot);
void rotate_bitmap_for_monitor_flip(void);
void init_bitmap_layer(uint8_t is_8bpp);
void get_bitmap_address_2(uint16_t x, uint16_t y);
void set_bitmap_scroll_2(uint16_t scroll_x, uint16_t scroll_y, uint8_t is_8bpp);
void set_bitmap_window_2(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
void set_bitmap_clip_2(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
void unpack_lz_bytes(uint32_t packed, uint32_t dest);
void clear_bitmap_layer(uint8_t is_8bpp, uint32_t *p_top, uint32_t *p_color, uint32_t *p_inner_count, uint32_t *p_row_pairs_left, uint32_t *p_blocks_per_row, uint32_t *p_lower_row_cursor);
void get_bitmap_address(uint16_t x, uint16_t y);
void set_bitmap_scroll(uint16_t scroll_x, uint16_t scroll_y, uint8_t is_8bpp);
void set_bitmap_window(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
void set_bitmap_clip(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
void draw_pixel(uint32_t x, uint32_t y, uint8_t color);
uint32_t read_pixel(uint32_t x, uint32_t y);
void draw_line(uint32_t end_x, uint32_t end_y, uint32_t *p_start_x, uint32_t *p_start_y, uint32_t *p_color, uint32_t *p_unclipped_y);
void fill_span_8bpp(uint32_t length, uint32_t *p_blocks, uint32_t *p_pixel_address, uint32_t *p_color);
uint32_t draw_vertical_line_8bpp(uint32_t pixel_address, uint8_t color, uint32_t length);
void draw_vertical_span_8bpp(uint32_t row_step, uint8_t color, uint32_t *p_pixel_address, uint32_t *p_length);
uint32_t draw_line_shallow_even_pixel(uint32_t pixel_address, uint16_t fraction, uint8_t low_nibble_color, uint8_t high_nibble_color, uint16_t slope, uint32_t steps_left, uint8_t high_nibble_mask, uint32_t row_step);
uint32_t draw_line_shallow_odd_pixel(uint32_t pixel_address, uint16_t fraction, uint8_t low_nibble_color, uint8_t high_nibble_color, uint16_t slope, uint32_t steps_left, uint8_t high_nibble_mask, uint32_t row_step);
uint32_t draw_line_shallow_set_high_nibble(uint8_t pixel_byte, uint32_t pixel_address, uint16_t fraction, uint8_t low_nibble_color, uint8_t high_nibble_color, uint16_t slope, uint32_t steps_left, uint8_t high_nibble_mask, uint32_t row_step);
void fill_span_4bpp(uint16_t end_x, uint32_t *p_start_x, uint32_t *p_start_y, uint32_t *p_color, uint32_t *p_length);
void draw_vertical_span_4bpp(uint16_t start_x, uint32_t row_step, uint32_t *p_start_y, uint32_t *p_color, uint32_t *p_length);
void draw_vertical_span_4bpp_here(uint8_t start_x, uint32_t row_step, uint32_t *p_pixel_address, uint32_t *p_color, uint32_t *p_length);
void draw_dotted_line_8bpp(uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint8_t color);
void draw_rect_outline(uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint32_t color);
uint32_t draw_rect_sides_8bpp(uint32_t pixel_address, uint32_t width, uint32_t height, uint8_t color);
uint32_t draw_rect_top_bottom_8bpp(uint32_t pixel_address, uint32_t width, uint32_t height, uint32_t color);
void fill_rect(uint32_t top, uint32_t right, uint32_t bottom, uint32_t *p_left, uint32_t *p_color, uint32_t *p_blocks_per_row);
void fill_rect_unclipped(uint32_t right, uint32_t bottom, uint32_t *p_left, uint32_t *p_top, uint32_t *p_color, uint32_t *p_inner_count, uint32_t *p_row_pairs_left, uint32_t *p_blocks_per_row, uint32_t *p_lower_row_cursor);

/* video/shapes.c: shapes on the bitmap layer, mostly unused: triangles, quadrilaterals, circles, flood fill, saved rectangles, and the clipping the lines and rectangles use */
void fill_triangle(uint32_t top_x, uint32_t top_y, uint32_t middle_x, uint32_t middle_y, uint32_t bottom_x, uint32_t bottom_y, uint8_t color);
void fill_triangle_sorted(uint32_t top_x, uint32_t top_y, uint32_t middle_x, uint32_t middle_y, uint32_t bottom_x, uint32_t bottom_y, uint8_t color, uint32_t span_cursor);
void fill_flat_top_triangle(uint32_t left_x, uint32_t top_y, uint32_t right_x, uint32_t bottom_x, uint32_t height, uint8_t color, uint32_t span_cursor);
void fill_span_list_from_r8(uint32_t span_count, uint8_t polygon_color);
void fill_span_list(uint32_t span_count, uint32_t color);
void fill_quad(uint32_t left_x, uint32_t top_y, uint32_t second_x, uint32_t second_y, uint32_t third_x, uint32_t third_y, uint32_t bottom_x, uint32_t bottom_y, uint8_t fill_color);
void draw_circle_polygon(uint32_t center_x, uint32_t center_y, uint32_t radius, uint8_t circle_color, uint32_t color);
void draw_circle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
void plot_circle_point_4bpp(uint32_t point_x, uint32_t point_y, uint8_t high_color, uint8_t low_color, uint32_t bitmap_base, uint16_t window_top, uint16_t window_right, uint16_t window_bottom);
void plot_circle_point_8bpp(uint32_t point_x, uint32_t point_y, uint8_t color, uint32_t bitmap_base, uint16_t window_top, uint16_t window_right, uint16_t window_bottom);
void flood_fill(uint16_t seed_x, uint16_t seed_y, uint8_t fill_color, uint8_t border_color);
void flood_fill_push_span(uint8_t border_color, uint32_t window_left, uint32_t window_right, uint8_t direction, uint32_t left_x, uint8_t y, uint32_t fill_pattern, uint32_t *p_right_address, uint32_t *p_right_x);
void flood_fill_span(uint8_t border_color, uint32_t window_left, uint32_t window_right, uint32_t fill_pattern, uint32_t *p_left_x, uint32_t *p_right_address, uint32_t *p_right_x);
void flood_fill_span_left_pair(uint32_t scan_count, uint8_t pixel_count, uint8_t border_color, uint32_t window_right, uint32_t fill_pattern, uint32_t seed_address, uint32_t *p_left_x, uint32_t *p_right_address, uint32_t *p_right_x);
void flood_fill_span_left_pixel(uint8_t pixel, uint32_t scan_count, uint8_t pixel_count, uint8_t border_color, uint32_t window_right, uint32_t fill_pattern, uint32_t seed_address, uint32_t *p_left_x, uint32_t *p_right_address, uint32_t *p_right_x);
void flood_fill_span_set_left_x(uint8_t border_color, uint32_t window_right, uint32_t fill_pattern, uint32_t seed_address, uint32_t *p_left_x, uint32_t *p_right_address, uint32_t *p_right_x);
void flood_fill_span_scan_right(uint8_t border_color, uint32_t window_right, uint32_t left_x, uint32_t fill_pattern, uint32_t seed_address, uint32_t *p_right_address, uint32_t *p_right_x);
void flood_fill_span_right_pair(uint32_t scan_count, uint8_t pixel_count, uint8_t border_color, uint32_t left_x, uint32_t fill_pattern, uint16_t seed_address, uint32_t *p_right_cursor, uint32_t *p_right_x);
void flood_fill_span_right_pixel(uint8_t pixel, uint32_t scan_count, uint8_t pixel_count, uint8_t border_color, uint32_t left_x, uint32_t fill_pattern, uint16_t seed_address, uint32_t *p_right_cursor, uint32_t *p_right_x);
void flood_fill_span_set_right_x(uint32_t left_x, uint32_t fill_pattern, uint16_t seed_address, uint32_t *p_right_cursor, uint32_t *p_right_x);
uint32_t flood_fill_span_fill_run(uint32_t left_x, uint32_t fill_pattern, uint32_t right_x);
uint32_t flood_fill_scan_seed_rows(uint8_t border_color, uint32_t window_left, uint16_t window_top, uint32_t window_right, uint16_t window_bottom, uint32_t scan_x, uint16_t y, uint32_t fill_pattern);
uint32_t flood_fill_scan_row(uint8_t border_color, uint32_t window_left, uint32_t window_right, uint32_t direction, uint8_t y, uint32_t fill_pattern);
uint32_t flood_fill_scan_span_rows(uint8_t border_color, uint32_t window_left, uint16_t window_top, uint32_t window_right, uint16_t window_bottom, uint32_t scan_x, uint16_t y, uint32_t fill_pattern);
uint32_t flood_fill_scan_row_skip_parent(uint8_t border_color, uint32_t window_left, uint32_t window_right, uint32_t direction, uint32_t scan_x, uint8_t y, uint32_t fill_pattern);
uint32_t flood_fill_skip_parent_right(uint8_t border_color, uint32_t window_left, uint32_t window_right, uint8_t direction, uint32_t scan_x, uint8_t y, uint32_t fill_pattern);
void fill_span_at_point(uint32_t point_x, uint32_t point_y, uint8_t fill_color, uint8_t border_color, uint32_t right_cursor);
void fill_span_at_point_left_byte(uint32_t scan_count, uint8_t fill_color, uint8_t border_color, uint32_t left_x, uint32_t right_cursor, uint32_t right_x);
void fill_span_at_point_left_even_pixel(uint8_t pixel_byte, uint32_t scan_count, uint8_t fill_color, uint8_t border_color, uint32_t left_x, uint32_t right_cursor, uint32_t right_x);
void fill_span_at_point_scan_right(uint8_t fill_color, uint8_t border_color, uint32_t left_x, uint32_t right_cursor, uint32_t right_x);
void fill_span_at_point_right_byte(uint32_t scan_count, uint8_t fill_color, uint8_t border_color, uint32_t left_x, uint32_t right_cursor, uint32_t right_x);
void fill_span_at_point_right_odd_pixel(uint8_t pixel_byte, uint32_t scan_count, uint8_t fill_color, uint8_t border_color, uint32_t left_x, uint32_t right_cursor, uint32_t right_x);
void fill_span_at_point_fill_run(uint32_t fill_color, uint32_t left_x, uint32_t right_x);
void save_bitmap_rect(uint16_t left, uint16_t top, uint32_t right, uint32_t bottom, uint32_t buffer);
void draw_saved_bitmap_rect(uint32_t buffer);
void get_window_outcode(uint32_t *p_start_x, uint32_t *p_start_y, uint32_t *p_end_x, uint32_t *p_end_y, uint32_t *p_outcode);
void get_point_outcode(uint32_t *p_point_x, uint32_t *p_point_y, uint32_t *p_outcode);
void clip_line_to_window(uint32_t outcode, uint32_t end_outcode, uint32_t *p_start_x, uint32_t *p_start_y, uint32_t *p_end_x, uint32_t *p_end_y, uint32_t *p_unclipped_y);
void clip_line_end_inside(void);
void clip_line_to_left_edge(uint32_t end_x, uint32_t end_y, uint32_t *p_start_x, uint32_t *p_start_y, uint32_t *p_unclipped_y);
void clip_line_to_right_edge(uint32_t end_x, uint32_t end_y, uint32_t *p_start_x, uint32_t *p_start_y, uint32_t *p_unclipped_y);
void clip_line_at_x(uint32_t end_x, uint32_t end_y, uint32_t dx, uint32_t *p_start_x, uint32_t *p_start_y, uint32_t *p_unclipped_y);
void clip_line_to_top_edge(uint32_t end_x, uint32_t end_y, uint32_t *p_start_x, uint32_t *p_start_y, uint32_t *p_unclipped_y);
void clip_line_to_bottom_edge(uint32_t end_x, uint32_t end_y, uint32_t *p_start_x, uint32_t *p_start_y, uint32_t *p_unclipped_y);
void clip_line_at_y(uint32_t end_x, uint32_t end_y, uint32_t dy, uint32_t *p_start_x, uint32_t *p_start_y, uint32_t *p_unclipped_y);
void clip_rect_to_window(uint32_t *p_left, uint32_t *p_top, uint32_t *p_right, uint32_t *p_bottom, uint32_t *p_window_min, uint32_t *p_unclipped_edges);

/* video/text.c: text: the text layer's font, printing and erasing text, lines, fractions and digits, and the strings drawn with sprites */
void queue_string_record(uint32_t record, uint16_t y, uint32_t scale, uint32_t *p_palette, uint32_t *p_text);
void queue_string_at(uint32_t x, uint16_t y, uint32_t *p_palette, uint32_t *p_scale, uint32_t *p_text);
void queue_string(uint32_t x, uint16_t y, uint8_t mode, uint32_t *p_palette, uint32_t *p_scale, uint32_t *p_text);
void draw_queued_strings(uint16_t size, uint16_t width, uint32_t sprite_entry, uint32_t *p_scale, uint32_t *p_cursor, uint32_t *p_height);
void load_text_font(uint32_t *p_pixels, uint32_t *p_color_index);
void clear_text_layer(uint32_t *p_value, uint32_t *p_count, uint32_t *p_dest);
void expand_font(uint8_t color, uint32_t count, uint32_t source, uint32_t dest, uint32_t *p_pixels, uint32_t *p_color_index);
void expand_font_2bpp(uint8_t color, uint32_t count, uint32_t source, uint32_t dest);
void expand_font_2bpp_transparent(uint8_t color, uint32_t count, uint32_t pixels, uint32_t opaque_mask, uint32_t source, uint32_t dest);
void draw_fraction(uint16_t attribute, uint32_t *p_numerator, uint32_t *p_denominator, uint32_t *p_text_position);
void print_text_list(uint32_t *p_text, uint32_t *p_text_position);
void print_text(uint32_t *p_text, uint32_t *p_text_position);
void print_text_here(uint32_t *p_cell, uint32_t *p_text, uint32_t *p_text_position);
void erase_text_list(uint32_t text);
uint32_t erase_text(uint32_t text);
void erase_text_here(uint16_t cell, uint32_t *p_text, uint32_t *p_text_position);
void print_text_lines(uint32_t *p_text, uint32_t *p_text_position);
void erase_text_lines(uint32_t *p_text, uint32_t *p_text_position);
void print_formatted_text(uint32_t text);
void formatted_text_position_single(uint32_t text, uint32_t text_position);
void formatted_text_attribute_single(uint32_t text, uint32_t text_position);
void formatted_text_single(uint16_t cell, uint32_t text, uint32_t text_position);
void formatted_text_skip_single(uint16_t cell, uint32_t text, uint32_t text_position);
void formatted_text_skip_long_single(uint16_t cell, uint32_t text, uint32_t text_position);
void formatted_text_position_double(uint32_t text, uint32_t text_position);
void formatted_text_attribute_double(uint32_t text, uint32_t text_position);
void formatted_text_double(uint16_t cell, uint32_t text, uint32_t text_position);
void formatted_text_skip_double(uint16_t cell, uint32_t text, uint32_t text_position);
void formatted_text_skip_long_double(uint16_t cell, uint32_t text, uint32_t text_position);
void formatted_text_position_triple(uint32_t text, uint32_t text_position);
void formatted_text_attribute_triple(uint32_t text, uint32_t text_position);
void formatted_text_triple(uint16_t cell, uint32_t text, uint32_t text_position);
void formatted_text_skip_triple(uint16_t cell, uint32_t text, uint32_t text_position);
void formatted_text_skip_long_triple(uint16_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text(uint32_t cell, uint32_t text);
void erase_formatted_text_position_single(uint32_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text_attribute_single(uint32_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text_single(uint32_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text_skip_single(uint32_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text_position_double(uint32_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text_attribute_double(uint32_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text_double(uint32_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text_skip_double(uint32_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text_position_triple(uint32_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text_attribute_triple(uint32_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text_triple(uint32_t cell, uint32_t text, uint32_t text_position);
void erase_formatted_text_skip_triple(uint32_t cell, uint32_t text, uint32_t text_position);
uint32_t draw_hex_2(uint32_t value, uint32_t attribute, uint32_t text_position);
void draw_hex_4(uint32_t *p_value, uint32_t *p_attribute, uint32_t *p_text_position);
void draw_hex_6(uint32_t value, uint32_t attribute, uint32_t text_position);
void draw_hex_8(uint32_t value, uint32_t attribute, uint32_t text_position);
void draw_hex_digits(uint32_t digit_count, uint32_t *p_value, uint32_t *p_attribute, uint32_t *p_text_position);
void draw_credit_digits(uint32_t value, uint32_t *p_attribute, uint32_t *p_text_position);
void draw_bcd_4(uint32_t value, uint32_t attribute, uint32_t text_position);
void draw_bcd_6(uint32_t value, uint32_t attribute, uint32_t text_position);
void draw_bcd_8(uint32_t value, uint32_t attribute, uint32_t text_position);
void draw_bcd_digits(uint32_t value, uint32_t digit_count, uint32_t *p_attribute, uint32_t *p_text_position);

/* video/video.c: the board's video: the video registers and the screen mode, block copies and fills, palette RAM and its copies at VBLANK, the VRAM scripts that build the tilemaps */
void init_video_registers(void);
uint32_t clear_page_0_and_scroll(void);
void init_video_registers_layers_off(void);
void empty_vblank_hook(void);
void apply_video_registers(void);
uint32_t set_screen_mode(uint16_t mode);
void copy_blocks(uint32_t count, uint32_t *p_source, uint32_t *p_dest);
void clear_blocks(uint32_t *p_value, uint32_t *p_count, uint32_t *p_dest);
uint32_t fill_halfwords(uint16_t value, uint32_t count, uint32_t dest);
void run_vram_script(uint32_t *p_count, uint32_t *p_script, uint32_t *p_dest);
void vram_script_end(void);
void vram_script_set_address(uint32_t script_code, uint32_t *p_script, uint32_t *p_dest);
void vram_script_skip(uint32_t *p_script, uint32_t *p_dest);
void vram_script_skip_long(uint32_t *p_script, uint32_t *p_dest);
void vram_script_fill(uint32_t *p_count, uint32_t *p_script, uint32_t *p_dest);
void vram_script_count_up(uint32_t *p_count, uint32_t *p_script, uint32_t *p_dest);
void vram_script_count_down(uint32_t *p_count, uint32_t *p_script, uint32_t *p_dest);
void vram_script_bytes(uint32_t *p_count, uint32_t *p_script, uint32_t *p_dest);
void vram_script_words(uint32_t *p_count, uint32_t *p_script, uint32_t *p_dest);
void vram_script_words_to_dictionary(uint32_t *p_count, uint32_t *p_script, uint32_t *p_dest, uint32_t *p_dictionary_cursor);
void vram_script_clear_pages(uint32_t *p_script, uint32_t *p_dest);
void vram_script_fill_down(uint32_t *p_count, uint32_t *p_script, uint32_t *p_dest);
void vram_script_fill_up(uint32_t *p_count, uint32_t *p_script, uint32_t *p_dest);
void vram_script_fill_down_wide(uint32_t *p_count, uint32_t *p_script, uint32_t *p_dest);
void vram_script_repeat(uint32_t dest, uint32_t *p_count, uint32_t *p_script);
void erase_vram_script(uint32_t script, uint32_t dest);
void erase_vram_script_end(void);
void erase_vram_script_fill(uint16_t cell, uint32_t *p_script, uint32_t *p_dest);
void erase_vram_script_bytes(uint16_t cell, uint32_t *p_script, uint32_t *p_dest);
void erase_vram_script_words(uint16_t cell, uint32_t *p_script, uint32_t *p_dest);
void erase_vram_script_fill_down(uint16_t cell, uint32_t *p_script, uint32_t *p_dest);
void erase_vram_script_fill_up(uint16_t cell, uint32_t *p_script, uint32_t *p_dest);
void erase_vram_script_fill_down_wide(uint16_t cell, uint32_t *p_script, uint32_t *p_dest);
uint32_t erase_vram_script_repeat(uint16_t cell, uint32_t script, uint32_t dest);
uint32_t load_default_palettes(void);
void load_palette_list(uint32_t palette_list, uint32_t *p_source, uint32_t *p_dest);
void copy_palettes_with_pen_0(uint16_t pen_0_color, uint32_t size, uint32_t *p_source, uint32_t *p_dest);
void copy_palette_block(uint32_t size, uint32_t *p_source, uint32_t *p_dest);
void repeat_palette_block(uint32_t size, uint32_t *p_source, uint32_t *p_dest, uint32_t *p_palette_list);
void queue_vblank_copy(uint32_t source, uint32_t dest, uint16_t halfwords, uint32_t *p_copy_offset, uint32_t *p_copy_entry);

#endif
