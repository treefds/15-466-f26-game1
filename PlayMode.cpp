#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <algorithm>

// my assets
#include "Assets.hpp"
#include "GameLevels.hpp"


Entity::Entity(int etype, int x, int y) {
	entity_type = etype;
	grid_x = x;
	grid_y = y;
	// set starting display position to just the corresponding world pos
	display_pos = glm::vec2(x * 16.0f, y * 16.0f);

	// Read the animation definitions and load them to sprites.
	if (entity_type == 0) {
		// Imma cube!
		sprites = std::vector<PPU466::Sprite>(Assets::ANIM_DEF_ICE_CUBE.size());
		// read every sprite index given in the anim definition
		for (uint32_t i = 0; i < sprites.size(); ++i) {
			sprites[i].index = Assets::ANIM_DEF_ICE_CUBE[i];
		}
		sprites[0].attributes = Assets::BCOL_ICETOP >> 8;   // >> 8 because they are stored rightshifted
		sprites[1].attributes = Assets::BCOL_ICETOP >> 8;
		for (int i = 2; i < 6; i++) sprites[i].attributes = Assets::BCOL_WALL >> 8;
	} else if (entity_type == 1) {
		// Lemon
		sprites = std::vector<PPU466::Sprite>(Assets::ANIM_DEF_LEMON.size());
		// read every sprite index given in the anim definition
		for (uint32_t i = 0; i < sprites.size(); ++i) {
			sprites[i].index = Assets::ANIM_DEF_LEMON[i];
		}
		for (int i = 0; i < 4; i++) sprites[i].attributes = Assets::BCOL_LEMON >> 8;
	} else if (entity_type == 2) {
		// Player
		sprites = std::vector<PPU466::Sprite>(Assets::ANIM_DEF_PLAYER_DOWN.size());
		// read every sprite index given in the anim definition
		for (uint32_t i = 0; i < sprites.size(); ++i) {
			sprites[i].index = Assets::ANIM_DEF_PLAYER_DOWN[i];
		}
		for (int i = 0; i <= 10; i++) sprites[i].attributes = Assets::BCOL_SNOWMAN >> 8;
		sprites[6].attributes = Assets::BCOL_CARROT >> 8;
	}
}

Entity::~Entity() {

}

