#include <SDL2/SDL.h>

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define RULE_SIZE 9

double FPS = 0;


struct Point {
    size_t x;
    size_t y;
};


struct Field {
    uint8_t* cells;
    uint8_t* backbuffer;

    size_t width;
    size_t height;
    size_t count;

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
    uint8_t rightMouseButtonPressed;

    uint32_t primaryColor;
    uint32_t secondaryColor;
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
    for (size_t i = 0; i < field->count; ++i) {
        uint8_t value = rand() % 100;
        field->cells[i] = value <= field->fillingPercentage ? 1 : 0;
    }
}


void clearField(struct Field* field) {
    for (size_t i = 0; i < field->count; ++i) 
        field->cells[i] = 0;
}


void processTick(struct Field* field) {
    memset(field->backbuffer, 0, field->count);

    for (size_t i = 0; i < field->count; ++i) {
        struct Point cellPoint = {i % field->width, i / field->width};

        uint8_t alive = 0;
        for (uint8_t j = 0; j < 9; ++j) {
            if (j == 4) continue;  // Cell itself

            struct Point subCellPoint = {cellPoint.x + (j / 3) - 1, cellPoint.y + (j % 3) - 1};

            struct Point pointOnTorus = subCellPoint;
            if (!(cellPoint.x && cellPoint.y) || cellPoint.x == field->width - 1 || cellPoint.y == field->height - 1) {
                pointOnTorus = mapToTorus(subCellPoint, field->width, field->height);
            }

            alive += field->cells[pointOnTorus.x + pointOnTorus.y * field->width];
        }

        if ((field->cells[i] && findRule(field->S, alive)) || 
            ((!field->cells[i]) && findRule(field->B, alive))) 
                field->backbuffer[i] = 1;
    }

    uint8_t* tmp_ptr = field->cells;
    field->cells = field->backbuffer;
    field->backbuffer = tmp_ptr;
}


void drawTexture(struct UI* ui) {
    for (size_t i = 0; i < ui->field->count; ++i) {
        ui->pixels[i] = ui->field->cells[i] ? ui->primaryColor : ui->secondaryColor;
    }
}


void handleControls(struct UI* ui) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE: ui->isRunning = 0; break;
                    case SDLK_SPACE: ui->isPaused = !ui->isPaused; break;
                    case SDLK_r: fillField(ui->field); break;
                    case SDLK_c: clearField(ui->field); break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    ui->leftMouseButtonPressed = 1; 
                    ui->field->cells[event.button.x + event.button.y * ui->field->width] = 1;
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    ui->rightMouseButtonPressed = 1; 
                    ui->field->cells[event.button.x + event.button.y * ui->field->width] = 0;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) ui->leftMouseButtonPressed = 0; 
                else if (event.button.button == SDL_BUTTON_RIGHT) ui->rightMouseButtonPressed= 0; 
                break;
            case SDL_MOUSEMOTION:
                if (ui->leftMouseButtonPressed) 
                    ui->field->cells[event.motion.x + event.motion.y * ui->field->width] = 1;
                else if (ui->rightMouseButtonPressed)
                    ui->field->cells[event.motion.x + event.motion.y * ui->field->width] = 0;
                break;
            case SDL_QUIT:
                ui->isRunning = 0; break;
            default: break;
        }
    }
}


void run(struct UI* ui) {
    uint32_t time_curr = 0, time_prev = 0;
    uint32_t time_delta;

    while (ui->isRunning) {
        time_curr = SDL_GetTicks();
        time_delta = time_curr - time_prev;

        handleControls(ui);

        if (time_delta >= (double)1000 / ui->maxFPS) {
            FPS = 1000.0 / (double)time_delta;
            drawTexture(ui);

            SDL_UpdateTexture(ui->texture, NULL, ui->pixels, ui->field->width * sizeof(uint32_t));

            SDL_RenderCopy(ui->renderer, ui->texture, NULL, NULL);
            SDL_RenderPresent(ui->renderer);

            if (!(ui->isPaused)) processTick(ui->field);
            
            time_prev = time_curr;
        }
    }
}


