#ifndef GAME_OF_LIFE_HPP
#define GAME_OF_LIFE_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <algorithm>
#include <array>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

#include "ffmpeg_wrapper.hpp"

using Pixel = std::array<sf::Uint8, 4>;

namespace  {
    constexpr Pixel alive_color({107,142,35,255});
    constexpr Pixel dead_color({241, 241,212,255});
}


template<size_t Height, size_t Width, size_t Cycles>
class game_of_life
{
public:

using Row = std::array<Pixel, Width>;

    game_of_life()
        :
          window(sf::VideoMode(100, 100), "Game of Life"),
          active(&buffer[0]),
          next(&buffer[1])
        {
        initialize_data();
    }
    void run() {
        for(size_t cycles = 0; cycles < Cycles; ++cycles) {
            step();
            paint();
            take_input();
        }
    }

private:



    struct point {
        point() = delete;
        point(size_t x, size_t y) : x(x), y(y) { }

        bool is_alive(std::array<Row, Height> const *data) const noexcept{
            return (*data)[y][x] == alive_color;
        }

        bool is_valid() const noexcept {
            return (x >= 0) and (x <= Width) and (y >= 0) and (y <= Height);
        }

        const size_t x;
        const size_t y;

    };


    void initialize_data() noexcept {
        std::random_device rd;
        std::mt19937 mt(rd());


        for(size_t y = 0; y < Height; ++y) {
            for(size_t x = 0; x < Width; ++x) {
                if( mt() % 2)
                    (*active)[y][x] = alive_color;
                else
                    (*active)[y][x] = dead_color;
            }
        }

        paint();
        window.display();
    }

    void step() noexcept {


        for(size_t y = 0; y < Height; ++y) {
            for(size_t x = 0; x < Width; ++x) {
                auto this_point = point(x,y);
                auto possible_neighbors = { point(x-1,y-1), point(x,y-1), point(x+1, y-1),
                                            point(x-1, y), point(x, y+1), point(x+1, y-1),
                                            point(x+1, y), point(x+1,y+1) };

                auto predicate = [data_ptr = active](point const &p) -> bool{
                    return p.is_valid() and p.is_alive(data_ptr);
                };

                size_t living_neighbors = std::count_if(possible_neighbors.begin(),
                                                        possible_neighbors.end(),
                                                        predicate);

                if( (this_point.is_alive(active) and (living_neighbors == 2 or living_neighbors == 3))
                    or (!this_point.is_alive(active) and living_neighbors == 3) ) {
                    (*next)[y][x] = alive_color;
                }
                else {
                    (*next)[y][x] = dead_color;
                }
            }
        }

        std::swap(active,next);
    }

    void paint() noexcept {

        
        sf::Image i;
        i.create(Width, Height, static_cast<sf::Uint8*>(& (*active)[0][0][0]));
        sf::Texture t;
        t.loadFromImage(i);
        sf::Sprite s(t);
        window.clear();
        window.draw(s);
        window.display();

        v.add_frame(active);
    }

    void take_input() {
        // You might want to set it so that you have to press a button, f.ex. [Space]
        // before you do the next tick.
    }

    std::array<std::array<Row, Height>,2> buffer;
    std::array<Row, Height> *active;
    std::array<Row, Height> *next;

    sf::RenderWindow window;

    ffmpeg_wrapper<Height, Width> v;
};

#endif // GAME_OF_LIFE_HPP