// The repose function! Reposes the sprites based on entity's positions, etc.
void Entity::repose(float time, int facing = 0) {
	if (entity_type == 0) {
		// Ice cube
		assert(sprites.size() == 6);
		sprites[0].x = sprites[2].x = sprites[4].x = display_pos.x;
		sprites[1].x = sprites[3].x = sprites[5].x = display_pos.x + 8.0;
		sprites[0].y = sprites[1].y = display_pos.y + 16.0;
		sprites[2].y = sprites[3].y = display_pos.y + 8.0;
		sprites[4].y = sprites[5].y = display_pos.y;
		if (submerged) {
			for (uint32_t i = 0; i < sprites.size(); ++i) {
				sprites[i].index = Assets::ANIM_DEF_ICE_FLOAT[i];
			}
			for (PPU466::Sprite& sprite: sprites) sprite.y -= 6 + static_cast<int>(time + grid_x * 0.38f) % 2;
		}
	} else if (entity_type == 1) {
		// Lemon
		assert(sprites.size() == 4);
		sprites[0].x = sprites[2].x = display_pos.x;
		sprites[1].x = sprites[3].x = display_pos.x + 8.0;
		sprites[0].y = sprites[1].y = display_pos.y + 8.0;
		sprites[2].y = sprites[3].y = display_pos.y + 0.0;

		if (flag == 0) {  // normal
			for (uint32_t i = 0; i < sprites.size(); ++i) {
				sprites[i].index = Assets::ANIM_DEF_LEMON[i];
			}
			for (int i = 0; i < 4; i++) sprites[i].attributes = Assets::BCOL_LEMON >> 8;
		} else {          // frosty
			for (uint32_t i = 0; i < sprites.size(); ++i) {
				sprites[i].index = Assets::ANIM_DEF_LEMON_CRUSH[i];
			}
			for (int i = 0; i < 4; i++) sprites[i].attributes = Assets::BCOL_FROSTY >> 8;
		}
		if (submerged) {
			sprites[2].index = sprites[3].index = 255;
			for (PPU466::Sprite& sprite: sprites) sprite.y -= 1;
		}

	} else if (entity_type == 2) {
		// Player!
		assert(sprites.size() == 11);
		sprites[0].x = sprites[2].x = sprites[4].x = sprites[7].x = sprites[9].x  = display_pos.x;
		sprites[1].x = sprites[3].x = sprites[5].x = sprites[8].x = sprites[10].x = display_pos.x + 8.0;
		sprites[0].y = sprites[1].y = display_pos.y + 16.0;
		sprites[2].y = sprites[3].y = display_pos.y + 8.0;
		sprites[7].y = sprites[8].y = display_pos.y + 6.0;
		sprites[4].y = sprites[5].y = sprites[9].y = sprites[10].y = display_pos.y;
		sprites[6].x = display_pos.x + 4.0, sprites[6].y = display_pos.y + 12.0;

		// give the hands a small animation
		sprites[7].x -= 3;
		sprites[8].x += 3;
		if (static_cast<int>(time * 20) % 30 <= 10) {
			sprites[7].y += 1;
		}
		if (static_cast<int>(time * 20) % 30 >= 2 && static_cast<int>(time * 20) % 30 <= 12) {
			sprites[8].y += 1;
		}

		// FACING.
		// If the player is moving up, make face invisible; else, show according to facing.
		// Key: 1 = +X, 2 = +Y, 3 = -X, 4 = -Y
		if (flag == 1) {
			for (uint32_t i = 0; i < sprites.size(); ++i) {
				sprites[i].index = Assets::ANIM_DEF_PLAYER_RIGHT[i];
			}
		} else if (flag == 2) {
			for (uint32_t i = 0; i < sprites.size(); ++i) {
				sprites[i].index = Assets::ANIM_DEF_PLAYER_UP[i];
			}
		} else if (flag == 3) {
			for (uint32_t i = 0; i < sprites.size(); ++i) {
				sprites[i].index = Assets::ANIM_DEF_PLAYER_LEFT[i];
			}
		} else if (flag == 4) {
			for (uint32_t i = 0; i < sprites.size(); ++i) {
				sprites[i].index = Assets::ANIM_DEF_PLAYER_DOWN[i];
			}
		}
		// if the player is moving, animate the foot
		if (std::abs(grid_x * 16.0f - display_pos.x) > 0.01f) {
			int salt = static_cast<int>(std::abs(grid_x * 16.0f - display_pos.x) / 5.0f) % 3;
			if (salt == 1) {
				sprites[9].y += 1;
			}
			if (salt == 2) {
				sprites[10].y += 1;
			}
		}
		if (std::abs(grid_y * 16.0f - display_pos.y) > 0.01f) {
			int salt = static_cast<int>(std::abs(grid_y * 16.0f - display_pos.y) / 5.0f) % 3;
			if (salt == 1) {
				sprites[9].y += 1;
			}
			if (salt == 2) {
				sprites[10].y += 1;
			}
		}
		

		if (submerged) {
			sprites[4].index = sprites[5].index = sprites[9].index = sprites[10].index = 255;
			for (PPU466::Sprite& sprite: sprites) sprite.y -= 1;
		}
	}
}

