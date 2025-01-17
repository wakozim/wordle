#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#include <stdbool.h>

#define SOURCE_FILE_PATH "./src/wordle.c"
#define CFLAGS "-O3", "-Wall", "-Wextra", "-g", "-pedantic"


bool generate_words_header()
{
    String_Builder words = {0};
    if (!read_entire_file("./src/words.txt", &words)) return false;
    String_View sv = nob_sb_to_sv(words);
    String_Builder sb = {0};

    sb_append_cstr(&sb, "#ifndef WORDS_H_\n");
    sb_append_cstr(&sb, "#define WORDS_H_\n");

    sb_append_cstr(&sb, "char words[][6] = {\n");
    int words_count = 0;
    while (sv.count) {
        sv = nob_sv_trim_left(sv);
        if (sv.count <= 0) break;

        String_View word = sv_chop_by_delim(&sv, '\n');

        sb_append_cstr(&sb, "    \"");
        sb_append_buf(&sb, word.data, word.count);
        sb_append_cstr(&sb, "\",\n");
        ++words_count;
    }
    sb_append_cstr(&sb, "};\n");

    sb_append_cstr(&sb, temp_sprintf("#define WORDS %d\n", words_count));
    sb_append_cstr(&sb, "#define WORD_LEN 5\n");

    sb_append_cstr(&sb, "#endif // WORDS_H_\n");

    return nob_write_entire_file("./build/words.h", sb.items, sb.count);
}


int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    bool debug = false;

    (void) shift_args(&argc, &argv); // Skip program name
    while (argc > 0) {
        char *arg = shift_args(&argc, &argv);
        if (strcmp(arg, "--debug") == 0) {
            debug = true;
        }
    }

    mkdir_if_not_exists("./build/");
    mkdir_if_not_exists("./wasm/");
    Cmd cmd = {0};

    /* Create words.h */
    if (!generate_words_header(&cmd)) return 1;

    /* Compile wordle for linux */
    if (debug || needs_rebuild1("./build/wordle", SOURCE_FILE_PATH) == 1) {
        cmd_append(&cmd, "clang", CFLAGS);
        cmd_append(&cmd, "-I./build/");
        cmd_append(&cmd, "-I./");
        cmd_append(&cmd, "-I./raylib/raylib-5.5_linux_amd64/include");
        cmd_append(&cmd, "-o", "./build/wordle", SOURCE_FILE_PATH);
        cmd_append(&cmd, "-L./raylib/raylib-5.5_linux_amd64/lib");
        cmd_append(&cmd, "-lraylib", "-lm");
        if (debug) cmd_append(&cmd, "-DDEBUG");
        if (!cmd_run_sync_and_reset(&cmd)) return 1;
    } else {
        nob_log(NOB_INFO, "'./build/wordle' is up to date. ");
    }

    /* Compile wordle for wasm */
    if (debug || needs_rebuild1("./wasm/wordle.wasm", SOURCE_FILE_PATH) == 1) {
        cmd_append(&cmd, "clang", CFLAGS);
        cmd_append(&cmd, "--target=wasm32", "--no-standard-libraries", "-Wl,--export-table", "-Wl,--no-entry", "-Wl,--allow-undefined", "-Wl,--export=main", "-Wl,--export=__head_base", "-Wl,--allow-undefined");
        cmd_append(&cmd, "-I./build/");
        cmd_append(&cmd, "-I./");
        cmd_append(&cmd, "-I./include");
        cmd_append(&cmd, "-I./raylib/raylib-5.5_linux_amd64/include");
        cmd_append(&cmd, "-o", "./wasm/wordle.wasm", SOURCE_FILE_PATH, "-DPLATFORM_WEB");
        if (debug) cmd_append(&cmd, "-DDEBUG");
        if (!cmd_run_sync_and_reset(&cmd)) return 1;
    } else {
        nob_log(NOB_INFO, "'./wasm/wordle.wasm' is up to date. ");
    }

    return 0;
}
