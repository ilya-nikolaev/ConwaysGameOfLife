#include <SDL2/SDL.h>

#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define THREADS_COUNT 12
#define RULE_SIZE 9

struct Point
{
    size_t x;
    size_t y;
};

struct Field
{
    uint8_t *cells;
    uint8_t *backbuffer;

    size_t width;
    size_t height;
    size_t count;

    uint8_t fillingPercentage;

    int8_t *B;
    int8_t *S;
};

struct UI
{
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Texture *texture;

    uint32_t *pixels;
    struct Field *field;

    uint8_t maxFPS;

    uint8_t isRunning;
    uint8_t isPaused;

    uint8_t leftMouseButtonPressed;
    uint8_t rightMouseButtonPressed;

    uint32_t primaryColor;
    uint32_t secondaryColor;
};

struct Point mapToTorus(struct Point orig, size_t width, size_t height)
{
    struct Point result = {
        (orig.x + width) % width,
        (orig.y + height) % height};

    return result;
}

uint8_t findRule(int8_t *rule, int8_t value)
{
    for (int i = 0; i < RULE_SIZE; ++i)
    {
        if (rule[i] == -1)
            break;
        if (rule[i] == value)
            return 1;
    }

    return 0;
}

void fillField(struct Field *field)
{
    for (size_t i = 0; i < field->count; ++i)
    {
        uint8_t value = rand() % 100;
        field->cells[i] = value <= field->fillingPercentage ? 1 : 0;
    }
}

void clearField(struct Field *field)
{
    for (size_t i = 0; i < field->count; ++i)
        field->cells[i] = 0;
}

struct TickArg
{
    struct Field *field;
    size_t start;
    size_t end;
};

void *processTick(void *arg)
{
    struct TickArg args = *(struct TickArg *)arg;

    for (size_t i = args.start; i < args.end; ++i)
    {
        struct Point cellPoint = {i % args.field->width, i / args.field->width};
        uint8_t isBorder = !(cellPoint.x && cellPoint.y) ||
                           cellPoint.x == args.field->width - 1 ||
                           cellPoint.y == args.field->height - 1;

        uint8_t alive = 0;
        for (uint8_t j = 0; j < 9; ++j)
        {
            if (j == 4)
                continue; // Cell itself

            struct Point subCellPoint = {cellPoint.x + (j / 3) - 1, cellPoint.y + (j % 3) - 1};
            struct Point pointOnTorus = subCellPoint;
            if (isBorder)
                pointOnTorus = mapToTorus(subCellPoint, args.field->width, args.field->height);

            alive += args.field->cells[pointOnTorus.x + pointOnTorus.y * args.field->width];
        }

        if ((args.field->cells[i] && findRule(args.field->S, alive)) ||
            ((!args.field->cells[i]) && findRule(args.field->B, alive)))
            args.field->backbuffer[i] = 1;
        else
            args.field->backbuffer[i] = 0;
    }

    free(arg);
    return NULL;
}

void swapBuffers(struct Field *field)
{
    uint8_t *tmp_ptr = field->cells;
    field->cells = field->backbuffer;
    field->backbuffer = tmp_ptr;
}

struct DrawArg
{
    struct UI *ui;
    size_t start;
    size_t end;
};

void *drawTexture(void *arg)
{
    struct DrawArg args = *(struct DrawArg *)arg;

    for (size_t i = args.start; i < args.end; ++i)
    {
        args.ui->pixels[i] = args.ui->field->cells[i] ? args.ui->primaryColor : args.ui->secondaryColor;
    }

    free(arg);
    return NULL;
}

void handleControls(struct UI *ui)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE:
                ui->isRunning = 0;
                break;
            case SDLK_SPACE:
                ui->isPaused = !ui->isPaused;
                break;
            case SDLK_r:
                fillField(ui->field);
                break;
            case SDLK_c:
                clearField(ui->field);
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                ui->leftMouseButtonPressed = 1;
                ui->field->cells[event.button.x + event.button.y * ui->field->width] = 1;
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                ui->rightMouseButtonPressed = 1;
                ui->field->cells[event.button.x + event.button.y * ui->field->width] = 0;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT)
                ui->leftMouseButtonPressed = 0;
            else if (event.button.button == SDL_BUTTON_RIGHT)
                ui->rightMouseButtonPressed = 0;
            break;
        case SDL_MOUSEMOTION:
            if (ui->leftMouseButtonPressed)
                ui->field->cells[event.motion.x + event.motion.y * ui->field->width] = 1;
            else if (ui->rightMouseButtonPressed)
                ui->field->cells[event.motion.x + event.motion.y * ui->field->width] = 0;
            break;
        case SDL_QUIT:
            ui->isRunning = 0;
            break;
        default:
            break;
        }
    }
}

int CGOL_sleep(long ms)
{
    if (ms <= 0)
        return 0;

    struct timespec req = {
        (int)(ms / 1000),
        (ms % 1000) * 1000000};

    return nanosleep(&req, NULL);
}

