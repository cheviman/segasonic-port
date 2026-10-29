/**
 *    Name:        objects/intro.c
 *    Description: the scenes that start a game: stage 6's opening (the rushing ground, the giant machine head) and stage 7's how to play (the panel, the hands on the button and the trackball, the metal wall that rises and breaks)
 *    Version:     0.1.0
 *    Created:     12-10-2026
 *    Modified:    23-10-2026
 *    Author:      Sebastian Toyos
 *
 *    SegaSonic The Hedgehog (arcade, Sega System 32): the game's V60 program decompiled into C by
 *    tools/decompile.py, with the names of symbols.txt.
 */
#include "game.h"

/* Sets up stage 7's metal wall and its seven pieces (left half, shadow, floor cracks, four dust puffs,
   the last two starting at dust frame 3): kind 5, health 4, a box hitbox, and with flipped set an
   x-flipped sprite with another hitbox; handler metal_wall_wait. */
void metal_wall_init(void)
{
	OBJECT(current_object)->handler = metal_wall_wait_ADDRESS;
	OBJECT(current_object)->metal_wall.rise_limit = 3;
	OBJECT(current_object)->sprite_palette = 0x8310u;
	OBJECT(current_object)->hitbox_size_x = 0x6Cu;
	OBJECT(current_object)->hitbox_size_y = 0x38u;
	OBJECT(current_object)->hitbox_offset_x = -0x10;
	OBJECT(current_object)->hitbox_offset_y = -0x30;
	OBJECT(current_object)->hitbox_height = 0x4Cu;
	OBJECT(current_object)->kind = 5u;
	OBJECT(current_object)->health = 4;
	OBJECT(current_object)->metal_wall.last_health = OBJECT(current_object)->health;
	OBJECT(current_object)->flags |= 2u;
	if (OBJECT(current_object)->metal_wall.flipped != 0u)
	{
		OBJECT(current_object)->sprite_flags |= 0x40u;
		OBJECT(current_object)->hitbox_size_x = 0x35u;
		OBJECT(current_object)->hitbox_size_y = 0x6Fu;
		OBJECT(current_object)->hitbox_offset_x = -0xD;
		OBJECT(current_object)->hitbox_offset_y = -0x60;
		OBJECT(current_object)->hitbox_height = 0x4Cu;
	}
	current_object = LINKED(current_object, OBJECT(current_object)->child);
	OBJECT(current_object)->sprite_palette = 0x8310u;
	OBJECT(current_object)->sprite_flags = OBJECT(object_cursor)->sprite_flags;
	current_object = LINKED(current_object, OBJECT(current_object)->child);
	OBJECT(current_object)->sprite_palette = 0x89B0u;
	OBJECT(current_object)->sprite_flags = OBJECT(object_cursor)->sprite_flags;
	current_object = LINKED(current_object, OBJECT(current_object)->child);
	OBJECT(current_object)->sprite_palette = 0x89C0u;
	OBJECT(current_object)->sprite_flags = OBJECT(object_cursor)->sprite_flags;
	current_object = LINKED(current_object, OBJECT(current_object)->child);
	OBJECT(current_object)->sprite_palette = 0x8260u;
	OBJECT(current_object)->sprite_flags = OBJECT(object_cursor)->sprite_flags;
	current_object = LINKED(current_object, OBJECT(current_object)->child);
	OBJECT(current_object)->sprite_palette = 0x8260u;
	OBJECT(current_object)->sprite_flags = OBJECT(object_cursor)->sprite_flags;
	current_object = LINKED(current_object, OBJECT(current_object)->child);
	OBJECT(current_object)->sprite_palette = 0x8260u;
	OBJECT(current_object)->direction = 3u;
	OBJECT(current_object)->sprite_flags = OBJECT(object_cursor)->sprite_flags;
	current_object = LINKED(current_object, OBJECT(current_object)->child);
	OBJECT(current_object)->sprite_palette = 0x8260u;
	OBJECT(current_object)->direction = 3u;
	OBJECT(current_object)->sprite_flags = OBJECT(object_cursor)->sprite_flags;
}

/* The metal wall waiting in the floor: measures the nearest player from 0x10 right of and below its
   projected position (face_nearest_player); within 0x80 it starts the dust (dust_timer 0x14) and rises
   (metal_wall_rise), else it runs metal_wall_update as it is. */
void metal_wall_wait(void)
{
	uint32_t octant = 0;

	OBJECT(current_object)->projected_x += 0x10u;
	OBJECT(current_object)->projected_y_int += 0x10u;
	octant = face_nearest_player(0u, 0u);
	OBJECT(current_object)->projected_x -= 0x10u;
	OBJECT(current_object)->projected_y_int -= 0x10u;
	if (nearest_player_distance > 0x80)
	{
		metal_wall_update(octant);
		return;
	}
	OBJECT(current_object)->metal_wall.dust_timer = 0x14u;
	OBJECT(current_object)->handler = metal_wall_rise_ADDRESS;
	metal_wall_rise(octant);
}

