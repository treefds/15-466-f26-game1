"""
Build spritesheets and background image from assets (./sprites)

Read the images, then transcribe them into the .hpp file that stores the binaries.

requires numpy, PIL
"""


import numpy as np
from PIL import Image

# Levels to build
LEVEL_PATH_DICT = {
    "LV01": "sprites/levels/map_tutorial.png",
    "LV02": "sprites/levels/map_hard.png",
    "LV03": "sprites/levels/map_last.png",
    "LV04": "sprites/levels/map_bonus.png"
}


# ----------------------- BUILD SPRITESHEET ------------------------- #


SPRITESHEET_PATH = "./sprites/spritesheet.png"   # The sprites
COLORSHEET_PATH = "./sprites/colorsheet.png"     # Region indicators for indexing sprite colors
CODE_TEMPLATE_SPRITE = """\
#include <vector>
#include "PPU466.hpp"

namespace Assets {{
    // The spritesheet bitplane 0
    const std::array<std::array< uint8_t, 8 >, 16 * 16> SPRITESHEET_TILES_0 = {{{{
{}
    }}}};

    // The spritesheet bitplane 1
    const std::array<std::array< uint8_t, 8 >, 16 * 16> SPRITESHEET_TILES_1 = {{{{
{}
    }}}};

    // The palette table
    const std::array<PPU466::Palette, 8UL> SPRITESHEET_PALETTE_TABLE = {{{{
{}
    }}}};

{}
}}

"""

# {
# 		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
# 		glm::u8vec4(0xff, 0xff, 0x00, 0xff),
# 		glm::u8vec4(0x00, 0x00, 0xff, 0xff),
# 		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
# };



PALETTE_NAMES = {
    (48, 81, 130): "LAND",
    (227, 81, 0): "WALL",
    (255, 162, 0): "ICETOP",
    (219, 65, 195): "LEMON",
    (56, 109, 0): "FROSTY",
    (227, 178, 255): "SNOWMAN",
    (113, 227, 146): "CARROT",
    (0, 0, 0): "UNUSED",
}

PALETTE_REV = {v:k for k, v in PALETTE_NAMES.items()}

palette: dict = {}


def build_sprite_assets_code() -> str:

    # Read the image files. Image shape = (128, 128, 4)
    with Image.open(SPRITESHEET_PATH) as image:
        spritesheet = np.array(image)
    with Image.open(COLORSHEET_PATH) as image:
        colorsheet = np.array(image)

    color_regions = np.unique(colorsheet.reshape(-1, 4), axis=0)
    # assert np.sum(color_regions[0]) == 0   # first color is NULL
    # color_regions = color_regions[1:]

    # Build the palette tables...
    for color in color_regions:
        mask = np.all(colorsheet == color, axis=-1)
        palette_colors = np.unique(spritesheet[mask].reshape(-1, 4), axis=0)
        assert len(palette_colors) == 4, (PALETTE_NAMES[ makecolor(color) ], palette_colors)

        color: np.ndarray
        palette[PALETTE_NAMES[ makecolor(color) ]] = [
            c for c in palette_colors
        ]

    # PALETTE =  name ---> [color0, color1, color2, color3]
    # Make the new sheet (2bit colors)
    new_sheet = np.zeros((128, 128), np.uint8)
    for coloridx in color_regions:
        for i, palcolor in enumerate(palette[PALETTE_NAMES[ makecolor(coloridx) ]]):
            mask = np.all(colorsheet == coloridx, axis=-1) & np.all(spritesheet == palcolor, axis=-1)
            new_sheet[mask] = i


    # Preview!
    image = Image.fromarray(new_sheet * 60)
    image.save("sprites/processed_sheet.png")

    # make the huge HPP file that has a big constant describing the whole spritesheet
    # in a format that playmode will use


    tiles_0 = []   # 256 x 8 x (1byte)
    tiles_1 = []   # 256 x 8 x (1byte)

    
    # Part 1. 
    for y in range(16):
        for x in range(16):
            blob = new_sheet[y*8:y*8+8, x*8:x*8+8]
            plane0 = blob & 0b01
            plane1 = blob & 0b10
            # bits0 & bits1 is an array like [0bxxxx, 0bxxxx, 0bxxxx, 0bxxxx]
            # it represents this tile's bit0/bit1 plane
            bits0 = [sum([int((1 if v else 0) << i) for i, v in enumerate(row)]) for row in plane0]
            bits1 = [sum([int((1 if v else 0) << i) for i, v in enumerate(row)]) for row in plane1]
            tiles_0.append([row for row in reversed(bits0)])
            tiles_1.append([row for row in reversed(bits1)])


    # Part 2, build strings

    codestring_0 = "\n".join(
        "\t\t{{ {} }},".format(",".join([str(v) for v in tilebits])) for tilebits in tiles_0
    )

    codestring_1 = "\n".join(
        "\t\t{{ {} }},".format(",".join([str(v) for v in tilebits])) for tilebits in tiles_1
    )

    # functional programming!
    # easy to write, hard to read! maybe not stupid code, but I argue yes!!!
    colorstring = "\n".join([   # FOR every palette in ALL palettes
        "\t\t{{ {} }},".format(
            " ".join([          # FOR every color in THE palette
                "glm::u8vec4( {} ),".format(
                    # RGBA
                    ",".join(str(int(v)) for v in color_ndarray)
                )  for color_ndarray in palette_ndarrays
            ])
        ) for palette_ndarrays in palette.values()
    ])

    # Part 3. Color names
    colordefs = '\n'.join([f"\tconst uint32_t BCOL_{palette_name} = {i << 8};" for i, palette_name in enumerate(palette.keys())])
        


    sprite_hpp_code = CODE_TEMPLATE_SPRITE.format(codestring_0, codestring_1, colorstring, colordefs)
    return sprite_hpp_code

