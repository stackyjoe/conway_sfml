#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "game_of_life.hpp"

constexpr size_t height = 720;
constexpr size_t width = 1280;
constexpr size_t number_of_ticks = 120;

int main()
{
    auto game = game_of_life<height,width,number_of_ticks>();

    game.run();

    return EXIT_SUCCESS;
}