/* Raises the metal wall out of the floor: frame_index steps through frames 0-4, one every 4 frames, then
   rise_limit = 7 and the handler becomes metal_wall_update. Runs metal_wall_update every frame, handing
   it octant. */
void metal_wall_rise(uint32_t octant)
{
	uint16_t frame_count = 0;

	frame_count = (uint16_t)OBJECT(current_object)->metal_wall.frame_count;
	OBJECT(current_object)->metal_wall.frame_count++;
	if ((int16_t)frame_count > 2)
	{
		OBJECT(current_object)->metal_wall.frame_count = 0;
		OBJECT(current_object)->metal_wall.frame_index++;
		if (OBJECT(current_object)->metal_wall.rise_limit < OBJECT(current_object)->metal_wall.frame_index)
		{
			OBJECT(current_object)->metal_wall.rise_limit = 7;
			OBJECT(current_object)->handler = metal_wall_update_ADDRESS;
			metal_wall_update(octant);
			return;
		}
	}
	metal_wall_update(octant);
}

/* The risen metal wall. When health drops: frame_index from metal_wall_damage_frames, a hop if it lost
   more than 1, (4 - health) * 2 debris chunks (spawn_arg_1 is last_health as a halfword) and a 2-pixel
   screen_x nudge that move_object_projected undoes; at 0 it scatters ring_count rings and becomes
   metal_wall_broken. Moves, lands, touchable while health remains; drawn. */
void metal_wall_update(uint32_t octant)
{
	uint8_t last_health_or_debris = 0;
	uint32_t allocated = 0, deleted = 0, health = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, ignored_4 = 0, ignored_5 = 0, ignored_6 = 0, last_health_or_debris_2 = 0, ring_count = 0, spawn_arg_1 = 0;

	if (OBJECT(current_object)->health <= 0)
	{
		OBJECT(current_object)->health = 0;
		OBJECT(current_object)->handler = metal_wall_broken_ADDRESS;
		if (OBJECT(current_object)->metal_wall.ring_count != 0u)
		{
			ring_count = OBJECT(current_object)->metal_wall.ring_count;
			scatter_rings_random(octant, ring_count, &allocated, &ignored);
		}
	}
	health = (uint8_t)OBJECT(current_object)->health;
	last_health_or_debris = OBJECT(current_object)->metal_wall.last_health;
	if (health != last_health_or_debris)
	{
		OBJECT(current_object)->metal_wall.last_health = OBJECT(current_object)->health;
		OBJECT(current_object)->metal_wall.frame_index = metal_wall_damage_frames[health];
		if ((int8_t)(last_health_or_debris - health) > 1)
			OBJECT(current_object)->z -= 0x16000u;
		OBJECT(current_object)->screen_x += 2u;
		spawn_arg_1 = OBJECT(current_object)->spawn_arg_1;
		last_health_or_debris_2 = (4u - spawn_arg_1) * 2u;
		if ((int32_t)last_health_or_debris_2 > 0)
			spawn_debris(spawn_arg_1, (uint16_t)last_health_or_debris_2, allocated);
	}
	delete_if_off_screen(&deleted, &ignored_2, &ignored_3);
	if ((uint8_t)deleted == 1u)
		return;
	move_object_projected(&ignored_4, &ignored_2, &ignored_3, &ignored_5);
	fall_to_floor(&ignored_4, &ignored_6, &ignored_2);
	if ((uint8_t)OBJECT(current_object)->health == 0u)
	{
		metal_wall_draw();
		return;
	}
	add_to_touchable_list(ignored_4);
	metal_wall_draw();
}

/* The broken metal wall: removed once off screen, else placed on screen
   (update_screen_position_no_offset) and drawn (metal_wall_draw). */
void metal_wall_broken(void)
{
	uint32_t deleted = 0, ignored = 0, ignored_2 = 0;

	delete_if_off_screen(&deleted, &ignored, &ignored_2);
	if ((uint8_t)deleted == 1u)
		return;
	update_screen_position_no_offset();
	metal_wall_draw();
}

/* Draws the metal wall: its sprite by frame_index (metal_wall_sprites) at depth screen_y - 0x14 (bucket
   0xFE once broken); pieces 1-3, the left half, shadow and floor cracks, from metal_wall_part_tables
   (the flipped offsets when flipped); and while dust_timer counts down, every other frame, four dust
   puffs, two at its base and two above them. */
