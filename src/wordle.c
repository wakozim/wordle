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
#define MAX_ATTEMPTS                 6
#define MAX_RESTART_TIMER            0.5f
#define MAX_KEY_TIMER                0.25f
#define MAX_USER_GUESS_CORRECT       1.0f
#define MAX_NON_EXISTENT_WORD_TIMER  0.50f
#define MAX_KEYBOARD_TIMER           0.4f
#define USER_GUESS_COLORING_TIME     0.5f
#define USER_GUESS_APPER_TIME        0.25f
#define MAX_CURSOR_TIMER             1.0f

#define BACKGROUND_COLOR           ColorFromHSV(0, 0.0f, 0.09f)
#define LETTER_BOX_COLOR           ColorFromHSV(199, 0.48f, 0.59f)
#define LETTER_COLOR               ColorFromHSV(0, 0.0f, 0.95f)
#define GREEN_BOX_COLOR            ColorFromHSV(127, 0.7f, 0.7f)
#define WRONG_BOX_COLOR            ColorFromHSV(0, 0.0f, 0.25f)
#define YELLOW_BOX_COLOR           ColorFromHSV(45, 0.9f, 0.8f)
#define CURSOR_COLOR               LETTER_COLOR
#define DEFAULT_KEYBOARD_KEY_COLOR ColorFromHSV(199, 0.15f, 0.45f)
#define PRESSED_KEYBOARD_KEY_COLOR ColorBrightness(DEFAULT_KEYBOARD_KEY_COLOR, 0.5f)
#define WRONG_KEYBOARD_KEY_COLOR   ColorFromHSV(0, 0.0f, 0.25f)
#define LOSE_BOX_COLOR             ColorFromHSV(0, 0.0f, 0.35f)

#ifdef PLATFORM_WEB
#   define FONT_SIZE              45
#   define PLATFORM_SCREEN_WIDTH  0
#   define PLATFORM_SCREEN_HEIGHT 0
#else
#   define FONT_SIZE              65
#   define PLATFORM_SCREEN_WIDTH  ((FIELD_WIDTH + 200) * 1.5)
#   define PLATFORM_SCREEN_HEIGHT ((FIELD_HEIGHT + KEYBOARD_HEIGHT) * 1.25)
#endif

#define LETTER_FONT_FILEPATH  "./assets/fonts/Oswald-Bold.ttf"
#define LETTER_FONT_SIZE      FONT_SIZE
#define LETTER_BOX_SIZE       72
#define LETTER_BOX_GAP        10
#define FIELD_WIDTH           ((WORD_LEN * LETTER_BOX_SIZE) + ((WORD_LEN - 1) * LETTER_BOX_GAP))
#define FIELD_HEIGHT          ((MAX_ATTEMPTS * LETTER_BOX_SIZE) + ((MAX_ATTEMPTS - 1) * LETTER_BOX_GAP))
#define FIELD_MARGIN          25
#define CURSOR_WIDTH          (LETTER_BOX_SIZE*0.1f)
#define CURSOR_HEIGHT         (LETTER_BOX_SIZE*0.75f)
#define KEYBOARD_KEY_SIZE     55
#define KEYBOARD_GAP          10
#define KEYBOARD_FONT_SIZE    (FONT_SIZE - 15)
#define KEYBOARD_HEIGHT       (KEYBOARD_KEY_SIZE * 3 + KEYBOARD_GAP * 2)
#define SCREEN_WIDTH          PLATFORM_SCREEN_WIDTH
#define SCREEN_HEIGHT         PLATFORM_SCREEN_HEIGHT


typedef struct Attempt {
    char word[WORD_LEN];
    Color colors[WORD_LEN];
} Attempt;

typedef enum State {
    STATE_PLAY = 0,
    STATE_NON_EXISTENT_WORD,
    STATE_USER_GUESS_CORRECT,
    STATE_USER_GUESS_COLORING,
    STATE_USER_GUESS_APPEAR,
    STATE_WIN,
    STATE_LOSE,
    STATE_RESTART_FADEIN,
    STATE_RESTART_FADEOUT,
} State;

typedef struct Key {
    Color color;
    float time;
} Key;

typedef struct Char {
    char chr;
    float time;
} Char;

