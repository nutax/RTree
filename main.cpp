#include "rtree.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <array>
/*
g++ -c main.cpp
g++ main.o -o sfml-app -lsfml-graphics -lsfml-window -lsfml-system
./sfml-app

g++ -c main.cpp && g++ main.o -o sfml-app -lsfml-graphics -lsfml-window -lsfml-system && ./sfml-app
*/

#define carro auto

RTree<3, 2, 1000, 70> r;

int width = 900, height = 600;
sf::RenderWindow window(sf::VideoMode(width, height), "SFML works!");
template<typename Box>
void draw_box(const Box& box) {
    auto color = sf::Color(rand() % 255, rand() % 255, rand() % 255);

    auto aX = box.mins[0];
    auto aY = box.mins[1];

    auto bX = box.maxs[0];
    auto bY = box.maxs[1];

    sf::Vertex line1[] = {
        sf::Vertex(sf::Vector2f(aX, aY), color),
        sf::Vertex(sf::Vector2f(aX, bY), color)
    };
    window.draw(line1, 2, sf::Lines);

    sf::Vertex line2[] = {
        sf::Vertex(sf::Vector2f(aX, aY), color),
        sf::Vertex(sf::Vector2f(bX, aY), color)
    };
    window.draw(line2, 2, sf::Lines);

    sf::Vertex line3[] = {
        sf::Vertex(sf::Vector2f(aX, bY), color),
        sf::Vertex(sf::Vector2f(bX, bY), color)
    };
    window.draw(line3, 2, sf::Lines);

    sf::Vertex line4[] = {
        sf::Vertex(sf::Vector2f(bX, aY), color),
        sf::Vertex(sf::Vector2f(bX, bY), color)
    };
    window.draw(line4, 2, sf::Lines);

}

template<typename Polygon>
void draw_poly(const Polygon& polygon) {
    int size = polygon.size;
    const auto& points = polygon.vertex;
    if (size < 3) {
        throw("Ga");
    }
    
    sf::Vertex a;
    sf::Vertex b;
    for (int i = 0; i < size-1; i++) {
        const sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(points[i][0], points[i][1])), 
            sf::Vertex(sf::Vector2f(points[i+1][0], points[i+1][1]))
        };
        int radius = 2;
        sf::CircleShape shape(2);
        shape.setPosition(sf::Vector2f(points[i][0]-radius, points[i][1]-radius));
        window.draw(line, 2, sf::Lines);
        window.draw(shape);
    }
        
    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(points[size-1][0], points[size-1][1])),
        sf::Vertex(sf::Vector2f(points[0][0], points[0][1]))
    };
    
    int radius = 2;
    sf::CircleShape shape(2);
    shape.setPosition(sf::Vector2f(points[size-1][0]-radius, points[size-1][1]-radius));
    window.draw(line, 2, sf::Lines);
    window.draw(shape);
}
    


int main(int argc, char **argv){
    srand(time(NULL));

    // r.insert({1, 3, { {100, 300},   {50, 350},      {500, 550} }});
    // r.insert({1, 3, { {400, 380},   {450, 500},      {600, 400} }});
    r.insert({1, 3, { {500, 5},   {400, 200},      {600, 210} }});
    r.insert({1, 3, { {700, 5},   {650, 200},      {800, 210} }});
    r.insert({1, 4, { {600, 405}, {500, 405},   {600, 500},      {500, 500} }});
    r.insert({1, 11, { {55, 50}, {151, 47},{101, 75},{219, 73}, {150, 110}, {273, 112}, {330, 142}, {383, 176}, {242, 203}, {86, 153}, {100, 100}}});
    // r.insert({2, 6, { {40,0},    {40,20},      {60,40},      {100,20}, {120,40}, {100,0} }});
    // r.insert({3, 5, { {160,60},   {160,80},     {180,500},    {200,80}, {200,60}}});
    // r.insert({4, 4, { {200,40},   {240,60},     {260,40},     {220,20} }});
    // r.insert({5, 3, { {140,20},   {140,40},     {160,40} }});
    // r.insert({6, 3, { {240,120},  {260,140},    {260,100} }});
    // r.insert({7, 3, { {180,20},   {180,40},     {200,20} }});
    // r.print();

    int i = 0;
    auto polys = r.get_polygons();
    while (polys[i].size) {
        const carro&  poly = polys[i];
        draw_poly(poly);
        i++;
    }
    i = 0;
    auto nodes = r.get_nudes();
    while (nodes[i].size) {
        const carro&  node = nodes[i];
        const carro& boxes = node.box;
        for (int i = 0; i < node.size; i++) {
            draw_box(boxes[i]);
        }
        i++;
    }

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }


        window.display();
    }

    return 0;
}