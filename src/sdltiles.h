#pragma once
#ifndef CATA_SRC_SDLTILES_H
#define CATA_SRC_SDLTILES_H

#include "point.h" // IWYU pragma: keep

namespace catacurses
{
class window;
} // namespace catacurses

#if defined(TILES)

#include <memory>
#include <string>

#include "color_loader.h"
#include "coordinates.h"
#include "sdl_wrappers.h"
#include "string_id.h"

class cata_tiles;

struct weather_type;

using weather_type_id = string_id<weather_type>;

namespace catacurses
{
class window;
} // namespace catacurses

namespace overmap_ui
{
// {note symbol, note color, offset to text}
std::tuple<char, nc_color, size_t> get_note_display_info( const std::string &note );
weather_type_id get_weather_at_point( const tripoint_abs_omt &pos );
} // namespace overmap_ui

extern std::unique_ptr<cata_tiles> tilecontext;
extern std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT> windowsPalette;
extern int fontheight;
extern int fontwidth;

void load_tileset();
void rescale_tileset( int size );
bool save_screenshot( const std::string &file_path );
void toggle_fullscreen_window();

struct window_dimensions {
    point scaled_font_size;
    point window_pos_cell;
    point window_size_cell;
    point window_pos_pixel;
    point window_size_pixel;
};
window_dimensions get_window_dimensions( const catacurses::window &win );
// Get dimensional info of an imaginary normal catacurses::window with the given
// position and size. Unlike real catacurses::window, size can be zero.
window_dimensions get_window_dimensions( const point &pos, const point &size );

#endif // TILES

// Text level, valid only for a point relative to the window, not a point in overall space.
bool window_contains_point_relative( const catacurses::window &win, const point &p );
#endif // CATA_SRC_SDLTILES_H
