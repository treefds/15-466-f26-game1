#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

const uint32_t GRID_H = PPU466::BackgroundHeight / 4;
const uint32_t GRID_W = PPU466::BackgroundWidth / 4;

struct Entity {
	// A game entity is either a lemon, or a cube, or the player.
	Entity(int etype, int x, int y);
	~Entity();

	// Reposing the sprites.
	void repose(float time);

	// ----- states ------
	// 0 = cube; 1 = lemon; 2 = player
	int entity_type = 0;
	// shared flag, used by icecubes/lemons to indicate aliveness; player facing.
	int flag = 0;
	// position in grid
	int grid_x;
	int grid_y;
	// display position, interpolated during draw
	glm::vec2 display_pos = glm::vec2(0.0f);

	std::vector<PPU466::Sprite> sprites;
};

struct WorldMap {
	// The world map; tilemap, plus helpers
	WorldMap();
	// ~WorldMap();

	// ------ data ------
	// The entire map.
	// 0 = void (death zone); 1 = ice; 2 = snow; 3 = wall; other unused
	std::array<uint8_t, 15 * 16> map = { };
};


struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	// helpers
	void redraw_background();

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//some weird background animation:
	float background_fade = 0.0f;

	//player position:
	glm::vec2 player_at = glm::vec2(0.0f);

	//entities and maps
	WorldMap map;
	std::vector<Entity> entities = { };

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
