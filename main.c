#include "raylib.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pool.h"
#include "trie.h"

#define GRID_WIDTH 6
#define GRID_HEIGHT 10
#define NUM_TILES (GRID_WIDTH * GRID_HEIGHT)

#define TILE_SIZE 72

#define SCREEN_WIDTH (GRID_WIDTH * TILE_SIZE) + (4 * TILE_SIZE)
#define SCREEN_HEIGHT (GRID_HEIGHT * TILE_SIZE)

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

typedef enum TileState {
    EMPTY,
    STATIC,
    PLAYER,
    FALLING,
    DESTROY,
} TileState;

typedef struct Tile {
    TileState state;
    char letter;
} Tile;

typedef struct Player {
    int gridIndices[2];
    PlayerRotation rotation;
} Player;

typedef struct SoundEffects {
    Sound move_success;
    Sound move_failure;
    Sound word_found;
} SoundEffects;

static GameState _state;
static SoundEffects _sfx;
static Tile _grid[GRID_WIDTH * GRID_HEIGHT];
static Player _player;
static Font _font;

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
    _player.gridIndices[0] = 1;
    _player.gridIndices[1] = 1;

    initSounds();
}

void drawGameBoard() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            int i = y * GRID_WIDTH + x;

            const int xPos = x * TILE_SIZE;
            const int yPos = y * TILE_SIZE;

            if (_grid[i].state == EMPTY) {
                DrawRectangle(xPos, yPos, TILE_SIZE, TILE_SIZE, GRAY);
                DrawRectangle(xPos + 1, yPos + 1, TILE_SIZE - 2, TILE_SIZE - 2, WHITE);
                continue;
            } else if (_grid[i].letter == ' ') {
                //printf("%d\n", i);
            }

            DrawRectangle(xPos, yPos, TILE_SIZE, TILE_SIZE, BLACK);
            DrawRectangle(xPos + 1, yPos + 1, TILE_SIZE - 2, TILE_SIZE - 2, LIGHTGRAY);

            Vector2 letterSize = MeasureTextEx(_font, &_grid[i].letter, TILE_SIZE, 0);

            const int xOffset = (TILE_SIZE - letterSize.x) / 2;
            const int yOffset = (TILE_SIZE - letterSize.y) / 2;
            DrawTextEx(_font, &_grid[i].letter,
                       (Vector2){xPos + xOffset, yPos + yOffset},
                       TILE_SIZE, 0, BLACK);
        }
    }
}

void clear(int *indices, size_t num) {
    for (int i = 0; i < num; i++) {
        _grid[indices[i]].letter = ' ';
        _grid[indices[i]].state =  EMPTY;
    }
}

void setPlayer(int i0, int i1, TileState state, PlayerRotation rotation) {
    printf("setPlayer\n");
    char l_c = _grid[_player.gridIndices[0]].letter;
    char r_c = _grid[_player.gridIndices[1]].letter;

    clear(_player.gridIndices, 2);

    _player.gridIndices[0] = i0;
    _player.gridIndices[1] = i1;
    _player.rotation = rotation;

    _grid[i0].state = state;
    _grid[i1].state = state;
    _grid[i0].letter = l_c;
    _grid[i1].letter = r_c;
}


void spawnPlayer() {
    // set current player block to static
    int i0, i1 = *(_player.gridIndices);

    int l_idx = (GRID_WIDTH / 2) - 1;
    int r_idx = l_idx + 1;

    const bool can_spawn = _grid[l_idx].state == EMPTY && _grid[r_idx].state == EMPTY;

    if (can_spawn) {

        //setPlayer(l_idx, r_idx, FALLING, ROT_0);
        _player.rotation = ROT_0;

        _grid[l_idx].state = PLAYER;
        _grid[l_idx].letter = getRandomLetter(&pool);

        _grid[r_idx].state = PLAYER;
        _grid[r_idx].letter = getRandomLetter(&pool);

        _player.gridIndices[0] = l_idx;
        _player.gridIndices[1] = r_idx;

    } else {
        _state = GAMEOVER;
    }
}

