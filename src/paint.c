#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "paint.h"

static const int WIDTH = 1200;
static const int HEIGHT = 800;

static const int TARGET_FPS = 60;

static const int PALETTE_X = 4;
static const int PALETTE_Y = 36;
static const int PALETTE_BOX_W = 24;
static const int PALETTE_BOX_H = 24;
static const int PALETTE_GAP = 4;
static const int PALETTE_COLUMNS = 14;
static const int FILE_MENU_X = 4;
static const int FILE_MENU_Y = 4;
static const int FILE_MENU_BUTTON_W = 56;
static const int FILE_MENU_BUTTON_H = 22;
static const int FILE_MENU_DROPDOWN_W = 136;
static const int FILE_MENU_ITEM_H = 20;
static const char *SAVE_DIR = "save";
static const char *DEFAULT_BMP_PATH = "save/drawing.bmp";
static const size_t MAX_PATH_LEN = 512;

typedef enum MenuAction {
    MENU_ACTION_NONE,
    MENU_ACTION_SAVE,
    MENU_ACTION_SAVE_AS,
    MENU_ACTION_OPEN,
    MENU_ACTION_REOPEN
} MenuAction;

static bool point_in_rect(int x, int y, SDL_Rect rect) {
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

static bool glyph_for_char(char c, unsigned char out_rows[7]) {
    memset(out_rows, 0, 7);
    switch (c) {
        case 'A': out_rows[0]=0x04; out_rows[1]=0x0A; out_rows[2]=0x11; out_rows[3]=0x1F; out_rows[4]=0x11; out_rows[5]=0x11; out_rows[6]=0x11; return true;
        case 'E': out_rows[0]=0x1F; out_rows[1]=0x10; out_rows[2]=0x10; out_rows[3]=0x1E; out_rows[4]=0x10; out_rows[5]=0x10; out_rows[6]=0x1F; return true;
        case 'F': out_rows[0]=0x1F; out_rows[1]=0x10; out_rows[2]=0x10; out_rows[3]=0x1E; out_rows[4]=0x10; out_rows[5]=0x10; out_rows[6]=0x10; return true;
        case 'I': out_rows[0]=0x1F; out_rows[1]=0x04; out_rows[2]=0x04; out_rows[3]=0x04; out_rows[4]=0x04; out_rows[5]=0x04; out_rows[6]=0x1F; return true;
        case 'L': out_rows[0]=0x10; out_rows[1]=0x10; out_rows[2]=0x10; out_rows[3]=0x10; out_rows[4]=0x10; out_rows[5]=0x10; out_rows[6]=0x1F; return true;
        case 'N': out_rows[0]=0x11; out_rows[1]=0x19; out_rows[2]=0x15; out_rows[3]=0x13; out_rows[4]=0x11; out_rows[5]=0x11; out_rows[6]=0x11; return true;
        case 'O': out_rows[0]=0x0E; out_rows[1]=0x11; out_rows[2]=0x11; out_rows[3]=0x11; out_rows[4]=0x11; out_rows[5]=0x11; out_rows[6]=0x0E; return true;
        case 'P': out_rows[0]=0x1E; out_rows[1]=0x11; out_rows[2]=0x11; out_rows[3]=0x1E; out_rows[4]=0x10; out_rows[5]=0x10; out_rows[6]=0x10; return true;
        case 'R': out_rows[0]=0x1E; out_rows[1]=0x11; out_rows[2]=0x11; out_rows[3]=0x1E; out_rows[4]=0x14; out_rows[5]=0x12; out_rows[6]=0x11; return true;
        case 'S': out_rows[0]=0x0F; out_rows[1]=0x10; out_rows[2]=0x10; out_rows[3]=0x0E; out_rows[4]=0x01; out_rows[5]=0x01; out_rows[6]=0x1E; return true;
        case 'V': out_rows[0]=0x11; out_rows[1]=0x11; out_rows[2]=0x11; out_rows[3]=0x11; out_rows[4]=0x11; out_rows[5]=0x0A; out_rows[6]=0x04; return true;
        case ' ': return true;
        default: return false;
    }
}

static void draw_text_5x7(SDL_Surface *surface, int x, int y, const char *text, Uint32 color, int scale) {
    for (size_t idx = 0; text[idx] != '\0'; idx++) {
        unsigned char rows[7];
        char c = (char)toupper((unsigned char)text[idx]);
        if (!glyph_for_char(c, rows)) {
            c = ' ';
            glyph_for_char(c, rows);
        }

        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if ((rows[row] >> (4 - col)) & 1) {
                    SDL_Rect pixel = {
                        x + (int)idx * (6 * scale) + col * scale,
                        y + row * scale,
                        scale,
                        scale
                    };
                    SDL_FillRect(surface, &pixel, color);
                }
            }
        }
    }
}