void metal_wall_draw(void)
{
	uint8_t bucket = 0, health = 0;
	uint16_t dust_frame = 0, dust_x = 0, dust_y = 0, part_dy = 0, screen_offset = 0;
	uint32_t depth = 0, direction = 0, dust_sprite_base = 0, dust_y_2 = 0, flip_offset = 0, frame_index = 0, ignored = 0, offset_pair = 0, part_index = 0, parts_left = 0;

	OBJECT(current_object)->sprite_frame = metal_wall_sprites[(uint8_t)OBJECT(current_object)->metal_wall.frame_index];
	if ((uint8_t)OBJECT(current_object)->health == 0u)
	{
		bucket = 0xFEu;
		queue_sprite_in_bucket(bucket);
	}
	else
	{
		depth = (uint16_t)(OBJECT(current_object)->screen_y - 0x14u);
		queue_sprite_at_depth(&ignored, &depth);
	}
	parts_left = 3u;
	flip_offset = (uint32_t)OBJECT(object_cursor)->metal_wall.flipped << 4;
	do
	{
		current_object = LINKED(current_object, OBJECT(current_object)->child);
		part_index = (parts_left & ~0xFFu) | (uint8_t)(parts_left - 1u);
		frame_index = (uint8_t)OBJECT(object_cursor)->metal_wall.frame_index;
		offset_pair = metal_wall_part_tables[(part_index & ~0xFFu) | (uint8_t)(part_index + 3u)] + (uint8_t)(frame_index * 2u) + flip_offset;
		screen_offset = (uint16_t)METAL_WALL_PART_OFFSET(offset_pair)->dx;
		part_dy = (uint16_t)METAL_WALL_PART_OFFSET(offset_pair)->dy;
		OBJECT(current_object)->screen_x = OBJECT(object_cursor)->screen_x;
		OBJECT(current_object)->screen_x += screen_offset;
		OBJECT(current_object)->screen_y = OBJECT(object_cursor)->screen_y;
		OBJECT(current_object)->screen_y += part_dy;
		OBJECT(current_object)->sprite_frame = METAL_WALL_PART_SPRITES(metal_wall_part_tables[part_index])[(uint8_t)OBJECT(object_cursor)->metal_wall.frame_index];
		health = (uint8_t)OBJECT(object_cursor)->health;
		if ((uint8_t)part_index != 2u)
		{
			bucket = 0xFEu;
			if (parts_left != 2u)
				bucket = 0xFFu;
			queue_sprite_in_bucket(bucket);
		}
		else
		{
			bucket = 0xFEu;
			if (health == 0u)
				queue_sprite_in_bucket(bucket);
			else
			{
				depth = (uint16_t)(OBJECT(current_object)->screen_y - 0x20u);
				queue_sprite_at_depth(&ignored, &depth);
			}
		}
		parts_left--;
	} while (parts_left != 0u);
	if (OBJECT(object_cursor)->metal_wall.dust_timer != 0u)
	{
		OBJECT(object_cursor)->metal_wall.dust_timer--;
		if ((OBJECT(object_cursor)->metal_wall.dust_timer & 1u) == 0u)
			return;
		dust_x = 0xFFF8u;
		dust_y = 6u;
		dust_sprite_base = 0u;
		parts_left = 4u;
		do
		{
			current_object = LINKED(current_object, OBJECT(current_object)->child);
			dust_frame = (uint16_t)(OBJECT(current_object)->direction + 1u);
			if ((int16_t)dust_frame > 7)
				dust_frame = 0u;
			if (parts_left == 2u)
			{
				dust_x = 0xFFF8u;
				dust_sprite_base = 0u;
			}
			OBJECT(current_object)->direction = dust_frame;
			direction = OBJECT(current_object)->direction;
			OBJECT(current_object)->sprite_frame = metal_wall_dust_sprites[dust_sprite_base + direction];
			if ((int32_t)parts_left <= 2)
			{
				screen_offset = (uint16_t)(0x20u - (direction << 2));
				dust_y -= screen_offset;
			}
			OBJECT(current_object)->screen_x = OBJECT(object_cursor)->screen_x;
			OBJECT(current_object)->screen_x += dust_x;
			OBJECT(current_object)->screen_y = OBJECT(object_cursor)->screen_y;
			OBJECT(current_object)->screen_y += dust_y;
			dust_y_2 = (uint16_t)OBJECT(current_object)->screen_y;
			if ((int32_t)parts_left <= 2)
				dust_y_2 = (uint16_t)(dust_y_2 + screen_offset);
			dust_y_2 = (uint16_t)(dust_y_2 - 0x14u);
			queue_sprite_at_depth(&ignored, &dust_y_2);
			dust_x = 0x2Cu;
			dust_y = 6u;
			dust_sprite_base = 8u;
			parts_left--;
		} while (parts_left != 0u);
	}
}

/* Sets up stage 7's how-to-play hand over the button: a multi-part sprite (part_list
   how_to_play_button_hand_parts, animation how_to_play_button_hand_animation) at screen 0x42, 0xB5, with
   next_press at the first pair of how_to_play_button_presses; handler how_to_play_button_hand_update. */
