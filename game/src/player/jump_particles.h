#pragma once

#include "engine/assets/render_queue.h"

#include <vector>

namespace rage {
    // Short, pixel-sized air bursts. Presentation only; never changes player physics.
    class JumpParticles {
    public:
        void Emit(Vector2 feet);
        void Update(float dt);
        void SubmitSprites(RenderQueue &queue, float pixelsPerUnit) const;
        void Clear() { particles.clear(); }
        std::size_t Count() const { return particles.size(); }

    private:
        struct Particle {
            Vector2 origin;
            Vector2 velocity;
            float size;
            float lifetime;
            float age = 0;
        };
        std::vector<Particle> particles;
    };
}
