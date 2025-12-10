#include "Physics.h"
#include "Header/Util.h"
#include <cmath>

namespace Physics {

    // Reduced friction (was 0.02f, now 0.005f)

    void handleBallCollision(Ball& ball1, Ball& ball2) {
        if (!ball1.active || !ball2.active) return;

        float dx = ball2.x - ball1.x;
        float dy = ball2.y - ball1.y;
        float dist = length(dx, dy);
        float minDist = ball1.radius + ball2.radius;

        if (dist < minDist && dist > 0.0001f) {
            float nx = dx / dist;
            float ny = dy / dist;

            float overlap = minDist - dist;
            ball1.x -= nx * overlap * 0.5f;
            ball1.y -= ny * overlap * 0.5f;
            ball2.x += nx * overlap * 0.5f;
            ball2.y += ny * overlap * 0.5f;

            float dvx = ball2.vx - ball1.vx;
            float dvy = ball2.vy - ball1.vy;
            float dvn = dot(dvx, dvy, nx, ny);

            if (dvn > 0) return;

            float impulse = dvn * COLLISION_DAMPING;
            ball1.vx += impulse * nx;
            ball1.vy += impulse * ny;
            ball2.vx -= impulse * nx;
            ball2.vy -= impulse * ny;
        }
    }

    void handleWallCollision(Ball& ball, const Table& table) {
        if (!ball.active) return;

        float cushion = table.cushionThickness;

        // Check if ball is near a pocket - if so, skip wall collision
        for (const auto& pocket : table.pockets) {
            float distToPocket = distance(ball.x, ball.y, pocket.x, pocket.y);
            if (distToPocket < pocket.radius + ball.radius * 2) {
                return;
            }
        }

        if (ball.x - ball.radius < table.left + cushion) {
            ball.x = table.left + cushion + ball.radius;
            ball.vx = -ball.vx * COLLISION_DAMPING;
        }
        if (ball.x + ball.radius > table.right - cushion) {
            ball.x = table.right - cushion - ball.radius;
            ball.vx = -ball.vx * COLLISION_DAMPING;
        }
        if (ball.y + ball.radius > table.top - cushion) {
            ball.y = table.top - cushion - ball.radius;
            ball.vy = -ball.vy * COLLISION_DAMPING;
        }
        if (ball.y - ball.radius < table.bottom + cushion) {
            ball.y = table.bottom + cushion + ball.radius;
            ball.vy = -ball.vy * COLLISION_DAMPING;
        }
    }

    void handlePocketCollision(Ball& ball, const Table& table) {
        if (!ball.active) return;

        for (const auto& pocket : table.pockets) {
            float dist = distance(ball.x, ball.y, pocket.x, pocket.y);

            // Ball falls into pocket if center is close enough to pocket center
            if (dist < pocket.radius * 0.7f) {
                ball.active = false;
                ball.stop();
                ball.x = -10.0f;
                ball.y = -10.0f;
                return;
            }

            // Gravity towards pocket - pulls ball when nearby
            if (dist < pocket.radius + ball.radius) {
                float pullStrength = 0.02f;
                float dx = pocket.x - ball.x;
                float dy = pocket.y - ball.y;
                ball.vx += (dx / dist) * pullStrength;
                ball.vy += (dy / dist) * pullStrength;
            }
        }
    }

    void updatePhysics(std::vector<Ball>& balls, const Table& table, float dt) {
        for (auto& ball : balls) {
            ball.update(dt);
            ball.applyFriction(1.0f - FRICTION);
        }

        for (size_t i = 0; i < balls.size(); ++i) {
            for (size_t j = i + 1; j < balls.size(); ++j) {
                handleBallCollision(balls[i], balls[j]);
            }
        }

        for (auto& ball : balls) {
            handlePocketCollision(ball, table);
            handleWallCollision(ball, table);
        }
    }

}