# ----------------------- BUILD LEVEL ----------------------- #

# Level tile definition
TILE_DEF = {
    (178, 82, 102): 0,    # VOID
    (39, 137, 205): 1,    # ICE
    (255, 255, 255): 2,   # SNOW
    (5, 4, 3): 3          # WALL
}

# Level entity definition
ENTITY_DEF = {
    # (0, 0, 0): 255,        # VOID
    (115, 239, 232): 0,   # CUBE
    (255, 240, 137): 1,   # LEMON
    (185, 69, 29): 2      # PLAYER
}


CODE_TEMPLATE_MAP = """\
#include <vector>

namespace Levels {{
{}
}}
"""

CODE_TEMPLATE_ONE_MAP_DEF = """\
const std::array<uint8_t, 15 * 16> MAP_LEVEL_{} = {{{{
{}
}}}};
"""

CODE_TEMPLATE_ENTITY_LIST_DEF = """\
const std::vector<std::array< uint8_t, 3>> ENTITY_LIST_LEVEL_{} = {{{{
{}
}}}};
"""


def build_level_code() -> str:

    all_defs = []
    for level_id, level_path in LEVEL_PATH_DICT.items():
        level_image = np.array(Image.open(level_path))
        map_image = level_image[:15, :, :]    # Upper half
        entity_image = level_image[15:, :, :] # Lower half

        map_def = np.zeros((15, 16), dtype=np.uint8)

        # Convert to tile ids
        for tilecolor, tileid in TILE_DEF.items():
            map_def[np.all(map_image == unmakecolor(tilecolor), axis=-1)] = tileid

        entities = []

        # Convert to entity lists
        for y in range(15):
            for x in range(16):
                colortuple = makecolor(entity_image[14 - y, x])
                if colortuple in ENTITY_DEF:
                    entities.append((x, y, ENTITY_DEF[colortuple]))

        # Create the map definition code
        lines = []
        for y in range(15):
            line = ', '.join([str(int(v)) for v in map_def[14 - y]]) + ','
            lines.append(line)
        code_map_def = CODE_TEMPLATE_ONE_MAP_DEF.format(level_id, '\t' + '\n\t'.join(lines))

        # Create the entity definition code
        # The entity definition list is hardcoded to be length 16; but it doesn't have to
        # (and, in fact, cannot) use up all of them. Ones unused shall be left (0, 0, 255)
        
        entity_inj = "\n".join([
            "{{ {} }},".format(", ".join([str(v) for v in entity]))
            for entity in entities
        ])
        code_entity_def = CODE_TEMPLATE_ENTITY_LIST_DEF.format(level_id, entity_inj)

        one_def = code_map_def + '\n\n' + code_entity_def

        all_defs.append(one_def)

    inj = "\n\n".join(all_defs)
    inj = '\n'.join(['\t' + s for s in inj.split("\n")])

    level_hpp_code = CODE_TEMPLATE_MAP.format(inj)
    return level_hpp_code



# -------------------- helpers ---------------------- #

def makecolor(rgba_color) -> tuple:
    return tuple([int(c) for c in rgba_color[:3]])

def unmakecolor(tuple_color) -> np.ndarray:
    return np.array([v for v in tuple_color] + [255 if sum(tuple_color) > 0 else 0], dtype=np.uint8)

def main():

    sprite_hpp_code = build_sprite_assets_code()
    level_hpp_code = build_level_code()

    with open("Assets.hpp", 'w') as f:
        f.write(sprite_hpp_code)

    with open("GameLevels.hpp", 'w') as f:
            f.write(level_hpp_code)

if __name__ == "__main__":
    main()