static void draw_file_menu(SDL_Surface *surface, bool menu_open) {
    SDL_Rect button = {FILE_MENU_X, FILE_MENU_Y, FILE_MENU_BUTTON_W, FILE_MENU_BUTTON_H};
    SDL_FillRect(surface, &button, 0x202020);

    SDL_Rect btop = {button.x, button.y, button.w, 2};
    SDL_Rect bbot = {button.x, button.y + button.h - 2, button.w, 2};
    SDL_Rect bleft = {button.x, button.y, 2, button.h};
    SDL_Rect bright = {button.x + button.w - 2, button.y, 2, button.h};
    SDL_FillRect(surface, &btop, 0xE0E0E0);
    SDL_FillRect(surface, &bbot, 0xE0E0E0);
    SDL_FillRect(surface, &bleft, 0xE0E0E0);
    SDL_FillRect(surface, &bright, 0xE0E0E0);
    draw_text_5x7(surface, FILE_MENU_X + 8, FILE_MENU_Y + 5, "FILE", 0xFFFFFF, 2);

    if (!menu_open) {
        return;
    }

    const char *items[] = {"SAVE", "SAVE AS", "OPEN", "REOPEN"};
    for (int i = 0; i < 4; i++) {
        SDL_Rect item = {
            FILE_MENU_X,
            FILE_MENU_Y + FILE_MENU_BUTTON_H + i * FILE_MENU_ITEM_H,
            FILE_MENU_DROPDOWN_W,
            FILE_MENU_ITEM_H
        };
        SDL_FillRect(surface, &item, 0x2A2A2A);

        SDL_Rect itop = {item.x, item.y, item.w, 1};
        SDL_Rect ibot = {item.x, item.y + item.h - 1, item.w, 1};
        SDL_Rect ileft = {item.x, item.y, 1, item.h};
        SDL_Rect iright = {item.x + item.w - 1, item.y, 1, item.h};
        SDL_FillRect(surface, &itop, 0x707070);
        SDL_FillRect(surface, &ibot, 0x707070);
        SDL_FillRect(surface, &ileft, 0x707070);
        SDL_FillRect(surface, &iright, 0x707070);

        draw_text_5x7(surface, item.x + 8, item.y + 4, items[i], 0xF0F0F0, 2);
    }
}

static MenuAction file_menu_hit_test(int mouse_x, int mouse_y, bool menu_open, bool *clicked_button) {
    SDL_Rect button = {FILE_MENU_X, FILE_MENU_Y, FILE_MENU_BUTTON_W, FILE_MENU_BUTTON_H};
    *clicked_button = false;

    if (point_in_rect(mouse_x, mouse_y, button)) {
        *clicked_button = true;
        return MENU_ACTION_NONE;
    }

    if (!menu_open) {
        return MENU_ACTION_NONE;
    }

    for (int i = 0; i < 4; i++) {
        SDL_Rect item = {
            FILE_MENU_X,
            FILE_MENU_Y + FILE_MENU_BUTTON_H + i * FILE_MENU_ITEM_H,
            FILE_MENU_DROPDOWN_W,
            FILE_MENU_ITEM_H
        };
        if (point_in_rect(mouse_x, mouse_y, item)) {
            return (MenuAction)(i + 1);
        }
    }

    return MENU_ACTION_NONE;
}

