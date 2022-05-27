#include "rtree.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <array>
#include <array>
/*
-----Distancia Entre 2 Poligonos-----
        Grupo Gaviotas

Idea principal:
    Es de conocimiento general (en la clase) que hay un algoritmo que permite calcular la distancia entre 1 punto y 1 poligono que
    utiliza la zona Berrospi y Pedro.
    Nuestra idea es un poco costosa pero simple. Utilizar ese algorito punto-poligono con cada punto de uno de los poligonos a comparar
    O sea, si se quiere saber la distancia entre un poligono A y uno B, entonces se hace el algoritmo punto-poligono con cada punto 
    del poligono A hacia el poligono B y see escoje la minima distancia. Esto nos dará la minima distancia poligono-poligono, siempre y cuando
    no la minima distancia de un poligino a otro se encuentre siempre de un vertice hacia una arísta o de una vertice a otro, o sea, nunca
    de una arista a otra arista. Esto si se medita es lógico, sin embargo, tambien se puede demostrar matematicamente (Mario hizo algo 
    así en el "adelanto" de esto).
    Es inificiente (probablemente O(n*m) donde n es el numero de lados de un poligono, y m del otro), se podria hacer las siguientes mejoras:
        Enonctrar la manera de descartar algunos puntos para no realizar la comparacion
        Evitar el uso de funciones pesadas como, arcotangente, etc
        Cosas raras y avanzadas de C++21


Requisitos:
    tener isntalado SFML

Constraints: 
    El polígono tiene que ser convexo
    Lo que se dibuje tiene que tener como mínimo 3 lados

Para ejecutar:
    Comandos: 
        g++ -c main.cpp
        g++ main.o -o sfml-app -lsfml-graphics -lsfml-window -lsfml-system
        ./sfml-app

    O, todo junto:
        g++ -std=c++17 -c main.cpp && g++ main.o -o sfml-app -lsfml-graphics -lsfml-window -lsfml-system && ./sfml-app

Uso:
    Se pueden dibujar puntos, utilizando el mouse.

    Una vez se tengan los puntos del poligono deseado, se presiona "ENTER", par crear este poligono

    Se repite los ultimos pasos para crear un 2do poligono

    Una vez creado el 2do, ya se mostrará la distancia entre estos 2.

    Se pueden utilizar las flechas (UP, DOWN, LEFT, RIGHT) para mover el último poligono creado y 
    ver como se actualiza la distancia

    Se pueden crear más poligonos y siempre se dibujar la distancia entre todos, sin embargo, 
    solo se podra controlar con las flechas el último poligono creado. 

Gracias 
https://image-aws-us-west-2.vsco.co/77c290/22229248/5f7fc521822a673d348d43db/1136x883/vsco5f7fc523adb3a.jpg
*/

#define carro auto

#define ORDER 3
#define DIM 2
#define MAX_POLY 1000
#define MAX_VERTEX 32
#define MAX_KNN 16

RTree<ORDER, DIM, MAX_POLY, MAX_VERTEX, MAX_KNN> r;
using Polygon = decltype(r)::Polygon;
using Box = decltype(r)::Box;

int width = 900, height = 600;
sf::RenderWindow window(sf::VideoMode(width, height), "SFML works!");

sf::Color colors[] = {
    sf::Color(0, 255, 255),
    sf::Color(255, 0, 255),
    sf::Color(255, 255, 0),
    sf::Color(255, 0, 0),
    sf::Color(0, 255, 0)
};


// template<typename Box>
int h = 1;

