#include <SDL2/SDL.h>

#include <random>


const int MAX_FPS = 30;
const int SCREEN_HEIGHT = 1080;
const int SCREEN_WIDTH = 1920;

const int CELLS_COUNT = SCREEN_HEIGHT * SCREEN_WIDTH;


void fill_field(bool (&)[CELLS_COUNT]);
void step(bool (&)[CELLS_COUNT]);
void draw(bool (&)[CELLS_COUNT], SDL_Renderer*);
int loop(int, int);


int main() {
	SDL_Event event;
	SDL_Renderer *renderer;
	SDL_Window *window;

	bool field[CELLS_COUNT];
	fill_field(field);

	bool is_stopped = false;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
	SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

	unsigned int time_prev = 0;
	unsigned int time_curr = 0;

	bool running = true;
	while (running) {
		time_curr = SDL_GetTicks();
		unsigned int time_delta = time_curr - time_prev;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym) {
					case SDLK_ESCAPE: running = false; break;
					case SDLK_SPACE: fill_field(field); break;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				switch(event.button.button) {
					case SDL_BUTTON_LEFT: is_stopped = !is_stopped; break;
					case SDL_BUTTON_RIGHT: fill_field(field); break;
				}
				break;
			case SDL_QUIT:
				running = false;
				break;
			default:
				break;
			}
		}

		if (time_delta > 1000 / (float)MAX_FPS) {
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
			SDL_RenderClear(renderer);

			SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
			draw(field, renderer);

			if (!is_stopped) step(field);

			SDL_RenderPresent(renderer);

			time_prev = time_curr;
		}
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}


void fill_field(bool (&field)[CELLS_COUNT]) {
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(0, 1);

	for (int i = 0; i < CELLS_COUNT; i++) field[i] = dist(rng);
}


void step(bool (&field)[CELLS_COUNT]) {
	bool backbuffer[CELLS_COUNT];

	for (int i = 0; i < CELLS_COUNT; i++) {
		int y = i / SCREEN_WIDTH,
			x = i - y * SCREEN_WIDTH;

		int alive = 0;

		for (int j = 0; j < 9; j++) {
			int dx = j / 3 - 1, dy = j % 3 - 1;
			if (dx == 0 && dy == 0) continue;

			int cx = loop(x + dx, SCREEN_WIDTH),
				cy = loop(y + dy, SCREEN_HEIGHT);

			alive += field[cy * SCREEN_WIDTH + cx];
		}

		if (field[i]) {
			if (alive == 2 || alive == 3) backbuffer[i] = true;
			else backbuffer[i] = false;
		} else {
			if (alive == 3) backbuffer[i] = true;
			else backbuffer[i] = false;
		}
	}

	std::swap(field, backbuffer);
}


void draw(bool (&field)[CELLS_COUNT], SDL_Renderer* renderer) {
	for (int i = 0; i < CELLS_COUNT; i++) {
		int y = i / SCREEN_WIDTH,
			x = i - y * SCREEN_WIDTH;

		if (field[i]) {
			SDL_RenderDrawPoint(renderer, x, y);
		}
	}
}


int loop(int value, int limit) {
	if (value > limit) return value - limit;
	else if (value < 0) return value + limit;
	else return value;
}
