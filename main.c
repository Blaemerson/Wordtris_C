#include "raylib.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "words.c"

#define GRID_X_TILES 6
#define GRID_Y_TILES 10

#define TILE_SIZE 72
#define LETTER_SIZE TILE_SIZE

#define SCREEN_WIDTH GRID_X_TILES *TILE_SIZE + (4 * TILE_SIZE)
#define SCREEN_HEIGHT GRID_Y_TILES *TILE_SIZE

char _letter_probs[] = {
    'A', 'A', 'A', 'A', 'A', 'A', 'B', 'B', 'C', 'C', 'C', 'D', 'D', 'D', 'D',
    'E', 'E', 'E', 'E', 'E', 'E', 'F', 'F', 'F', 'G', 'G', 'H', 'H', 'I', 'I',
    'I', 'I', 'I', 'J', 'K', 'K', 'L', 'L', 'L', 'L', 'M', 'M', 'M', 'O', 'O',
    'O', 'O', 'O', 'P', 'P', 'P', 'Q', 'R', 'R', 'R', 'S', 'S', 'S', 'T', 'T',
    'T', 'T', 'U', 'U', 'U', 'U', 'U', 'V', 'V', 'W', 'W', 'X', 'Y', 'Y', 'Z',
};

bool bin_search(char *target) {
  int number_words = sizeof(WORDS) / sizeof(WORDS[0]);
  int bottom = 0;
  int middle;
  int top = number_words -1;

  while (bottom <= top) {
    middle = (bottom + top) / 2;
    if (strcmp(WORDS[middle], target) == 0) {
      return true;
    } else if (strcmp(WORDS[middle], target) > 0) {
      top = middle - 1;
    } else if (strcmp(WORDS[middle], target) < 0) {
      bottom = middle + 1;
    }
  }
  return false;
}

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
  COUNTER_CLOCKW,
  CLOCKW,
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
  Sound move_success;
  Sound move_failure;
  Sound word_found;
} SoundEffects;

GameState _state;
SoundEffects _sfx;
Grid _grid;
Player _player;
Font _font;


void init_grid() {
  for (int i = 0; i < GRID_X_TILES * GRID_Y_TILES; i++) {
    _grid.tiles[i].state = EMPTY;
    _grid.tiles[i].letter = ' ';
  }
}

void init_sounds() {
  _sfx.move_success = LoadSound("resources/move.wav");
  _sfx.move_failure = LoadSound("resources/stuck.wav");
  _sfx.word_found = LoadSound("resources/correct.wav");
}

void init_game() {
  _state = PLAYING;
  init_grid();
  init_sounds();
}

void scan_game_board() {
  char line[GRID_X_TILES];
  for (int row = 0; row < GRID_Y_TILES; row++) {
    for (int x = GRID_X_TILES * row; x < GRID_X_TILES * (row + 1); x++) {
      line[x - (GRID_X_TILES * row)] = _grid.tiles[x].letter;
    }
    printf("line = %s\n", line);

    int n = strlen(line);

    for (int i = 0; i < n; i++)
    {
        char temp[n - i + 1];
        int tempindex = 0;
        for (int j = i; j < n; j++)
        {
            temp[tempindex++] = tolower(line[j]);
            temp[tempindex] = '\0';
            if (strlen(temp) > 2 && bin_search(temp)) {
              printf("word found: %s\n", temp);
            }
            // printf("%s\n", temp);
        }
    }
  }

  char v_line[GRID_Y_TILES];

  for (int x = 0; x < GRID_X_TILES; x++) {
    for (int y = 0; y < GRID_Y_TILES * GRID_X_TILES; y += GRID_X_TILES) {
      v_line[y / GRID_X_TILES] = _grid.tiles[x + y].letter;
    }
    printf("v_line = %s\n", v_line);

    int n = strlen(v_line);

    for (int i = 0; i < n; i++)
    {
        char temp[n - i + 1];
        int tempindex = 0;
        for (int j = i; j < n; j++)
        {
            temp[tempindex++] = tolower(v_line[j]);
            temp[tempindex] = '\0';
            if (strlen(temp) > 2 && bin_search(temp)) {
              printf("word found: %s\n", temp);
            }
            // printf("%s\n", temp);
        }
    }
  }
  // for(int i = 0; i < n; i++) {
  //   for(int j = i; j < n; j++) {   /* print subline from i to j */
  //     for(int k = i; k <= j; k++) {
  //       if (line[k] != ' ') {
  //         printf("%c", line[k]);
  //       } else {
  //         break;
  //       }
  //     }
  //     printf("\nnext\n");
  //   }
  // }

    /*
     * Fix start index in outer loop.
     * Reveal new character in inner loop till end of string.
     * Print till-now-formed string.
     */
}

void draw_game_board() {
  for (int y = 0; y < GRID_Y_TILES; y++) {
    for (int x = 0; x < GRID_X_TILES; x++) {
      int i = y * GRID_X_TILES + x;
      if (_grid.tiles[i].state == EMPTY) {
        DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
        DrawRectangle(x * TILE_SIZE + 1, y * TILE_SIZE + 1, TILE_SIZE - 2,
                      TILE_SIZE - 2, WHITE);
        continue;
      }

      DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, BLACK);
      DrawRectangle(x * TILE_SIZE + 1, y * TILE_SIZE + 1, TILE_SIZE - 2,
                    TILE_SIZE - 2, LIGHTGRAY);
      Vector2 letterSize =
          MeasureTextEx(_font, &_grid.tiles[i].letter, LETTER_SIZE, 0);
      DrawTextEx(_font, &_grid.tiles[i].letter,
                 (Vector2){x * TILE_SIZE + ((TILE_SIZE - letterSize.x) / 2),
                           y * TILE_SIZE + ((TILE_SIZE - letterSize.y) / 2)},
                 TILE_SIZE, 0, BLACK);
    }
  }
}