void how_to_play_button_hand_start(void)
{
	OBJECT(current_object)->sprite_flags = 0xD000u;
	OBJECT(current_object)->sprite_offset_x = 0;
	OBJECT(current_object)->sprite_offset_y = 0;
	OBJECT(current_object)->flags = 0u;
	OBJECT(current_object)->handler = how_to_play_button_hand_update_ADDRESS;
	OBJECT(current_object)->part_list = how_to_play_button_hand_parts_ADDRESS;
	OBJECT(current_object)->animation = how_to_play_button_hand_animation_ADDRESS;
	OBJECT(current_object)->animation_frame = 0u;
	OBJECT(current_object)->how_to_play_button_hand.next_press = how_to_play_button_presses_ADDRESS;
	OBJECT(current_object)->screen_x = 0x42;
	OBJECT(current_object)->screen_y = 0xB5;
	OBJECT(current_object)->handler = how_to_play_button_hand_update_ADDRESS;
}

/* Stage 7's hand on the button, removed once stage is not 7. When game_mode_timer reaches the time of
   next_press's (time, frame) pair it sets that animation_frame and steps it (animate_part_list_step, so
   the part list after it shows), then moves to the next pair; at 0xC8 it opens message box kind 0xC.
   Queued at depth 0xEE. */
void CONSOLE_ORIGINAL(how_to_play_button_hand_update)(void)
{
	uint16_t frames = 0, kind = 0, screen_x = 0, screen_y = 0;
	uint32_t depth = 0, ignored = 0;

	if (stage != 7u)
	{
		delete_current_object();
		return;
	}
	if ((uint16_t)game_mode_timer == 0xC8u)
	{
		screen_x = 0xA0u;
		screen_y = 0x7Cu;
		kind = 0xCu;
		frames = 0x12Cu;
		spawn_message_box(screen_x, screen_y, kind, frames, 0u);
	}
	if ((uint16_t)game_mode_timer == HOW_TO_PLAY_BUTTON_PRESS(OBJECT(current_object)->how_to_play_button_hand.next_press)->press_time)
	{
		OBJECT(current_object)->animation_frame = (uint8_t)HOW_TO_PLAY_BUTTON_PRESS(OBJECT(current_object)->how_to_play_button_hand.next_press)->frame;
		OBJECT(current_object)->how_to_play_button_hand.next_press += 4u;
		animate_part_list_step();
	}
	depth = 0xEEu;
	queue_sprite_at_depth(&ignored, &depth);
}

/* Sets up stage 7's hand on the trackball: sprite 0x71C8 at screen 0xFB, 0xB3 (projected_x keeps that x,
   the hand's rest point), and as its child a second-pool block for the ball, sprite 0x71C0 0x38 left of
   and 8 above it, which how_to_play_trackball_animation turns every 4 frames. */
void how_to_play_trackball_hand_start(void)
{
	uint32_t free_count = 0, ignored = 0, object = 0;

	OBJECT(current_object)->sprite_flags = 0x1000u;
	OBJECT(current_object)->sprite_frame = 0x71C8u;
	OBJECT(current_object)->sprite_palette = 0x8530u;
	OBJECT(current_object)->sprite_offset_x = 0;
	OBJECT(current_object)->sprite_offset_y = 0;
	OBJECT(current_object)->flags = 0u;
	OBJECT(current_object)->handler = how_to_play_trackball_hand_update_ADDRESS;
	OBJECT(current_object)->how_to_play_trackball_hand.step = 0u;
	OBJECT(current_object)->screen_x = 0xFB;
	OBJECT(current_object)->screen_y = 0xB3;
	OBJECT(current_object)->projected_x = OBJECT(current_object)->screen_x;
	OBJECT(current_object)->handler = how_to_play_trackball_hand_update_ADDRESS;
	OBJECT(current_object)->animation = how_to_play_trackball_animation_ADDRESS;
	OBJECT(current_object)->animation_frame = 0u;
	OBJECT(current_object)->animation_speed = 4u;
	OBJECT(current_object)->animation_timer = 0;
	OBJECT(current_object)->child = 0u;
	allocate_object_2(&free_count, &ignored, &object);
	if (free_count != 0u)
	{
		OBJECT(object)->sprite_flags = 0x1000u;
		OBJECT(object)->sprite_frame = 0x71C0u;
		OBJECT(object)->sprite_palette = 0x8550u;
		OBJECT(object)->screen_x = OBJECT(current_object)->screen_x;
		OBJECT(object)->screen_x -= 0x38u;
		OBJECT(object)->screen_y = OBJECT(current_object)->screen_y;
		OBJECT(object)->screen_y -= 8u;
		OBJECT(current_object)->child = object;
	}
}

/* Stage 7's hand on the trackball, removed with the ball once stage is not 7. From game_mode_timer 0x32
   it rubs the ball (step: down-left to 0x20 left of projected_x, 8 frames' rest, back) while the ball
   turns. At timer 0xA it lights up until 0xBE and opens message boxes kinds 0xD and 0xB; from 0x20 the
   scroll runs at speed 0x10. */
