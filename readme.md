# Conway's Game of Life

## Description
The Game of Life, also known simply as Life, is a cellular automaton devised by the British mathematician John Horton Conway in 1970. It is a zero-player game, meaning that its evolution is determined by its initial state, requiring no further input. One interacts with the Game of Life by creating an initial configuration and observing how it evolves. It is Turing complete and can simulate a universal constructor or any other Turing machine.
[Wikipedia](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life)

## Implementation
This implementation is written in C99 using the standard library, unistd and the SDL2 library

## Console line parameters
+ `-w {int}` - Window width (your resolution by default)
+ `-h {int}` - Window height (your resolution by default)
+ `-n` - Run windowed
+ `-B {string}` - Rules for cell birth (example: `-B 345`, default `3`)
+ `-S {string}` - Rules for cell survival (example: `-S 567`, default `23`)
+ `-p {int 0-100}` - Percentage of field filling (default `10`)
+ `-f {int}` - Max FPS (also ticks per second, default `60`)

## Controls
+ Escape - Exit
+ Space - Pause
+ R - Refresh field
+ C - Clear field
+ Left Mouse Button - Draw on field

## Build
```Bash
git clone https://github.com/ilya-nikolaev/ConwaysGameOfLife.git
mkdir build && cd build
cmake .. && make
./game
```