void spawn_player() {
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
    _player.tiles[0]->letter = _letter_probs[GetRandomValue(0, 74)];

    do {
      _player.tiles[1]->letter = _letter_probs[GetRandomValue(0, 74)];
    } while (_player.tiles[0]->letter == _player.tiles[1]->letter);

  } else {
    _state = GAMEOVER;
  }
}

bool check_collision(int index, int tileIndex) {

  bool moving_down = (index == tileIndex + GRID_X_TILES);
  bool moving_right = !moving_down && (index > tileIndex);
  bool moving_up = (index == tileIndex - GRID_X_TILES);
  bool moving_left = !moving_up && (index < tileIndex);

  bool collided = false;

  if (moving_down) {
    collided = tileIndex > (GRID_X_TILES * (GRID_Y_TILES - 1) - 1);
  } else if (moving_right) {
    collided = tileIndex % GRID_X_TILES == GRID_X_TILES - 1;
  } else if (moving_up) {
    collided = tileIndex < GRID_X_TILES;
  } else if (moving_left) {
    collided = tileIndex % GRID_X_TILES == 0;
  }

  return (!collided && _grid.tiles[index].state != STATIC);
}

void unset_tile(Tile *t) {
  t->state = EMPTY;
  t->letter = ' ';
}

void set_player(int i0, int i1) {
  Tile tmp0 = *_player.tiles[0];
  Tile tmp1 = *_player.tiles[1];
  unset_tile(_player.tiles[0]);
  unset_tile(_player.tiles[1]);
  _player.indexes[0] = i0;
  _player.indexes[1] = i1;
  _grid.tiles[i0] = tmp0;
  _grid.tiles[i1] = tmp1;
  _player.tiles[0] = &_grid.tiles[i0];
  _player.tiles[1] = &_grid.tiles[i1];
}

bool move_player(int dir_x, int dir_y) {
  int i0 = _player.indexes[0] + dir_x + (dir_y * GRID_X_TILES);
  int i1 = _player.indexes[1] + dir_x + (dir_y * GRID_X_TILES);
  if (check_collision(i0, _player.indexes[0]) &&
      check_collision(i1, _player.indexes[1])) {

    set_player(i0, i1);
    return true;
  }

  return false;
}

bool rotate_player(Spin spin) {
  int i0 = _player.indexes[0];
  int i1 = _player.indexes[1];
  PlayerRotation rot = _player.rotation;

  if ((rot == NONE && spin == COUNTER_CLOCKW) ||
      (rot == THREE_QUARTER && spin == CLOCKW)) {
    i0 += -GRID_X_TILES;
    i1 += -1;
    rot = spin == COUNTER_CLOCKW ? QUARTER : HALF;
  } else if ((rot == QUARTER && spin == COUNTER_CLOCKW) ||
             (rot == NONE && spin == CLOCKW)) {
    i0 += 1;
    i1 += -GRID_X_TILES;
    rot = spin == COUNTER_CLOCKW ? HALF : THREE_QUARTER;
  } else if ((rot == HALF && spin == COUNTER_CLOCKW) ||
             (rot == QUARTER && spin == CLOCKW)) {
    i0 += GRID_X_TILES;
    i1 += 1;
    rot = spin == COUNTER_CLOCKW ? THREE_QUARTER : NONE;
  } else if ((rot == THREE_QUARTER && spin == COUNTER_CLOCKW) ||
             (rot == HALF && spin == CLOCKW)) {
    i0 += -1;
    i1 += GRID_X_TILES;
    rot = spin == COUNTER_CLOCKW ? NONE : QUARTER;
  }

  if (check_collision(i0, _player.indexes[0]) &&
      check_collision(i1, _player.indexes[1])) {
    _player.rotation = rot;
    set_player(i0, i1);
    return true;
  }
  return false;
}

void draw_frame() {
  BeginDrawing();

  ClearBackground(RAYWHITE);
  draw_game_board();

  EndDrawing();
}

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Wordtris");
  InitAudioDevice();
  SetTargetFPS(60);
  init_game();

  _font = LoadFontEx("./Arialbd.TTF", LETTER_SIZE, 0, 0);
  spawn_player();

  double tick_time = 1.0;
  double time = GetTime();
  while (!WindowShouldClose()) {
    // in-game tick
    if (GetTime() >= time + tick_time) {
      time = GetTime();
      if (!move_player(0, 1)) {
        PlaySound(_sfx.move_failure);
        scan_game_board();
        spawn_player();
      }
    }

    switch (GetKeyPressed()) {
    case (KEY_A):
      move_player(-1, 0) ? PlaySound(_sfx.move_success)
                        : PlaySound(_sfx.move_failure);
      break;
    case (KEY_D):
      move_player(1, 0) ? PlaySound(_sfx.move_success)
                       : PlaySound(_sfx.move_failure);
      break;
    case (KEY_S):
      if (move_player(0, 1)) {
        time = GetTime();
        PlaySound(_sfx.move_success);
      } else {
        time = tick_time;
      }
      break;
    case (KEY_K):
      rotate_player(COUNTER_CLOCKW);
      break;
    case (KEY_J):
      rotate_player(CLOCKW);
      break;
    };

    if (_state == GAMEOVER) {
      break;
    }
    draw_frame();
  }

  UnloadFont(_font);
  CloseWindow();

  return 0;
}
