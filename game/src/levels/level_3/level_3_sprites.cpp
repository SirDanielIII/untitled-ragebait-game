// Level 3's sprites: which Game Data Asset ID each thing in Level 3 is drawn with.
//
// To give a thing a sprite, add a row to the Game Data sheet with the Asset ID on its line
// (or change it here), export the CSV and build. "platform-*" means every thing whose name
// starts with "platform-"; a later line wins. The size is the thing's box in world pixels:
// draw the PNG at about half of it. To add a new thing, add it to Level 3's .objects
// (level_3.cpp), then give it a line here.
#include "levels/level_3/level_3.h"

namespace rage {
    std::vector<SpriteAssignment> Level3Sprites() {
        return {
            {"floor", "platform"},       // 1600 x 50
            {"platform-*", "platform"},  // 180-200 x 24
        };
    }
}
