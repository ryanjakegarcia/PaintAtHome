#ifndef PAINT_H
#define PAINT_H

#include <SDL2/SDL.h>

typedef enum Color {
    BLACK,
    DARK_GREY,
    DARK_RED,
    DARK_YELLOW,
    DARK_GREEN,
    DARK_TEAL,
    DARK_BLUE,
    DARK_PURPLE,
    MILITARY_GREEN,
    DEEP_GREEN,
    DESERT_BLUE,
    NAVY_BLUE,
    ROYAL_PURPLE,
    MAROON_BROWN,
    WHITE,
    LIGHT_GREY,
    BRIGHT_RED,
    BRIGHT_YELLOW,
    BRIGHT_GREEN,
    CYAN_AQUA,
    BRIGHT_BLUE,
    MAGENTA,
    PALE_YELLOW,
    SPRING_GREEN,
    LIGHT_CYAN,
    LIGHT_BLUE,
    LIGHT_PINK,
    ORANGE,
    COLOR_COUNT
} Color;

extern Uint32 color_palette[];
static void draw_palette(SDL_Surface *surface, int origin_x, int origin_y, int box_w, int box_h, int gap, int columns, Color active_color);
static int palette_hit_test(int mouse_x, int mouse_y, int origin_x, int origin_y, int box_w, int box_h, int gap, int columns);
int distance(int x1, int y1, int x2, int y2);
void updateXY(int *x, int *y, int *uX, int *uY);
void draw_pixel(int x, int y, SDL_Surface *canvas, Uint32 color);
void draw_circle(int xc, int yc, int r, SDL_Surface *canvas, Color color);
void draw_stroke_segment(int x0, int y0, int x1, int y1, int brush_radius, SDL_Surface *canvas, Color color);
SDL_Surface *copy_surface(SDL_Surface *src);
void push_undo(SDL_Surface *undo_stack[], int *undo_top, int max_undo, SDL_Surface *canvas);
void pop_undo(SDL_Surface *undo_stack[], int *undo_top, SDL_Surface *canvas);

#endif
