#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>

#include "words.h"


#ifdef PLATFORM_WEB
    extern void print_word(char *word);
    extern void raylib_js_set_entry(void (*entry)(void));
#endif
#define MAX_ATTEMPTS             6
#define MAX_GAME_TIMER           1.0f
#define USER_GUESS_COLORING_TIME 1.0f
#define USER_GUESS_APPER_TIME    1.0f
#define MAX_CURSOR_TIMER         1.0f

#define BACKGROUND_COLOR           CLITERAL(Color) { 0x18, 0x18, 0x18, 0xFF }
#define LETTER_BOX_COLOR           CLITERAL(Color) { 0x4E, 0x80, 0x98, 0xFF }
#define LETTER_COLOR               ColorFromHSV(0, 0.0f, 0.95f)
#define GREEN_BOX_COLOR            CLITERAL(Color) { 0x23, 0xEF, 0x3C, 0xFF }
#define WRONG_BOX_COLOR            ColorFromHSV(0, 0.0f, 0.25f)
#define YELLOW_BOX_COLOR           ColorFromHSV(45, 1.0f, 0.85f)
#define CURSOR_COLOR               LETTER_COLOR
#define DEFAULT_KEYBOARD_KEY_COLOR ColorFromHSV(0, 0.0f, 0.45f)
#define WRONG_KEYBOARD_KEY_COLOR   ColorFromHSV(0, 0.0f, 0.25f)

#ifdef PLATFORM_WEB
#   define FONT_SIZE              50
#   define PLATFORM_SCREEN_WIDTH  0
#   define PLATFORM_SCREEN_HEIGHT 0
#else
#   define FONT_SIZE              65
#   define PLATFORM_SCREEN_WIDTH  ((FIELD_WIDTH + 200) * 1.5) 
#   define PLATFORM_SCREEN_HEIGHT ((FIELD_HEIGHT + KEYBOARD_HEIGHT) * 1.25)
#endif

#define LETTER_FONT_FILEPATH  "./assets/fonts/Oswald-Bold.ttf"
#define LETTER_FONT_SIZE      FONT_SIZE
#define LETTER_BOX_SIZE       75
#define LETTER_BOX_GAP        10
#define FIELD_WIDTH           ((WORD_LEN * LETTER_BOX_SIZE) + ((WORD_LEN - 1) * LETTER_BOX_GAP))
#define FIELD_HEIGHT          ((MAX_ATTEMPTS * LETTER_BOX_SIZE) + ((MAX_ATTEMPTS - 1) * LETTER_BOX_GAP))
#define FIELD_MARGIN          25
#define CURSOR_WIDTH          (LETTER_BOX_SIZE*0.1f)
#define CURSOR_HEIGHT         (LETTER_BOX_SIZE*0.75f)
#define KEYBOARD_KEY_SIZE     60
#define KEYBOARD_GAP          15
#define KEYBOARD_FONT_SIZE    (FONT_SIZE - 10)
#define KEYBOARD_HEIGHT       (KEYBOARD_KEY_SIZE * 3 + KEYBOARD_GAP * 2)
#define SCREEN_WIDTH          PLATFORM_SCREEN_WIDTH 
#define SCREEN_HEIGHT         PLATFORM_SCREEN_HEIGHT 


typedef struct Attempt {
    char word[WORD_LEN];
    Color colors[WORD_LEN];
} Attempt;

typedef enum State {
    STATE_PLAY = 0,
    STATE_USER_GUESS_COLORING,
    STATE_USER_GUESS_APPEAR,
    STATE_WIN,
    STATE_LOSE,
} State;

typedef struct Game {
    char *word;                     // Hidden word
    int attempt;                    // Current attempt
    Attempt attempts[MAX_ATTEMPTS]; // Previous attemps
    char current_guess[WORD_LEN];   // Current user guess buffer
    int current_guess_len;          // Current user guess buffer length
    State state;                    // Game state
    float time;                     // Game time
    Color keyboard[3][12];          // Keyboard state
} Game;

