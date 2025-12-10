#ifndef TABLE_H
#define TABLE_H

#include <vector>
#include <GL/glew.h>

struct Pocket {
    float x, y;
    float radius;

    Pocket(float x, float y, float radius) : x(x), y(y), radius(radius) {}
};

class Table {
public:
    float left, right, top, bottom;
    float cushionThickness;
    std::vector<Pocket> pockets;

    Table();
    Table(float left, float right, float top, float bottom);

    void setupPockets();
    void draw(unsigned int shaderProgram, unsigned int tableVAO, unsigned int pocketVAO, int numPocketSegments);
    bool isInPocket(float x, float y, float ballRadius) const;

    static void generateTableVertices(std::vector<float>& vertices, float left, float right, float top, float bottom);
};

#endif