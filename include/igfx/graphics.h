#include "igfx/nums.h"

namespace igfx {
    struct Sprite {
        u32 index = 0;
    };

    struct DrawSpriteOptions {
        vec2 position;
        vec2 scale;
    };

    struct Frame {
        u32 index;
        void DrawSprite(Sprite, DrawSpriteOptions);
    };

    Frame frame();

}