void parseRules(int8_t ruleSet[RULE_SIZE]) {
    int c = 0;
    for (int i = 0; i < RULE_SIZE; ++i) {
        char e = optarg[i];
        if (!e || e < '0' || e > '8') break;
        ruleSet[c] = e - '0'; c++;
    }
}


int8_t getHexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');

    return -1;  // Not hex digit
}


uint32_t parseColor() {
    uint32_t color = 0xFF000000;

    for (int i = 0; i < 6; ++i) {
        char c = optarg[i];
        if (!c) break;

        int8_t hexValue = getHexValue(c);
        if (hexValue == -1) return 0;  // Error, returning black

        color |= (uint32_t)hexValue << ((5 - i) * 4);
    }

    return color;
}


int main(int argc, char* argv[]) {
    srand(time(NULL));

    SDL_Init(SDL_INIT_VIDEO);

    SDL_DisplayMode DM;
    SDL_GetDesktopDisplayMode(0, &DM);

    size_t width = DM.w, height = DM.h;
    uint8_t fillingPercentage = 10, maxFPS = 60, fullscreenEnabled = 1;
    uint32_t primary = 0xFF00FF00, secondary = 0xFF000000;

    int8_t B[RULE_SIZE] = {3, -1, -1, -1, -1, -1, -1, -1, -1};
    int8_t S[RULE_SIZE] = {2, 3, -1, -1, -1, -1, -1, -1, -1};

    int rez;
    while ((rez = getopt(argc, argv, "w:h:B:S:p:nf:c:b:")) != -1) {
        switch (rez) {
            case 'w': width = (size_t)atoi(optarg); break;
            case 'h': height = (size_t)atoi(optarg); break;
            case 'n': fullscreenEnabled = 0; break;
            case 'B': 
                B[0] = -1;  // Drop default settings
                parseRules(B); break;
            case 'S':  
                S[0] = -1; S[1] = -1;  // Drop default settings
                parseRules(S); break;
            case 'p': fillingPercentage = (uint8_t)atoi(optarg); break;
            case 'f': maxFPS = (uint8_t)atoi(optarg); break;
            case 'c': primary = parseColor(); break;
            case 'b': secondary = parseColor(); break;
            default: break;
        }
    }

    struct Field field;

    field.width = width; 
    field.height = height;
    field.count = width * height;

    field.cells = malloc(field.count * sizeof(uint8_t));
    field.backbuffer= malloc(field.count * sizeof(uint8_t));

    field.fillingPercentage = fillingPercentage;

    field.B = B; 
    field.S = S;

    fillField(&field);

    struct UI ui;

    ui.pixels = malloc(field.count * sizeof(uint32_t));

    ui.field = &field;

    SDL_CreateWindowAndRenderer(width, height, 0, &ui.window, &ui.renderer);

    if (fullscreenEnabled) SDL_SetWindowFullscreen(ui.window, SDL_WINDOW_FULLSCREEN);
    else SDL_SetWindowFullscreen(ui.window, SDL_WINDOW_MAXIMIZED);

    ui.texture = SDL_CreateTexture(ui.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);

    ui.maxFPS = maxFPS;

    ui.primaryColor = primary;
    ui.secondaryColor = secondary;

    ui.isRunning = 1;
    ui.isPaused = 0;

    ui.leftMouseButtonPressed = 0;
    ui.rightMouseButtonPressed = 0;

    run(&ui);

    SDL_DestroyTexture(ui.texture);
    SDL_DestroyRenderer(ui.renderer);
    SDL_DestroyWindow(ui.window);

    free(field.cells);
    free(ui.pixels);

    printf("%f\n", FPS);

    SDL_Quit();
    return EXIT_SUCCESS;
}
