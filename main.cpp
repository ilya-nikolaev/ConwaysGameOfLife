#include <SDL2/SDL.h>
#include <unistd.h>

#include <random>


const int MAX_FPS = 60;
const int RULE_LENGTH = 9;


class Field {
    private:
        uint8_t* field;
        int x, y, p, c;

        int8_t *B, *S;

        int loop(int value, int limit) {
            if (value > limit) return value - limit;
            if (value < 0) return value + limit;
            return value;
        }

        int find(int8_t* container, int8_t value) {
            for (int i = 0; i < RULE_LENGTH; ++i) {
                if (container[i] == -1) break;
                if (container[i] == value) return 1;
            }

            return 0;
        }

    public:
        Field(int x_, int y_, int p_, int8_t B_[9], int8_t S_[9]) {
            x = x_; y = y_; p = p_;
            c = x * y;
            B = B_; S = S_;

            field = (uint8_t*)malloc(c * sizeof(uint8_t));

            fill_field();
        }

        ~Field() {
            free(field);
        }

        void fill_field() {
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution<std::mt19937::result_type> dist(1, 100);

            for (int i = 0; i < c; i++) field[i] = (uint8_t)((uint32_t)dist(rng) < (uint32_t)p);
        }

        void clear_field() {
            for (int i = 0; i < c; i++) field[i] = 0;
        }

        void step() {
            uint8_t* backbuffer = (uint8_t*)calloc(c, sizeof(uint8_t));

            for (int i = 0; i < c; i++) {
                int iy = i / x, ix = i - iy * x;

                int8_t alive = 0;

                for (int j = 0; j < 9; j++) {
                    int jx = j / 3 - 1, jy = j % 3 - 1;
                    if (jx == 0 && jy == 0) continue;

                    int cx = loop(ix + jx, x),
                        cy = loop(iy + jy, y);

                    alive += field[cy * x + cx];
                }

                if ((field[i] && find(S, alive)) || (!(field[i]) && find(B, alive))) 
                    backbuffer[i] = 1;
            }

            std::swap(field, backbuffer);
            free(backbuffer);
        }

        uint8_t get_cell(int i) {
            return field[i];
        }

        void set_cell(int i, uint8_t value) {
            field[i] = value;
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
        int x, y, c;

        bool running;
        bool is_stopped;
        bool lmbDown = false;

        void draw() {
            for (int i = 0; i < c; i++) pixels[i] = field->get_cell(i) ? 0xFF00FF00 : 0x00000000;
        }

        void handle_controls() {
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_ESCAPE: running = false; break;
                        case SDLK_SPACE: is_stopped = !is_stopped; break;
                        case SDLK_r: field->fill_field(); break;
                        case SDLK_c: field->clear_field(); break;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) lmbDown = true;
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) lmbDown = false;
                    break;
                case SDL_MOUSEMOTION:
                    if (lmbDown) {
                        field->set_cell(event.motion.x + event.motion.y * x, 1);
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
        UI(Field* field_, int x_, int y_, int f_) {
            x = x_; y = y_; c = x * y;
            SDL_CreateWindowAndRenderer(x, y, 0, &window, &renderer);
            if (f_) SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
            else SDL_SetWindowFullscreen(window, SDL_WINDOW_MAXIMIZED);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, x, y);

            running = true;
            is_stopped = false;

            field = field_;

            pixels = (uint32_t*)malloc(c * sizeof(uint32_t));
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

                    SDL_UpdateTexture(texture, NULL, pixels, x * sizeof(uint32_t));

                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, texture, NULL, NULL);
                    SDL_RenderPresent(renderer);

                    if (!is_stopped) field->step();
                    time_prev = time_curr;
                }
            }
        }
};


int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_DisplayMode DM;
    SDL_GetDesktopDisplayMode(0, &DM);

    int width = DM.w, height = DM.h, p = 10;

    int8_t B[RULE_LENGTH] = {3, -1, -1, -1, -1, -1, -1, -1, -1};
    int8_t S[RULE_LENGTH] = {2, 3, -1, -1, -1, -1, -1, -1, -1};

    int fullscreen = 1;

    int rez = 0, c;
    while ((rez = getopt(argc, argv, "w:h:B:S:p:n")) != -1) {
        switch (rez) {
            case 'w': width = atoi(optarg); break;
            case 'h': height = atoi(optarg); break;
            case 'n': fullscreen = 0; break;
            case 'B': 
                c = 0;
                B[0] = -1;
                for (int i = 0; i < RULE_LENGTH; ++i) {
                    int e = optarg[i];
                    if (!e || e < '0' || e > '8') break;
                    B[c] = e - '0'; c++;
                } break;
            case 'S': 
                c = 0; S[0] = -1; S[1] = -1;
                for (int i = 0; i < RULE_LENGTH; ++i) {
                    int e = optarg[i];
                    if (!e || e < '0' || e > '8') break;
                    S[c] = e - '0'; c++;
                } break;
            case 'p': p = atoi(optarg); break;
            default: break;
        }
    }

    Field field(width, height, p, B, S);
    UI ui(&field, width, height, fullscreen);

    ui.run();

    return EXIT_SUCCESS;
}
