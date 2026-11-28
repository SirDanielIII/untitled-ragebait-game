// Level 2's sprites: which Game Data Asset ID each thing in Level 2 is drawn with.
//
// To give a thing a sprite, add a row to the Game Data sheet with the Asset ID on its line
// (or change it here, for example {"lamppost-*", "lamppost"}), export the CSV and build.
// "awning-*" means every thing whose name starts with "awning-"; a later line wins. The
// size is the thing's box in world pixels: draw the PNG at about half of it. To add a new
// thing, add it to Level 2's .objects (level_2.cpp), then give it a line here.
// The birds are drawn by objects/bird.cpp with Asset ID "bird".
#include "levels/level_2/level_2.h"

namespace rage {
    std::vector<SpriteAssignment> Level2Sprites() {
        return {
            {"floor", "platform"},
            {"lamppost-*", "platform"},
            {"awning-*", "platform"},
            {"window-ledge-*", "platform"},
            {"fire-escape-*", "platform"},
            {"ac-unit-*", "platform"},
            {"upper-ledge-*", "platform"},
            {"roof-ledge-*", "platform"},
            {"rooftop", "platform"},
        };
    }
}
