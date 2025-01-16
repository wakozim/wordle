#include "raylib.h"

#include "words.h"

#define BACKGROUND_COLOR CLITERAL(Color){0x18,0x18,0x18,0xFF}

#ifdef PLATFORM_WEB
#   define NULL ((void *) 0)
    extern int rand(void);
    extern void srand(unsigned int seed);
    extern int time(void *time); 
    extern void print_word(char *word);
#else
#   include <stdio.h>
#   include <stdlib.h>
#   include <time.h>
#endif

void raylib_js_set_entry(void (*entry)(void));


#define MAX_ATTEMPTS 6
#define MAX_TIMER 1.0f

typedef struct Attempt {
    char word[WORD_LEN];
    Color colors[WORD_LEN];
} Attempt;

typedef struct State {
    char *word;                     // Hidden word
    int attempt;                    // Current attempt
    Attempt attempts[MAX_ATTEMPTS]; // Previous attemps
    char current_guess[WORD_LEN];   // Current user guess buffer
    int current_guess_len;          // Current user guess buffer length
} State;

static State state = {0};
bool win = false;

float game_timer = 0.0f;
static Font font = {0};

#define LETTER_FONT_FILEPATH "./assets/fonts/Oswald-Bold.ttf"
#define LETTER_FONT_SIZE     65
#define LETTER_BOX_SIZE      75
#define LETTER_BOX_GAP       10
#define FIELD_WIDTH          ((WORD_LEN * LETTER_BOX_SIZE) + ((WORD_LEN - 1) * LETTER_BOX_GAP))
#define FIELD_HEIGHT         ((MAX_ATTEMPTS * LETTER_BOX_SIZE) + ((MAX_ATTEMPTS - 1) * LETTER_BOX_GAP))
#define CURSOR_WIDTH         (LETTER_BOX_SIZE*0.1f)
#define CURSOR_HEIGHT        (LETTER_BOX_SIZE*0.75f)
#define SCREEN_WIDTH  (FIELD_WIDTH + 200)
#define SCREEN_HEIGHT (FIELD_HEIGHT + 200)


void restart_game(void)
{
    state.word = words[rand() % WORDS];
#ifdef DEBUG
#   ifdef PLATFORM_WEB
        print_word(state.word);
#   else
        TraceLog(LOG_ERROR, "Word is %s", state.word);
#   endif
#endif
    state.attempt = 0;
    state.current_guess_len = 0;
    for (int i = 0; i < WORD_LEN; ++i) {
        state.current_guess[i] = '\0'; 
    }
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
    for (int i = 0; i < state.attempt; ++i) {
        int y = start_y + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * i;
        for (int j = 0; j < WORD_LEN; ++j) {
            int x = start_x + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * j;
            Color color = state.attempts[i].colors[j];
            DrawRectangle(x, y, LETTER_BOX_SIZE, LETTER_BOX_SIZE, color);
            draw_letter(state.attempts[i].word[j], x, y);
        }
    }
    return;
}


void make_attempt(void)
{
    if (state.attempt == MAX_ATTEMPTS) return;

    /* Check for win */
    bool is_win = true;
    for (int i = 0; i < WORD_LEN; ++i) {
        if (state.current_guess[i] != state.word[i]) is_win = false;
    }
    win = is_win;

    /* Copy user guess to previous attempts */
    for (int i = 0; i < WORD_LEN; ++i) {
        state.attempts[state.attempt].word[i] = state.current_guess[i];
        state.current_guess[i] = 0;
    }

    /* Calculate colors for letters in guess */

    char word_buffer[WORD_LEN] = {0};
    char guess_buffer[WORD_LEN] = {0};
    for (int i = 0; i < WORD_LEN; ++i) {
        word_buffer[i] = state.word[i]; 
        guess_buffer[i] = state.attempts[state.attempt].word[i]; 
    }

    for (int i = 0; i < WORD_LEN; ++i) {
        if (guess_buffer[i] == word_buffer[i]) {
            state.attempts[state.attempt].colors[i] = GREEN;
            guess_buffer[i] = '\0';
            word_buffer[i] = '\0';
        } else {
            state.attempts[state.attempt].colors[i] = RED;
        } 
    }
    
    for (int i = 0; i < WORD_LEN; ++i) {
        if (word_buffer[i] == '\n') continue;
        for (int j = 0; j < WORD_LEN; ++j) {
            if (guess_buffer[j] == '\0') continue;
            if (word_buffer[i] == guess_buffer[j]) {
                state.attempts[state.attempt].colors[j] = YELLOW;
                guess_buffer[j] = '\0';
                word_buffer[i] = '\0';
                break;
            }
        }  
    }

    /* The end of calculates */

    state.current_guess_len = 0;
    state.attempt += 1;
}


void process_input(void)
{
    if (IsKeyPressed(KEY_BACKSPACE) && state.current_guess_len > 0) {
        if (state.current_guess_len < WORD_LEN) {
            state.current_guess[state.current_guess_len] = 0;
        }
        --state.current_guess_len;
    } else if (IsKeyPressed(KEY_ENTER)) {
        make_attempt(); 
        return;
    } 

    if (state.current_guess_len >= WORD_LEN) return;

    for (int key = KEY_A; key <= KEY_Z; ++key) {
        if (IsKeyPressed(key)) {
            state.current_guess[state.current_guess_len] = key;
            ++state.current_guess_len;
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
    if (state.attempt >= MAX_ATTEMPTS) return;

    int start_x = GetScreenWidth()/2 - FIELD_WIDTH/2; 
    int start_y = GetScreenHeight()/2 - FIELD_HEIGHT/2; 
    
    int y = start_y + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * state.attempt;
    for (int c = 0; c < WORD_LEN; ++c) {
        int x = start_x + (LETTER_BOX_SIZE + LETTER_BOX_GAP) * c;
        DrawRectangle(x, y, LETTER_BOX_SIZE, LETTER_BOX_SIZE, GRAY);
        if (c < state.current_guess_len) draw_letter(state.current_guess[c], x, y);
        if (c == state.current_guess_len) draw_cursor(x, y);
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
            if (!win) {
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
