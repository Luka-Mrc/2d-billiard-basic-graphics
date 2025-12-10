#ifndef PHYSICS_H
#define PHYSICS_H

#include "Ball.h"
#include "Table.h"
#include <vector>

namespace Physics {
    // Provera i resavanje sudara izmedu dve kugle
    void handleBallCollision(Ball& ball1, Ball& ball2);

    // Provera i resavanje sudara kugle sa zidovima stola
    void handleWallCollision(Ball& ball, const Table& table);

    // Provera da li je kugla usla u dzep
    void handlePocketCollision(Ball& ball, const Table& table);

    // Obraduje sve sudare u sistemu
    void updatePhysics(std::vector<Ball>& balls, const Table& table, float dt);

    // Konstante
    const float FRICTION = 0.98f;           // trenje (0-1, gde je 1 bez trenja)
    const float COLLISION_DAMPING = 0.95f;  // gubitak energije pri sudaru
}

#endif