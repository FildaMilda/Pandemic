#ifndef ACTION_DECODER_H
#define ACTION_DECODER_H

#include "Game.h"

namespace ActionRanges {
    const int DRIVE_START = 0;
    const int DIRECT_FLIGHT_START = 48;
    const int CHARTER_FLIGHT_START = 96;
    const int SHUTTLE_FLIGHT_START = 144;
    const int BUILD_STATION = 192;     
    const int TREAT_DISEASE_START = 240;
    const int SHARE_START = 244;
    const int CURE_DISEASE_START = 436;
    const int RESILIENT_POP_START = 440;
    const int ONE_QUIET_NIGHT_START = 488;
}

class ActionDecoder {
public:
    // Neural Net (Int) -> Game Engine (Action)
    static Action GetActionFromIndex(int index, const GameState& state);

    // Game Engine (Action) -> Neural Net (Int)
    // Returns -1 if the action cannot be mapped (shouldn't happen)
    static int GetIndexFromAction(const Action& action);
};

#endif
