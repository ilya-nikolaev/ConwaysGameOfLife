#include <SDL2/SDL.h>

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define RULE_SIZE 9

#define getAttr(object, attr) ((*object).attr)
#define setAttr(object, attr, value) ((*object).attr = value)
#define setIndexAttr(object, attr, index, value) ((*object).attr[index] = value)


struct Point {
    size_t x;
    size_t y;
};


struct Field {
    uint8_t* cells;

    size_t width;
    size_t height;

    uint8_t fillingPercentage;

    int8_t* B;
    int8_t* S;
};


struct UI {
    SDL_Renderer* renderer;
    SDL_Window* window;
    SDL_Texture* texture;

    uint32_t* pixels;
    struct Field* field;

    uint8_t maxFPS;

    uint8_t isRunning;
    uint8_t isPaused;

    uint8_t leftMouseButtonPressed;
};


struct Point mapToTorus(struct Point orig, size_t width, size_t height) {
    struct Point result = {
        (orig.x + width) % width,
        (orig.y + height) % height
    };

    return result;
}


uint8_t findRule(int8_t* rule, int8_t value) {
    for (int i = 0; i < RULE_SIZE; ++i) {
        if (rule[i] == -1) break;
        if (rule[i] == value) return 1;
    }

    return 0;
}


void fillField(struct Field* field) {
    for (size_t i = 0; i < getAttr(field, width) * getAttr(field, height); ++i) {
        uint8_t value = rand() % 100;
        setIndexAttr(field, cells, i, value <= getAttr(field, fillingPercentage));
    }
}


void clearField(struct Field* field) {
    for (size_t i = 0; i < getAttr(field, width) * getAttr(field, height); ++i) 
        setIndexAttr(field, cells, i, 0);
}


void processTick(struct Field* field) {
    uint8_t* backbuffer = calloc(getAttr(field, width) * getAttr(field, height), sizeof(uint8_t));

    for (size_t i = 0; i < getAttr(field, width) * getAttr(field, height); ++i) {
        struct Point cellPoint = {i % getAttr(field, width), i / getAttr(field, width)};

        uint8_t alive = 0;
        for (uint8_t j = 0; j < 9; ++j) {
            if (j == 4) continue;  // cell itself
            struct Point subCellPoint = {cellPoint.x + (j / 3) - 1, cellPoint.y + (j % 3) - 1};
            struct Point pointOnTorus = mapToTorus(subCellPoint, getAttr(field, width), getAttr(field, height));
            alive += getAttr(field, cells)[pointOnTorus.x + pointOnTorus.y * getAttr(field, width)];
        }

        if ((getAttr(field, cells)[i] && findRule(getAttr(field, S), alive)) || 
            ((!getAttr(field, cells)[i]) && findRule(getAttr(field, B), alive))) 
                backbuffer[i] = 1;
    }

    free(getAttr(field, cells));
    setAttr(field, cells, backbuffer);
}


void drawTexture(struct UI* ui) {
    for (size_t i = 0; i < getAttr(getAttr(ui, field), height) * getAttr(getAttr(ui, field), width); ++i) {
        setIndexAttr(ui, pixels, i, getAttr(getAttr(ui, field), cells)[i] ? 0xFF00FF00 : 0x00000000);
    }
}


void handleControls(struct UI* ui) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE: setAttr(ui, isRunning, 0); break;
                    case SDLK_SPACE: setAttr(ui, isPaused, !getAttr(ui, isPaused)); break;
                    case SDLK_r: fillField(getAttr(ui, field)); break;
                    case SDLK_c: clearField(getAttr(ui, field)); break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) setAttr(ui, leftMouseButtonPressed, 1); 
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) setAttr(ui, leftMouseButtonPressed, 0); 
                break;
            case SDL_MOUSEMOTION:
                if (getAttr(ui, leftMouseButtonPressed)) 
                    setIndexAttr(getAttr(ui, field), cells, event.motion.x + event.motion.y * getAttr(getAttr(ui, field), width), 1); 
                break;
            case SDL_QUIT:
                setAttr(ui, isRunning, 0); break;
            default: break;
        }
    }
}


