// Level 1's sprites: which Game Data Asset ID each thing in Level 1 is drawn with.
//
// To give a thing a sprite, add a row to the Game Data sheet with the Asset ID on its line
// (or change the Asset ID here to the one you want), export the CSV and build. Until that
// row exists, the thing keeps its original drawing. Nothing else needs to change.
//
// - "car-*" means every thing whose name starts with "car-". A later line wins, so adding
//   {"car-3", "sports_car"} below the car line gives only car-3 different art.
// - The size is the thing's box in world pixels; draw the PNG at about half of it. The sprite
//   is not stretched to the box: its Pivot is placed on the same point of the box
//   (bottom-centre on the box's bottom edge, top-left on its corner, centre in its middle).
// - States: add a row per state, "<Asset ID>_<state>" (traffic_light_red, traffic_light_green).
//   With only one row, it is used for every state.
// - Effects marked "plays once" start their animation from the first frame each time.
// - The birds are drawn by objects/bird.cpp with Asset ID "bird".
// - A thing named here that doesn't exist prints "Sprites: ... has no object named" when the
//   game starts. New things are added to the list in WithArtObjects (level_1.cpp).
#include "levels/level_1/level_1.h"

namespace rage {
    std::vector<SpriteAssignment> Level1Sprites() {
        return {
            // --- Sidewalk and road ---
            {"sidewalk", "sidewalk"},                 // 580 x 125, in front of the cars going under it
            {"road", "road"},                         // 2460 x 120, with its lane markings
            {"underpass-road", "underpass_road"},     // 240 x 132, the ramp under the sidewalk
            {"tunnel-back", "tunnel_back"},           // 70 x 103, the dark tunnel mouth behind the cars
            {"tunnel-front", "tunnel_front"},         // 424 x 135, the wall in front of the cars
            {"trash-can", "trash_can"},               // 46 x 54, a hazard
            {"hydrant", "hydrant"},                   // 38 x 43, a hazard
            {"sign-school", "sign_school"},           // 82 x 138, with its post; "SCHOOL >" stays text
            {"sign-wrong-way", "sign_wrong_way"},     // 100 x 130, with its post; the words stay text
            {"traffic-light-*", "traffic_light"},     // 33 x 190; states: red, green

            // --- Traffic and the crash ---
            {"car-*", "car"},                         // 86-120 x 38-52, drives left; state "crash" while crashing
            {"chicken", "chicken"},                   // 50 x 41, faces left
            {"sign-chicken", "sign_chicken"},         // 146 x 33; "CHICKEN CROSSING" stays text
            {"wreck-car-*", "wreck_car"},             // 132 x 54, the three roofs you climb
            {"scrap-car-*", "scrap_car"},             // 132 x 50, stacked under the roofs
            {"crash-blast", "crash_blast"},           // up to 280 x 280, plays once; "CRASH!" stays text

            // --- Telephone poles and wires ---
            {"utility-pole-*", "utility_pole"},       // box is the 52 x 18 crossarm (the last is 100 wide); the pole hangs below (Pivot top-left)
            {"wire-mast-*", "wire_mast"},             // about 38-60 wide, from the wires down to the ground
            {"cable-*", "wire"},                      // 220-240 x 12 cable; insulators hang below (Pivot top-left); states: idle, warning, live
            {"sign-duck", "sign_duck"},               // 112 x 22; "HOLD S: DUCK" stays text

            // --- Clouds and sky ---
            {"sky-cloud-*", "cloud"},                 // 92-200 x 16-22, the top you stand on (every cloud, including those above the wires)
            {"checkpoint-flag", "checkpoint_flag"},   // 24 x 27; states: idle, reached
            {"pig-*", "flying_pig"},                  // 127 x 51, flies left
            {"gas-*", "gas_puff"},                    // up to 186 x 161, plays once
            {"star-warning-*", "star_warning"},       // 28 wide, the screen's height
            {"falling-star-*", "falling_star"},       // 34 x 82, with its trail

            // --- Rainbow and campus ---
            {"rainbow-slide", "rainbow_slide"},       // 358 x 703
            {"rainbow-gate", "rainbow_gate"},         // 12 x 504
            {"sign-rainbow", "sign_rainbow"},         // 132 x 24; "RAINBOW SLIDE >" stays text
            {"campus-path", "campus_sidewalk"},       // 325 x 120
            {"lecture-hall", "lecture_hall"},         // 250 x 235; "LECTURE HALL" stays text
            {"anvil", "anvil"},                       // 124 x 44
            {"anvil-poof", "anvil_poof"},             // 184 x 84, plays once when the anvil lands
        };
    }
}
