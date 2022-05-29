#include "rtree.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <array>
#include <complex>
/*
g++ -c main.cpp
g++ main.o -o sfml-app -lsfml-graphics -lsfml-window -lsfml-system
./sfml-app

g++ -std=c++17 -g -c main.cpp &&  g++ -g main.o -o sfml-app -lsfml-graphics -lsfml-window -lsfml-system && ./sfml-app
*/

#define carro auto

#define ORDER 3
#define DIM 2
#define MAX_POLY 1000
#define MAX_VERTEX 32
#define MAX_KNN 16

RTree<ORDER, DIM, MAX_POLY, MAX_VERTEX, MAX_KNN> r;
using Polygon = decltype(r)::Polygon;
using Point = decltype(r)::Point;
using Box = decltype(r)::Box;

int width = 900, height = 600;
int variable = 0;

sf::RenderWindow window(sf::VideoMode(width, height), "SFML works!");

sf::Color colors[] = {
    sf::Color(0, 255, 255),
    sf::Color(255, 0, 255),
    sf::Color(255, 255, 0),
    sf::Color(255, 0, 0),
    sf::Color(0, 255, 0)
};


int h = 1;

void draw_box(const Box& box, int lvl, int height) {
    sf::Color const& color = colors[lvl%5];
    int offset = (lvl < height)*2*(height - lvl);

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

void line_to_poly(Polygon const& polygon) {
    int size = polygon.size;
    const auto& points = polygon.vertex;
    auto pos = sf::Mouse::getPosition(window);
    float min_x, min_y, min_hypot = 1e10;
    float min_x_a, min_y_a, min_hypot_a = 1e10; 
    for (int i = 0; i < size; i++) {
        float current_hypot = hypot(points[i][0]-pos.x, points[i][1]-pos.y);
        if (min_hypot > current_hypot) {
            if (min_hypot_a > min_hypot) {
                min_hypot_a = min_hypot;
                min_x_a = min_x;
                min_y_a = min_y;
            }
            min_hypot = current_hypot;
            min_x = points[i][0];
            min_y = points[i][1];
        } else if (min_hypot_a > current_hypot) {
                min_hypot_a = current_hypot;
                min_x_a = points[i][0];
                min_y_a = points[i][1];
            }
    }

    sf::Vector2f ab = { min_x - min_x_a, min_y - min_y_a };
    sf::Vector2f cb = { min_x - pos.x, min_y - pos.y };

    float dot = (ab.x * cb.x + ab.y * cb.y); 
    float cross = (ab.x * cb.y - ab.y * cb.x); 

    float alpha = atan2(cross, dot);

    int theta = floor(alpha * 180. / M_PI + 0.5);
    if (abs(theta) < 90) {
        float m =  (min_y-min_y_a)/(min_x-min_x_a);
        float c = (min_x*min_y_a-min_x_a*min_y)/(min_x-min_x_a);
        float d = (pos.x + (pos.y - c)*m)/(1 + m*m);
        float px = 2*d - pos.x;
        float py = 2*d*m - pos.y + 2*c;
        py = (py + (pos.y))/2;
        px = (px + (pos.x))/2;
        const sf::Vertex line_a[] = {
                sf::Vertex(sf::Vector2f(px, py)),
                sf::Vertex(sf::Vector2f(pos.x, pos.y))
            };
        window.draw(line_a, 2, sf::Lines);
    } else {
        const sf::Vertex line_a[] = {
                sf::Vertex(sf::Vector2f(min_x, min_y)),
                sf::Vertex(sf::Vector2f(pos.x, pos.y))
            };
        window.draw(line_a, 2, sf::Lines);

    };

}

void draw_nearest(Polygon const& polygon, Point const& source, Point const& target, int distance) {
    sf::Color color(250, 0, 0);
    int size = polygon.size;
    const auto& points = polygon.vertex;

    sf::Vertex a;
    sf::Vertex b;

    for (int i = 0; i < size-1; i++) {
        const sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(points[i][0], points[i][1]), color),
            sf::Vertex(sf::Vector2f(points[i+1][0], points[i+1][1]), color)
        };
        int radius = 2;
        sf::CircleShape shape(2);
        shape.setPosition(sf::Vector2f(points[i][0]-radius, points[i][1]-radius));
        window.draw(line, 2, sf::Lines);
        window.draw(shape);
    }

    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(points[size-1][0], points[size-1][1]), color),
        sf::Vertex(sf::Vector2f(points[0][0], points[0][1]), color)
    };
    

    int radius = 2;
    sf::CircleShape shape(2);
    shape.setPosition(sf::Vector2f(points[size-1][0]-radius, points[size-1][1]-radius));
    window.draw(line, 2, sf::Lines);
    window.draw(shape);

     sf::Vertex la_linea[] = {
        sf::Vertex(sf::Vector2f(source[0], source[1]), color),
        sf::Vertex(sf::Vector2f(target[0], target[1]), color)
    };
    window.draw(la_linea, 2, sf::Lines);
    
}


void draw_poly(Polygon const& polygon) {
    
    int size = polygon.size;
    const auto& points = polygon.vertex;

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
    r.print();
    std::vector<sf::Shape*> points;

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
                            decltype(r)::Polygon poly;
                            poly.size = points.size();
                            for(int i = 0; i < points.size(); i++)
                            {
                                sf::Vector2f pos = points[i]->getPosition();
                                poly.vertex[i][0] = pos.x;
                                poly.vertex[i][1] = pos.y;
                            }

                            r.insert(poly);
                            r.print();
                            points.clear();
                            break;
                        }

                        case sf::Keyboard::Add:
                        {
                            if (variable < r.get_size()) 
                                variable++;
                            break;
                        }
                        case sf::Keyboard::Subtract:
                        {
                            if (variable) 
                                variable--;
                            break;
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
        
        r.for_each_polygon(draw_poly);
    
        auto pos = sf::Mouse::getPosition(window);

        if (variable) 
            r.for_each_nearest(variable, {pos.x, pos.y}, draw_nearest);

        r.for_each_box(draw_box);

        int radius = 2;
        sf::CircleShape *shape = new sf::CircleShape(radius);
        shape->setPosition(sf::Mouse::getPosition(window).x - radius, sf::Mouse::getPosition(window).y - radius);
        shape->setFillColor(sf::Color(250, 250, 250));
        window.draw(*shape);


        window.display();

    }
    return 0;
}