void CONSOLE_ORIGINAL(how_to_play_trackball_hand_update)(void)
{
	uint8_t last_step = 0;
	uint16_t child = 0, child_palette = 0, frames = 0, kind = 0, screen_x = 0, screen_y = 0;
	uint32_t box = 0, depth = 0, ignored_2 = 0, next_step = 0, step_list = 0;
	uint8_t pause_timer = 0, animation_timer = 0;

	if (stage != 7u)
	{
		if (OBJECT(current_object)->child != 0u)
		{
			child = OBJECT(current_object)->child;
			free_object_2(child);
		}
		delete_current_object();
		return;
	}
	if (game_mode_timer >= 0x32)
	{
		switch (OBJECT(current_object)->how_to_play_trackball_hand.step)
		{
		case 0:
		case 3:
			OBJECT(current_object)->screen_x -= 0xAu;
			OBJECT(current_object)->screen_y += 0xAu;
			if ((int16_t)(OBJECT(current_object)->screen_x - OBJECT(current_object)->projected_x) <= -0x20)
			{
				OBJECT(current_object)->how_to_play_trackball_hand.step++;
				OBJECT(current_object)->how_to_play_trackball_hand.pause_timer = 8;
			}
			break;
		case 1:
			pause_timer = OBJECT(current_object)->how_to_play_trackball_hand.pause_timer;
			OBJECT(current_object)->how_to_play_trackball_hand.pause_timer = pause_timer - 1u;
			if ((int8_t)pause_timer <= 1)
				OBJECT(current_object)->how_to_play_trackball_hand.step++;
			break;
		case 2:
			OBJECT(current_object)->screen_x += 4u;
			OBJECT(current_object)->screen_y -= 4u;
			if (OBJECT(current_object)->projected_x <= OBJECT(current_object)->screen_x)
				OBJECT(current_object)->how_to_play_trackball_hand.step = 0u;
			break;
		default:
			invalid_jump(0x83E6Du + OBJECT(current_object)->how_to_play_trackball_hand.step * 2u);
		}
	}
	if ((uint16_t)game_mode_timer == 0xAu)
		OBJECT(current_object)->sprite_palette = 0x8760u;
	else if ((uint16_t)game_mode_timer == 0xBEu)
		OBJECT(current_object)->sprite_palette = 0x8530u;
	if ((uint16_t)game_mode_timer == 0xAu)
	{
		screen_x = 0x40u;
		screen_y = 0x30u;
		kind = 0xDu;
		frames = 0x1E0u;
		box = spawn_message_box(screen_x, screen_y, kind, frames, 0u);
		screen_x = 0xA0u;
		screen_y = 0x7Cu;
		kind = 0xBu;
		frames = 0xB4u;
		spawn_message_box(screen_x, screen_y, kind, frames, box);
	}
	if (game_mode_timer >= 0x20)
	{
		scroll_mode = 0u;
		scroll_speed_low = 0x10;
	}
	depth = 0xEEu;
	queue_sprite_at_depth(&ignored_2, &depth);
	current_object = LINKED(current_object, OBJECT(current_object)->child);
	if ((uint16_t)current_object == 0u)
		return;
	if (game_mode_timer >= 0x32)
	{
		animation_timer = OBJECT(object_cursor)->animation_timer;
		OBJECT(object_cursor)->animation_timer = animation_timer - 1u;
		if ((int8_t)animation_timer > 1)
		{
			child_palette = 0x8750u;
			if (OBJECT(object_cursor)->sprite_palette != 0x8760u)
				child_palette = 0x8550u;
			OBJECT(current_object)->sprite_palette = child_palette;
			depth = 0xEDu;
			queue_sprite_at_depth(&ignored_2, &depth);
			return;
		}
		OBJECT(object_cursor)->animation_timer = OBJECT(object_cursor)->animation_speed;
		step_list = OBJECT(object_cursor)->animation;
		last_step = (uint8_t)FRAME_ANIMATION(step_list)->last_frame;
		next_step = OBJECT(object_cursor)->animation_frame;
		next_step = (uint8_t)(next_step + 1u);
		step_list += 2u;
		if ((int8_t)last_step < (int8_t)next_step)
			next_step = 0u;
		OBJECT(object_cursor)->animation_frame = next_step;
		OBJECT(current_object)->sprite_frame = SPRITE_FRAME_NUMBERS(step_list)[next_step];
	}
	child_palette = 0x8750u;
	if (OBJECT(object_cursor)->sprite_palette != 0x8760u)
		child_palette = 0x8550u;
	OBJECT(current_object)->sprite_palette = child_palette;
	depth = 0xEDu;
	queue_sprite_at_depth(&ignored_2, &depth);
}

/* Sets up stage 7's control panel strip (sprite 0x71F0, palette 0x8780) at screen 0, 0x88 and shows it:
   handler how_to_play_panel_show. */
void how_to_play_panel_start(void)
{
	OBJECT(current_object)->sprite_flags = 0x100Au;
	OBJECT(current_object)->sprite_frame = 0x71F0u;
	OBJECT(current_object)->sprite_palette = 0x8780u;
	OBJECT(current_object)->screen_x = 0;
	OBJECT(current_object)->screen_y = 0x88;
	OBJECT(current_object)->handler = how_to_play_panel_show_ADDRESS;
	how_to_play_panel_show();
}

