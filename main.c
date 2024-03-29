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

#define TILE_SIZE 72

#define SCREEN_WIDTH (GRID_WIDTH * TILE_SIZE) + (4 * TILE_SIZE)
#define SCREEN_HEIGHT (GRID_HEIGHT * TILE_SIZE)

typedef enum GameState {
    GAMEOVER,
    PLAYING,
    PAUSED,
} GameState;

enum Spin {
    CCW,
    CW,
} Spin;

typedef enum PlayerRotation {
    HORI_1,
    VERT_1,
    HORI_2,
    VERT_2,
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

static GameState _game_state;
static SoundEffects _sfx;
static Tile _grid[GRID_WIDTH * GRID_HEIGHT];
static Player _player;
static Font _font;
struct TrieNode *_dict_trie_root;
struct LetterPool _letter_pool;

void initGrid() {
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
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
    _game_state = PLAYING;

    initGrid();
    _player.gridIndices[0] = 1;
    _player.gridIndices[1] = 1;

    initSounds();
}

void drawBox(int startPosX, int startPosY, int width, int height, int lineThickness, Color borderColor) {
    DrawLineEx((Vector2){startPosX, startPosY}, (Vector2){startPosX + width, startPosY}, lineThickness, borderColor);
    DrawLineEx((Vector2){startPosX, startPosY}, (Vector2){startPosX, startPosY + height}, lineThickness, borderColor);
    DrawLineEx((Vector2){startPosX, startPosY + height}, (Vector2){startPosX + width, startPosY + height}, lineThickness, borderColor);
    DrawLineEx((Vector2){startPosX + width, startPosY}, (Vector2){startPosX + width, startPosY + height}, lineThickness, borderColor);
}

void drawBoxFilled(int startPosX, int startPosY, int width, int height, int lineThickness, Color borderColor, Color fillColor) {
    drawBox(startPosX, startPosY, width, height, lineThickness, borderColor);

    DrawRectangle(startPosX + (lineThickness / 2), startPosY + (lineThickness / 2), width - (lineThickness), height - (lineThickness), fillColor);
}

void drawGameBoard() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            int i = y * GRID_WIDTH + x;

            const int x_pos = x * TILE_SIZE;
            const int y_pos = y * TILE_SIZE;


            if (_grid[i].state == EMPTY) {
                drawBoxFilled(x_pos, y_pos, TILE_SIZE, TILE_SIZE, 2, GRAY, WHITE);
                continue;
            }

            drawBox(x_pos, y_pos, TILE_SIZE, TILE_SIZE, 2, GRAY);

            Vector2 letter_size = MeasureTextEx(_font, &_grid[i].letter, TILE_SIZE, 0);

            const int x_offset = (TILE_SIZE - letter_size.x) / 2;
            const int y_offset = (TILE_SIZE - letter_size.y) / 2;
            DrawTextEx(_font, &_grid[i].letter,
                       (Vector2){x_pos + x_offset, y_pos + y_offset},
                       TILE_SIZE, 0, BLACK);
        }
    }
}


void drawScore() {
    const int boardEndX = TILE_SIZE * GRID_WIDTH;
    const int y = 32;
    drawBoxFilled(boardEndX + 16, y, 256, 32, 4, ORANGE, BLUE);

    DrawText("Score: ", boardEndX + 20, y + 4, 32, BLACK);
}

void clear(int *indices, size_t num) {
    for (int i = 0; i < num; i++) {
        _grid[indices[i]].letter = ' ';
        _grid[indices[i]].state =  EMPTY;
    }
}

void setPlayer(int i0, int i1, TileState state, PlayerRotation rotation) {
    char l_ch = _grid[_player.gridIndices[0]].letter;
    char r_ch = _grid[_player.gridIndices[1]].letter;

    clear(_player.gridIndices, 2);

    _player.gridIndices[0] = i0;
    _player.gridIndices[1] = i1;
    _player.rotation = rotation;

    _grid[i0].state = state;
    _grid[i1].state = state;
    _grid[i0].letter = l_ch;
    _grid[i1].letter = r_ch;
}

void spawnPlayer() {
    int l_idx = (GRID_WIDTH / 2) - 1;
    int r_idx = l_idx + 1;

    const bool can_spawn = _grid[l_idx].state == EMPTY && _grid[r_idx].state == EMPTY;

    if (can_spawn) {
        _player.rotation = HORI_1;

        _grid[l_idx].state = PLAYER;
        _grid[l_idx].letter = getRandomLetter(&_letter_pool);
        _player.gridIndices[0] = l_idx;

        _grid[r_idx].state = PLAYER;
        _grid[r_idx].letter = getRandomLetter(&_letter_pool);
        _player.gridIndices[1] = r_idx;

    } else {
        _game_state = GAMEOVER;
    }
}

// word is viable if it contains and vowel and a consonant
static bool checkWordViability(char* word) {
    bool contains_vowel = false;
    bool contains_consonant = false;

    for (int i = 0; i < strlen(word); i++) {
        if (word[i] == 'a' || word[i] == 'e' || word[i] == 'i' || word[i] == 'o' || word[i] == 'u') {
            contains_vowel = true;
        } else {
            contains_consonant = true;
        }
    }

    return contains_vowel && contains_consonant;
}