void run(struct UI* ui) {
    uint32_t time_curr = 0, time_prev = 0;
    uint32_t time_delta;

    while (getAttr(ui, isRunning)) {
        time_curr = SDL_GetTicks();
        time_delta = time_curr - time_prev;

        handleControls(ui);

        if (time_delta >= (double)1000 / getAttr(ui, maxFPS)) {
            drawTexture(ui);

            SDL_UpdateTexture(getAttr(ui, texture), NULL, getAttr(ui, pixels), getAttr(getAttr(ui, field), width) * sizeof(uint32_t));

            SDL_RenderClear(getAttr(ui, renderer));
            SDL_RenderCopy(getAttr(ui, renderer), getAttr(ui, texture), NULL, NULL);
            SDL_RenderPresent(getAttr(ui, renderer));

            if (!getAttr(ui, isPaused)) processTick(getAttr(ui, field));
            
            time_prev = time_curr;
        }
    }
}


int main(int argc, char* argv[]) {
    srand(time(NULL));

    SDL_Init(SDL_INIT_VIDEO);

    SDL_DisplayMode DM;
    SDL_GetDesktopDisplayMode(0, &DM);

    size_t width = DM.w, height = DM.h;
    uint8_t fillingPercentage = 10, maxFPS = 60, fullscreenEnabled = 1;

    int8_t B[] = {3, -1, -1, -1, -1, -1, -1, -1, -1};
    int8_t S[] = {2, 3, -1, -1, -1, -1, -1, -1, -1};

    int rez, c;
    while ((rez = getopt(argc, argv, "w:h:B:S:p:nf:")) != -1) {
        switch (rez) {
            case 'w': width = (size_t)atoi(optarg); break;
            case 'h': height = (size_t)atoi(optarg); break;
            case 'n': fullscreenEnabled = 0; break;
            case 'B': 
                c = 0;
                B[0] = -1;  // Drop default settings
                for (int i = 0; i < RULE_SIZE; ++i) {
                    int e = optarg[i];
                    if (!e || e < '0' || e > '8') break;
                    B[c] = e - '0'; c++;
                } break;
            case 'S': 
                c = 0; 
                S[0] = -1; S[1] = -1;  // Drop default settings
                for (int i = 0; i < RULE_SIZE; ++i) {
                    int e = optarg[i];
                    if (!e || e < '0' || e > '8') break;
                    S[c] = e - '0'; c++;
                } break;
            case 'p': fillingPercentage = (uint8_t)atoi(optarg); break;
            case 'f': maxFPS = (uint8_t)atoi(optarg); break;
            default: break;
        }
    }

    struct Field field;
    field.cells = malloc(width * height * sizeof(uint8_t));
    field.width = width; field.height = height;
    field.fillingPercentage = fillingPercentage;
    field.B = B; field.S = S;

    struct UI ui;
    SDL_CreateWindowAndRenderer(width, height, 0, &ui.window, &ui.renderer);
    if (fullscreenEnabled) SDL_SetWindowFullscreen(ui.window, SDL_WINDOW_FULLSCREEN);
    else SDL_SetWindowFullscreen(ui.window, SDL_WINDOW_MAXIMIZED);
    ui.texture = SDL_CreateTexture(ui.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
    ui.maxFPS = maxFPS;
    ui.isRunning = 1; ui.isPaused = 0;
    ui.field = &field;
    ui.pixels = malloc(width * height * sizeof(uint32_t));

    fillField(&field);
    run(&ui);

    free(field.cells);

    SDL_DestroyTexture(ui.texture);
    SDL_DestroyRenderer(ui.renderer);
    SDL_DestroyWindow(ui.window);
    free(ui.pixels);

    SDL_Quit();
    return EXIT_SUCCESS;
}