/* Stage 7's control panel strip: queued at depth 0xEC while stage is 7, removed otherwise. */
void how_to_play_panel_show(void)
{
	uint32_t depth = 0, ignored = 0;

	if (stage == 7u)
	{
		depth = 0xECu;
		queue_sprite_at_depth(&ignored, &depth);
		return;
	}
	delete_current_object();
}

/* Sets up stage 6's opening ground rushing toward the viewer: opening_ground_animation, a step every
   frame, palette 0x9120, at x 0xA0, y 0xB8 (screen coordinates); handler opening_ground_run. */
void opening_ground_start(void)
{
	OBJECT(current_object)->sprite_flags = 0x1000u;
	OBJECT(current_object)->sprite_palette = 0x9120u;
	OBJECT(current_object)->flags = 0u;
	OBJECT(current_object)->speed_y = 0;
	OBJECT(current_object)->handler = opening_ground_run_ADDRESS;
	OBJECT(current_object)->x = 0xA00000;
	OBJECT(current_object)->y = 0xB80000;
	OBJECT(current_object)->animation_frame = 0u;
	OBJECT(current_object)->animation = opening_ground_animation_ADDRESS;
	OBJECT(current_object)->animation_timer = 0;
	OBJECT(current_object)->animation_speed = 1u;
}

/* Stage 6's rushing ground: removed once scene_step reaches 2; moves by speed_y, shows at its x/y shaken
   up or down by stage_06_shake_pixels (stage_06_shake_sign picks which), animates and is queued at depth
   0x20. */
void opening_ground_run(void)
{
	uint16_t shake_dy = 0;
	uint32_t depth = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, ignored_4 = 0;

	if ((int8_t)scene_step >= 2)
	{
		delete_current_object();
		return;
	}
	OBJECT(current_object)->y += OBJECT(current_object)->speed_y;
	OBJECT(current_object)->screen_x = OBJECT(current_object)->x_int;
	OBJECT(current_object)->screen_y = OBJECT(current_object)->y_int;
	shake_dy = stage_06_shake_pixels;
	if (stage_06_shake_sign < 0)
		shake_dy = (uint16_t)(0u - shake_dy);
	OBJECT(current_object)->screen_y += shake_dy;
	animate(&ignored, &ignored_2, &ignored_3);
	depth = 0x20u;
	queue_sprite_at_depth(&ignored_4, &depth);
}

/* Sets up stage 6's opening machine: scene_step_seen = 0xFF, so that its handler, opening_machine_update
   (run at once), sets it up for the current scene_step. */
void opening_machine_start(void)
{
	OBJECT(current_object)->opening_machine.scene_step_seen = 0xFFu;
	OBJECT(current_object)->handler = opening_machine_update_ADDRESS;
	opening_machine_update();
}

/* Stage 6's opening machine, set up again on entering scene_step 0, 1, 2 or 4. In steps 0, 1 and 4 the
   giant head (a multi-part sprite, opening_machine_object) runs its phase (leap in, approach, lunge,
   mouth, engulf, hold), shaken with the ground; in steps 2 and 3 it flies off into the distance,
   sprite_scale 0x4000 / (zoom + 0x10), with its shadow. */
