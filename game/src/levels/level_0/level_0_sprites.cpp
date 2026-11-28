// Level 0's sprites: which Game Data Asset ID each thing in Level 0 is drawn with.
//
// To give a thing a sprite, add a row to the Game Data sheet with the Asset ID on its line
// (or change it here), export the CSV and build. "step-*" means every thing whose name
// starts with "step-"; a later line wins. The size is the thing's box in world pixels:
// draw the PNG at about half of it. To add a new thing, add it to Level 0's .objects
// (level_0.cpp), then give it a line here.
#include "levels/level_0/level_0.h"

namespace rage {
    std::vector<SpriteAssignment> Level0Sprites() {
        return {
            {"floor", "platform"},   // 1500 x 50
            {"step-*", "platform"},  // 180 x 24
        };
    }
}