void Entity::step_pos(float elapsed) {
	const float speed = 60.0f;
	if (grid_x * 16.0f - display_pos.x > 0.01f) {
		display_pos.x = std::min(grid_x * 16.0f, display_pos.x + speed * elapsed);
	} else if (grid_x * 16.0f - display_pos.x < -0.01f) {
		display_pos.x = std::max(grid_x * 16.0f, display_pos.x - speed * elapsed);
	} else {
		display_pos.x = grid_x * 16.0f;
	}
	if (grid_y * 16.0f - display_pos.y > 0.01f) {
		display_pos.y = std::min(grid_y * 16.0f, display_pos.y + speed * elapsed);
	} else if (grid_y * 16.0f - display_pos.y < -0.01f) {
		display_pos.y = std::max(grid_y * 16.0f, display_pos.y - speed * elapsed);
	} else {
		display_pos.y = grid_y * 16.0f;
	}
}

WorldMap::WorldMap() {

}

WorldMap::WorldMap(int level_index) {
	// hardcoded to read LV02
	// std::array<uint8_t, 15 * 16> map = { };
	if (level_index == 1)
		map = Levels::MAP_LEVEL_LV01;
	else if (level_index == 2)
		map = Levels::MAP_LEVEL_LV02;
	else if (level_index == 3)
		map = Levels::MAP_LEVEL_LV03;
	else if (level_index == 4)
		map = Levels::MAP_LEVEL_LV04;
	else if (level_index == 5)
		map = Levels::MAP_LEVEL_LV05;
	else if (level_index == 6)
		map = Levels::MAP_LEVEL_LV06;
}




PlayMode::PlayMode() {
	{ //use tiles 0-16 as some weird dot pattern thing:
		std::array< uint8_t, 8*8 > distance;
		for (uint32_t y = 0; y < 8; ++y) {
			for (uint32_t x = 0; x < 8; ++x) {
				float d = glm::length(glm::vec2((x + 0.5f) - 4.0f, (y + 0.5f) - 4.0f));
				d /= glm::length(glm::vec2(4.0f, 4.0f));
				distance[x+8*y] = uint8_t(std::max(0,std::min(255,int32_t( 255.0f * d ))));
			}
		}
		for (uint32_t index = 0; index < 16; ++index) {
			PPU466::Tile tile;
			uint8_t t = uint8_t((255 * index) / 16);
			for (uint32_t y = 0; y < 8; ++y) {
				uint8_t bit0 = 0;
				uint8_t bit1 = 0;
				for (uint32_t x = 0; x < 8; ++x) {
					uint8_t d = distance[x+8*y];
					if (d > t) {
						bit0 |= (1 << x);
					} else {
						bit1 |= (1 << x);
					}
				}
				tile.bit0[y] = bit0;
				tile.bit1[y] = bit1;
			}
			ppu.tile_table[index] = tile;
		}
	}

	{ // STEP 1: override the tile table with our tiles from Assets.hpp...
		// for every single tile (0-255)
		for (uint32_t index = 0; index < 16 * 16; ++index) {
			PPU466::Tile tile;
			tile.bit0 = Assets::SPRITESHEET_TILES_0[index];
			tile.bit1 = Assets::SPRITESHEET_TILES_1[index];
			ppu.tile_table[index] = tile;
		}

		// for every color. ONLY 8 COLORS
		// hardcoded magic number here; if it got wrong, we will have fancy buffer overflow glitches
		// just like NES !? 
		for (uint32_t index = 0; index < 8; ++index) {
			ppu.palette_table[index] = Assets::SPRITESHEET_PALETTE_TABLE[index];
		}
	}

	{ // STEP 2: read the levels, make map, draw the background
		// use default constructor
		reset_level(1);
	}
}

PlayMode::~PlayMode() {
}

