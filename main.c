#include "raylib.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pool.c"
#include "trie.c"

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

struct TrieNode *dictTrieRoot;

struct LetterPool pool;

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

void drawGameBoard() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            int i = y * GRID_WIDTH + x;
            if (_grid[i].state == EMPTY) {
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
                DrawRectangle(x * TILE_SIZE + 1, y * TILE_SIZE + 1, TILE_SIZE - 2,
                              TILE_SIZE - 2, WHITE);
                continue;
            } else if (_grid[i].letter == ' ') {
                printf("%d\n", i);
            }

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, BLACK);
            DrawRectangle(x * TILE_SIZE + 1, y * TILE_SIZE + 1, TILE_SIZE - 2,
                          TILE_SIZE - 2, LIGHTGRAY);
            Vector2 letterSize = MeasureTextEx(_font, &_grid[i].letter, TILE_SIZE, 0);
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

    Tile *l_tile = &_grid[l_idx];
    Tile *r_tile = &_grid[r_idx];

    bool can_spawn = l_tile->state == EMPTY && r_tile->state == EMPTY;
    if (can_spawn) {
        _player.rotation = ROT_0;

        _player.tiles[0] = l_tile;
        _player.indexes[0] = l_idx;
        _player.tiles[0]->state = FALLING;
        _player.tiles[0]->letter = getRandomLetter(&pool);

        _player.tiles[1] = r_tile;
        _player.indexes[1] = r_idx;
        _player.tiles[1]->state = FALLING;
        _player.tiles[1]->letter = getRandomLetter(&pool);
    } else {
        _state = GAMEOVER;
    }
}

bool checkWordViability(char* word) {
    bool contains_vowel = false;
    bool contains_consonant = false;
    for (int i = 0; i < strlen(word); i++) {
        if (word[i] == 'a' | word[i] == 'e' | word[i] == 'i' | word[i] == 'o' | word[i] == 'u') {
            contains_vowel = true;
        } else {
            contains_consonant = true;
        }
    }

    return contains_vowel && contains_consonant;
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
bool isValidSubstring(const char* substring) {
    int length = strlen(substring);
    if (length < 3) {
        return false; // Substring is less than 3 characters
    }
    for (int i = 0; i < length; i++) {
        if (substring[i] == ' ') {
            return false; // Substring contains a space
        }
    }
    return true; // Substring is valid
}

void unsetTile(Tile *t) {
    t->state = EMPTY;
    t->letter = ' ';
}

void drawFrame() {
    BeginDrawing();

    ClearBackground(RAYWHITE);
    drawGameBoard();

    EndDrawing();
}



void checkSubstrings(const char* str, const int* indexes) {
    int len = strlen(str);
    int max_len = len < GRID_HEIGHT ? len : GRID_HEIGHT;

    for (int sub_len = max_len; sub_len >= 3; sub_len--) {
        for (int i = 0; i <= len - sub_len; i++) {
            char substr[sub_len + 1];
            strncpy(substr, str + i, sub_len);
            substr[sub_len] = '\0';


            if (checkWordViability(substr) && isValidSubstring(substr)) {
                if (searchWord(dictTrieRoot, substr)) {
                    int index_start = i;
                    int index_end = i + sub_len;
                    for (int x = index_start; x < index_end; x++) {
                        _grid[indexes[x]].state = EMPTY;
                        _grid[indexes[x]].letter = ' ';
                        printf("indexes[x] = %d\n", indexes[x]);
                        printf("%d = %d\n", indexes[x], _grid[indexes[x]].state);
                        // unsetTile(&_grid[indexes[x]]);
                    }
                    printf("found: %s\n", substr);
                    return;
                }
            }
        }
    }
}

void checkForWords(Tile* grid) {
    // Horizontal check
    for (int i = 0; i < GRID_HEIGHT * GRID_WIDTH; i += GRID_WIDTH) {
        struct {
            char letters[GRID_WIDTH];
            int indexes[GRID_WIDTH];
        } row;
        // char line[GRID_WIDTH];
        for (int j = 0; j < GRID_WIDTH; j++) {
            row.letters[j] = tolower(grid[i+j].letter);
            row.indexes[j] = i + j;
        }
        checkSubstrings(row.letters, row.indexes);
    }
    // Vertical check

    for (int i = 0; i < GRID_WIDTH; i += 1) {
        struct {
            char letters[GRID_HEIGHT];
            int indexes[GRID_HEIGHT];
        } col;
        // char column[GRID_HEIGHT];
        for (int j = 0; j < GRID_HEIGHT; j += 1) {
            int idx = j * GRID_WIDTH + i;
            col.letters[j] = tolower(grid[idx].letter);
            col.indexes[j] = idx;
            // column[j] = tolower(grid[idx].letter);
        }
        checkSubstrings(col.letters, col.indexes);
    }
}

void setPlayer(int i0, int i1, TileState state, PlayerRotation rotation) {
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
    _player.tiles[0]->state = state;
    _player.tiles[1]->state = state;
    _player.rotation = rotation;
}

bool movePlayer(int dir_x, int dir_y) {
    bool success = false;
    int i0 = _player.indexes[0] + dir_x + (dir_y * GRID_WIDTH);
    int i1 = _player.indexes[1] + dir_x + (dir_y * GRID_WIDTH);
    if (checkCollision(i0, _player.indexes[0]) &&
        checkCollision(i1, _player.indexes[1])) {

        setPlayer(i0, i1, FALLING, _player.rotation);
        success = true;
    }

    return success;
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
        setPlayer(i0, i1, FALLING, rot);
        return true;
    }
    return false;
}


void populateLetterPool(struct LetterPool *pool) {
    addLetter(pool, 'A', 6);
    addLetter(pool, 'B', 2);
    addLetter(pool, 'C', 3);
    addLetter(pool, 'D', 6);
    addLetter(pool, 'E', 10);
    addLetter(pool, 'F', 4);
    addLetter(pool, 'G', 4);
    addLetter(pool, 'H', 3);
    addLetter(pool, 'I', 8);
    addLetter(pool, 'J', 1);
    addLetter(pool, 'K', 2);
    addLetter(pool, 'L', 3);
    addLetter(pool, 'M', 2);
    addLetter(pool, 'N', 4);
    addLetter(pool, 'O', 10);
    addLetter(pool, 'P', 3);
    addLetter(pool, 'Q', 1);
    addLetter(pool, 'R', 3);
    addLetter(pool, 'S', 4);
    addLetter(pool, 'T', 3);
    addLetter(pool, 'U', 7);
    addLetter(pool, 'V', 1);
    addLetter(pool, 'W', 2);
    addLetter(pool, 'X', 1);
    addLetter(pool, 'Y', 2);
    addLetter(pool, 'Z', 1);
}

int main() {

    dictTrieRoot = createNode();
    constructTrie(dictTrieRoot, "./dictionary.txt");

    srand(time(NULL));
    initializeLetterPool(&pool);
    populateLetterPool(&pool);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Wordtris");
    InitAudioDevice();
    SetTargetFPS(60);
    initGame();

    _font = LoadFontEx("./Arialbd.TTF", TILE_SIZE, 0, 0);

    spawnPlayer();

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
                checkForWords(_grid);
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

    destroyLetterPool(&pool);
    destroyTrie(dictTrieRoot);

    UnloadFont(_font);
    CloseWindow();

    return 0;
}