void draw_box(const Box& box, int lvl, int height) {
    sf::Color const& color = colors[lvl%5];
    int offset = (lvl < height)*3*(height - lvl);

    auto aX = box.mins[0]-(offset);
    auto aY = box.mins[1]-(offset);

    auto bX = box.maxs[0]+(offset);
    auto bY = box.maxs[1]+(offset);

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

float distance(float aX,float  aY,float  bX,float  bY) {
    return hypot(aX - bX, aY - bY);
}

sf::Vector2f line_to_poly(std::vector<sf::Vector2f> points, sf::Vector2f pos) {
    int size = points.size();
    
    float min_hypot = 1e10;
    int min_idx;
    sf::Vector2f left, right, min;
    for (int i = 0; i < size; i++) {
        float current_hypot = hypot(points[i].x-pos.x, points[i].y-pos.y);
        if (min_hypot > current_hypot) {
            min_hypot = current_hypot;
            min = points[i];
            min_idx = i;
        }
    }

    left = points[(min_idx+1)%size];
    right = points[(min_idx > 0 ? min_idx-1 : size-1)];
    
    sf::Vector2f ab = { min.x - left.x , min.y - left.y };
    sf::Vector2f cb = { min.x - pos.x   , min.y - pos.y };

    float dot = (ab.x * cb.x + ab.y * cb.y);
    float cross = (ab.x * cb.y - ab.y * cb.x);

    float theta1 = atan2(cross, dot);

    ab = { min.x - right.x , min.y - right.y };
    cb = { min.x - pos.x   , min.y - pos.y };

    dot = (ab.x * cb.x + ab.y * cb.y);
    cross = (ab.x * cb.y - ab.y * cb.x);

    float theta2 = atan2(cross, dot);
    sf::Vector2f pointA;
    sf::Vector2f pointB;
    if (abs(theta1) < M_PI/2) 
    {
        float m =  (min.y-left.y)/(min.x-left.x);
        float c = (min.x*left.y-left.x*min.y)/(min.x-left.x);
        float d = (pos.x + (pos.y - c)*m)/(1 + m*m);
        float px = 2*d - pos.x;
        float py = 2*d*m - pos.y + 2*c;
        py = (py + (pos.y))/2;
        px = (px + (pos.x))/2;
        pointA = {px, py};
    } 
    else 
    {
        pointA = min;
    }

    if (abs(theta2) < M_PI/2) 
    {
        float m =  (min.y-right.y)/(min.x-right.x);
        float c = (min.x*right.y-right.x*min.y)/(min.x-right.x);
        float d = (pos.x + (pos.y - c)*m)/(1 + m*m);
        float px = 2*d - pos.x;
        float py = 2*d*m - pos.y + 2*c;
        py = (py + (pos.y))/2;
        px = (px + (pos.x))/2;
        pointB = {px, py};
    } 
    else 
    {
        pointB = min;
    }
    if (distance(pointA.x, pointA.y, pos.x, pos.y) < distance(pointB.x, pointB.y, pos.x, pos.y)) {
        return pointA;
    } else {
        return pointB;
    }
}

void draw_poly(std::vector<sf::Vector2f> points) {
    int size = points.size();
    if (size < 3) {
        throw("Ga");
    }

    sf::Vertex a;
    sf::Vertex b;
    for (int i = 0; i < size-1; i++) {
        const sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(points[i].x, points[i].y)),
            sf::Vertex(sf::Vector2f(points[i+1].x, points[i+1].y))
        };
        int radius = 2;
        sf::CircleShape shape(2);
        shape.setPosition(sf::Vector2f(points[i].x-radius, points[i].y-radius));
        window.draw(line, 2, sf::Lines);
        window.draw(shape);
    }

    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(points[size-1].x, points[size-1].y)),
        sf::Vertex(sf::Vector2f(points[0].x, points[0].y))
    };
    

    int radius = 2;
    sf::CircleShape shape(2);
    shape.setPosition(sf::Vector2f(points[size-1].x-radius, points[size-1].y-radius));
    window.draw(line, 2, sf::Lines);
    window.draw(shape);
}


// void draw_polygons() {
//     int i = 0;
//     auto polys = r.get_polygons();
//     while (polys[i].size) {
//         const carro&  poly = polys[i];
//         draw_poly(poly);
//         i++;
//     }
// }

// void draw_boxes() {
//     int i = 0;
//     auto nodes = r.get_nudes();
//     while (nodes[i].size) {
//         const carro&  node = nodes[i];
//         const carro& boxes = node.box;
//         for (int i = 0; i < node.size; i++) {
//             draw_box(boxes[i]);
//         }
//         i++;
//     }
// }



