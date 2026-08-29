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
}


WorldMap::WorldMap() {
	// hardcoded to read LV03
	// std::array<uint8_t, 15 * 16> map = { };

	map = Levels::MAP_LEVEL_LV03;
}




PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	//Also, *don't* use these tiles in your game:

	// XXX: TO REMOVE
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
		map = WorldMap();

		redraw_background();

	}
}

PlayMode::~PlayMode() {
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
	auto is_bottom = [&](uint32_t gx, uint32_t gy) {
		return gy == 0 || map.map[(gy - 1) * GRID_W + gx] != 2;
	};
	auto is_top = [&](uint32_t gx, uint32_t gy) {
		return gy == GRID_H-1 || map.map[(gy + 1) * GRID_W + gx] != 2;
	};
	auto is_left = [&](uint32_t gx, uint32_t gy) {
		return gx == 0 || map.map[gy * GRID_W + gx - 1] != 2;
	};
	auto is_right = [&](uint32_t gx, uint32_t gy) {
		return gx == GRID_W-1 || map.map[gy * GRID_W + gx + 1] != 2;
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
				if (_top && _left) {
					ppu.background[to_ppu_tile(gx, gy, 0)] = 18 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 2)] = 2 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 3)] = 3 | Assets::BCOL_LAND;
				} else if (_top && _right) {
					ppu.background[to_ppu_tile(gx, gy, 1)] = 20 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 2)] = 3 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 3)] = 4 | Assets::BCOL_LAND;
				} else if (_top) {
					ppu.background[to_ppu_tile(gx, gy, 2)] = 3 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 3)] = 3 | Assets::BCOL_LAND;
				} else if (_bottom && _left) {
					ppu.background[to_ppu_tile(gx, gy, 0)] = 34 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 1)] = 35 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 2)] = 18 | Assets::BCOL_LAND;
				} else if (_bottom && _right) {
					ppu.background[to_ppu_tile(gx, gy, 0)] = 35 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 1)] = 36 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 3)] = 20 | Assets::BCOL_LAND;
				} else if (_bottom) {
					ppu.background[to_ppu_tile(gx, gy, 0)] = 35 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 1)] = 35 | Assets::BCOL_LAND;
				} else if (_left) {
					ppu.background[to_ppu_tile(gx, gy, 0)] = 18 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 2)] = 18 | Assets::BCOL_LAND;
				} else if (_right) {
					ppu.background[to_ppu_tile(gx, gy, 1)] = 20 | Assets::BCOL_LAND;
					ppu.background[to_ppu_tile(gx, gy, 3)] = 20 | Assets::BCOL_LAND;
				}
			} else if (map.map[gy * GRID_W + gx] == 3) {
				ppu.background[to_ppu_tile(gx, gy, 0)] = 80 | Assets::BCOL_WALL;
				ppu.background[to_ppu_tile(gx, gy, 1)] = 81 | Assets::BCOL_WALL;
				ppu.background[to_ppu_tile(gx, gy, 2)] = 64 | Assets::BCOL_WALL;
				ppu.background[to_ppu_tile(gx, gy, 3)] = 65 | Assets::BCOL_WALL;
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
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	constexpr float PlayerSpeed = 30.0f;
	if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	if (up.pressed) player_at.y += PlayerSpeed * elapsed;

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	// ppu.background_color = glm::u8vec4(
	// 	std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 0.0f / 3.0f) ) ) ))),
	// 	std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 1.0f / 3.0f) ) ) ))),
	// 	std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 2.0f / 3.0f) ) ) ))),
	// 	0xff
	// );

	//tilemap gets recomputed every frame as some weird plasma thing:
	//NOTE: don't do this in your game! actually make a map or something :-)
	// for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
	// 	for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
	// 		//TODO: make weird plasma thing
	// 		ppu.background[x+PPU466::BackgroundWidth*y] = ((x+y)%16);
	// 	}
	// }

	//background scroll:
	ppu.background_position.x = int32_t(-0.5f * player_at.x);
	ppu.background_position.y = int32_t(-0.5f * player_at.y);

	//player sprite:
	ppu.sprites[0].x = int8_t(player_at.x);
	ppu.sprites[0].y = int8_t(player_at.y);
	ppu.sprites[0].index = 32;
	ppu.sprites[0].attributes = 7;

	//some other misc sprites:
	for (uint32_t i = 1; i < 63; ++i) {
		float amt = (i + 2.0f * background_fade) / 62.0f;
		ppu.sprites[i].x = int8_t(0.5f * float(PPU466::ScreenWidth) + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player_at.x) * 0.4f * float(PPU466::ScreenWidth));
		ppu.sprites[i].y = int8_t(0.5f * float(PPU466::ScreenHeight) + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player_at.y) * 0.4f * float(PPU466::ScreenWidth));
		ppu.sprites[i].index = 32;
		ppu.sprites[i].attributes = 6;
		if (i % 2) ppu.sprites[i].attributes |= 0x80; //'behind' bit
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