void opening_machine_update(void)
{
	uint16_t shake_dy = 0;
	uint32_t depth = 0, free_count = 0, ignored = 0, ignored_2 = 0, object = 0, scale = 0;

	switch (scene_step)
	{
	case 0:
	case 1:
	case 4:
		if (OBJECT(current_object)->opening_machine.scene_step_seen != scene_step)
		{
			OBJECT(current_object)->opening_machine.scene_step_seen = scene_step;
			OBJECT(current_object)->sprite_flags = 0xC000u;
			OBJECT(current_object)->opening_machine.zoom = 0xC00000;
			OBJECT(current_object)->sprite_scale = OBJECT(current_object)->opening_machine.zoom_int;
			OBJECT(current_object)->opening_machine.zoom_speed = 0x8000;
			OBJECT(current_object)->flags = 0u;
			OBJECT(current_object)->opening_machine.phase = 0;
			OBJECT(current_object)->speed_y = -0xC0000;
			OBJECT(current_object)->x = 0xA00000;
			OBJECT(current_object)->y = 0xF80000;
			OBJECT(current_object)->animation_frame = 0u;
			OBJECT(current_object)->animation = opening_machine_leap_animation_ADDRESS;
			OBJECT(current_object)->animation_timer = 0;
			OBJECT(current_object)->animation_speed = 2u;
			opening_machine_object = current_object;
		}
		OBJECT(current_object)->opening_machine.draw_depth = OBJECT(current_object)->screen_y;
		switch ((uint8_t)OBJECT(current_object)->opening_machine.phase)
		{
		case 0:
			opening_machine_leap_in();
			break;
		case 1:
			opening_machine_approach();
			break;
		case 2:
			opening_machine_lunge();
			break;
		case 3:
			opening_machine_show_mouth();
			break;
		case 4:
			opening_machine_engulf();
			break;
		case 5:
			opening_machine_hold();
			break;
		case 6:
			opening_machine_leap_in();
			break;
		default:
			invalid_jump(0x84157u + (uint8_t)OBJECT(current_object)->opening_machine.phase * 3u);
		}
		OBJECT(current_object)->screen_x = OBJECT(current_object)->x_int;
		OBJECT(current_object)->screen_y = OBJECT(current_object)->y_int;
		shake_dy = stage_06_shake_pixels;
		if (stage_06_shake_sign < 0)
			shake_dy = (uint16_t)(0u - shake_dy);
		OBJECT(current_object)->screen_y += shake_dy;
		if (OBJECT(current_object)->opening_machine.phase < 3)
			animate_part_list();
		depth = OBJECT(current_object)->opening_machine.draw_depth;
		queue_sprite_at_depth(&ignored_2, &depth);
		return;
	case 2:
	case 3:
		if (OBJECT(current_object)->opening_machine.scene_step_seen != scene_step)
		{
			OBJECT(current_object)->opening_machine.scene_step_seen = scene_step;
			if (OBJECT(current_object)->opening_machine.scene_step_seen == 2u)
			{
				OBJECT(current_object)->sprite_flags = 0xC000u;
				OBJECT(current_object)->opening_machine.zoom = 0;
				OBJECT(current_object)->flags = 0u;
				OBJECT(current_object)->y = -0x600000;
				OBJECT(current_object)->animation_frame = 0u;
				OBJECT(current_object)->animation = opening_machine_fly_away_animation_ADDRESS;
				OBJECT(current_object)->animation_timer = 0;
				OBJECT(current_object)->animation_speed = 2u;
				OBJECT(current_object)->child = 0xFFFFu;
				allocate_object_2(&free_count, &ignored, &object);
				if (free_count != 0u)
				{
					OBJECT(object)->sprite_flags = 0u;
					OBJECT(object)->sprite_frame = 0x7078u;
					OBJECT(object)->sprite_palette = 0x87A0u;
					OBJECT(object)->sprite_width = 0;
					OBJECT(object)->sprite_height = 0x18;
					OBJECT(current_object)->child = object;
				}
			}
		}
		OBJECT(current_object)->opening_machine.zoom += 2u;
		OBJECT(current_object)->sprite_scale = divide_s32(0x4000u, OBJECT(current_object)->opening_machine.zoom + 0x10u);
		OBJECT(current_object)->y += 0x10000u;
		scale = OBJECT(current_object)->sprite_scale;
		OBJECT(current_object)->screen_y = ((int32_t)(OBJECT(current_object)->y_int * scale) >> 10) + 0xB8u;
		animate_part_list();
		queue_sprite(scale);
		draw_opening_machine_shadow();
		return;
	default:
		invalid_jump(0x84089u + scene_step * 3u);
	}
}

/* The opening machine leaps in from below: from scene_timer 0x3C it moves by speed_y under gravity
   0x6000 (at depth 0x10, behind, while going up); landing at y 0x63 it starts the shake (stage_06_shake
   0x800), the landed animation and sound 0xA, and moves to the next phase. */
void opening_machine_leap_in(void)
{
	uint8_t sound_id = 0;
	uint32_t ignored = 0, ignored_2 = 0;

	if (OBJECT(current_object)->speed_y_int < 0)
		OBJECT(current_object)->opening_machine.draw_depth = 0x10u;
	if (scene_timer >= 0x3C)
	{
		OBJECT(current_object)->speed_y += 0x6000u;
		OBJECT(current_object)->y += OBJECT(current_object)->speed_y;
		if (OBJECT(current_object)->speed_y_int >= 0 && OBJECT(current_object)->y_int >= 0x63)
		{
			OBJECT(current_object)->y = 0x630000;
			stage_06_shake = 0x800u;
			stage_06_shake_sign = 0;
			OBJECT(current_object)->animation_frame = 0u;
			OBJECT(current_object)->animation = opening_machine_landed_animation_ADDRESS;
			OBJECT(current_object)->animation_timer = 0;
			OBJECT(current_object)->animation_speed = 2u;
			sound_id = 0xAu;
			play_sound(sound_id, &ignored, &ignored_2);
			OBJECT(current_object)->opening_machine.phase++;
		}
	}
}

/* The landed opening machine comes closer: it grows by zoom_speed a frame (sprite_scale = zoom) while
   the landing shake decays (stage_06_shake - 0x80 a frame down to 0, stage_06_shake_sign flipping every
   frame); the next phase once scene_timer passes 0xC0. */