static bool checkWordViability(char* word) {
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

static const bool checkSubstringValidity(const char* substring) {
    const int length = strlen(substring);

    if (length < 3) {
        return false;
    }

    for (int i = 0; i < length; i++) {
        if (substring[i] == ' ') {
            return false;
        }
    }

    return true;
}

static const bool checkCollision(int index, int tileIndex) {

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


static void drawFrame() {
    BeginDrawing();

    ClearBackground(RAYWHITE);
    drawGameBoard();

    EndDrawing();
}



static bool checkSubstrings(const char* str, const int* position) {
    int len = strlen(str);
    int maxLen = len < GRID_HEIGHT ? len : GRID_HEIGHT;

    for (int subLen = maxLen; subLen >= 3; subLen--) {
        for (int i = 0; i <= len - subLen; i++) {
            char substr[subLen + 1];
            strncpy(substr, str + i, subLen);
            substr[subLen] = '\0';

            if (checkWordViability(substr) && checkSubstringValidity(substr)) {
                if (searchWord(dictTrieRoot, substr)) {
                    int indexStart = i;
                    int indexEnd = i + subLen;

                    for (int x = indexStart; x < indexEnd; x++) {
                        _grid[position[x]].state = EMPTY;
                        _grid[position[x]].letter = ' ';
                    }

                    return true;
                }
            }
        }
    }

    return false;
}

static bool checkForWords(Tile* grid) {
    bool wordsFound = false;
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

        wordsFound = checkSubstrings(row.letters, row.indexes);
    }

    // Vertical check
    for (int i = 0; i < GRID_WIDTH; i += 1) {
        struct {
            char letters[GRID_HEIGHT];
            int indexes[GRID_HEIGHT];
        } col;

        for (int j = 0; j < GRID_HEIGHT; j += 1) {
            int idx = j * GRID_WIDTH + i;
            col.letters[j] = tolower(grid[idx].letter);
            col.indexes[j] = idx;
        }

        wordsFound = wordsFound || checkSubstrings(col.letters, col.indexes);
    }

    return wordsFound;
}

//void clear(int *indices, size_t num) {
//    for (int i = 0; i < num; i++) {
//        _grid[indices[i]].letter =  ' ';
//        _grid[indices[i]].state =  ' ';
//    }
//}

bool moveTile() {
    bool success = false;
    for (int i = ((GRID_WIDTH * GRID_HEIGHT) - GRID_WIDTH) - 1; i > -1; i--) {
        if (_grid[i].state == FALLING && _grid[i + GRID_WIDTH].state == EMPTY) {
            printf("Swapped\n");

            _grid[i + GRID_WIDTH] = _grid[i];
            _grid[i].letter = ' ';
            _grid[i].state = EMPTY;

            if (i + GRID_WIDTH > (((GRID_HEIGHT * GRID_WIDTH) - GRID_WIDTH) - 1)) {
                _grid[i + GRID_WIDTH].state = STATIC;
            }
        }
        if (_grid[i].letter == ' ') {
            _grid[i].state = EMPTY;
        }
    }

    return success;
}

bool movePlayer(int dir_x, int dir_y) {
    bool success = false;
    int i0 = _player.gridIndices[0] + dir_x + (dir_y * GRID_WIDTH);
    int i1 = _player.gridIndices[1] + dir_x + (dir_y * GRID_WIDTH);

    if (checkCollision(i0, _player.gridIndices[0]) &&
        checkCollision(i1, _player.gridIndices[1])) {

        setPlayer(i0, i1, PLAYER, _player.rotation);
        success = true;
    }

    return success;
}

bool rotatePlayer(Spin spin) {
    int i0 = _player.gridIndices[0];
    int i1 = _player.gridIndices[1];

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

    if (checkCollision(i0, _player.gridIndices[0]) &&
        checkCollision(i1, _player.gridIndices[1])) {
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

void printBoard() {
    for (int i = 0; i < GRID_HEIGHT; i++) {
        for (int j = 0; j < GRID_WIDTH; j++) {
            printf("%d", _grid[i * GRID_WIDTH + j].state);
        }
        printf("\n");
    }
}

void solidify() {
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
        if (_grid[i].state == FALLING || _grid[i].state == PLAYER) {
            _grid[i].state = STATIC;
        }
    }
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
                solidify();

                PlaySound(_sfx.move_failure);

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
            case (KEY_P):
                printBoard();
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