#define KEYBOARD_ROWS 3
char keyboard_keys[KEYBOARD_ROWS][12] = {
    "QWERTYUIOP",
    "ASDFGHJKL",
    "ZXCVBNM"
};
static Game game = {0};
static Font font = {0};

static float cursor_timer = 0.0f;

void find_keyboard_key(char chr, int *i, int *j)
{
    for (int row = 0; row < KEYBOARD_ROWS; ++row) {
        for (int key = 0; keyboard_keys[row][key]; ++key) {
            if (keyboard_keys[row][key] == chr) {
                *i = row;
                *j = key;
            }
        }
    }
}


void restart_game(void)
{
    for (int i = 0; i < KEYBOARD_ROWS; ++i) {
        for (int j = 0; j < 12; ++j) {
            game.keyboard[i][j] = DEFAULT_KEYBOARD_KEY_COLOR;
        }
    }
    game.word = words[rand() % WORDS];
#ifdef DEBUG
#   ifdef PLATFORM_WEB
        print_word(game.word);
#   else
        TraceLog(LOG_ERROR, "Word is %s", game.word);
#   endif
#endif
    game.attempt = 0;
    game.current_guess_len = 0;
    for (int i = 0; i < WORD_LEN; ++i) {
        game.current_guess[i] = '\0';
    }
    game.state = STATE_PLAY;
}


void init_game(void)
{
    restart_game();
}


void draw_text(char *text, int x, int y, int font_size, Color color)
{
    DrawTextEx(font, text, CLITERAL(Vector2){x, y}, font_size, 1, color);
}


void draw_char(char chr, int char_box_size, int char_box_x, int char_box_y, int font_size)
{
    char text[10] = {0};
    text[0] = chr;
    Vector2 text_size = MeasureTextEx(font, text, font_size, 1);
    int x = (char_box_x + char_box_size/2) - text_size.x/2;
    int y = (char_box_y + char_box_size/2) - text_size.y/2;
    draw_text(text, x, y, font_size, LETTER_COLOR);
}

#define draw_letter(chr, x, y) draw_char((chr), LETTER_BOX_SIZE, (x), (y), LETTER_FONT_SIZE)


void draw_attempts(float t)
{
    int start_x = GetScreenWidth()/2 - FIELD_WIDTH/2;
    int start_y = GetScreenHeight()/2 - (FIELD_HEIGHT+FIELD_MARGIN*2+KEYBOARD_HEIGHT)/2;

    for (int i = 0; i < game.attempt; ++i) {
        int y = start_y + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * i;
        for (int j = 0; j < WORD_LEN; ++j) {
            int x = start_x + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * j;
            Color color = game.attempts[i].colors[j];
            if (i == (game.attempt - 1)) {
                color = ColorLerp(LETTER_BOX_COLOR, color, t);
            }
            DrawRectangle(x, y, LETTER_BOX_SIZE, LETTER_BOX_SIZE, color);
            draw_letter(game.attempts[i].word[j], x, y);
        }
    }
    return;
}

bool is_colors_equals(Color c1, Color c2)
{
    return (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b);
}