int main(int argc, char **argv){
    srand(time(NULL));

    // r.insert({1, 3, { {10, 10},   {5, 20},      {15, 20} }});
    // r.insert({1, 3, { {800, 540},   {790, 555},      {850, 555} }});
    // r.insert({1, 3, { {780, 570},   {760, 595},      {785, 595} }});
    // r.insert({1, 3, { {391, 419},   {242, 264},      {400, 440} }});



    // r.insert({1, 3, { {300, 200},   {280, 220},      {320, 220} }});

    // r.insert({1, 4, { {600, 405}, {500, 405},   {600, 500},      {500, 500} }});
    // r.insert({1, 11, { {55, 50}, {151, 47},{101, 75},{219, 73}, {150, 110}, {273, 112}, {330, 142}, {383, 176}, {242, 203}, {86, 153}, {100, 100}}});
    // r.insert({2, 6, { {40,0},    {40,20},      {60,40},      {100,20}, {120,40}, {100,0} }});
    // r.insert({3, 5, { {160,60},   {160,80},     {180,500},    {200,80}, {200,60}}});
    // r.insert({4, 4, { {200,40},   {240,60},     {260,40},     {220,20} }});
    // r.insert({5, 3, { {140,20},   {140,40},     {160,40} }});
    // r.insert({6, 3, { {240,120},  {260,140},    {260,100} }});
    // r.insert({7, 3, { {180,20},   {180,40},     {200,20} }});
    r.print();

    std::vector<sf::Shape*> points;
    std::vector<std::vector<sf::Vector2f>> polys;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type){
                case sf::Event::Closed:
                {
                    window.close();
                    return 0;
                }

                case sf::Event::MouseButtonPressed:
                {
                    int radius = 2;
                    sf::CircleShape *shape = new sf::CircleShape(radius);
                    shape->setPosition(event.mouseButton.x - radius, event.mouseButton.y - radius);
                    shape->setFillColor(sf::Color(250, 250, 250));
                    points.push_back(shape);
                }

                

                case sf::Event::KeyReleased:
                {
                    switch (event.key.code)
                    {
                        case sf::Keyboard::Enter:
                        {
                            std::cout << "Enter\n";
                            std::vector<sf::Vector2f> shape;                            
                            for(int i = 0; i < points.size(); i++)
                            {
                                sf::Vector2f pos = points[i]->getPosition();
                                shape.push_back(pos);
                            }
                            polys.push_back(shape);
                            // r.insert(poly);
                            // r.print();
                            points.clear();
                        }
                    }
                }

                case sf::Event::KeyPressed:
                {
                    if (event.key.code == sf::Keyboard::Left)
                    {
                        auto shape = &polys.back();
                        for (int i = 0; i < shape->size(); i++) {
                            shape->at(i) = {shape->at(i).x-2, shape->at(i).y};
                        }
                    }
                    if (event.key.code == sf::Keyboard::Right)
                    {
                        auto shape = &polys.back();
                        for (int i = 0; i < shape->size(); i++) {
                            shape->at(i) = {shape->at(i).x+2, shape->at(i).y};
                        }
                    }
                    if (event.key.code == sf::Keyboard::Up)
                    {
                        auto shape = &polys.back();
                        for (int i = 0; i < shape->size(); i++) {
                            shape->at(i) = {shape->at(i).x, shape->at(i).y-2};
                        }
                    }
                    if (event.key.code == sf::Keyboard::Down)
                    {
                        auto shape = &polys.back();
                        for (int i = 0; i < shape->size(); i++) {
                            shape->at(i) = {shape->at(i).x, shape->at(i).y+2};
                        }
                    }
                    
                }
            }
        }
        window.clear();

        for(auto it=points.begin();it!=points.end();it++)
        {
            window.draw(**it);
        }
        // r.for_each_polygon(line_to_poly);

        for (int i = 0; i < polys.size(); i++) {
            auto shape_a = polys[i];
            sf::Vector2f  min_vertex_a = {1e5, 1e6};
            sf::Vector2f  min_vertex_b = {1e10, 1e10};

            for (int j = i+1; j < polys.size(); j++) {

                float min_distance = 1e10;
                auto shape_b = polys[j];

                for (auto vertex: shape_b) {
                    auto current_vertex = line_to_poly(shape_a, vertex);
                    float current_distance = distance(current_vertex.x, current_vertex.y, vertex.x, vertex.y);
                    if (min_distance > current_distance) {
                        min_distance = current_distance;
                        min_vertex_a = current_vertex;
                        min_vertex_b = vertex;
                    }
                }
            }
            const sf::Vertex line_a[] = {
                    sf::Vertex(sf::Vector2f(min_vertex_a.x, min_vertex_a.y)),
                    sf::Vertex(sf::Vector2f(min_vertex_b.x, min_vertex_b.y))
                };
            window.draw(line_a, 2, sf::Lines);
            draw_poly(shape_a);
        }
        // r.for_each_polygon(draw_poly);
        // r.for_each_box(draw_box);

        int radius = 2;
        sf::CircleShape *shape = new sf::CircleShape(radius);
        shape->setPosition(sf::Mouse::getPosition(window).x - radius, sf::Mouse::getPosition(window).y - radius);
        shape->setFillColor(sf::Color(250, 250, 250));
        window.draw(*shape);


        window.display();

    }
    return 0;
}