// check that a substring is the minimum length and contains no spaces.
static const bool checkStringValidity(const char* substring) {
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

// check that the tile at index <from_index> can move to the tile at position <to_index>
static const bool checkCollision(int to_index, int from_index) {

    if (_grid[to_index].state == STATIC) {
        return false;
    }

    const bool moving_down = (to_index == from_index + GRID_WIDTH);
    const bool moving_right = !moving_down && (to_index > from_index);
    const bool moving_up = (to_index == from_index - GRID_WIDTH);
    const bool moving_left = !moving_up && (to_index < from_index);

    bool collided = false;

    if (moving_down) {
        collided = from_index > (GRID_WIDTH * (GRID_HEIGHT - 1) - 1);
    }
    else if (moving_right) {
        collided = from_index % GRID_WIDTH == GRID_WIDTH - 1;
    }
    else if (moving_up) {
        collided = from_index < GRID_WIDTH;
    }
    else if (moving_left) {
        collided = from_index % GRID_WIDTH == 0;
    }

    return !collided;
}


static void drawFrame() {
    BeginDrawing();

    ClearBackground(RAYWHITE);
    drawGameBoard();
    drawScore();

    EndDrawing();
}



// check for words in a given row of characters
static bool checkSubstrings(const char* str, const int* position) {
    int len = strlen(str);
    int maxLen = len < GRID_HEIGHT ? len : GRID_HEIGHT;

    for (int subLen = maxLen; subLen >= 3; subLen--) {
        for (int i = 0; i <= len - subLen; i++) {
            char substr[subLen + 1];
            strncpy(substr, str + i, subLen);
            substr[subLen] = '\0';

            if (checkWordViability(substr) && checkStringValidity(substr)) {
                if (searchWord(_dict_trie_root, substr)) {
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
    bool found_word = false;
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

        found_word = checkSubstrings(row.letters, row.indexes);
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

        found_word = found_word || checkSubstrings(col.letters, col.indexes);
    }

    return found_word;
}

// update the y-positions of all falling tiles
void advanceFallingTiles() {

    // falling tiles are advanced starting from bottom to top, right to left.
    int next_to_last_row_end = ((GRID_WIDTH * GRID_HEIGHT) - GRID_WIDTH) - 1;
    for (int i = next_to_last_row_end; i > -1; i--) {
        if (_grid[i].state == FALLING && _grid[i + GRID_WIDTH].state == EMPTY) {

            _grid[i + GRID_WIDTH] = _grid[i];
            _grid[i].letter = ' ';
            _grid[i].state = EMPTY;

            if (i + GRID_WIDTH > next_to_last_row_end) {
                _grid[i + GRID_WIDTH].state = STATIC;
            }
        }
    }
}

// try to move player on x-axis by <dir_x> and on y-axis by <dir_y>
// return true IFF successful
bool movePlayer(int dir_x, int dir_y) {
    int i0 = _player.gridIndices[0] + dir_x + (dir_y * GRID_WIDTH);
    int i1 = _player.gridIndices[1] + dir_x + (dir_y * GRID_WIDTH);

    if (checkCollision(i0, _player.gridIndices[0]) &&
        checkCollision(i1, _player.gridIndices[1])) {

        setPlayer(i0, i1, PLAYER, _player.rotation);
        return true;
    }

    return false;
}

// try to rotate the player's tiles either clockwise or counter clockwise
// return true IFF successful
bool rotatePlayer(enum Spin spin) {
    int i0 = _player.gridIndices[0];
    int i1 = _player.gridIndices[1];

    PlayerRotation playerRotaton = _player.rotation;

    if ((playerRotaton == HORI_1 && spin == CCW) || (playerRotaton == VERT_2 && spin == CW)) {
        i0 += -GRID_WIDTH;
        i1 += -1;
        playerRotaton = spin == CCW ? VERT_1 : HORI_2;
    }
    else if ((playerRotaton == VERT_1 && spin == CCW) || (playerRotaton == HORI_1 && spin == CW)) {
        i0 += 1;
        i1 += -GRID_WIDTH;
        playerRotaton = spin == CCW ? HORI_2 : VERT_2;
    }
    else if ((playerRotaton == HORI_2 && spin == CCW) || (playerRotaton == VERT_1 && spin == CW)) {
        i0 += GRID_WIDTH;
        i1 += 1;
        playerRotaton = spin == CCW ? VERT_2 : HORI_1;
    }
    else if ((playerRotaton == VERT_2 && spin == CCW) || (playerRotaton == HORI_2 && spin == CW)) {
        i0 += -1;
        i1 += GRID_WIDTH;
        playerRotaton = spin == CCW ? HORI_1 : VERT_1;
    }

    if (checkCollision(i0, _player.gridIndices[0]) && checkCollision(i1, _player.gridIndices[1])) {
        // rotaton success
        setPlayer(i0, i1, FALLING, playerRotaton);
        return true;
    }

    // failed to rotate
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

void setAllToStatic() {
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
        if (_grid[i].state == FALLING || _grid[i].state == PLAYER) {
            _grid[i].state = STATIC;
        }
    }
}

int main() {

    _dict_trie_root = createNode();
    constructTrie(_dict_trie_root, "./dictionary.txt");

    srand(time(NULL));
    initializeLetterPool(&_letter_pool);
    populateLetterPool(&_letter_pool);

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
                setAllToStatic();

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
                rotatePlayer(CCW);
                break;
            case (KEY_J):
                rotatePlayer(CW);
                break;
            case (KEY_P):
                printBoard();
                break;
        };

        if (_game_state == GAMEOVER) {
            break;
        }
        drawFrame();
    }

    destroyLetterPool(&_letter_pool);
    destroyTrie(_dict_trie_root);

    UnloadFont(_font);
    CloseWindow();

    return 0;
}