Uint32 color_palette[] = {
    // Top Row (Darker tones/Neutrals)
    0x000000, // BLACK
    0x808080, // DARK GREY
    0x800000, // DARK RED
    0x808000, // DARK YELLOW/OLIVE
    0x008000, // DARK GREEN
    0x008080, // DARK TEAL
    0x000080, // DARK BLUE
    0x800080, // DARK PURPLE
    0x808040, // MILITARY GREEN
    0x004040, // DEEP GREEN
    0x0080FF, // DESERT BLUE
    0x004080, // NAVY BLUE
    0x8000FF, // ROYAL PURPLE
    0x804000, // MAROON/BROWN

    // Bottom Row (Brighter colors/Pastels)
    0xFFFFFF, // WHITE
    0xC0C0C0, // LIGHT GREY
    0xFF0000, // BRIGHT RED
    0xFFFF00, // BRIGHT YELLOW
    0x00FF00, // BRIGHT GREEN
    0x00FFFF, // CYAN/AQUA
    0x0000FF, // BRIGHT BLUE
    0xFF00FF, // MAGENTA
    0xFFFF80, // PALE YELLOW
    0x00FF80, // SPRING GREEN
    0x80FFFF, // LIGHT CYAN
    0x8080FF, // LIGHT BLUE
    0xFF0080, // LIGHT PINK
    0xFF8040  // ORANGE
};

static const size_t MAX_UNDO = 20;

static void draw_palette(
    SDL_Surface *surface,
    int origin_x,
    int origin_y,
    int box_w,
    int box_h,
    int gap,
    int columns,
    Color active_color
) {
    int palette_count = COLOR_COUNT;

    for (int i = 0; i < palette_count; i++) {
        int row = i / columns;
        int col = i % columns;

        int x = origin_x + col * (box_w + gap);
        int y = origin_y + row * (box_h + gap);

        SDL_Rect swatch = { x, y, box_w, box_h };
        SDL_FillRect(surface, &swatch, color_palette[i]);

        Uint32 border_color = (i == (int)active_color) ? 0xFFFFFF : 0x404040;

        SDL_Rect top = { x, y, box_w, 2 };
        SDL_Rect bottom = { x, y + box_h - 2, box_w, 2 };
        SDL_Rect left = { x, y, 2, box_h };
        SDL_Rect right = { x + box_w - 2, y, 2, box_h };

        SDL_FillRect(surface, &top, border_color);
        SDL_FillRect(surface, &bottom, border_color);
        SDL_FillRect(surface, &left, border_color);
        SDL_FillRect(surface, &right, border_color);
    }
}

static int palette_hit_test(
    int mouse_x,
    int mouse_y,
    int origin_x,
    int origin_y,
    int box_w,
    int box_h,
    int gap,
    int columns
) {
    for (int i = 0; i < COLOR_COUNT; i++) {
        int row = i / columns;
        int col = i % columns;

        int x = origin_x + col * (box_w + gap);
        int y = origin_y + row * (box_h + gap);

        if (mouse_x >= x && mouse_x < x + box_w &&
            mouse_y >= y && mouse_y < y + box_h) {
            return i;
        }
    }

    return -1;
}

int distance(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    return dx * dx + dy * dy;
}

void updateXY(int *x, int *y, int *uX, int *uY) {
    *x = *uX;
    *y = *uY;
}

void draw_pixel(int x, int y, SDL_Surface *canvas, Uint32 color) {
    SDL_Rect pixel = {x, y, 1, 1};
    SDL_FillRect(canvas, &pixel, color);
}

