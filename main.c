#include "raylib.h"
#include <stdio.h>

#define GRID_X_TILES 8
#define GRID_Y_TILES 14

#define TILE_SIZE 64

#define SCREEN_WIDTH GRID_X_TILES * TILE_SIZE
#define SCREEN_HEIGHT GRID_Y_TILES * TILE_SIZE

typedef enum TileState {
  FALLING, STATIC, EMPTY,
} TileState;

typedef struct Tile {
  TileState state;
  char letter;
} Tile;

typedef struct Grid {
  Tile* tiles[GRID_X_TILES * GRID_Y_TILES];
} Grid;

typedef enum GameState {
  GAMEOVER, PLAYING,
} GameState;

typedef struct Game {
  GameState state;
} Game;

Game InitGame() {
  Game game = {PLAYING};
  return game;
}

Grid InitGrid() {
  Grid grid;
  for (int i = 0; i < GRID_X_TILES * GRID_Y_TILES; i++) {
    grid.tiles[i]->state = EMPTY;
  }

  return grid;
}

void DrawFrame() {
  BeginDrawing();

  ClearBackground(RAYWHITE);

  EndDrawing();
}

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Wordtris");
  InitGame();

  while (!WindowShouldClose()) {
    DrawFrame();
  }
}
