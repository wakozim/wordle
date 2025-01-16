#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>

#include "words.h"

#define BACKGROUND_COLOR CLITERAL(Color){0x18,0x18,0x18,0xFF}

#ifdef PLATFORM_WEB
    extern void print_word(char *word);
    extern void raylib_js_set_entry(void (*entry)(void));
#endif

#define MAX_ATTEMPTS 6
#define MAX_TIMER 1.0f

#define LETTER_FONT_FILEPATH "./assets/fonts/Oswald-Bold.ttf"
#define LETTER_FONT_SIZE     65
#define LETTER_BOX_SIZE      75
#define LETTER_BOX_GAP       10
#define FIELD_WIDTH          ((WORD_LEN * LETTER_BOX_SIZE) + ((WORD_LEN - 1) * LETTER_BOX_GAP))
#define FIELD_HEIGHT         ((MAX_ATTEMPTS * LETTER_BOX_SIZE) + ((MAX_ATTEMPTS - 1) * LETTER_BOX_GAP))
#define CURSOR_WIDTH         (LETTER_BOX_SIZE*0.1f)
#define CURSOR_HEIGHT        (LETTER_BOX_SIZE*0.75f)
#define SCREEN_WIDTH         (FIELD_WIDTH + 200)
#define SCREEN_HEIGHT        (FIELD_HEIGHT + 200)


typedef struct Attempt {
    char word[WORD_LEN];
    Color colors[WORD_LEN];
} Attempt;

typedef enum State {
    STATE_PLAY = 0,
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
} Game;

static Game game = {0};

float game_timer = 0.0f;
static Font font = {0};


void restart_game(void)
{
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


void draw_text(char *text, int x, int y, Color color)
{
    DrawTextEx(font, text, CLITERAL(Vector2){x, y}, LETTER_FONT_SIZE, 1, color);
}


void draw_letter(char chr, int letter_box_x, int letter_box_y)
{
    char text[10] = {0};
    text[0] = chr;
    Vector2 text_size = MeasureTextEx(font, text, LETTER_FONT_SIZE, 1);
    int x = (letter_box_x + LETTER_BOX_SIZE/2) - text_size.x/2;
    int y = (letter_box_y + LETTER_BOX_SIZE/2) - text_size.y/2;
    draw_text(text, x, y, WHITE);
}

void draw_attempts(void)
{
    int start_x = GetScreenWidth()/2 - FIELD_WIDTH/2;
    int start_y = GetScreenHeight()/2 - FIELD_HEIGHT/2;
    for (int i = 0; i < game.attempt; ++i) {
        int y = start_y + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * i;
        for (int j = 0; j < WORD_LEN; ++j) {
            int x = start_x + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * j;
            Color color = game.attempts[i].colors[j];
            DrawRectangle(x, y, LETTER_BOX_SIZE, LETTER_BOX_SIZE, color);
            draw_letter(game.attempts[i].word[j], x, y);
        }
    }
    return;
}


bool make_attempt(void)
{
    if (game.attempt == MAX_ATTEMPTS) return false;

    /* Check is word exists */
    if (game.current_guess_len < WORD_LEN) return false;

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
    if (!is_word_exists) return false;

    /* Check for win */
    bool is_win = true;
    for (int i = 0; i < WORD_LEN; ++i) {
        if (game.current_guess[i] != game.word[i]) is_win = false;
    }
    if (is_win) {
        game.state = STATE_WIN;
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

    for (int i = 0; i < WORD_LEN; ++i) {
        if (guess_buffer[i] == word_buffer[i]) {
            game.attempts[game.attempt].colors[i] = GREEN;
            guess_buffer[i] = '\0';
            word_buffer[i] = '\0';
        } else {
            game.attempts[game.attempt].colors[i] = RED;
        }
    }

    for (int i = 0; i < WORD_LEN; ++i) {
        if (word_buffer[i] == '\n') continue;
        for (int j = 0; j < WORD_LEN; ++j) {
            if (guess_buffer[j] == '\0') continue;
            if (word_buffer[i] == guess_buffer[j]) {
                game.attempts[game.attempt].colors[j] = YELLOW;
                guess_buffer[j] = '\0';
                word_buffer[i] = '\0';
                break;
            }
        }
    }

    /* The end of calculates */

    game.current_guess_len = 0;
    game.attempt += 1;
    if (game.attempt == MAX_ATTEMPTS && game.state != STATE_WIN) {
        game.state = STATE_LOSE;
    }

    return true;
}


void process_input(void)
{
    if (IsKeyPressed(KEY_BACKSPACE) && game.current_guess_len > 0) {
        if (game.current_guess_len < WORD_LEN) {
            game.current_guess[game.current_guess_len] = 0;
        }
        --game.current_guess_len;
    } else if (IsKeyPressed(KEY_ENTER)) {
        make_attempt();
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

void draw_cursor(int letter_box_x, int letter_box_y)
{
    game_timer += GetFrameTime();
    if (game_timer >= 0.25f) {
        int x = (letter_box_x + LETTER_BOX_SIZE/2) - CURSOR_WIDTH/2;
        int y = (letter_box_y + LETTER_BOX_SIZE/2) - CURSOR_HEIGHT/2;
        DrawRectangle(x, y, CURSOR_WIDTH, CURSOR_HEIGHT, WHITE);
    }

    if (game_timer >= MAX_TIMER) {
        game_timer = 0.0f;
    }
}

void draw_user_guess(void)
{
    if (game.attempt >= MAX_ATTEMPTS) return;

    int start_x = GetScreenWidth()/2 - FIELD_WIDTH/2;
    int start_y = GetScreenHeight()/2 - FIELD_HEIGHT/2;

    int y = start_y + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * game.attempt;
    for (int c = 0; c < WORD_LEN; ++c) {
        int x = start_x + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * c;
        DrawRectangle(x, y, LETTER_BOX_SIZE, LETTER_BOX_SIZE, GRAY);
        if (c < game.current_guess_len) draw_letter(game.current_guess[c], x, y);
        if (c == game.current_guess_len) draw_cursor(x, y);
    }

    process_input();

    return;
}

void game_frame(void)
{
    BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_R)) {
            restart_game();
        } else {
            draw_attempts();
            if (game.state == STATE_PLAY) {
                draw_user_guess();
            }
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
