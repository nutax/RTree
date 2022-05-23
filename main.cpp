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

RTree<3, 2, 1000, 7> r;

int width = 900, height = 600;
sf::RenderWindow window(sf::VideoMode(width, height), "SFML works!");

void drawPoly(std::array<int, 2> points[], int size) {
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
        window.draw(line, radius, sf::Lines);
        window.draw(shape);
    }
        
    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(points[size-1][0], points[size-1][1])),
        sf::Vertex(sf::Vector2f(points[0][0], points[0][1]))
    };
    
    int radius = 2;
    sf::CircleShape shape(2);
    shape.setPosition(sf::Vector2f(points[size-1][0]-radius, points[size-1][1]-radius));
    window.draw(line, radius, sf::Lines);
    window.draw(shape);     
}
    

int main(int argc, char **argv){
    r.insert({1, 3, { {40,120},   {40,80},      {80,90} }});
    r.insert({2, 6, { {40,0},    {40,20},      {60,40},      {100,20}, {120,40}, {100,0} }});
    r.insert({3, 5, { {160,60},   {160,80},     {180,100},    {200,80}, {200,60}}});
    r.insert({4, 4, { {200,40},   {240,60},     {260,40},     {220,20} }});
    r.insert({5, 3, { {140,20},   {140,40},     {160,40} }});
    r.insert({6, 3, { {240,120},  {260,140},    {260,100} }});
    r.insert({7, 3, { {180,20},   {180,40},     {200,20} }});
    r.print();

    std::array<int, 2> vector1[] = { {40,120},   {40,80},      {80,90} };
    std::array<int, 2> vector2[] = { {40,0},    {40,20},      {60,40},      {100,20}, {120,40}, {100,0} };
    std::array<int, 2> vector3[] = { {160,60},   {160,80},     {180,100},    {200,80}, {200,60}};
    std::array<int, 2> vector4[] = { {200,40},   {240,60},     {260,40},     {220,20} };
    std::array<int, 2> vector5[] = { {140,20},   {140,40},     {160,40} };
    std::array<int, 2> vector6[] = { {240,120},  {260,140},    {260,100} };
    std::array<int, 2> vector7[] = { {180,20},   {180,40},     {200,20} };

    float radius = 15;
    std::array<int, 2> vector[] = {{25,100}, {123,250}, {500,342}, {600,302}};
    drawPoly(vector1, sizeof(vector1)/sizeof(vector1[0]));
    drawPoly(vector2, sizeof(vector2)/sizeof(vector2[0]));
    drawPoly(vector3, sizeof(vector3)/sizeof(vector3[0]));
    drawPoly(vector4, sizeof(vector4)/sizeof(vector4[0]));
    drawPoly(vector5, sizeof(vector5)/sizeof(vector5[0]));
    drawPoly(vector6, sizeof(vector6)/sizeof(vector6[0]));
    drawPoly(vector7, sizeof(vector7)/sizeof(vector7[0]));
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