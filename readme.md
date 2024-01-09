# Conway's Game of Life

## Description
The Game of Life, also known simply as Life, is a cellular automaton devised by the British mathematician John Horton Conway in 1970. It is a zero-player game, meaning that its evolution is determined by its initial state, requiring no further input. One interacts with the Game of Life by creating an initial configuration and observing how it evolves. It is Turing complete and can simulate a universal constructor or any other Turing machine.
[Wikipedia](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life)

## Implementation
This implementation is written in C++ using the standard library and the SDL2 library

## Parameters
+ `-w {int}` - Window width (default `1280`)
+ `-h {int}` - Window height (default `720`)
+ `-f` - Use fullscreen (default `false`)
+ `-B {string}` - Rules for cell birth (example: `-B 345`, default `3`)
+ `-S {string}` - Rules for cell survival (example: `-S 567`, default `23`)
+ `-p {int 0-100}` - Percentage of field filling (default `10`)


## Controls
+ Escape - Exit
+ Space - Pause
+ R - Refresh field
+ C - Clear field
+ Left Mouse Button - Draw on field
