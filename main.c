#include "raylib.h"
#include <math.h>
#include <stdio.h>

#define GRID_X_TILES 8
#define GRID_Y_TILES 14

#define TILE_SIZE 64

#define SCREEN_WIDTH GRID_X_TILES *TILE_SIZE
#define SCREEN_HEIGHT GRID_Y_TILES *TILE_SIZE

typedef enum TileState {
  FALLING,
  STATIC,
  EMPTY,
} TileState;

typedef struct Tile {
  TileState state;
  char letter;
} Tile;

typedef struct Grid {
  Tile tiles[GRID_X_TILES * GRID_Y_TILES];
} Grid;

typedef enum GameState {
  GAMEOVER,
  PLAYING,
} GameState;

typedef struct Player {
  Tile *tiles[2];
  int indexes[2];
} Player;

GameState _state;
Grid _grid;
Player _player;

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
  for (int y = 0; y < GRID_Y_TILES; y++) {
    for (int x = 0; x < GRID_X_TILES; x++) {
      int index = y * GRID_X_TILES + x;
      if (_grid.tiles[index].state == EMPTY) {
        DrawRectangle(x * TILE_SIZE, (y)*TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
        DrawRectangle(x * TILE_SIZE + 2, (y)*TILE_SIZE + 2, TILE_SIZE - 4,
                      TILE_SIZE - 4, WHITE);
        continue;
      } else {
        DrawRectangle(x * TILE_SIZE, (y)*TILE_SIZE, TILE_SIZE, TILE_SIZE,
                      BLACK);
        DrawRectangle(x * TILE_SIZE + 2, (y)*TILE_SIZE + 2, TILE_SIZE - 4,
                      TILE_SIZE - 4, BROWN);
      }
    }
  }
}

void SpawnPiece() {
  int leftIndex = (GRID_X_TILES / 2) - 1;
  int rightIndex = (GRID_X_TILES / 2);
  if (_grid.tiles[leftIndex].state == EMPTY &&
      _grid.tiles[rightIndex].state == EMPTY) {
    _grid.tiles[leftIndex].state = FALLING;
    _grid.tiles[rightIndex].state = FALLING;

    _player.tiles[0] = &_grid.tiles[leftIndex];
    _player.tiles[0] = &_grid.tiles[rightIndex];
    _player.indexes[0] = leftIndex;
    _player.indexes[1] = rightIndex;
  } else {
    _state = GAMEOVER;
  }
}

bool CheckCollision(int index) {
  bool movingRight =
      (_player.indexes[0] < index || _player.indexes[1] < index) &&
      (index != _player.indexes[0] + GRID_X_TILES &&
       index != _player.indexes[1] + GRID_X_TILES);
  bool movingDown = !movingRight && index == _player.indexes[0] + GRID_X_TILES;
  bool movingLeft = !movingDown && index < _player.indexes[0];

  bool collided = false;
  if (movingRight) {
    collided = index % GRID_X_TILES == 0;
  } else if (movingDown) {
    collided = index > ((GRID_X_TILES * GRID_Y_TILES)) - 1;
  } else if (movingLeft) {
    collided = (index + 1) % GRID_X_TILES == 0;
  }

  return ((!collided &&
           (_grid.tiles[index].state == EMPTY || index != _player.indexes[0] ||
            index != _player.indexes[1])));
}

void MovePlayer(int dirX, int dirY) {
  if (CheckCollision(_player.indexes[0] + dirX + (dirY * GRID_X_TILES)) &&
      CheckCollision(_player.indexes[1] + dirX + (dirY * GRID_X_TILES))) {
    _player.indexes[0] += dirX + (dirY * GRID_X_TILES);
    _player.indexes[1] += dirX + (dirY * GRID_X_TILES);
    Tile *tmp1 = &_grid.tiles[_player.indexes[0]];
    Tile *tmp2 = &_grid.tiles[_player.indexes[1]];
    _grid.tiles[_player.indexes[0] - dirX - (dirY * GRID_X_TILES)].state =
        EMPTY;
    _grid.tiles[_player.indexes[1] - dirX - (dirY * GRID_X_TILES)].state =
        EMPTY;
    _player.tiles[0] = tmp1;
    _player.tiles[1] = tmp2;
    tmp1->state = FALLING;
    tmp2->state = FALLING;
  }
}

void DrawFrame() {
  BeginDrawing();

  if (IsKeyPressed(KEY_A)) {
    MovePlayer(-1, 0);
  } else if (IsKeyPressed(KEY_D)) {
    MovePlayer(1, 0);
  } else if (IsKeyPressed(KEY_S)) {
    MovePlayer(0, 1);
  } else if (IsKeyPressed(KEY_W)) {
    MovePlayer(0, -1);
  }

  ClearBackground(RAYWHITE);
  DrawGameBoard();

  EndDrawing();
}

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Wordtris");
  InitGame();
  SpawnPiece();
  while (!WindowShouldClose()) {
    DrawFrame();
  }
}