void draw_circle(int xc, int yc, int r, SDL_Surface *canvas, Color color) {
    for (int i = xc - r; i < xc + r; i++) {
        for (int j = yc - r; j < yc + r; j++) {
            if (distance(xc, yc, i, j) <= r * r) {
                draw_pixel(i, j, canvas, color_palette[color]);
            }
        }
    }
}

void draw_stroke_segment(int x0, int y0, int x1, int y1, int brush_radius, SDL_Surface *canvas, Color color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    if (steps == 0) {
        draw_circle(x1, y1, brush_radius, canvas, color);
        return;
    }

    for (int i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;
        int x = x0 + (int)(t * dx);
        int y = y0 + (int)(t * dy);
        draw_circle(x, y, brush_radius, canvas, color);
    }
}

SDL_Surface *copy_surface(SDL_Surface *src) {
    SDL_Surface *dst = SDL_CreateRGBSurface(
        0, src->w, src->h, src->format->BitsPerPixel,
        src->format->Rmask, src->format->Gmask,
        src->format->Bmask, src->format->Amask
    );
    if (!dst) {
        return NULL;
    }
    SDL_BlitSurface(src, NULL, dst, NULL);
    return dst;
}

void push_undo(SDL_Surface *undo_stack[], int *undo_top, int max_undo, SDL_Surface *canvas) {
    if (*undo_top == max_undo) {
        SDL_FreeSurface(undo_stack[0]);
        for (int i = 1; i < max_undo; i++) {
            undo_stack[i - 1] = undo_stack[i];
        }
        *undo_top = max_undo - 1;
    }

    SDL_Surface *snapshot = copy_surface(canvas);
    if (snapshot) {
        undo_stack[*undo_top] = snapshot;
        (*undo_top)++;
    }
}

void pop_undo(SDL_Surface *undo_stack[], int *undo_top, SDL_Surface *canvas) {
    if (*undo_top <= 0) {
        return;
    }

    (*undo_top)--;
    SDL_Surface *snapshot = undo_stack[*undo_top];
    SDL_BlitSurface(snapshot, NULL, canvas, NULL);
    SDL_FreeSurface(snapshot);
    undo_stack[*undo_top] = NULL;
}

static void resize_surfaces(
    SDL_Window *window,
    SDL_Surface **window_surface,
    SDL_Surface **canvas,
    SDL_Surface **cursor_preview,
    int new_width,
    int new_height
) {
    SDL_Surface *new_canvas = SDL_CreateRGBSurface(0, new_width, new_height, 32, 0, 0, 0, 0);
    if (!new_canvas) {
        return;
    }

    SDL_BlitScaled(*canvas, NULL, new_canvas, NULL);

    SDL_Surface *new_cursor_preview = SDL_CreateRGBSurface(0, new_width, new_height, 32, 0, 0, 0, 0);
    if (!new_cursor_preview) {
        SDL_FreeSurface(new_canvas);
        return;
    }

    SDL_FreeSurface(*canvas);
    SDL_FreeSurface(*cursor_preview);

    *canvas = new_canvas;
    *cursor_preview = new_cursor_preview;
    *window_surface = SDL_GetWindowSurface(window);
}

static bool load_image_into_canvas(SDL_Surface *canvas, const char *path) {
    SDL_Surface *loaded = IMG_Load(path);
    if (!loaded) {
        printf("Open failed (%s): %s\n", path, IMG_GetError());
        return false;
    }

    SDL_BlitScaled(loaded, NULL, canvas, NULL);
    SDL_FreeSurface(loaded);
    printf("Opened %s\n", path);
    return true;
}

static void clear_undo_stack(SDL_Surface *undo_stack[], int *undo_top) {
    for (int i = 0; i < *undo_top; i++) {
        SDL_FreeSurface(undo_stack[i]);
        undo_stack[i] = NULL;
    }
    *undo_top = 0;
}