// Fully reset this level!
// By reloading all entities.
void PlayMode::reset_level(int level_index) {
	winning_timer = 0.0f;
	game_lost = false;
	map = WorldMap(level_index);
	redraw_background();
	entities.clear();

	// My attempt to use alias to make the code compact failed,
	// and this will copy the vectors, so it's not memory efficient.
	const std::vector<std::vector<std::array< uint8_t, 3>>> level_catalog = {
		Levels::ENTITY_LIST_LEVEL_LV01,
		Levels::ENTITY_LIST_LEVEL_LV02,
		Levels::ENTITY_LIST_LEVEL_LV03,
		Levels::ENTITY_LIST_LEVEL_LV04,
		Levels::ENTITY_LIST_LEVEL_LV05,
		Levels::ENTITY_LIST_LEVEL_LV06
	};
	// gracefully fail if the level overflows
	if (level_index > level_catalog.size()) return;
	for (uint32_t idx = 0; idx < level_catalog[level_index - 1].size(); idx++) {
		std::array<uint8_t, 3> entity_def = level_catalog[level_index - 1][idx];
		entities.push_back(Entity(entity_def[2], entity_def[0], entity_def[1]));
		entities[entities.size() - 1].repose(0.0f);
	}

	entity_moving = std::vector<int>(entities.size());
	// Create a table denoting whether some entities are moving
	for (int i = 0; i < entity_moving.size(); i++) {
		entity_moving[i] = 0;
	}
}

