#include <SDL2/SDL.h>

#include <iostream>
#include <random>


const int MAX_FPS = 60;
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int FILL_PERCENT = 10;

const int CELLS_COUNT = SCREEN_HEIGHT * SCREEN_WIDTH;


class Field {
    private:
        uint8_t* pixels;

        int loop(int value, int limit) {
            if (value > limit) return value - limit;
            if (value < 0) return value + limit;
            return value;
        }

    public:
        Field() {
            pixels = (uint8_t*)malloc(CELLS_COUNT * sizeof(uint8_t));
            fill_field();
        }

        ~Field() {
            free(pixels);
        }

        void fill_field() {
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution<std::mt19937::result_type> dist(1, 100);

            for (int i = 0; i < CELLS_COUNT; i++) pixels[i] = (int)(dist(rng) < FILL_PERCENT);
        }

        void step() {
            uint8_t* backbuffer = (uint8_t*)calloc(CELLS_COUNT, sizeof(uint8_t));

            for (int i = 0; i < CELLS_COUNT; i++) {
                int y = i / SCREEN_WIDTH,
                    x = i - y * SCREEN_WIDTH;

                int alive = 0;

                for (int j = 0; j < 9; j++) {
                    int dx = j / 3 - 1, dy = j % 3 - 1;
                    if (dx == 0 && dy == 0) continue;

                    int cx = loop(x + dx, SCREEN_WIDTH),
                        cy = loop(y + dy, SCREEN_HEIGHT);

                    alive += pixels[cy * SCREEN_WIDTH + cx];
                }

                if ((pixels[i] && (alive == 2 || alive == 3)) || (!(pixels[i]) && (alive == 3))) backbuffer[i] = 1;
            }

            std::swap(pixels, backbuffer);
            free(backbuffer);
        }

        uint8_t get_pixel(int i) {
            return pixels[i];
        }
};


class UI {
    private:
        SDL_Event event;
        SDL_Renderer* renderer;
        SDL_Window* window;
        SDL_Texture* texture;
        uint32_t* pixels;

        Field* field;

        bool running;
        bool is_stopped;

        void draw() {
            for (int i = 0; i < CELLS_COUNT; i++) pixels[i] = field->get_pixel(i) ? 0xFF00FF00 : 0x00000000;
        }

        void handle_controls() {
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_ESCAPE: running = false; break;
                        case SDLK_SPACE: field->fill_field(); break;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    switch(event.button.button) {
                        case SDL_BUTTON_LEFT: is_stopped = !is_stopped; break;
                        case SDL_BUTTON_RIGHT: field->fill_field(); break;
                    }
                    break;
                case SDL_QUIT:
                    running = false;
                    break;
                default:
                    break;
                }
            }
        }

    public:
        UI(Field* field) {
            SDL_Init(SDL_INIT_VIDEO);
            SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);

            running = true;
            is_stopped = false;

            this->field = field;

            pixels = (uint32_t*)malloc(CELLS_COUNT * sizeof(uint32_t));
        }

        ~UI() {
            SDL_DestroyTexture(texture);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);

            free(pixels);

            SDL_Quit();
        }

        void run() {
            unsigned int time_curr = 0, time_prev = 0;

            while (running) {
                time_curr = SDL_GetTicks();
                unsigned int time_delta = time_curr - time_prev;

                handle_controls();

                if (time_delta > 1000.0 / MAX_FPS) {
                    draw();

                    SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));

                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, texture, NULL, NULL);
                    SDL_RenderPresent(renderer);

                    if (!is_stopped) field->step();
                    time_prev = time_curr;
                }
            }
        }
};


int main() {
    Field field;
    UI ui(&field);

    ui.run();

    return EXIT_SUCCESS;
}
