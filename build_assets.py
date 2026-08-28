"""
Build spritesheets and background image from assets (./sprites)

Read the images, then transcribe them into the .hpp file that stores the binaries.

requires numpy, PIL
"""


import numpy as np
from PIL import Image

SPRITESHEET_PATH = "./sprites/spritesheet.png"   # The sprites
COLORSHEET_PATH = "./sprites/colorsheet.png"     # Region indicators for indexing sprite colors



CODE_TEMPLATE = """\
#include <vector>
#include "PPU466.hpp"

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

PALETTE: dict = {}


def main():

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
        PALETTE[PALETTE_NAMES[ makecolor(color) ]] = [
            c for c in palette_colors
        ]

    # PALETTE =  name ---> [color0, color1, color2, color3]
    # Make the new sheet (2bit colors)
    new_sheet = np.zeros((128, 128), np.uint8)
    for coloridx in color_regions:
        for i, palcolor in enumerate(PALETTE[PALETTE_NAMES[ makecolor(coloridx) ]]):
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
        "\t{{ {} }},".format(",".join([str(v) for v in tilebits])) for tilebits in tiles_0
    )

    codestring_1 = "\n".join(
        "\t{{ {} }},".format(",".join([str(v) for v in tilebits])) for tilebits in tiles_1
    )

    # functional programming!
    # easy to write, hard to read! maybe not stupid code, but I argue yes!!!
    colorstring = "\n".join([   # FOR every palette in ALL palettes
        "\t{{ {} }},".format(
            " ".join([          # FOR every color in THE palette
                "glm::u8vec4( {} ),".format(
                    # RGBA
                    ",".join(str(int(v)) for v in color_ndarray)
                )  for color_ndarray in palette_ndarrays
            ])
        ) for palette_ndarrays in PALETTE.values()
    ])
    print(colorstring)

    
    full_hpp_code = CODE_TEMPLATE.format(codestring_0, codestring_1, colorstring)

    with open("Assets.hpp", 'w') as f:
        f.write(full_hpp_code)





def makecolor(rgba_color) -> tuple:
    return tuple([int(c) for c in rgba_color[:3]])


if __name__ == "__main__":
    main()