static bool ensure_directory_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }

    if (mkdir(path, 0755) == 0) {
        return true;
    }

    printf("Directory error (%s): %s\n", path, strerror(errno));
    return false;
}

static bool backup_and_replace_file(const char *final_path, const char *temp_path) {
    char backup_path[MAX_PATH_LEN];
    int written = snprintf(backup_path, sizeof(backup_path), "%s.bak", final_path);
    if (written < 0 || (size_t)written >= sizeof(backup_path)) {
        printf("Backup path too long\n");
        unlink(temp_path);
        return false;
    }

    if (access(final_path, F_OK) == 0) {
        unlink(backup_path);
        if (rename(final_path, backup_path) != 0) {
            printf("Backup failed (%s): %s\n", backup_path, strerror(errno));
            unlink(temp_path);
            return false;
        }
    }

    if (rename(temp_path, final_path) != 0) {
        printf("Finalize save failed (%s): %s\n", final_path, strerror(errno));
        unlink(temp_path);
        if (access(backup_path, F_OK) == 0) {
            rename(backup_path, final_path);
        }
        return false;
    }

    return true;
}

static bool save_canvas_atomic(SDL_Surface *canvas, const char *final_path) {
    char temp_path[MAX_PATH_LEN];
    int written = snprintf(temp_path, sizeof(temp_path), "%s.tmp", final_path);
    if (written < 0 || (size_t)written >= sizeof(temp_path)) {
        printf("Temp path too long\n");
        return false;
    }

    if (SDL_SaveBMP(canvas, temp_path) != 0) {
        printf("Save failed (%s): %s\n", temp_path, SDL_GetError());
        unlink(temp_path);
        return false;
    }

    if (!backup_and_replace_file(final_path, temp_path)) {
        return false;
    }

    printf("Saved %s\n", final_path);
    return true;
}

static bool choose_file_path(bool save_mode, char *out_path, size_t out_path_len) {
    const char *command = save_mode
        ? "zenity --file-selection --save --confirm-overwrite --filename=save/ 2>/dev/null"
        : "zenity --file-selection --filename=save/ 2>/dev/null";

    FILE *pipe = popen(command, "r");
    if (!pipe) {
        printf("File picker unavailable. Install zenity for Open/Save As dialogs.\n");
        return false;
    }

    if (!fgets(out_path, (int)out_path_len, pipe)) {
        pclose(pipe);
        return false;
    }

    int status = pclose(pipe);
    if (status != 0) {
        return false;
    }

    size_t len = strlen(out_path);
    while (len > 0 && (out_path[len - 1] == '\n' || out_path[len - 1] == '\r')) {
        out_path[len - 1] = '\0';
        len--;
    }

    return len > 0;
}

