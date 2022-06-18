#include "rtree.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include <array>
#include <complex>
#include <iomanip>
/*
g++ -c main.cpp
g++ main.o -o sfml-app -lsfml-graphics -lsfml-window -lsfml-system
./sfml-app

g++ -std=c++17 -g -c main.cpp &&  g++ -g main.o -o sfml-app -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system && ./sfml-app
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

bool FUN_MODE = false;
int vertical_offset = 150;
int horizontal_offset = 50;
int width = 1024, height = 1024;
int variable = 0;
sf::Color guiColor = sf::Color(250, 250, 250);
sf::RenderWindow window(sf::VideoMode(width, height), "R-Tree Eren la Gaviota");
sf::Music music;

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


void draw_layout() {
    
    sf::Texture t = sf::Texture();
    t.loadFromFile("background.jpg");
    
    sf::Sprite s(t);
    s.setScale({width/s.getLocalBounds().width, height/s.getLocalBounds().height});
    if (FUN_MODE) {
        window.draw(s);
        if (music.getStatus() != sf::SoundSource::Status(2)) {
            music.play();
        }
    } else {
        music.stop();
    }
    
    sf::RectangleShape aplha({width, height});
    aplha.setFillColor(sf::Color(0, 0, 0, 130));
    aplha.setOutlineColor(guiColor);
    window.draw(aplha);
    sf::RectangleShape rectangle1(sf::Vector2f(width-horizontal_offset, height-vertical_offset));
    rectangle1.setPosition(sf::Vector2f(horizontal_offset/2, vertical_offset/2));
    rectangle1.setFillColor(sf::Color::Transparent);
    rectangle1.setOutlineThickness(5);
    // rectangle1.setOutlineColor();
    window.draw(rectangle1);

    sf::Text title;
    sf::Text info;
    sf::Font font;
    if(!font.loadFromFile("ComicMono.ttf"))
    {
        // cout << "can't load font" << endl;
    }
    
    info.setFont(font); 
    title.setFont(font); 
    info.setString("Use el \"LEFT CLICK\" para dibujar puntos y \"ENTER\" para unirlos. \nPuede modificar el K de KNN con \"+\" Y \"-\".\nCualquier bug o duda, comunicarse con jose.huby@utec.edu.pe\n\"F\" para activar/desactivar FUN MODE");
    title.setString("R-Tree by Eren la Gaviota");
    info.setPosition(sf::Vector2f(horizontal_offset/2, height - vertical_offset/2 + 10));
    title.setPosition(sf::Vector2f(horizontal_offset/2, 0));
    info.setColor(guiColor);
    title.setColor(guiColor);
    // set the character size
    info.setCharacterSize(12); // in pixels, not points!
    title.setCharacterSize(44); // in pixels, not points!

    window.draw(info);
    window.draw(title);
}
std::vector<std::vector<int>> hilbert_matrix(width, std::vector<int>(height));
std::vector<sf::Vector2i> hilbert_vec;
int hilber_counter = 0;
//hilbert curve algorithm
void hilbert(float n, float x, float y, float xi, float xj, float yi, float yj) {
    if (n <= 0) {
        hilbert_matrix[x + (xi + yi)/2][y + (xj + yj)/2] = hilber_counter++;
        // hilbert_vec.push_back({x + (xi + yi)/2, y + (xj + yj)/2});
    } else {
        hilbert(n - 1, x            , y,                yi/2, yj/2, xi/2, xj/2);
        hilbert(n - 1, x+xi/2       , y+xj/2,           xi/2, xj/2, yi/2, yj/2);
        hilbert(n - 1, x+xi/2+yi/2  , y+xj/2+yj/2,      xi/2, xj/2, yi/2, yj/2);
        hilbert(n - 1, x+xi/2+yi    , y+xj/2+yj,        -yi/2, -yj/2, -xi/2, -xj/2);
    }
}

int main(int argc, char **argv){
    srand(time(NULL));
    r.print();
    std::vector<sf::Shape*> points;

    if (!music.openFromFile("music.wav"))
        return -1; // error

    hilbert(std::log2(height), 0, 0, width, 0, 0, height);
    //print each value of a matrix
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            std::cout << std::setw(4) << hilbert_matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
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
                    
                    if ((event.mouseButton.x > horizontal_offset/2 && event.mouseButton.x < width-horizontal_offset/2
                        && event.mouseButton.y > vertical_offset/2 && event.mouseButton.y < height-vertical_offset/2)) {
                            if (event.mouseButton.button == sf::Mouse::Left) {
                                int radius = 2;
                                sf::CircleShape *shape = new sf::CircleShape(radius);
                                shape->setPosition(event.mouseButton.x - radius, event.mouseButton.y - radius);
                                shape->setFillColor(sf::Color(250, 250, 250));
                                points.push_back(shape);
                            }
                            else if (event.mouseButton.button == sf::Mouse::Right) {
                                r.erase({event.mouseButton.x, event.mouseButton.y});
                            }
                    }
                    break;
                }
                case sf::Event::KeyReleased:
                {
                    switch (event.key.code)
                    {
                        case sf::Keyboard::F:
                        {
                            FUN_MODE = !FUN_MODE;
                            break;
                        }
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
                    break;
                }
            }
        }
        window.clear();


        draw_layout();

        for(auto it=points.begin();it!=points.end();it++)
        {
            window.draw(**it);
        }
        
        r.for_each_polygon(draw_poly);
    
        auto pos = sf::Mouse::getPosition(window);

        if (variable) 
            r.for_each_nearest(variable, {pos.x, pos.y}, draw_nearest);

        r.for_each_box(draw_box);

        if ((sf::Mouse::getPosition(window).x > horizontal_offset/2 && sf::Mouse::getPosition(window).x < width-horizontal_offset/2
            && sf::Mouse::getPosition(window).y > vertical_offset/2 && sf::Mouse::getPosition(window).y < height-vertical_offset/2)) {
            int radius = 4;
            sf::CircleShape *shape = new sf::CircleShape(radius);
            shape->setPosition(sf::Mouse::getPosition(window).x - radius, sf::Mouse::getPosition(window).y - radius);
            shape->setFillColor(sf::Color(250, 250, 250));
            window.draw(*shape);
            window.setMouseCursorVisible(false);
        } else {
            window.setMouseCursorVisible(true);
        }


        window.display();



    }
    return 0;
}