#include "raylib.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trie.c"

// #include "words.c"

#define GRID_WIDTH 6
#define GRID_HEIGHT 10
#define NUM_TILES (GRID_WIDTH * GRID_HEIGHT)

#define TILE_SIZE 72

#define SCREEN_WIDTH (GRID_WIDTH * TILE_SIZE) + (4 * TILE_SIZE)
#define SCREEN_HEIGHT (GRID_HEIGHT * TILE_SIZE)

typedef enum TileState {
    FALLING,
    STATIC,
    EMPTY,
} TileState;

typedef struct Tile {
    TileState state;
    char letter;
} Tile;

typedef enum GameState {
    GAMEOVER,
    PLAYING,
    PAUSED,
} GameState;

typedef enum Spin {
    COUNTER_CLOCKW,
    CLOCKW,
} Spin;

typedef enum PlayerRotation {
    ROT_0,
    ROT_1,
    ROT_2,
    ROT_3,
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
Tile _grid[GRID_WIDTH * GRID_HEIGHT];
Player _player;
Font _font;

char _letterProbs[] = {
    'A', 'A', 'A', 'A', 'A', 'A', 'B', 'B', 'C', 'C', 'C', 'D', 'D', 'D', 'D',
    'E', 'E', 'E', 'E', 'E', 'E', 'F', 'F', 'F', 'G', 'G', 'H', 'H', 'I', 'I',
    'I', 'I', 'I', 'J', 'K', 'K', 'L', 'L', 'L', 'L', 'M', 'M', 'M', 'O', 'O',
    'O', 'O', 'O', 'P', 'P', 'P', 'Q', 'R', 'R', 'R', 'S', 'S', 'S', 'T', 'T',
    'T', 'T', 'U', 'U', 'U', 'U', 'U', 'V', 'V', 'W', 'W', 'X', 'Y', 'Y', 'Z',
};

void initGrid() {
    for (int i = 0; i < NUM_TILES; i++) {
        _grid[i].state = EMPTY;
        _grid[i].letter = ' ';
    }
}

void initSounds() {
    _sfx.move_success = LoadSound("resources/move.wav");
    _sfx.move_failure = LoadSound("resources/stuck.wav");
    _sfx.word_found = LoadSound("resources/correct.wav");
}

void initGame() {
    _state = PLAYING;
    initGrid();
    initSounds();
}

// void find_substrs(char *str) {
//     int n = strlen(str);
//
//     for (int i = 0; i < n; i++) {
//         char temp[n - i + 1];
//         int tempindex = 0;
//         for (int j = i; j < n; j++) {
//             temp[tempindex++] = tolower(str[j]);
//             temp[tempindex] = '\0';
//             if (strlen(temp) > 2 && bin_search(temp)) {
//                 printf("word found: %s\n", temp);
//             }
//         }
//     }
// }

// void scan_game_board() {
//     char line[GRID_WIDTH];
//     for (int row = 0; row < GRID_HEIGHT; row++) {
//         for (int x = GRID_WIDTH * row; x < GRID_WIDTH * (row + 1); x++) {
//             line[x - (GRID_WIDTH * row)] = _grid[x].letter;
//         }
//
//         find_substrs(line);
//     }
//
//     char v_line[GRID_HEIGHT];
//     for (int x = 0; x < GRID_WIDTH; x++) {
//         for (int y = 0; y < NUM_TILES; y += GRID_WIDTH) {
//             v_line[y / GRID_WIDTH] = _grid[x + y].letter;
//         }
//
//         find_substrs(v_line);
//     }
// }

void drawGameBoard() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            int i = y * GRID_WIDTH + x;
            if (_grid[i].state == EMPTY) {
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
                DrawRectangle(x * TILE_SIZE + 1, y * TILE_SIZE + 1, TILE_SIZE - 2,
                              TILE_SIZE - 2, WHITE);
                continue;
            }

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, BLACK);
            DrawRectangle(x * TILE_SIZE + 1, y * TILE_SIZE + 1, TILE_SIZE - 2,
                          TILE_SIZE - 2, LIGHTGRAY);
            Vector2 letterSize =
                MeasureTextEx(_font, &_grid[i].letter, TILE_SIZE, 0);
            DrawTextEx(_font, &_grid[i].letter,
                       (Vector2){x * TILE_SIZE + ((TILE_SIZE - letterSize.x) / 2),
                       y * TILE_SIZE + ((TILE_SIZE - letterSize.y) / 2)},
                       TILE_SIZE, 0, BLACK);
        }
    }
}

void stopBlock(Tile *t) {
    if (t) {
        t->state = STATIC;
    }
}

void spawnPlayer() {
    // set current player block to static
    stopBlock(_player.tiles[0]);
    stopBlock(_player.tiles[1]);

    int l_idx = (GRID_WIDTH / 2) - 1;
    int r_idx = l_idx + 1;

    Tile* l_tile = &_grid[l_idx];
    Tile* r_tile = &_grid[r_idx];

    bool can_spawn = l_tile->state == EMPTY && r_tile->state == EMPTY;
    if (can_spawn) {
        _player.tiles[0] = l_tile;
        _player.tiles[1] = r_tile;
        _player.indexes[0] = l_idx;
        _player.indexes[1] = r_idx;
        _player.tiles[0]->state = FALLING;
        _player.tiles[1]->state = FALLING;
        _player.rotation = ROT_0;
        _player.tiles[0]->letter = _letterProbs[GetRandomValue(0, 74)];
        _player.tiles[1]->letter = _letterProbs[GetRandomValue(0, 74)];
    } else {
        _state = GAMEOVER;
    }
}