int main(int argc, char *argv[]){
    bool done = false;
    SDL_Init(SDL_INIT_VIDEO);

    int img_flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP;
    int img_initted = IMG_Init(img_flags);
    if ((img_initted & img_flags) == 0) {
        printf("IMG_Init warning: %s\n", IMG_GetError());
    }

    SDL_Window* window = SDL_CreateWindow(
       "paint@home",
       SDL_WINDOWPOS_CENTERED_DISPLAY(0),
       SDL_WINDOWPOS_CENTERED_DISPLAY(0),
       WIDTH,
       HEIGHT,
       SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    SDL_Surface *window_surface = SDL_GetWindowSurface(window);
    SDL_Surface *canvas = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0, 0, 0, 0);
    SDL_Surface *cursor_preview = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0, 0, 0, 0);

    // Fill both with background color
    SDL_FillRect(canvas, NULL, color_palette[BLACK]);
    SDL_FillRect(cursor_preview, NULL, color_palette[BLACK]);
    
    float delay = (1.0 / TARGET_FPS) * 1000; //time in milliseconds

    SDL_Surface *undo_stack[MAX_UNDO];
    int undo_top = 0;
    bool draw = false;
    bool file_menu_open = false;
    bool is_dirty = false;
    int mx, my, last_x, last_y, drawSize = 10;
    Color current_color = BRIGHT_RED;
    char current_file_path[MAX_PATH_LEN];
    char original_file_path[MAX_PATH_LEN];

    if (!ensure_directory_exists(SAVE_DIR)) {
        done = true;
    }

    snprintf(current_file_path, sizeof(current_file_path), "%s", DEFAULT_BMP_PATH);
    snprintf(original_file_path, sizeof(original_file_path), "%s", DEFAULT_BMP_PATH);

    if (argc > 1) {
        if (load_image_into_canvas(canvas, argv[1])) {
            snprintf(current_file_path, sizeof(current_file_path), "%s", argv[1]);
            snprintf(original_file_path, sizeof(original_file_path), "%s", argv[1]);
        }
    }

    if (argc > 2) {
        snprintf(current_file_path, sizeof(current_file_path), "%s", argv[2]);
    }

    while(!done){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)){
                done = true;
            }

            if(event.type == SDL_WINDOWEVENT &&
            (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)){
                resize_surfaces(
                    window,
                    &window_surface,
                    &canvas,
                    &cursor_preview,
                    event.window.data1,
                    event.window.data2
                );
            }

            if(event.type == SDL_MOUSEMOTION){
                updateXY(&mx, &my, &event.motion.x, &event.motion.y);
                
                if (draw) {
                    int dx = mx - last_x;
                    int dy = my - last_y;
                    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
                    if (steps == 0) steps = 1;

                    for (int i = 0; i <= steps; i++) {
                        float t = (float)i / (float)steps;
                        int x = last_x + (int)(t * dx);
                        int y = last_y + (int)(t * dy);
                        draw_circle(x, y, drawSize, canvas, current_color);
                        is_dirty = true;
                    }

                    last_x = mx;
                    last_y = my;
                }
            }

            if(event.type == SDL_MOUSEBUTTONDOWN){
                if(event.button.button == SDL_BUTTON_LEFT){
                    bool clicked_file_button = false;
                    MenuAction menu_action = file_menu_hit_test(
                        event.button.x,
                        event.button.y,
                        file_menu_open,
                        &clicked_file_button
                    );

                    if (clicked_file_button) {
                        file_menu_open = !file_menu_open;
                        continue;
                    }

                    if (menu_action != MENU_ACTION_NONE) {
                        file_menu_open = false;
                        if (menu_action == MENU_ACTION_SAVE) {
                            if (save_canvas_atomic(canvas, current_file_path)) {
                                is_dirty = false;
                            }
                        } else if (menu_action == MENU_ACTION_SAVE_AS) {
                            char save_as_path[MAX_PATH_LEN];
                            if (choose_file_path(true, save_as_path, sizeof(save_as_path))) {
                                snprintf(current_file_path, sizeof(current_file_path), "%s", save_as_path);
                                if (save_canvas_atomic(canvas, current_file_path)) {
                                    is_dirty = false;
                                }
                            } else {
                                printf("Save As cancelled\n");
                            }
                        } else if (menu_action == MENU_ACTION_OPEN) {
                            char open_path[MAX_PATH_LEN];
                            if (choose_file_path(false, open_path, sizeof(open_path))) {
                                push_undo(undo_stack, &undo_top, (int)MAX_UNDO, canvas);
                                if (!load_image_into_canvas(canvas, open_path)) {
                                    pop_undo(undo_stack, &undo_top, canvas);
                                } else {
                                    snprintf(current_file_path, sizeof(current_file_path), "%s", open_path);
                                    is_dirty = false;
                                }
                            }
                        } else if (menu_action == MENU_ACTION_REOPEN) {
                            push_undo(undo_stack, &undo_top, (int)MAX_UNDO, canvas);
                            if (!load_image_into_canvas(canvas, original_file_path)) {
                                pop_undo(undo_stack, &undo_top, canvas);
                            } else {
                                snprintf(current_file_path, sizeof(current_file_path), "%s", original_file_path);
                                is_dirty = false;
                            }
                        }
                        continue;
                    }

                    if (file_menu_open) {
                        file_menu_open = false;
                    }

                    int hit = palette_hit_test(
                        event.button.x,
                        event.button.y,
                        PALETTE_X,
                        PALETTE_Y,
                        PALETTE_BOX_W,
                        PALETTE_BOX_H,
                        PALETTE_GAP,
                        PALETTE_COLUMNS
                    );
                    if(hit != -1)
                        current_color = (Color)hit;
                    else{
                        push_undo(undo_stack, &undo_top, (int)MAX_UNDO, canvas);
                        updateXY(&mx, &my, &event.button.x, &event.button.y);
                        draw = true;
                        last_x = event.button.x;
                        last_y = event.button.y;
                        draw_circle(last_x, last_y, drawSize, canvas, current_color); // initial stamp
                        is_dirty = true;
                    }
                }
                if(event.button.button == SDL_BUTTON_RIGHT) current_color = (current_color + 1) % COLOR_COUNT;

            }
            if(event.type == SDL_MOUSEBUTTONUP) draw = false;

            if(event.type == SDL_MOUSEWHEEL){
                drawSize = drawSize + event.wheel.y <= 50 ? drawSize + event.wheel.y : 50;
                drawSize = drawSize < 1 ? 1 : drawSize;
                printf("Draw Size: %d\n", drawSize);
            }

            if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s && (event.key.keysym.mod & KMOD_CTRL)){
                if((event.key.keysym.mod & KMOD_SHIFT) != 0){
                    char save_as_path[MAX_PATH_LEN];
                    if (choose_file_path(true, save_as_path, sizeof(save_as_path))) {
                        snprintf(current_file_path, sizeof(current_file_path), "%s", save_as_path);
                        if (save_canvas_atomic(canvas, current_file_path)) {
                            is_dirty = false;
                        }
                    } else {
                        printf("Save As cancelled\n");
                    }
                } else if (save_canvas_atomic(canvas, current_file_path)) {
                    is_dirty = false;
                }
            }

            if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_o && (event.key.keysym.mod & KMOD_CTRL)){
                push_undo(undo_stack, &undo_top, (int)MAX_UNDO, canvas);
                if (!load_image_into_canvas(canvas, original_file_path)) {
                    pop_undo(undo_stack, &undo_top, canvas);
                } else {
                    snprintf(current_file_path, sizeof(current_file_path), "%s", original_file_path);
                    is_dirty = false;
                }
            }

            if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_z && (event.key.keysym.mod & KMOD_CTRL)){
                pop_undo(undo_stack, &undo_top, canvas);
                is_dirty = true;
            }
        }

        if(draw){
            draw_circle(mx, my, drawSize, canvas, current_color);
        }

        SDL_BlitSurface(canvas, NULL, cursor_preview, NULL);

        draw_palette(
            cursor_preview,
            PALETTE_X,
            PALETTE_Y,
            PALETTE_BOX_W,
            PALETTE_BOX_H,
            PALETTE_GAP,
            PALETTE_COLUMNS,
            current_color
        );

        draw_file_menu(cursor_preview, file_menu_open);

        draw_circle(mx, my, drawSize, cursor_preview, current_color);

        SDL_BlitSurface(cursor_preview, NULL, window_surface, NULL);
        SDL_UpdateWindowSurface(window);

        SDL_Delay(delay);
    }

    clear_undo_stack(undo_stack, &undo_top);

    if (is_dirty) {
        printf("Warning: unsaved changes in %s\n", current_file_path);
    }

    SDL_FreeSurface(canvas);
    SDL_FreeSurface(cursor_preview);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