void PlayMode::redraw_background() {
	// h&w are divided by 4. bg size is twice the screen, and each tile is 8x8;
	// but the game level is only one screen, and each game tile is 16x16.
	// As a convention, `gy` and `gx` means the grid coords.

	for (uint32_t idx = 0; idx < ppu.background.size(); idx++) {
		ppu.background[idx] = 0;
	}

	// convert gx, gy to the tilemap 8x8 tile coords
	auto to_ppu_tile = [](uint32_t gx, uint32_t gy, uint32_t corner) {
		return (gx * 2 + corner % 2) + PPU466::BackgroundWidth * (gy * 2 + corner / 2);
	};

	// autoconnect helpers
	auto is_bottom = [&](uint32_t gx, uint32_t gy, uint8_t v = 2) {
		if (gy == 0) return false;
		return map.map[(gy - 1) * GRID_W + gx] != v;
	};
	auto is_top = [&](uint32_t gx, uint32_t gy, uint8_t v = 2) {
		if (gy == GRID_H-1) return false;
		return map.map[(gy + 1) * GRID_W + gx] != v;
	};
	auto is_left = [&](uint32_t gx, uint32_t gy, uint8_t v = 2) {
		if (gx == 0) return false;
		return map.map[gy * GRID_W + gx - 1] != v;
	};
	auto is_right = [&](uint32_t gx, uint32_t gy, uint8_t v = 2) {
		if (gx == GRID_W - 1) return false;
		return map.map[gy * GRID_W + gx + 1] != v;
	};

	// We are semi-automating the world drawing here (instead of specifying it in level files)
	// to enable autotiling (autoconnect, random peppering)
	// we can, of course, define const names for every single tile; but I chose not to
	// because it felt redundant
	for (uint32_t gy = 0; gy < GRID_H; ++gy) {
		for (uint32_t gx = 0; gx < GRID_W; ++gx) {
			// bit0-7: tile
			// bit8-10: palette
			int pepper_1 = ((37 * gy + 19 * gx + 11) % 79 + 19) % 4;
			int pepper_2 = ((21 * gy + 37 * gx + 41) % 91 + 17) % 4;
			if (map.map[gy * GRID_W + gx] == 1) {
				// ice surface
				ppu.background[to_ppu_tile(gx, gy, 0)] = 0 | Assets::BCOL_LAND;
				ppu.background[to_ppu_tile(gx, gy, 3)] = 0 | Assets::BCOL_LAND;
				// randomly choose from 0, 1, 16, 17
				ppu.background[to_ppu_tile(gx, gy, 1)] = (16 * (pepper_1 & 0b1) + ((pepper_2 >> 1) & 0b1)) | Assets::BCOL_LAND;
				ppu.background[to_ppu_tile(gx, gy, 2)] = (8 * (pepper_1 & 0b10) + (pepper_2 & 0b1))  | Assets::BCOL_LAND;
			} else if (map.map[gy * GRID_W + gx] == 2) {
				// snowy surface (19, 32, 33)
				ppu.background[to_ppu_tile(gx, gy, 0)] = 19 | Assets::BCOL_LAND;
				ppu.background[to_ppu_tile(gx, gy, 3)] = 19 | Assets::BCOL_LAND;
				// randomly choose from 0, 1, 16, 17
				ppu.background[to_ppu_tile(gx, gy, 1)] = (pepper_2 > 1 ? 33 : 19) | Assets::BCOL_LAND;
				ppu.background[to_ppu_tile(gx, gy, 2)] = (pepper_1 > 1 ? 32 : 19) | Assets::BCOL_LAND;
				// override: autoconnect
				// below are hardcoded corner handling. c.f. spritesheet
				bool _top = is_top(gx, gy), _bottom = is_bottom(gx, gy), _left = is_left(gx, gy), _right = is_right(gx, gy);

				if (_left) {
					ppu.background[to_ppu_tile(gx, gy, 0)] = 18 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 2)] = 18 | Assets::BCOL_LAND;
				}
				if (_right) {
					ppu.background[to_ppu_tile(gx, gy, 1)] = 20 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 3)] = 20 | Assets::BCOL_LAND;
				}
				if (_top) {
					ppu.background[to_ppu_tile(gx, gy, 2)] = 3 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 3)] = 3 | Assets::BCOL_LAND;
				}
				if (_bottom) {
					ppu.background[to_ppu_tile(gx, gy, 0)] = 35 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 1)] = 35 | Assets::BCOL_LAND;
				} 
				if (_top && _left) {
					ppu.background[to_ppu_tile(gx, gy, 2)] = 2 | Assets::BCOL_LAND;
				}
				if (_top && _right) {
					ppu.background[to_ppu_tile(gx, gy, 3)] = 4 | Assets::BCOL_LAND;
				}
				if (_bottom && _left) {
					ppu.background[to_ppu_tile(gx, gy, 0)] = 34 | Assets::BCOL_LAND;
				}
				if (_bottom && _right) {
					ppu.background[to_ppu_tile(gx, gy, 1)] = 36 | Assets::BCOL_LAND;
				} 

			} else if (map.map[gy * GRID_W + gx] == 3) {
				ppu.background[to_ppu_tile(gx, gy, 0)] = 80 | Assets::BCOL_WALL;
				ppu.background[to_ppu_tile(gx, gy, 1)] = 81 | Assets::BCOL_WALL;
				ppu.background[to_ppu_tile(gx, gy, 2)] = 64 | Assets::BCOL_WALL;
				ppu.background[to_ppu_tile(gx, gy, 3)] = 65 | Assets::BCOL_WALL;
			} else {
				ppu.background[to_ppu_tile(gx, gy, 0)] = (pepper_1 > 2 ? 22 : 255) | Assets::BCOL_LAND;
				ppu.background[to_ppu_tile(gx, gy, 1)] = 255;
				ppu.background[to_ppu_tile(gx, gy, 2)] = 255;
				ppu.background[to_ppu_tile(gx, gy, 3)] = (pepper_1 > 2 ? 24 : 255) | Assets::BCOL_LAND;

				// autoconnecting
				bool _top = is_top(gx, gy, 0), _bottom = is_bottom(gx, gy, 0), _left = is_left(gx, gy, 0), _right = is_right(gx, gy, 0);
				if (_top) {
					ppu.background[to_ppu_tile(gx, gy, 2)] = 6 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 3)] = 6 | Assets::BCOL_LAND;
				}
				if (_bottom) {
					ppu.background[to_ppu_tile(gx, gy, 0)] = 38 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 1)] = 38 | Assets::BCOL_LAND;
				}
				if (_left) {
					ppu.background[to_ppu_tile(gx, gy, 0)] = 21 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 2)] = 21 | Assets::BCOL_LAND;
				}
				if (_right) {
					ppu.background[to_ppu_tile(gx, gy, 1)] = 23 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 3)] = 23 | Assets::BCOL_LAND;
				}
				if (_top && _left) {
					ppu.background[to_ppu_tile(gx, gy, 2)] = 5 | Assets::BCOL_LAND;
				}
				if (_top && _right) {
					ppu.background[to_ppu_tile(gx, gy, 3)] = 7 | Assets::BCOL_LAND;
				}
				if (_bottom && _left) {
					ppu.background[to_ppu_tile(gx, gy, 0)] = 37 | Assets::BCOL_LAND;
				}
				if (_bottom && _right) {
					ppu.background[to_ppu_tile(gx, gy, 1)] = 39 | Assets::BCOL_LAND;
				}
			}
		}
	}
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_R) {
			reset.downs += 1;
			reset.pressed = true;
		} else if (evt.key.key == SDLK_P) {
			skip.downs += 1;
			skip.pressed = true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_DOWN) {
			down.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_R) {
			reset.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_P) {
			reset.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	anim_timer += elapsed;
	if (anim_timer > 60.0f) {
		anim_timer -= 60.0f;
	}

	do { // Handle player inputs
		// Only handle input if the game anim interpolation is finished.
		bool game_stable = true;
		for (Entity& entity: entities) {
			if (abs(entity.grid_x * 16.0f - entity.display_pos.x) > 0.01 || 
				abs(entity.grid_y * 16.0f - entity.display_pos.y) > 0.01) {
					// game not ready to accept input.
					game_stable = false;
					break;
			}
		}

		bool input_allowed = true;    // stopping input while game is still sliding		
		uint32_t player_idx = -1;
		for (int i = 0; i < entity_moving.size(); i++) {
			if (entities[i].entity_type == 2) {
				player_idx = i;
			}
			if (entity_moving[i]) input_allowed = false;
		}

		if (player_idx == -1) break;
		// entered an undefined level! Break from doing any updates.
		// make the game enter a softlock.
		// assert(player_idx != -1);

		bool level_ended = false;

		auto get_entity_index_at = [&](int gx, int gy, uint32_t exclude) {
			for (uint32_t idx = 0; idx < entities.size(); ++idx) {
				if (idx == exclude) continue;
				if (entities[idx].grid_x == gx && entities[idx].grid_y == gy) {
					return idx;
				}
			}
			return ~0u;
		};

		if (game_stable) {
			// check if entity is submerged (fallen into water)
			// then, check if a lemon has been crushed
			for (uint32_t cidx = 0; cidx < entities.size(); cidx++) {
				Entity& entity =  entities[cidx];
				if (map.map[entity.grid_y * GRID_W + entity.grid_x] == 0) {
					// check if an existing ice cube exists on this tile
					uint32_t other_idx = get_entity_index_at(entity.grid_x, entity.grid_y, cidx);
					if (other_idx >= entities.size() || entities[other_idx].entity_type != 0) {
						entity.submerged = true;
						if (entity.entity_type != 0) {
							level_ended = true;
							game_lost = true;
							input_allowed = false;
						}
					}
					
				}
				if (entity.entity_type == 1 && entity.flag == 1) {
					// game won!
					level_ended = true;
					input_allowed = false;
					winning_timer += elapsed;
				}
			}



			// comsume input. only take one command
			// Key: 1 = +X, 2 = +Y, 3 = -X, 4 = -Y

			if (input_allowed) {
				if (left.pressed) {
					entity_moving[player_idx] = 3;
					entities[player_idx].flag = 3;
				} else if (right.pressed) {
					entity_moving[player_idx] = 1;
					entities[player_idx].flag = 1;
				} else if (down.pressed) {
					entity_moving[player_idx] = 4;
					entities[player_idx].flag = 4;
				} else if (up.pressed) {
					entity_moving[player_idx] = 2;
					entities[player_idx].flag = 2;
				} else if (reset.pressed) {
					// RESET GAME
					reset_level(current_level);
				} else if (skip.pressed && skip.downs > 0) {
					current_level++;
					current_level = current_level > total_level ? 6 : current_level;
					reset_level(current_level);
					skip.downs = 0;
				}
			}

			if (level_ended) {
				if (reset.pressed) {
					std::cout << "reset game!!" << std::endl;
					// RESET GAME
					reset_level(current_level);
				}
			}

			auto dir_to_delta_x = [](int dir) {
				if (dir == 3) return -1;
				if (dir == 1) return 1;
				return 0;
			};

			auto dir_to_delta_y = [](int dir) {
				if (dir == 4) return -1;
				if (dir == 2) return 1;
				return 0;
			};

			std::vector<bool> processed = std::vector<bool>(entities.size());

			// make a recursive push function
			auto push = [&](auto &&self, uint32_t idx) {
				if (entity_moving[idx] == 0) {
					// stopped.
					return false;
				}
				// if it is an submerged ice cube, it becomes ice surface.
				if (entities[idx].entity_type == 0 && entities[idx].submerged) {
					entity_moving[idx] = 0;
					return true;
				}

				processed[idx] = true;
				assert(entity_moving[idx] <= 4 && entity_moving[idx] >= 1);
				int target_x = entities[idx].grid_x + dir_to_delta_x(entity_moving[idx]);
				int target_y = entities[idx].grid_y + dir_to_delta_y(entity_moving[idx]);

				if (target_x < 0 || target_x >= GRID_W || target_y < 0 || target_y >= GRID_H) {
					entity_moving[idx] = 0;
					return false;
				}

				// look for tile occupation
				uint8_t target_tile = map.map[target_y * GRID_W + target_x];
				if (target_tile == 3) {
					entity_moving[idx] = 0;
					return false;
				}
				// look for other entities; if there is, try push it based on type
				for (uint32_t other_idx = 0; other_idx < entities.size(); other_idx++) {
					if (other_idx == idx) continue;
					if (entities[other_idx].grid_x == target_x && entities[other_idx].grid_y == target_y) {
						// Pushable when:
						// Player   --> Lemon | Ice cube
						// Ice cube --> Lemon
						// Anything --> submerged ice cube
						if (
							entities[idx].entity_type == 2 || 
							(entities[idx].entity_type == 0 && entities[other_idx].entity_type == 1) ||
							(entities[other_idx].entity_type == 0 && entities[other_idx].submerged)
						) {
							// make it moving
							entity_moving[other_idx] = entity_moving[idx];
							// recursively call push
							bool push_succ = self(self, other_idx);

							// if failed, but I am an ice cube and target is a lemon
							if (!push_succ) {
								if (entities[other_idx].entity_type == 1 && entities[idx].entity_type == 0) {
									// icecube + lemon
									entities[other_idx].flag = 1;
								} else {
									entity_moving[idx] = 0;
									return false;
								}
							}
							// else: the target has been pushed away.
						} else {
							// cannot push
							entity_moving[idx] = 0;
							return false;
						}
					}
				}
				// move to new position
				entities[idx].grid_x = target_x;
				entities[idx].grid_y = target_y;

				// check if ground is ice or not
				if (map.map[target_y * GRID_W + target_x] != 1) {
					// check for floating ice cube
					bool has_floating_ice = false;
					for (Entity& entity : entities) {
						if (entity.grid_x == target_x && entity.grid_y == target_y && entity.entity_type == 0 && entity.submerged)
							has_floating_ice = true;
					}
					// Stop moving.
					if (!has_floating_ice)
						entity_moving[idx] = 0;
				}
				// yay, moved!
				return true;
			};

			// while not all movements are consumed, run

			for (uint32_t idx = 0; idx < entities.size(); idx++) {
				if (entity_moving[idx] && !processed[idx]) {
					game_stable = false;
					push(push, idx);
				}
			}
		}

		// Step the entity positions
		for (Entity& entity: entities) {
			entity.step_pos(elapsed);
		}

	} while (false);

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	skip.downs = 0;
	reset.downs = 0;

	// If winning timer > 4.0s, jump to next level
	if (winning_timer > 4.0) {
		current_level += 1;
		current_level = current_level > total_level ? 6 : current_level;
		reset_level(current_level);
		winning_timer = 0.0;
	}

}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	// background color (water)
	ppu.background_color = glm::u8vec4(65, 146, 195, 0);

	{ // Draw the entities as sprites
		// Step 1. cleaning. restore to tile 255
		for (uint32_t i = 0; i < ppu.sprites.size(); ++i) {
			ppu.sprites[i].index = 255;
		}

		// Step 2. Y-sorting
		std::vector<uint32_t> order = std::vector<uint32_t>(entities.size());
		for (uint32_t i = 0; i < order.size(); ++i) {
			order[i] = i;
		}
		// sort `order` based on entities grid y. Results in 
		std::sort(order.begin(), order.end(), [&](const uint32_t& a, const uint32_t& b) {
			// special case: when lemon and cube overlaps, prioritize cube
			return entities[a].grid_y > entities[b].grid_y || 
				(entities[a].submerged && !entities[b].submerged);
		});

		// Step 3. drawing. 
		// Although PPU only has a 64-sprite memory, our own machines have virtually infinite memory
		// to store sprites! So we process them in the dedicated objects, then copy them to PPU.
		int pt = 0;
		for (uint32_t idx: order) {
			entities[idx].repose(anim_timer, entity_moving[idx]);    // 0.0f is a temporary number
			for (PPU466::Sprite spr: entities[idx].sprites) {
				if (pt >= 64) break;      // reached limit, stop drawing
				ppu.sprites[pt] = spr;
				pt++;
			}
			// std::cout << "Drawing IDX " << idx << "; pt is " << pt << ".\n";
			if (pt >= 64) break;
		}
		if (pt >= 64) {
			// print a warning
			std::cout << "More sprites to draw than PPU supports; aborting halfway\n";
		}

		// Step 4a. override some background with prompts
		{
			for (int y = 0; y < 3; y++) {
				for (int x = 0; x < 10; x++) {
					ppu.background[y * PPU466::BackgroundWidth + x] = 16 * (13 - y) + x;
					if (game_lost && static_cast<int>(anim_timer * 2) % 3 > 0) ppu.background[y * PPU466::BackgroundWidth + x] |= Assets::BCOL_FROSTY;
				}
			}
		}

		// Step 4b. overriding winning animation...
		if (winning_timer > 0.0f) {
			int progress = static_cast<int>(winning_timer * 8.0f);
			progress = std::min(16, progress);
			for (int x = 0; x < progress; x++) {
				ppu.background[28 * PPU466::BackgroundWidth + x + 8] = 128 + x;
				ppu.background[27 * PPU466::BackgroundWidth + x + 8] = 144 + x;
				ppu.background[26 * PPU466::BackgroundWidth + x + 8] = 160 + x;
			}
		}

		// Step 4c. thanks for playing screen
		if (current_level == total_level) {
			for (int x = 0; x < 16; x++) {
				ppu.background[28 * PPU466::BackgroundWidth + x + 8] = 128 + x;
				ppu.background[27 * PPU466::BackgroundWidth + x + 8] = 144 + x;
				ppu.background[26 * PPU466::BackgroundWidth + x + 8] = 160 + x;
			}
			for (int x = 1; x < 6; x++) {
				ppu.background[27 * PPU466::BackgroundWidth + x + 8] = 240 + x;
			}
		}
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