bool checkCollision(int index, int tileIndex) {

    bool moving_down = (index == tileIndex + GRID_WIDTH);
    bool moving_right = !moving_down && (index > tileIndex);
    bool moving_up = (index == tileIndex - GRID_WIDTH);
    bool moving_left = !moving_up && (index < tileIndex);

    bool collided = false;

    if (moving_down) {
        collided = tileIndex > (GRID_WIDTH * (GRID_HEIGHT - 1) - 1);
    } else if (moving_right) {
        collided = tileIndex % GRID_WIDTH == GRID_WIDTH - 1;
    } else if (moving_up) {
        collided = tileIndex < GRID_WIDTH;
    } else if (moving_left) {
        collided = tileIndex % GRID_WIDTH == 0;
    }

    return (!collided && _grid[index].state != STATIC);
}

void unsetTile(Tile *t) {
    t->state = EMPTY;
    t->letter = ' ';
}

void set_player(int i0, int i1) {
    Tile tmp0 = *_player.tiles[0];
    Tile tmp1 = *_player.tiles[1];
    unsetTile(_player.tiles[0]);
    unsetTile(_player.tiles[1]);
    _player.indexes[0] = i0;
    _player.indexes[1] = i1;
    _grid[i0] = tmp0;
    _grid[i1] = tmp1;
    _player.tiles[0] = &_grid[i0];
    _player.tiles[1] = &_grid[i1];
}

bool movePlayer(int dir_x, int dir_y) {
    int i0 = _player.indexes[0] + dir_x + (dir_y * GRID_WIDTH);
    int i1 = _player.indexes[1] + dir_x + (dir_y * GRID_WIDTH);
    if (checkCollision(i0, _player.indexes[0]) &&
        checkCollision(i1, _player.indexes[1])) {

        set_player(i0, i1);
        return true;
    }

    return false;
}

bool rotatePlayer(Spin spin) {
    int i0 = _player.indexes[0];
    int i1 = _player.indexes[1];
    PlayerRotation rot = _player.rotation;

    if ((rot == ROT_0 && spin == COUNTER_CLOCKW) ||
        (rot == ROT_3 && spin == CLOCKW)) {
            i0 += -GRID_WIDTH;
            i1 += -1;
            rot = spin == COUNTER_CLOCKW ? ROT_1 : ROT_2;
        } else if ((rot == ROT_1 && spin == COUNTER_CLOCKW) ||
            (rot == ROT_0 && spin == CLOCKW)) {
        i0 += 1;
        i1 += -GRID_WIDTH;
        rot = spin == COUNTER_CLOCKW ? ROT_2 : ROT_3;
    } else if ((rot == ROT_2 && spin == COUNTER_CLOCKW) ||
            (rot == ROT_1 && spin == CLOCKW)) {
        i0 += GRID_WIDTH;
        i1 += 1;
        rot = spin == COUNTER_CLOCKW ? ROT_3 : ROT_0;
    } else if ((rot == ROT_3 && spin == COUNTER_CLOCKW) ||
            (rot == ROT_2 && spin == CLOCKW)) {
    i0 += -1;
    i1 += GRID_WIDTH;
    rot = spin == COUNTER_CLOCKW ? ROT_0 : ROT_1;
}

    if (checkCollision(i0, _player.indexes[0]) &&
        checkCollision(i1, _player.indexes[1])) {
        _player.rotation = rot;
        set_player(i0, i1);
        return true;
    }
    return false;
}

void drawFrame() {
    BeginDrawing();

    ClearBackground(RAYWHITE);
    drawGameBoard();

    EndDrawing();
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Wordtris");
    InitAudioDevice();
    SetTargetFPS(60);
    initGame();

    _font = LoadFontEx("./Arialbd.TTF", TILE_SIZE, 0, 0);
    spawnPlayer();

    struct TrieNode* dictTrieRoot = createNode();
    constructTrie(dictTrieRoot, "./dictionary.txt");

    double tick_time = 1.0;
    double time = GetTime();
    while (!WindowShouldClose()) {
        // in-game tick
        if (GetTime() >= time + tick_time) {
            time = GetTime();
            if (!movePlayer(0, 1)) {
                PlaySound(_sfx.move_failure);
                // scan_game_board();
                spawnPlayer();
            }
        }

        switch (GetKeyPressed()) {
            case (KEY_A):
                movePlayer(-1, 0) ? PlaySound(_sfx.move_success)
                    : PlaySound(_sfx.move_failure);
                break;
            case (KEY_D):
                movePlayer(1, 0) ? PlaySound(_sfx.move_success)
                    : PlaySound(_sfx.move_failure);
                break;
            case (KEY_S):
                if (movePlayer(0, 1)) {
                    time = GetTime();
                    PlaySound(_sfx.move_success);
                } else {
                    time = tick_time;
                }
                break;
            case (KEY_K):
                rotatePlayer(COUNTER_CLOCKW);
                break;
            case (KEY_J):
                rotatePlayer(CLOCKW);
                break;
        };

        if (_state == GAMEOVER) {
            break;
        }
        drawFrame();
    }

    freeTrie(dictTrieRoot);

    UnloadFont(_font);
    CloseWindow();

    return 0;
}
