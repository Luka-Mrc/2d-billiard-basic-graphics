#ifndef BALL_H
#define BALL_H

#include <GL/glew.h>
#include <vector>

class Ball {
public:
    float x, y;           // pozicija
    float vx, vy;         // brzina
    float radius;         // radijus
    float r, g, b;        // boja
    bool active;          // da li je kugla jos na stolu
    bool isWhite;         // da li je bela kugla (glavna)

    Ball();
    Ball(float x, float y, float radius, float r, float g, float b, bool isWhite = false);

    void update(float dt);
    void applyFriction(float friction);
    void draw(unsigned int shaderProgram, unsigned int VAO, int numSegments);

    bool isStopped() const;
    void stop();

    // Generiše vertekse za kružnicu
    static void generateCircleVertices(std::vector<float>& vertices, int numSegments);
};

#endif