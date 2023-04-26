#include "raylib.h"
#include <math.h>
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
  Tile tiles[GRID_X_TILES * GRID_Y_TILES];
} Grid;

typedef enum GameState {
  GAMEOVER, PLAYING,
} GameState;

typedef struct Game {
  GameState state;
  Grid grid;
} Game;

GameState _state;
Grid _grid;

void InitGrid() {
  for (int i = 0; i < GRID_X_TILES * GRID_Y_TILES; i++) {
    _grid.tiles[i].state = EMPTY;
    _grid.tiles[i].letter = ' ';
  }
}

void InitGame() {
  _state = PLAYING;
  InitGrid();
}

void DrawGameBoard() {
  int row = 0;
  for (int i = 0; i < GRID_Y_TILES; i++) {
    for (int j = 0; j < GRID_X_TILES; j++) {
      if (_grid.tiles[i+j].state == EMPTY) {
        DrawRectangle(j * TILE_SIZE, (i) * TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
        DrawRectangle(j * TILE_SIZE + 2, (i) * TILE_SIZE + 2, TILE_SIZE - 4, TILE_SIZE - 4, WHITE);
        continue;
      }
    }
  }
}

void DrawFrame() {
  BeginDrawing();

  ClearBackground(RAYWHITE);
  DrawGameBoard();

  EndDrawing();
}

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Wordtris");
  InitGame();
  while (!WindowShouldClose()) {
    DrawFrame();
  }
}