State make_attempt(void)
{
    State state = STATE_USER_GUESS_COLORING;

    if (game.attempt == MAX_ATTEMPTS) return STATE_PLAY;

#ifndef DEBUG
    /* Check is word exists */
    if (game.current_guess_len < WORD_LEN) return STATE_PLAY;

    bool is_word_exists = false;
    for (int i = 0; i < WORDS; ++i) {
        bool is_same_words = true;
        for (int j = 0; j < WORD_LEN; ++j) {
            if (words[i][j] != game.current_guess[j]) {
                is_same_words = false;
            }
        }
        if (is_same_words) {
            is_word_exists = true;
            break;
        }
    }
    if (!is_word_exists) return STATE_PLAY;
#endif

    /* Check for win */
    bool is_win = true;
    for (int i = 0; i < WORD_LEN; ++i) {
        if (game.current_guess[i] != game.word[i]) is_win = false;
    }
    if (is_win) {
        state = STATE_WIN;
    }

    /* Copy user guess to previous attempts */
    for (int i = 0; i < WORD_LEN; ++i) {
        game.attempts[game.attempt].word[i] = game.current_guess[i];
        game.current_guess[i] = 0;
    }

    /* Calculate colors for letters in guess */

    char word_buffer[WORD_LEN] = {0};
    char guess_buffer[WORD_LEN] = {0};
    for (int i = 0; i < WORD_LEN; ++i) {
        word_buffer[i] = game.word[i];
        guess_buffer[i] = game.attempts[game.attempt].word[i];
    }

    int row, col;
    for (int i = 0; i < WORD_LEN; ++i) {
        find_keyboard_key(guess_buffer[i], &row, &col);
        if (guess_buffer[i] == word_buffer[i]) {
            game.keyboard[row][col] = GREEN_BOX_COLOR;
            game.attempts[game.attempt].colors[i] = GREEN_BOX_COLOR;
            guess_buffer[i] = '\0';
            word_buffer[i] = '\0';
        } else {
            game.attempts[game.attempt].colors[i] = WRONG_BOX_COLOR;
            if (is_colors_equals(game.keyboard[row][col], DEFAULT_KEYBOARD_KEY_COLOR))
                game.keyboard[row][col] = WRONG_KEYBOARD_KEY_COLOR;
        }
    }

    for (int i = 0; i < WORD_LEN; ++i) {
        if (word_buffer[i] == '\n') continue;
        for (int j = 0; j < WORD_LEN; ++j) {
            if (guess_buffer[j] == '\0') continue;
            if (word_buffer[i] == guess_buffer[j]) {
                find_keyboard_key(guess_buffer[j], &row, &col);
                if (!is_colors_equals(game.keyboard[row][col], GREEN_BOX_COLOR))
                    game.keyboard[row][col] = YELLOW_BOX_COLOR;
                game.attempts[game.attempt].colors[j] = YELLOW_BOX_COLOR;
                guess_buffer[j] = '\0';
                word_buffer[i] = '\0';
                break;
            }
        }
    }

    /* The end of calculates */

    game.current_guess_len = 0;
    game.attempt += 1;
    if (game.attempt == MAX_ATTEMPTS && state != STATE_WIN) {
        state = STATE_LOSE;
    }

    return state;
}


void process_input(void)
{
    if (IsKeyPressed(KEY_BACKSPACE) && game.current_guess_len > 0) {
        if (game.current_guess_len < WORD_LEN) {
            game.current_guess[game.current_guess_len] = 0;
        }
        --game.current_guess_len;
    } else if (IsKeyPressed(KEY_ENTER)) {
        State state = make_attempt();
        if (state == STATE_USER_GUESS_COLORING) {
            game.time = USER_GUESS_COLORING_TIME;
        }
        game.state = state;
        return;
    }

    if (game.current_guess_len >= WORD_LEN) return;

    for (int key = KEY_A; key <= KEY_Z; ++key) {
        if (IsKeyPressed(key)) {
            game.current_guess[game.current_guess_len] = key;
            ++game.current_guess_len;
        }
    }
}

void draw_cursor(int letter_box_x, int letter_box_y, float time)
{
    cursor_timer += GetFrameTime();
    if (cursor_timer >= 0.25f) {
        int x = (letter_box_x + LETTER_BOX_SIZE/2) - CURSOR_WIDTH/2;
        int y = (letter_box_y + LETTER_BOX_SIZE/2) - CURSOR_HEIGHT/2;
        DrawRectangle(x, y, CURSOR_WIDTH, CURSOR_HEIGHT, ColorAlpha(CURSOR_COLOR, time));
    }

    if (cursor_timer >= MAX_CURSOR_TIMER) {
        cursor_timer = 0.0f;
    }
}

