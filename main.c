#include "raylib.h"
#include <math.h>
#include <stdio.h>

#define GRID_X_TILES 6
#define GRID_Y_TILES 10

#define TILE_SIZE 72
#define LETTER_SIZE TILE_SIZE

#define SCREEN_WIDTH GRID_X_TILES *TILE_SIZE + (4 * TILE_SIZE)
#define SCREEN_HEIGHT GRID_Y_TILES *TILE_SIZE

char _letters[] = {
    'A', 'A', 'A', 'A', 'A', 'A', 'B', 'B', 'C', 'C', 'C', 'D', 'D', 'D', 'D',
    'E', 'E', 'E', 'E', 'E', 'E', 'F', 'F', 'F', 'G', 'G', 'H', 'H', 'I', 'I',
    'I', 'I', 'I', 'J', 'K', 'K', 'L', 'L', 'L', 'L', 'M', 'M', 'M', 'O', 'O',
    'O', 'O', 'O', 'P', 'P', 'P', 'Q', 'R', 'R', 'R', 'S', 'S', 'S', 'T', 'T',
    'T', 'T', 'U', 'U', 'U', 'U', 'U', 'V', 'V', 'W', 'W', 'X', 'Y', 'Y', 'Z',
};

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

typedef enum Spin {
  C_CLOCKWISE,
  CLOCKWISE,
} Spin;

typedef enum PlayerRotation {
  NONE = 0,
  QUARTER = 1,
  HALF = 2,
  THREE_QUARTER = 3,
} PlayerRotation;

typedef struct Player {
  Tile *tiles[2];
  int indexes[2];
  PlayerRotation rotation;
} Player;

typedef struct SoundEffects {
  Sound moveSuccess;
  Sound moveFailure;
  Sound wordFound;
} SoundEffects;

GameState _state;
SoundEffects _sfx;
Grid _grid;
Player _player;
Font _font;

void InitGrid() {
  for (int i = 0; i < GRID_X_TILES * GRID_Y_TILES; i++) {
    _grid.tiles[i].state = EMPTY;
    _grid.tiles[i].letter = ' ';
  }
}

void InitSounds() {
  _sfx.moveSuccess = LoadSound("resources/move.wav");
  _sfx.moveFailure = LoadSound("resources/stuck.wav");
  _sfx.wordFound = LoadSound("resources/correct.wav");
}

void InitGame() {
  _state = PLAYING;
  InitGrid();
  InitSounds();
}

void DrawGameBoard() {
  int row = 0;
  for (int y = 0; y < GRID_Y_TILES; y++) {
    for (int x = 0; x < GRID_X_TILES; x++) {
      int index = y * GRID_X_TILES + x;
      if (_grid.tiles[index].state == EMPTY) {
        DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
        DrawRectangle(x * TILE_SIZE + 1, y * TILE_SIZE + 1, TILE_SIZE - 2,
                      TILE_SIZE - 2, WHITE);
        continue;
      } else {
        DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE,
                      BLACK);
        DrawRectangle(x * TILE_SIZE + 1, y * TILE_SIZE + 1, TILE_SIZE - 2,
                      TILE_SIZE - 2, LIGHTGRAY);
        Vector2 letterSize =
            MeasureTextEx(_font, &_grid.tiles[index].letter, LETTER_SIZE, 0);
        DrawTextEx(_font, &_grid.tiles[index].letter,
                   (Vector2){x * TILE_SIZE + ((TILE_SIZE - letterSize.x) / 2),
                             y * TILE_SIZE + ((TILE_SIZE - letterSize.y) / 2)},
                   TILE_SIZE, 0, BLACK);
      }
    }
  }
}

void SpawnPiece() {
  if (_player.tiles[0] && _player.tiles[1]) {
    _player.tiles[0]->state = STATIC;
    _player.tiles[1]->state = STATIC;
  }
  int leftIndex = (GRID_X_TILES / 2) - 1;
  int rightIndex = (GRID_X_TILES / 2);
  if (_grid.tiles[leftIndex].state == EMPTY &&
      _grid.tiles[rightIndex].state == EMPTY) {

    _player.tiles[0] = &_grid.tiles[leftIndex];
    _player.tiles[1] = &_grid.tiles[rightIndex];
    _player.tiles[0]->state = FALLING;
    _player.tiles[1]->state = FALLING;
    _player.indexes[0] = leftIndex;
    _player.indexes[1] = rightIndex;
    _player.rotation = NONE;

    _player.tiles[0]->letter = _letters[GetRandomValue(0, 74)];
    do {
      _player.tiles[1]->letter = _letters[GetRandomValue(0, 74)];
    } while (_player.tiles[0]->letter == _player.tiles[1]->letter);
  } else {
    _state = GAMEOVER;
  }
}