void opening_machine_approach(void)
{
	uint16_t shake_before = 0;

	if (stage_06_shake == 0u)
	{
		OBJECT(current_object)->opening_machine.zoom += OBJECT(current_object)->opening_machine.zoom_speed;
		OBJECT(current_object)->sprite_scale = OBJECT(current_object)->opening_machine.zoom_int;
		if (scene_timer > 0xC0)
			OBJECT(current_object)->opening_machine.phase++;
		return;
	}
	shake_before = stage_06_shake;
	stage_06_shake = shake_before - 0x80u;
	if ((int16_t)shake_before < 0x80)
		stage_06_shake = 0u;
	stage_06_shake_sign = ~stage_06_shake_sign;
	OBJECT(current_object)->opening_machine.zoom += OBJECT(current_object)->opening_machine.zoom_speed;
	OBJECT(current_object)->sprite_scale = OBJECT(current_object)->opening_machine.zoom_int;
	if (scene_timer > 0xC0)
		OBJECT(current_object)->opening_machine.phase++;
}

/* The opening machine lunges at the viewer: it grows faster every frame (zoom_speed + 0x1000), moves up
   once sprite_scale reaches 0x1B3, and goes to the next phase at zoom 0x32B. */
void opening_machine_lunge(void)
{
	OBJECT(current_object)->opening_machine.zoom_speed += 0x1000u;
	OBJECT(current_object)->opening_machine.zoom += OBJECT(current_object)->opening_machine.zoom_speed;
	OBJECT(current_object)->sprite_scale = OBJECT(current_object)->opening_machine.zoom_int;
	if ((int16_t)OBJECT(current_object)->sprite_scale >= 0x1B3)
		OBJECT(current_object)->y -= 0xE000u;
	if (OBJECT(current_object)->opening_machine.zoom_int >= 0x32B)
		OBJECT(current_object)->opening_machine.phase++;
}

/* Turns the machine's multi-part sprite into the single sprite of its open jaws (0x7198, palette
   0x8510), moved down by 0x1E pixels times the zoom, and goes straight on to the next phase,
   opening_machine_engulf. */
void opening_machine_show_mouth(void)
{
	OBJECT(current_object)->opening_machine.phase++;
	OBJECT(current_object)->sprite_flags = 0u;
	OBJECT(current_object)->sprite_frame = 0x7198u;
	OBJECT(current_object)->sprite_palette = 0x8510u;
	OBJECT(current_object)->y += (OBJECT(current_object)->sprite_scale * 0x1Eu) << 8;
	opening_machine_engulf();
}

/* The machine's open jaws swallow the screen: zoom grows by zoom_speed, itself + 1 a frame (sprite_scale
   = zoom); at 0x5B0 the next phase (opening_machine_hold) and palette 0x87A0, black. */
void opening_machine_engulf(void)
{
	OBJECT(current_object)->opening_machine.zoom_speed += 0x10000u;
	OBJECT(current_object)->opening_machine.zoom += OBJECT(current_object)->opening_machine.zoom_speed;
	OBJECT(current_object)->sprite_scale = OBJECT(current_object)->opening_machine.zoom_int;
	if (OBJECT(current_object)->opening_machine.zoom_int < 0x5B0)
	{
		opening_machine_hold();
		return;
	}
	OBJECT(current_object)->opening_machine.phase++;
	OBJECT(current_object)->sprite_palette = 0x87A0u;
	opening_machine_hold();
}

/* The opening machine's last phase: an empty function; the machine stays as opening_machine_engulf left
   it, still drawn by opening_machine_update. */
void opening_machine_hold(void)
{
}

/* Places the opening machine's shadow, its child, as the machine flies away: the shadow's sprite_width
   and sprite_height hold its x and y offsets, scaled by the machine's sprite_scale and added to the
   machine's screen position; it takes the same sprite_scale and is queued at the machine's screen_y -
   0x10. */
void draw_opening_machine_shadow(void)
{
	uint16_t scaled_dy = 0;
	uint32_t depth = 0, ignored = 0, sprite_scale = 0;

	current_object = LINKED(current_object, OBJECT(current_object)->child);
	sprite_scale = OBJECT(object_cursor)->sprite_scale;
	OBJECT(current_object)->screen_x = ((int32_t)(OBJECT(current_object)->sprite_width * sprite_scale) >> 8) + OBJECT(object_cursor)->screen_x;
	scaled_dy = (uint16_t)((int32_t)(OBJECT(current_object)->sprite_height * sprite_scale) >> 8);
	OBJECT(current_object)->screen_y = OBJECT(object_cursor)->screen_y;
	OBJECT(current_object)->screen_y += scaled_dy;
	OBJECT(current_object)->sprite_scale = OBJECT(object_cursor)->sprite_scale;
	depth = (uint16_t)(OBJECT(object_cursor)->screen_y - 0x10u);
	queue_sprite_at_depth(&ignored, &depth);
}

/* Unused: an empty function, a lone return the original leaves after draw_opening_machine_shadow. */
void draw_opening_machine_shadow_spare_return(void)
{
}