void draw_user_guess(float t)
{
    if (game.attempt >= MAX_ATTEMPTS) return;

    int start_x = GetScreenWidth()/2 - FIELD_WIDTH/2;
    int start_y = GetScreenHeight()/2 - (FIELD_HEIGHT+FIELD_MARGIN*2+KEYBOARD_HEIGHT)/2;

    int min_y = start_y + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * (game.attempt - 1);
    int max_y = start_y + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * game.attempt;
    int y = Lerp(min_y, max_y, t);

    for (int c = 0; c < WORD_LEN; ++c) {
        int x = start_x + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * c;
        DrawRectangle(x, y, LETTER_BOX_SIZE, LETTER_BOX_SIZE, ColorAlpha(LETTER_BOX_COLOR, t));
        if (c < game.current_guess_len) draw_letter(game.current_guess[c], x, y);
        if (c == game.current_guess_len) draw_cursor(x, y, t);
    }

    return;
}

void draw_game_play(void)
{
    draw_user_guess(1.0f);
    draw_attempts(1.0f);
    process_input();
}

void draw_game_user_guess_coloring(void)
{
    float t = (1.0f - game.time/(float)USER_GUESS_COLORING_TIME);
    draw_attempts(t);
}

void draw_game_user_guess_appear(void)
{
    float t = (1.0f - game.time/(float)USER_GUESS_APPER_TIME);
    draw_user_guess(t);
    draw_attempts(1.0f);
}

void draw_game_win(void)
{
    draw_attempts(1.0f);
}

void draw_game_lose(void)
{
    draw_attempts(1.0f);
    draw_text(game.word, 0, 0, LETTER_FONT_SIZE, WHITE);
}

size_t strlen(const char *string)
{
    if (string == NULL) return 0;

    int size = 0;
    while (string[0] != '\0') {
        ++size;
        ++string;
    }
    return size;
}

void draw_keyboard(void)
{
    for (int i = 0; i < 3; ++i) {
        int start_y = GetScreenHeight()/2 - (FIELD_HEIGHT + FIELD_MARGIN*2 + KEYBOARD_HEIGHT)/2 + FIELD_HEIGHT + FIELD_MARGIN*2;
        int y = start_y + i * KEYBOARD_KEY_SIZE + i * KEYBOARD_GAP;
        for (int j = 0; keyboard_keys[i][j]; ++j) {
            int len = strlen(keyboard_keys[i]);
            int row_width = len * KEYBOARD_KEY_SIZE + ((len - 1) * KEYBOARD_GAP);
            int start_x = GetScreenWidth()/2 - row_width/2;
            int x = start_x + (j * KEYBOARD_KEY_SIZE + j * KEYBOARD_GAP);
            Color color = game.keyboard[i][j];
            DrawRectangle(x, y, KEYBOARD_KEY_SIZE, KEYBOARD_KEY_SIZE, color);
            draw_char(keyboard_keys[i][j], KEYBOARD_KEY_SIZE, x, y, KEYBOARD_FONT_SIZE);
        }
    }
}

void draw_game_state(void)
{
    draw_keyboard();

    if (game.time > 0.0f) game.time -= GetFrameTime();

    switch (game.state) {
        case STATE_PLAY: {
            draw_game_play();
        } break;
        case STATE_USER_GUESS_COLORING: {
            draw_game_user_guess_coloring();
            if (game.time <= 0.0f) {
                game.state = STATE_USER_GUESS_APPEAR;
                game.time = USER_GUESS_APPER_TIME;
            }
        } break;
        case STATE_USER_GUESS_APPEAR: {
            draw_game_user_guess_appear();
            if (game.time <= 0.0f) game.state = STATE_PLAY;
        } break;
        case STATE_WIN: {
            draw_game_win();
        } break;
        case STATE_LOSE: {
            draw_game_lose();
        } break;
    }

    return;
}


void game_frame(void)
{
    BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_R)) {
            restart_game();
        } else {
            draw_game_state();
        }
    EndDrawing();
}


int main(void)
{
    srand(time(NULL));
    init_game();

    SetTraceLogLevel(LOG_WARNING);
    SetTargetFPS(60);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Wordle");

    font = LoadFontEx(LETTER_FONT_FILEPATH, LETTER_FONT_SIZE, NULL, 0);

#ifdef PLATFORM_WEB
    raylib_js_set_entry(game_frame);
#else
    while (!WindowShouldClose()) {
        game_frame();
    }
    CloseWindow();
#endif
    return 0;
}