typedef struct Game {
    char *word;                     // Hidden word
    int attempt;                    // Current attempt
    Attempt attempts[MAX_ATTEMPTS]; // Previous attemps
    Char current_guess[WORD_LEN];   // Current user guess buffer
    int current_guess_len;          // Current user guess buffer length
    State state;                    // Game state
    float time;                     // Game time
    Key keyboard[3][12];            // Keyboard state
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

bool find_keyboard_key(char chr, int *i, int *j)
{
    for (int row = 0; row < KEYBOARD_ROWS; ++row) {
        for (int key = 0; keyboard_keys[row][key]; ++key) {
            if (keyboard_keys[row][key] == chr) {
                *i = row;
                *j = key;
                return true;
            }
        }
    }
    return false;
}


void restart_game(void)
{
    for (int i = 0; i < KEYBOARD_ROWS; ++i) {
        for (int j = 0; j < 12; ++j) {
            game.keyboard[i][j].color = DEFAULT_KEYBOARD_KEY_COLOR;
            game.keyboard[i][j].time = 0.0f;
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
        game.current_guess[i].chr = '\0';
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
    char text[2] = {0};
    text[0] = chr;
    text[1] = '\0';
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
        int row_y = start_y + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * i;
        for (int j = 0; j < WORD_LEN; ++j) {
            int x = start_x + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * j;
            int y = row_y;
            Color color = game.attempts[i].colors[j];
            if (i == (game.attempt - 1)) {
                color = ColorLerp(LETTER_BOX_COLOR, color, t);
            }
            if (game.state == STATE_USER_GUESS_CORRECT && (i == game.attempt - 1)) {
                float amount = 1.0f - (game.time/MAX_USER_GUESS_CORRECT);
                y = Lerp(y, y+5, sinf(amount*6*PI+j));
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
            if (words[i][j] != game.current_guess[j].chr) {
                is_same_words = false;
            }
        }
        if (is_same_words) {
            is_word_exists = true;
            break;
        }
    }
    if (!is_word_exists) return STATE_NON_EXISTENT_WORD;
#endif

    /* Check for win */
    bool is_win = true;
    for (int i = 0; i < WORD_LEN; ++i) {
        if (game.current_guess[i].chr != game.word[i]) is_win = false;
    }
    if (is_win) {
        state = STATE_USER_GUESS_CORRECT;
        game.time = MAX_USER_GUESS_CORRECT;
    }

    /* Copy user guess to previous attempts */
    for (int i = 0; i < WORD_LEN; ++i) {
        game.attempts[game.attempt].word[i] = game.current_guess[i].chr;
        game.current_guess[i].chr = '\0';
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
        bool key_found = find_keyboard_key(guess_buffer[i], &row, &col);
        if (guess_buffer[i] == word_buffer[i]) {
            game.keyboard[row][col].color = GREEN_BOX_COLOR;
            game.attempts[game.attempt].colors[i] = GREEN_BOX_COLOR;
            guess_buffer[i] = '\0';
            word_buffer[i] = '\0';
        } else {
            game.attempts[game.attempt].colors[i] = WRONG_BOX_COLOR;
            if (key_found && is_colors_equals(game.keyboard[row][col].color, DEFAULT_KEYBOARD_KEY_COLOR))
                game.keyboard[row][col].color = WRONG_KEYBOARD_KEY_COLOR;
        }
    }

    for (int i = 0; i < WORD_LEN; ++i) {
        if (word_buffer[i] == '\0') continue;
        for (int j = 0; j < WORD_LEN; ++j) {
            if (guess_buffer[j] == '\0') continue;
            if (word_buffer[i] == guess_buffer[j]) {
                find_keyboard_key(guess_buffer[j], &row, &col);
                if (!is_colors_equals(game.keyboard[row][col].color, GREEN_BOX_COLOR))
                    game.keyboard[row][col].color = YELLOW_BOX_COLOR;
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
    if (game.attempt == MAX_ATTEMPTS && state != STATE_USER_GUESS_CORRECT) {
        state = STATE_LOSE;
    }

    return state;
}


void process_input(void)
{
    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (game.current_guess_len > 0) --game.current_guess_len;
        if (game.current_guess_len < WORD_LEN) {
            game.current_guess[game.current_guess_len].chr = '\0';
        }
    } else if (IsKeyPressed(KEY_ENTER)) {
        State state = make_attempt();
        if (state == STATE_USER_GUESS_COLORING) {
            game.time = USER_GUESS_COLORING_TIME;
        } else if (state == STATE_NON_EXISTENT_WORD) {
            game.time = MAX_NON_EXISTENT_WORD_TIMER;
        }
        game.state = state;
        return;
    }

    if (game.current_guess_len >= WORD_LEN) return;

    for (int key = KEY_A; key <= KEY_Z; ++key) {
        if (IsKeyPressed(key)) {
            game.current_guess[game.current_guess_len].chr = key;
            game.current_guess[game.current_guess_len].time = MAX_KEY_TIMER;
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

    int base_y = Lerp(min_y, max_y, t);
    for (int c = 0; c < WORD_LEN; ++c) {
        if (game.current_guess[c].time > 0.0f) game.current_guess[c].time -= GetFrameTime();

        float amount = 1.0f - game.current_guess[c].time/MAX_KEY_TIMER;
        int offset = Lerp(0, 10, sinf(PI*amount));
        int size = LETTER_BOX_SIZE + offset;

        int y = base_y - offset/2;
        int x = start_x + ((LETTER_BOX_SIZE + LETTER_BOX_GAP) * c) - offset/2;

        if (game.state == STATE_NON_EXISTENT_WORD) {
            float t = 1.0f - (game.time/MAX_NON_EXISTENT_WORD_TIMER);
            int offset = Lerp(0, 10, sinf(4*PI*t));
            y += offset;
            DrawRectangle(x, y, size, size, ColorLerp(LETTER_BOX_COLOR, RED, sinf(PI*t)));
        } else {
            DrawRectangle(x, y, size, size, ColorAlpha(LETTER_BOX_COLOR, t));
        }
        if (c < game.current_guess_len) draw_char(game.current_guess[c].chr, size, x, y, LETTER_FONT_SIZE);
        if (c == game.current_guess_len) draw_cursor(x, y, t);
    }

    return;
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

void draw_enter(bool active)
{
    int start_y = GetScreenHeight()/2 - (FIELD_HEIGHT + FIELD_MARGIN*2 + KEYBOARD_HEIGHT)/2 + FIELD_HEIGHT + FIELD_MARGIN*2;
    int len = strlen(keyboard_keys[0]);
    int row_width = len * KEYBOARD_KEY_SIZE + ((len - 1) * KEYBOARD_GAP);
    int start_x = GetScreenWidth()/2 - row_width/2;
    len = strlen(keyboard_keys[2]);
    row_width = len * KEYBOARD_KEY_SIZE + ((len - 1) * KEYBOARD_GAP);
    int end_x = GetScreenWidth()/2 - row_width/2 - KEYBOARD_GAP;
    int enter_width = end_x - start_x;
    int y = start_y + 2 * KEYBOARD_KEY_SIZE + 2 * KEYBOARD_GAP;

    Rectangle key_rect = { start_x, y, enter_width, KEYBOARD_KEY_SIZE };

    bool is_hovered = CheckCollisionPointRec(
        GetMousePosition(),
        key_rect
    );

    Color color = DEFAULT_KEYBOARD_KEY_COLOR;
    Color outline_color = is_hovered && active ? WHITE : color;

    DrawRectangleRounded(key_rect, 0.2f, 0, color);
    DrawRectangleRoundedLinesEx(key_rect, 0.2f, 0, 2, outline_color);

    char text[] = "Enter";
    Vector2 text_size = MeasureTextEx(font, text, KEYBOARD_FONT_SIZE, 1);
    int tx = start_x + (enter_width/2 - text_size.x/2);
    int ty = y + (KEYBOARD_KEY_SIZE/2 - text_size.y/2);
    draw_text(text, tx, ty, KEYBOARD_FONT_SIZE, LETTER_COLOR);

    if (active && is_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        State state = make_attempt();
        if (state == STATE_USER_GUESS_COLORING) {
            game.time = USER_GUESS_COLORING_TIME;
        }
        game.state = state;
    }
}

void draw_backspace(bool active)
{
    int start_y = GetScreenHeight()/2 - (FIELD_HEIGHT + FIELD_MARGIN*2 + KEYBOARD_HEIGHT)/2 + FIELD_HEIGHT + FIELD_MARGIN*2;
    int len = strlen(keyboard_keys[0]);
    int row_width = len * KEYBOARD_KEY_SIZE + ((len - 1) * KEYBOARD_GAP);
    int end_x = GetScreenWidth()/2 + row_width/2;
    len = strlen(keyboard_keys[2]);
    row_width = len * KEYBOARD_KEY_SIZE + ((len - 1) * KEYBOARD_GAP);
    int start_x = GetScreenWidth()/2 + row_width/2 + KEYBOARD_GAP;
    int enter_width = end_x - start_x;
    int y = start_y + 2 * KEYBOARD_KEY_SIZE + 2 * KEYBOARD_GAP;

    Rectangle key_rect = { start_x, y, enter_width, KEYBOARD_KEY_SIZE };

    bool is_hovered = CheckCollisionPointRec(
        GetMousePosition(),
        key_rect
    );

    Color color = DEFAULT_KEYBOARD_KEY_COLOR;
    Color outline_color = is_hovered && active ? WHITE : color;

    DrawRectangleRounded(key_rect, 0.2f, 0, color);
    DrawRectangleRoundedLinesEx(key_rect, 0.2f, 0, 2, outline_color);

    char text[] = "<";
    Vector2 text_size = MeasureTextEx(font, text, KEYBOARD_FONT_SIZE, 1);
    int tx = start_x + (enter_width/2 - text_size.x/2);
    int ty = y + (KEYBOARD_KEY_SIZE/2 - text_size.y/2);
    draw_text(text, tx, ty, KEYBOARD_FONT_SIZE, LETTER_COLOR);

    if (active && is_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (game.current_guess_len > 0) --game.current_guess_len;
        if (game.current_guess_len < WORD_LEN) {
            game.current_guess[game.current_guess_len].chr = '\0';
        }
    }
}

int calc_size_with_gaps(int size_px, int gap_px, int count)
{
    return count * size_px + (count - 1) * gap_px;
}

void draw_keyboard(bool active)
{
    int keyboard_y = GetScreenHeight()/2 - (FIELD_HEIGHT + FIELD_MARGIN*2 + KEYBOARD_HEIGHT)/2 + FIELD_HEIGHT + FIELD_MARGIN*2;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; keyboard_keys[i][j]; ++j) {
            Key *key = &game.keyboard[i][j];

            bool is_key_was_pressed = key->time >= 0.0f;
            float t = sinf(((key->time/MAX_KEYBOARD_TIMER)) * PI);
            if (is_key_was_pressed) key->time -= GetFrameTime();
            int margin = is_key_was_pressed ? Lerp(0, 3, t) : 0;
            int size = KEYBOARD_KEY_SIZE + margin*2;

            int keys_in_row = strlen(keyboard_keys[i]);
            int row_width = calc_size_with_gaps(KEYBOARD_KEY_SIZE, KEYBOARD_GAP, keys_in_row);
            int row_x = GetScreenWidth()/2 - row_width/2;
            int x = row_x + (j*KEYBOARD_KEY_SIZE + j*KEYBOARD_GAP) - margin;
            int y = keyboard_y + (i*KEYBOARD_KEY_SIZE + i*KEYBOARD_GAP) - margin;

            Rectangle key_rect = { x, y, size, size };

            bool is_hovered = CheckCollisionPointRec(
                GetMousePosition(),
                key_rect
            );

            Color color = ColorLerp(game.keyboard[i][j].color, PRESSED_KEYBOARD_KEY_COLOR, t);
            DrawRectangleRounded(key_rect, 0.2f, 0, color);

            Color outline_color = is_hovered && active ? WHITE : color;
            DrawRectangleRoundedLinesEx(key_rect, 0.2f, 0, 2, outline_color);

            draw_char(keyboard_keys[i][j], size, x, y, KEYBOARD_FONT_SIZE);

            if (active && IsKeyPressed(keyboard_keys[i][j]) && game.current_guess_len < WORD_LEN) {
                game.keyboard[i][j].time = MAX_KEYBOARD_TIMER;
            }

            if (active && is_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (game.current_guess_len >= WORD_LEN) continue;
                game.current_guess[game.current_guess_len].chr = keyboard_keys[i][j];
                game.current_guess[game.current_guess_len].time = MAX_KEY_TIMER; 
                ++game.current_guess_len;
                game.keyboard[i][j].time = MAX_KEYBOARD_TIMER;
            }
        }
    }

    draw_enter(active);
    draw_backspace(active);
}


void draw_game_play(void)
{
    draw_user_guess(1.0f);
    draw_attempts(1.0f);
    draw_keyboard(true);
    process_input();
}

void draw_game_non_existent_word(void)
{
    draw_attempts(1.0f);
    draw_keyboard(false);
    draw_user_guess(1.0f);
}

void draw_game_user_guess_coloring(void)
{
    float t = (1.0f - game.time/(float)USER_GUESS_COLORING_TIME);
    draw_attempts(t);
    draw_keyboard(false);
}

void draw_game_user_guess_appear(void)
{
    float t = (1.0f - game.time/(float)USER_GUESS_APPER_TIME);
    draw_user_guess(t);
    draw_attempts(1.0f);
    draw_keyboard(false);
}

void draw_user_guess_correct(void)
{
    draw_attempts(1.0f);
    draw_keyboard(false);
}

void draw_game_win(void)
{
    draw_attempts(1.0f);
    draw_keyboard(false);
}

void draw_game_lose(void)
{
    draw_attempts(1.0f);
    draw_keyboard(false);
    Vector2 text_size = MeasureTextEx(font, game.word, LETTER_FONT_SIZE, 1);
    int width = text_size.x + 100;
    int height = text_size.y + 10;
    int x = GetScreenWidth()/2 - width/2;
    int y = GetScreenHeight()/2 - (FIELD_HEIGHT+FIELD_MARGIN*2+KEYBOARD_HEIGHT)/2;
    DrawRectangle(x, y, width, height, LOSE_BOX_COLOR);
    DrawRectangleLines(x, y, width, height, ColorBrightness(LOSE_BOX_COLOR, -0.5f));
    Vector2 text_pos = {
        .x = (x + width/2 - text_size.x/2),
        .y = (y + height/2 - text_size.y/2)
    };
    DrawTextEx(font, game.word, text_pos, LETTER_FONT_SIZE, 1.0f, LETTER_COLOR);
}

void draw_game_restart(float t)
{
    draw_attempts(1.0f);
    draw_user_guess(1.0f);
    draw_keyboard(false);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, t));
}


void draw_game_state(void)
{

    if (game.time > 0.0f) game.time -= GetFrameTime();

    switch (game.state) {
        case STATE_PLAY: {
            draw_game_play();
        } break;
        case STATE_RESTART_FADEIN: {
            float t = 1.0f - game.time / MAX_RESTART_TIMER;
            draw_game_restart(t);
            if (game.time <= 0.0f) {
                restart_game();
                game.state = STATE_RESTART_FADEOUT;
                game.time = MAX_RESTART_TIMER;
            }
        } break;
        case STATE_RESTART_FADEOUT: {
            float t = game.time / MAX_RESTART_TIMER;
            draw_game_restart(t);
            if (game.time <= 0.0f) {
                game.state = STATE_PLAY;
                game.time = 0.0f;
            }
        } break;
        case STATE_USER_GUESS_CORRECT: {
            draw_user_guess_correct();
            if (game.time <= 0.0f) {
                game.state = STATE_WIN;
                game.time = 0.0f;
            }
        } break;
        case STATE_NON_EXISTENT_WORD: {
            draw_game_non_existent_word();
            if (game.time <= 0.0f) {
                game.state = STATE_PLAY;
                game.time = 0.0f;
            }
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
        draw_game_state();
        if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_R)) {
            game.state = STATE_RESTART_FADEIN;
            game.time = MAX_RESTART_TIMER;
        } else if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_U)) {
            for (int i = 0; i < WORD_LEN; ++i) {
                game.current_guess[i].chr = '\0';
            }
            game.current_guess_len = 0;
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