bool CheckCollision(int index, int tileIndex) {

  bool movingDown = (index == tileIndex + GRID_X_TILES);
  bool movingRight = !movingDown && (index > tileIndex);
  bool movingLeft = !movingRight && (index < tileIndex);

  bool collided = false;

  if (movingRight) {
    collided = (tileIndex % GRID_X_TILES) == GRID_X_TILES - 1;
  } else if (movingDown) {
    collided = index > (GRID_X_TILES * GRID_Y_TILES) - 1;
  } else if (movingLeft) {
    collided = (tileIndex % GRID_X_TILES) == 0;
  }

  return (!collided && _grid.tiles[index].state != STATIC);
}

void UnsetTile(Tile *t) {
  t->state = EMPTY;
  t->letter = ' ';
}

void SetPlayer(int new_index0, int new_index1) {
  Tile tmp1 = *_player.tiles[0];
  Tile tmp2 = *_player.tiles[1];
  UnsetTile(&_grid.tiles[_player.indexes[0]]);
  UnsetTile(&_grid.tiles[_player.indexes[1]]);
  _player.indexes[0] = new_index0;
  _player.indexes[1] = new_index1;
  _grid.tiles[_player.indexes[0]] = tmp1;
  _grid.tiles[_player.indexes[1]] = tmp2;
  _player.tiles[0] = &_grid.tiles[_player.indexes[0]];
  _player.tiles[1] = &_grid.tiles[_player.indexes[1]];
}

bool MovePlayer(int dirX, int dirY) {
  int new_index0 = _player.indexes[0] + dirX + (dirY * GRID_X_TILES);
  int new_index1 = _player.indexes[1] + dirX + (dirY * GRID_X_TILES);
  if (CheckCollision(new_index0, _player.indexes[0]) &&
      CheckCollision(new_index1, _player.indexes[1])) {

    SetPlayer(new_index0, new_index1);
    return true;
  }

  return false;
}

bool RotatePlayer(Spin spin) {
  int index0 = _player.indexes[0];
  int index1 = _player.indexes[1];
  PlayerRotation rotation = _player.rotation;

  if ((_player.rotation == NONE && spin == C_CLOCKWISE) ||
      (_player.rotation == THREE_QUARTER && spin == CLOCKWISE)) {
    index0 += -GRID_X_TILES;
    index1 += -1;
    rotation = spin == C_CLOCKWISE ? QUARTER : HALF;
  } else if ((_player.rotation == QUARTER && spin == C_CLOCKWISE) ||
             (_player.rotation == NONE && spin == CLOCKWISE)) {
    index0 += 1;
    index1 += -GRID_X_TILES;
    rotation = spin == C_CLOCKWISE ? HALF : THREE_QUARTER;
  } else if ((_player.rotation == HALF && spin == C_CLOCKWISE) ||
             (_player.rotation == QUARTER && spin == CLOCKWISE)) {
    index0 += GRID_X_TILES;
    index1 += 1;
    rotation = spin == C_CLOCKWISE ? THREE_QUARTER : NONE;
  } else if ((_player.rotation == THREE_QUARTER && spin == C_CLOCKWISE) ||
             (_player.rotation == HALF && spin == CLOCKWISE)) {
    index0 += -1;
    index1 += GRID_X_TILES;
    rotation = spin == C_CLOCKWISE ? NONE : QUARTER;
  }

  if (CheckCollision(index0, index1)) {
    _player.rotation = rotation;
    SetPlayer(index0, index1);
    return true;
  }

  return false;
}

void DrawFrame() {
  BeginDrawing();

  ClearBackground(RAYWHITE);
  DrawGameBoard();

  EndDrawing();
}

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Wordtris");
  InitAudioDevice();
  SetTargetFPS(60);
  InitGame();
  _font = LoadFontEx("./Arialbd.TTF", LETTER_SIZE, 0, 0);
  SpawnPiece();

  double tick_time = 1.0;
  double time = GetTime();
  while (!WindowShouldClose()) {
    // in-game tick
    if (GetTime() >= time + tick_time) {
      time = GetTime();
      if (!MovePlayer(0, 1)) {
        PlaySound(_sfx.moveFailure);
        SpawnPiece();
      }
    }

    switch (GetKeyPressed()) {
    case (KEY_A):
      MovePlayer(-1, 0) ? PlaySound(_sfx.moveSuccess)
                        : PlaySound(_sfx.moveFailure);
      break;
    case (KEY_D):
      MovePlayer(1, 0) ? PlaySound(_sfx.moveSuccess)
                       : PlaySound(_sfx.moveFailure);
      break;
    case (KEY_S):
      if (MovePlayer(0, 1)) {
        time = GetTime();
        PlaySound(_sfx.moveSuccess);
      } else {
        time = tick_time;
      }
      break;
    case (KEY_K):
      RotatePlayer(C_CLOCKWISE);
      break;
    case (KEY_J):
      RotatePlayer(CLOCKWISE);
      break;
    };

    if (_state == GAMEOVER) {
      break;
    }
    DrawFrame();
  }

  UnloadFont(_font);
  CloseWindow();

  return 0;
}
