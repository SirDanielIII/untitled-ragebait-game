#include "player/jump_particles.h"

#include <algorithm>
#include <cmath>

namespace rage {
    void JumpParticles::Emit(Vector2 feet) {
        // Mirrored gusts fan out from the feet. Fixed variation keeps the effect
        // consistent without consuming the gameplay random-number sequence.
        for (int i = 0; i < 8; ++i) {
            for (float side : {-1.0f, 1.0f}) {
                particles.push_back({
                    {feet.x + side * (2 + i * 2), feet.y - 2 - (i % 3) * 2},
                    {side * (32 + i * 9), -18.0f - (i % 3) * 16},
                    pixelScale * (1 + i % 3), 0.28f + i * 0.025f
                });
            }
        }
    }

    void JumpParticles::Update(float dt) {
        if (!std::isfinite(dt) || dt <= 0) return;
        for (auto &particle : particles) particle.age += dt;
        std::erase_if(particles, [](const Particle &p) { return p.age >= p.lifetime; });
    }

    void JumpParticles::SubmitSprites(RenderQueue &queue, float pixelsPerUnit) const {
        for (const auto &particle : particles) {
            const float t = particle.age;
            const float progress = t / particle.lifetime;
            const float size = std::max(pixelScale, particle.size - pixelScale * std::floor(progress * 2));
            // Snap positions and shrinking sizes to whole art pixels and the canvas.
            const auto snap = [pixelsPerUnit](float value) {
                const float pixel = std::round(value / pixelScale) * pixelScale;
                return std::round(pixel * pixelsPerUnit) / pixelsPerUnit;
            };
            const Rectangle bounds{snap(particle.origin.x + particle.velocity.x * t - size / 2),
                                   snap(particle.origin.y + particle.velocity.y * t + 30 * t * t - size / 2),
                                   size, size};
            const Color color = Fade(particle.size > pixelScale ? Color{203, 231, 240, 255} : RAYWHITE,
                                     1.0f - std::floor(progress * 3) / 3.0f);
            queue.Submit(Layer::Middle, [bounds, color] { DrawRectangleRec(bounds, color); });
        }
    }
}