void run(struct UI *ui)
{
    uint32_t timePrev = 0, timeCurr, timeDelta;
    double maxDelay = 1000.0 / ui->maxFPS;

    pthread_t threads[THREADS_COUNT];
    size_t cellsPerThread = ui->field->count / THREADS_COUNT;

    while (ui->isRunning)
    {
        timeCurr = SDL_GetTicks();
        timeDelta = timeCurr - timePrev;
        CGOL_sleep(maxDelay - timeDelta);

        handleControls(ui);

        for (int i = 0; i < THREADS_COUNT; ++i)
        {
            struct DrawArg *arg = malloc(sizeof(struct DrawArg));

            struct DrawArg myArg = {
                .ui = ui,
                .start = i * cellsPerThread,
                .end = (i + 1) * cellsPerThread,
            };

            *arg = myArg;

            if (pthread_create(&threads[i], NULL, &drawTexture, (void *)arg) != 0)
                perror("Failed");
        }

        for (int i = 0; i < THREADS_COUNT; ++i)
            if (pthread_join(threads[i], NULL) != 0)
                perror("Failed");

        SDL_UpdateTexture(ui->texture, NULL, ui->pixels, ui->field->width * sizeof(uint32_t));

        SDL_RenderCopy(ui->renderer, ui->texture, NULL, NULL);
        SDL_RenderPresent(ui->renderer);

        if (!(ui->isPaused))
        {
            for (int i = 0; i < THREADS_COUNT; ++i)
            {
                struct TickArg *arg = malloc(sizeof(struct TickArg));

                struct TickArg myArg = {
                    .field = ui->field,
                    .start = i * cellsPerThread,
                    .end = (i + 1) * cellsPerThread,
                };

                *arg = myArg;

                if (pthread_create(&threads[i], NULL, &processTick, (void *)arg) != 0)
                    perror("Failed");
            }

            for (int i = 0; i < THREADS_COUNT; ++i)
                if (pthread_join(threads[i], NULL) != 0)
                    perror("Failed");

            swapBuffers(ui->field);
        }

        timePrev = timeCurr;
    }
}

void parseRules(int8_t ruleSet[RULE_SIZE])
{
    int c = 0;
    for (int i = 0; i < RULE_SIZE; ++i)
    {
        char e = optarg[i];
        if (!e || e < '0' || e > '8')
            break;
        ruleSet[c] = e - '0';
        c++;
    }
}

int8_t getHexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');

    return -1; // Not hex digit
}

uint32_t parseColor()
{
    uint32_t color = 0xFF000000;

    for (int i = 0; i < 6; ++i)
    {
        char c = optarg[i];
        if (!c)
            break;

        int8_t hexValue = getHexValue(c);
        if (hexValue == -1)
            return 0; // Error, returning black

        color |= (uint32_t)hexValue << ((5 - i) * 4);
    }

    return color;
}

int main(int argc, char *argv[])
{
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
    while ((rez = getopt(argc, argv, "w:h:B:S:p:nf:c:b:")) != -1)
    {
        switch (rez)
        {
        case 'w':
            width = (size_t)atoi(optarg);
            break;
        case 'h':
            height = (size_t)atoi(optarg);
            break;
        case 'n':
            fullscreenEnabled = 0;
            break;
        case 'B':
            B[0] = -1; // Drop default settings
            parseRules(B);
            break;
        case 'S':
            S[0] = -1;
            S[1] = -1; // Drop default settings
            parseRules(S);
            break;
        case 'p':
            fillingPercentage = (uint8_t)atoi(optarg);
            break;
        case 'f':
            maxFPS = (uint8_t)atoi(optarg);
            break;
        case 'c':
            primary = parseColor();
            break;
        case 'b':
            secondary = parseColor();
            break;
        default:
            break;
        }
    }

    struct Field field = {
        .width = width,
        .height = height,
        .count = width * height,

        .fillingPercentage = fillingPercentage,

        .B = B,
        .S = S,
    };

    field.cells = malloc(field.count * sizeof(uint8_t));
    field.backbuffer = malloc(field.count * sizeof(uint8_t));

    fillField(&field);

    struct UI ui = {
        .field = &field,

        .maxFPS = maxFPS,

        .primaryColor = primary,
        .secondaryColor = secondary,

        .isRunning = 1,
        .isPaused = 0,

        .leftMouseButtonPressed = 0,
        .rightMouseButtonPressed = 0,
    };

    ui.pixels = malloc(field.count * sizeof(uint32_t));

    SDL_CreateWindowAndRenderer(width, height, 0, &ui.window, &ui.renderer);

    if (fullscreenEnabled)
        SDL_SetWindowFullscreen(ui.window, SDL_WINDOW_FULLSCREEN);
    else
        SDL_SetWindowFullscreen(ui.window, SDL_WINDOW_MAXIMIZED);

    ui.texture = SDL_CreateTexture(ui.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);

    run(&ui);

    SDL_DestroyTexture(ui.texture);
    SDL_DestroyRenderer(ui.renderer);
    SDL_DestroyWindow(ui.window);

    free(field.cells);
    free(field.backbuffer);
    free(ui.pixels);

    SDL_Quit();
    return EXIT_SUCCESS;
}
