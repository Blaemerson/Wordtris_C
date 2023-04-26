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

typedef enum PlayerRotation {
  NONE,
  QUARTER,
  HALF,
  THREE_QUARTER,
} PlayerRotation;

typedef struct Player {
  Tile *tiles[2];
  int indexes[2];
  PlayerRotation rotation;
} Player;

GameState _state;
Grid _grid;
Player _player;
Font _font;

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
        DrawTextEx(_font, &_grid.tiles[index].letter,(Vector2){ x * TILE_SIZE + 15, y * TILE_SIZE + 10}, TILE_SIZE - 10, 0, BLACK);
      }
    }
  }
}

void SpawnPiece() {
  int leftIndex = (GRID_X_TILES / 2) - 1;
  int rightIndex = (GRID_X_TILES / 2);
  if (_grid.tiles[leftIndex].state == EMPTY &&
      _grid.tiles[rightIndex].state == EMPTY) {
    // _grid.tiles[leftIndex].state = FALLING;
    // _grid.tiles[rightIndex].state = FALLING;

    _player.tiles[0] = &_grid.tiles[leftIndex];
    _player.tiles[1] = &_grid.tiles[rightIndex];
    _player.tiles[0]->state = FALLING;
    _player.tiles[1]->state = FALLING;
    _player.indexes[0] = leftIndex;
    _player.indexes[1] = rightIndex;
    _player.rotation = NONE;
    _player.tiles[0]->letter = 'A';
    _player.tiles[1]->letter = 'B';
  } else {
    _state = GAMEOVER;
  }
}

bool CheckCollision(int index) {
  bool movingRight =
      (_player.indexes[0] < index || _player.indexes[1] < index) &&
      (index != _player.indexes[0] + GRID_X_TILES &&
       index != _player.indexes[1] + GRID_X_TILES);
  bool movingDown = !movingRight && (index == _player.indexes[0] + GRID_X_TILES || index == _player.indexes[1] + GRID_X_TILES);
  bool movingLeft = !movingDown && (index < _player.indexes[0] || index < _player.indexes[0]);

  bool collided = false;
  if (movingRight) {
    collided = index % GRID_X_TILES == 0 || _grid.tiles[index].state == STATIC;
  } else if (movingDown) {
    collided = index > ((GRID_X_TILES * GRID_Y_TILES)) - 1 || _grid.tiles[index].state == STATIC;
    printf("%u\n", _grid.tiles[index].state);
  } else if (movingLeft) {
    collided = (index + 1) % GRID_X_TILES == 0 || _grid.tiles[index].state == STATIC;
  }

  return ((!collided &&
           ( index != _player.indexes[0] ||
            index != _player.indexes[1])));
}

void UnsetTile(Tile* t) {
  t->state = EMPTY;
  t->letter = ' ';
}

void MovePlayer(int dirX, int dirY) {
  if (CheckCollision(_player.indexes[0] + dirX + (dirY * GRID_X_TILES)) &&
      CheckCollision(_player.indexes[1] + dirX + (dirY * GRID_X_TILES))) {

    printf("%d %d\n", _player.indexes[0], _player.indexes[1]);
    Tile tmp1 = *_player.tiles[0];
    Tile tmp2 = *_player.tiles[1];
    UnsetTile(&_grid.tiles[_player.indexes[0]]);
    UnsetTile(&_grid.tiles[_player.indexes[1]]);
    _player.indexes[0] += dirX + (dirY * GRID_X_TILES);
    _player.indexes[1] += dirX + (dirY * GRID_X_TILES);
    _grid.tiles[_player.indexes[0]] = tmp1;
    _grid.tiles[_player.indexes[1]] = tmp2;
    _player.tiles[0] = &_grid.tiles[_player.indexes[0]];
    _player.tiles[1] = &_grid.tiles[_player.indexes[1]];
    _player.tiles[0]->state = FALLING;
    _player.tiles[1]->state = FALLING;
  }
}

void RotatePlayer(int spin) {
  // Counter clockwise
  int index1 = _player.indexes[0];
  int index2 = _player.indexes[1];
  if (spin == -1) {
    if (_player.rotation == NONE) {
      if (CheckCollision(_player.indexes[0] - GRID_X_TILES) &&
          CheckCollision(_player.indexes[1] - 1)) {
        index1 += -GRID_X_TILES;
        index2 += -1;
        _player.rotation = QUARTER;
      }
    } else if (_player.rotation == QUARTER) {
      if (CheckCollision(_player.indexes[0] - GRID_X_TILES) &&
          CheckCollision(_player.indexes[1] + 1)) {
        index1 += 1;
        index2 += -GRID_X_TILES;
        _player.rotation = HALF;
      }
    } else if (_player.rotation == HALF) {
      if (CheckCollision(_player.indexes[0] + GRID_X_TILES) &&
          CheckCollision(_player.indexes[1] + 1)) {
        index1 += GRID_X_TILES;
        index2 += 1;
        _player.rotation = THREE_QUARTER;
      }

    } else {
      if (CheckCollision(_player.indexes[0] + GRID_X_TILES) &&
          CheckCollision(_player.indexes[1] - 1)) {
        index1 += -1;
        index2 += GRID_X_TILES;
        _player.rotation = NONE;
      }
    }
  }


  Tile tmp1 = *_player.tiles[0];
  Tile tmp2 = *_player.tiles[1];
  UnsetTile(&_grid.tiles[_player.indexes[0]]);
  UnsetTile(&_grid.tiles[_player.indexes[1]]);
  _player.indexes[0] = index1;
  _player.indexes[1] = index2;
  _grid.tiles[_player.indexes[0]] = tmp1;
  _grid.tiles[_player.indexes[1]] = tmp2;
  _player.tiles[0] = &_grid.tiles[_player.indexes[0]];
  _player.tiles[1] = &_grid.tiles[_player.indexes[1]];
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

  if (IsKeyPressed(KEY_B)) {
    _player.tiles[0]->state = STATIC;
    _player.tiles[1]->state = STATIC;
    SpawnPiece();
  }

  if (IsKeyPressed(KEY_R)) {
    RotatePlayer(-1);
  }

  ClearBackground(RAYWHITE);
  DrawGameBoard();

  EndDrawing();
}

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Wordtris");
  InitGame();
  _font = LoadFontEx("./Arial.TTF", TILE_SIZE * 2, 0, 255);
  SpawnPiece();
  while (!WindowShouldClose()) {
    DrawFrame();
  }

  UnloadFont(_font);
  CloseWindow();

  return 0;
}
