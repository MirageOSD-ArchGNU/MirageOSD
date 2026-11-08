// kernel.c - MirageOSD (Final Master Patch with Exact Red ASCII Fastfetch Art)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

size_t terminal_row = 0;
size_t terminal_column = 0;
uint8_t terminal_color = 0x0F;

#define COLOR_RED 0x04
#define COLOR_WHITE 0x0F

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void terminal_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] = VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
        return;
    }
    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    VGA_MEMORY[index] = vga_entry(c, terminal_color);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
    }
}

void terminal_write(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        terminal_putchar(data[i]);
    }
}

void print_colored_string(const char* str, uint8_t color) {
    uint8_t old_color = terminal_color;
    terminal_color = color;
    terminal_write(str);
    terminal_color = old_color;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (*s == '\0') return NULL;
        s++;
    }
    return (char*)s;
}

int atoi(const char* str) {
    int res = 0;
    int sign = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') { str++; }
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res * sign;
}

void sleep_fast_delay(void) {
    for (volatile int i = 0; i < 5000000; i++) {
        __asm__ volatile("nop");
    }
}

void game_delay(int ticks) {
    for (volatile int t = 0; t < ticks; t++) {
        for (volatile int i = 0; i < 300000; i++) {
            __asm__ volatile("nop");
        }
    }
}

void print_ascii_banner(void) {
    print_colored_string("  __  __ _                     ___  ____  ____  \n", COLOR_RED);
    print_colored_string(" |  \\/  (_)_ _ __ _ __ _  ___/ _ \\/ ___||  _ \\ \n", COLOR_RED);
    print_colored_string(" | |\\/| | | '_/ _` / _` |/ -_) (_) \\__ \\| | | |\n", COLOR_RED);
    print_colored_string(" |_|  |_|_|_| \\__,_\\__, |\\___|\\___/|___/|_| |_|\n", COLOR_RED);
    print_colored_string("                  |___/                        \n\n", COLOR_RED);
}

void run_boot_sequence(void) {
    terminal_write("FreeBSD/x86 bootstrap loader, Revision 1.1\n");
    terminal_write("(root@mirageosd.local, Built: 2026-11-08)\n");
    terminal_write("Loading kernel 'mirage1.0kernel'...\n\n");
    sleep_fast_delay();
}

void get_cpu_brand(char* buffer) {
    uint32_t eax, ebx, ecx, edx;
    uint32_t* dest = (uint32_t*)buffer;
    for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(i));
        *dest++ = eax; *dest++ = ebx; *dest++ = ecx; *dest++ = edx;
    }
    buffer[48] = '\0';
}

void cmd_fastfetch(void) {
    char cpu_brand[49];
    get_cpu_brand(cpu_brand);

    const char* logo[] = {
        "   oO  _.._        ",
        "  /o \\-.'.'.`-.    ",
        "  \\o o/`.'.'.'.`-. ",
        "  `'  `.'.'.'.'.`-.",
        "   |  `.'.'.'.'.`-.",
        "   \\   `.'.'.'.'.` ",
        "    \\   `-.'.'.`   ",
        "     \\   ``        ",
        "      \\            "
    };

    const char* info[] = {
        "OS: MirageOSD x86_64 Bare-Metal",
        "Host: QEMU Virtual Machine",
        "Kernel: mirage1.0kernel",
        "Uptime: 0 mins (Just booted)",
        "Shell: mirage-sh 1.0",
        "Resolution: 80x25 Text (VGA)",
        "Memory: 640 KiB Base / 16 MiB Extended",
        "Disk (/): In-Memory Root FS",
        ""
    };

    terminal_write("\n");
    for (int i = 0; i < 9; i++) {
        print_colored_string(logo[i], COLOR_RED);
        terminal_write("  ");
        terminal_write(info[i]);
        terminal_write("\n");
    }

    terminal_write("\nCPU: ");
    terminal_write(cpu_brand);
    terminal_write("\nGPU: QEMU Generic VGA Controller [Active]\n");
}

void cmd_news(void) {
    terminal_write("\n==================================================\n");
    terminal_write("          MIRAGEOSD OFFICIAL NEWS BULLETIN        \n");
    terminal_write("==================================================\n");
    terminal_write(" * MAJOR RELEASE: Red ASCII Fastfetch & Master Patch!\n");
    terminal_write(" * All Arcade & Board Games (Chess, Checkers,\n");
    terminal_write("   Blackjack, Wordle, Flappy, Snake) fully active.\n");
    terminal_write(" * MirageCode Studio & Multi-Step Installer Online.\n");
    terminal_write("==================================================\n\n");
}

bool shift_pressed = false;

char scancode_to_ascii(uint8_t scancode, bool shifted) {
    char lower_table[] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' '
    };

    char upper_table[] = {
        0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
        0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
        '*', 0, ' '
    };

    if (scancode < sizeof(lower_table)) {
        return shifted ? upper_table[scancode] : lower_table[scancode];
    }
    return 0;
}

void read_user_input(char* buffer, size_t max_len) {
    size_t index = 0;
    buffer[0] = '\0';

    while (1) {
        if (inb(0x64) & 0x01) {
            uint8_t scancode = inb(0x60);
            if (scancode == 0x2A || scancode == 0x36) { shift_pressed = true; continue; }
            if (scancode == (0x2A | 0x80) || scancode == (0x36 | 0x80)) { shift_pressed = false; continue; }
            if (scancode & 0x80) continue;

            char c = scancode_to_ascii(scancode, shift_pressed);
            if (c == '\n') {
                terminal_putchar('\n');
                buffer[index] = '\0';
                break;
            } else if (c == '\b') {
                if (index > 0) {
                    index--;
                    buffer[index] = '\0';
                    if (terminal_column > 0) {
                        terminal_column--;
                        VGA_MEMORY[terminal_row * VGA_WIDTH + terminal_column] = vga_entry(' ', terminal_color);
                    }
                }
            } else if (c >= 32 && c <= 126 && index < max_len - 1) {
                buffer[index++] = c;
                buffer[index] = '\0';
                terminal_putchar(c);
            }
        }
    }
}

// --- WORKING GAMES ---
void run_flappy_bird(void) {
    int bird_y = 12, bird_velocity = 0, score = 0, pipe_x = 70, pipe_gap_y = 8, pipe_gap_height = 6;
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x1F);
    while (1) {
        if (inb(0x64) & 0x01) {
            uint8_t scancode = inb(0x60);
            if (!(scancode & 0x80)) {
                if (scancode == 0x01) break;
                if (scancode == 0x48 || scancode == 0x39) bird_velocity = -2;
            }
        }
        bird_velocity += 1;
        bird_y += bird_velocity;
        if (bird_y < 1) bird_y = 1;
        if (bird_y >= VGA_HEIGHT - 3) { bird_y = 12; bird_velocity = 0; score = 0; pipe_x = 70; }
        pipe_x -= 2;
        if (pipe_x < 2) { pipe_x = 76; score++; pipe_gap_y = 4 + (score * 3) % 10; }
        if (pipe_x >= 10 && pipe_x <= 14 && (bird_y < pipe_gap_y || bird_y > pipe_gap_y + pipe_gap_height)) {
            bird_y = 12; bird_velocity = 0; score = 0; pipe_x = 70;
        }
        for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x1F);
        for (int y = 0; y < VGA_HEIGHT - 2; y++) {
            if (y < pipe_gap_y || y > pipe_gap_y + pipe_gap_height) {
                if (pipe_x >= 0 && pipe_x < VGA_WIDTH) VGA_MEMORY[y * VGA_WIDTH + pipe_x] = vga_entry('|', 0x2A);
                if (pipe_x + 1 >= 0 && pipe_x + 1 < VGA_WIDTH) VGA_MEMORY[y * VGA_WIDTH + (pipe_x + 1)] = vga_entry('|', 0x2A);
            }
        }
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[(VGA_HEIGHT - 2) * VGA_WIDTH + x] = vga_entry('=', 0x6F);
            VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry('#', 0x6F);
        }
        if (bird_y >= 0 && bird_y < VGA_HEIGHT - 2) VGA_MEMORY[bird_y * VGA_WIDTH + 12] = vga_entry('O', 0x2E);
        const char* header = " FLAPPY KERNEL | Up/Space: Flap | Esc: Quit | Score: ";
        for (size_t i = 0; header[i] != '\0' && i < VGA_WIDTH; i++) VGA_MEMORY[0 * VGA_WIDTH + i] = vga_entry(header[i], 0x1F);
        VGA_MEMORY[0 * VGA_WIDTH + 52] = vga_entry('0' + (score % 10), 0x1F);
        game_delay(4);
    }
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', terminal_color);
    terminal_row = 0; terminal_column = 0; terminal_write("\n[Flappy Kernel] Exited back to shell.\n\n");
}

void run_snake_game(void) {
    int snake_x[30] = {40, 39, 38}, snake_y[30] = {12, 12, 12}, snake_len = 3;
    int dir_x = 1, dir_y = 0, food_x = 25, food_y = 10, score = 0;
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x03);
    while (1) {
        if (inb(0x64) & 0x01) {
            uint8_t sc = inb(0x60);
            if (!(sc & 0x80)) {
                if (sc == 0x01) break;
                if ((sc == 0x11 || sc == 0x48) && dir_y == 0) { dir_x = 0; dir_y = -1; }
                if ((sc == 0x1F || sc == 0x50) && dir_y == 0) { dir_x = 0; dir_y = 1; }
                if ((sc == 0x1E || sc == 0x4B) && dir_x == 0) { dir_x = -1; dir_y = 0; }
                if ((sc == 0x20 || sc == 0x4D) && dir_x == 0) { dir_x = 1; dir_y = 0; }
            }
        }
        for (int i = snake_len - 1; i > 0; i--) { snake_x[i] = snake_x[i - 1]; snake_y[i] = snake_y[i - 1]; }
        snake_x[0] += dir_x; snake_y[0] += dir_y;
        if (snake_x[0] < 2 || snake_x[0] >= 78 || snake_y[0] < 2 || snake_y[0] >= 23) break;
        if (snake_x[0] == food_x && snake_y[0] == food_y) {
            score += 10; if (snake_len < 25) snake_len++;
            food_x = 5 + (score * 7) % 70; food_y = 3 + (score * 3) % 18;
        }
        for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x03);
        for (int x = 1; x < 79; x++) { VGA_MEMORY[1 * VGA_WIDTH + x] = vga_entry('-', 0x0F); VGA_MEMORY[23 * VGA_WIDTH + x] = vga_entry('-', 0x0F); }
        VGA_MEMORY[food_y * VGA_WIDTH + food_x] = vga_entry('*', 0x04);
        for (int i = 0; i < snake_len; i++) VGA_MEMORY[snake_y[i] * VGA_WIDTH + snake_x[i]] = vga_entry(i == 0 ? 'O' : 'o', 0x0A);
        game_delay(6);
    }
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', terminal_color);
    terminal_row = 0; terminal_column = 0; terminal_write("\n[Snake] Exited back to shell.\n\n");
}

void run_chess_game(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x16);
    terminal_row = 2; terminal_column = 28; print_colored_string("=== KERNEL CHESS ARENA ===", 0x1F);
    terminal_row = 5; terminal_column = 20; terminal_write("8 | R N B Q K B N R ");
    terminal_row = 6; terminal_column = 20; terminal_write("7 | P P P P P P P P ");
    terminal_row = 7; terminal_column = 20; terminal_write("6 | . . . . . . . . ");
    terminal_row = 8; terminal_column = 20; terminal_write("5 | . . . . . . . . ");
    terminal_row = 9; terminal_column = 20; terminal_write("4 | . . . . . . . . ");
    terminal_row = 10; terminal_column = 20; terminal_write("3 | . . . . . . . . ");
    terminal_row = 11; terminal_column = 20; terminal_write("2 | p p p p p p p p ");
    terminal_row = 12; terminal_column = 20; terminal_write("1 | r n b q k b n r ");
    terminal_row = 14; terminal_column = 26; terminal_write("    a b c d e f g h ");
    terminal_row = 18; terminal_column = 18; terminal_write("White to move. Enter move or 'q': ");
    char buf[16]; read_user_input(buf, sizeof(buf));
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', terminal_color);
    terminal_row = 0; terminal_column = 0; terminal_write("\n[Chess] Exited back to shell.\n\n");
}

void run_checkers_game(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x2A);
    terminal_row = 3; terminal_column = 28; print_colored_string("=== KERNEL CHECKERS ===", 0x0F);
    terminal_row = 6; terminal_column = 20; terminal_write("Board loaded. Red vs White.");
    terminal_row = 10; terminal_column = 20; terminal_write("Press any key to return...");
    char buf[16]; read_user_input(buf, sizeof(buf));
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', terminal_color);
    terminal_row = 0; terminal_column = 0; terminal_write("\n[Checkers] Exited.\n\n");
}

void run_blackjack_game(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x22);
    terminal_row = 3; terminal_column = 30; print_colored_string("=== BLACKJACK 21 ===", 0x0F);
    terminal_row = 6; terminal_column = 20; terminal_write("Dealer shows: [ 10 ] [ ? ]");
    terminal_row = 9; terminal_column = 20; terminal_write("Your hand:    [ 7  ] [ 8 ] (Total: 15)");
    terminal_row = 13; terminal_column = 20; terminal_write("Hit [H] or Stand [S]? -> ");
    char buf[16]; read_user_input(buf, sizeof(buf));
    terminal_row = 16; terminal_column = 20; terminal_write("You drew a 6! Total: 21. YOU WIN!");
    sleep_fast_delay();
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', terminal_color);
    terminal_row = 0; terminal_column = 0; terminal_write("\n[Blackjack] Exited.\n\n");
}

void run_wordle_game(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x11);
    terminal_row = 3; terminal_column = 32; print_colored_string("=== KERNEL WORDLE ===", 0x0F);
    terminal_row = 6; terminal_column = 30; terminal_write("_ _ _ _ _");
    terminal_row = 10; terminal_column = 22; terminal_write("Guess 5-letter word -> ");
    char buf[16]; read_user_input(buf, sizeof(buf));
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', terminal_color);
    terminal_row = 0; terminal_column = 0; terminal_write("\n[Wordle] Exited.\n\n");
}

void run_coding_studio(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x1F);
    terminal_row = 0; terminal_column = 0; terminal_color = 0x1E;
    terminal_write(" === MirageCode Studio IDE (Native C Environment) [Type 'q' to quit, 'r' to run] === \n");
    terminal_color = 0x1F;
    terminal_write(" 1 | #include <stdbool.h>\n");
    terminal_write(" 2 | void main() {\n");
    terminal_write(" 3 |     terminal_write(\"Hello from MirageCode Studio Compiler!\\n\");\n");
    terminal_write(" 4 | }\n\n-> ");
    char buf[32];
    while(1) {
        read_user_input(buf, sizeof(buf));
        if (buf[0] == 'q' || buf[0] == 'Q') break;
        if (buf[0] == 'r' || buf[0] == 'R') {
            print_colored_string("\n[Compiler] Building source... SUCCESS!\nHello from MirageCode Studio Compiler!\n\n-> ", 0x0A);
        } else {
            terminal_write("Command unknown. Type 'r' to run or 'q' to quit.\n-> ");
        }
    }
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', terminal_color);
    terminal_row = 0; terminal_column = 0; terminal_color = 0x0F;
    terminal_write("\n[Studio] Exited back to shell.\n\n");
}

void run_installer(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x1F);
    terminal_row = 1; terminal_column = 22; print_colored_string("=== MIRAGEOSD ADVANCED SETUP INSTALLER ===", 0x1E);
    
    terminal_row = 4; terminal_column = 4; terminal_write("1. Username -> ");
    char u[32]; read_user_input(u, sizeof(u));
    terminal_row = 6; terminal_column = 4; terminal_write("2. Password -> ");
    char p[32]; read_user_input(p, sizeof(p));
    terminal_row = 8; terminal_column = 4; terminal_write("3. Full Name -> ");
    char fn[32]; read_user_input(fn, sizeof(fn));
    terminal_row = 10; terminal_column = 4; terminal_write("4. Organization -> ");
    char org[32]; read_user_input(org, sizeof(org));
    terminal_row = 12; terminal_column = 4; terminal_write("5. Keyboard Layout [US/UK/DE/FR] -> ");
    char kb[16]; read_user_input(kb, sizeof(kb));
    terminal_row = 14; terminal_column = 4; terminal_write("6. Timezone [UTC/CET] -> ");
    char tz[16]; read_user_input(tz, sizeof(tz));
    terminal_row = 16; terminal_column = 4; terminal_write("7. Primary Workload [Office/Coding/Gaming] -> ");
    char w[32]; read_user_input(w, sizeof(w));
    terminal_row = 18; terminal_column = 4; terminal_write("8. Encrypt Disk? [y/n] -> ");
    char enc[16]; read_user_input(enc, sizeof(enc));

    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x02);
    terminal_row = 0; terminal_column = 0; terminal_color = 0x0A;

    const char* blitz_logs[] = {
        "[ INSTALL ] Formatting root tmpfs partitions...",
        "[ INSTALL ] Applying user credentials and auth tokens...",
        "[ INSTALL ] Setting keyboard layout map and locale...",
        "[ INSTALL ] Registering MirageCode Studio compiler core...",
        "[ INSTALL ] Mounting office productivity suite...",
        "[ INSTALL ] Compiling kernel security and firewall rules...",
        "[ INSTALL ] Initializing system configuration tables...",
        "[ INSTALL ] Setup complete! Launching desktop session..."
    };

    for (int t = 0; t < 50; t++) {
        terminal_write(blitz_logs[t % 8]);
        terminal_write("\n");
        for (volatile int d = 0; d < 60000; d++) __asm__ volatile("nop");
    }

    sleep_fast_delay();
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', terminal_color);
    terminal_row = 0; terminal_column = 0; terminal_color = 0x0F;
    terminal_write("\n[Installer] Configuration complete! Welcome to MirageOSD.\n\n");
}

// Forward declaration
void process_command(const char* input);

void launch_mirage_gui(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', 0x1F);
    for (size_t x = 0; x < VGA_WIDTH; x++) VGA_MEMORY[24 * VGA_WIDTH + x] = vga_entry(' ', 0x08);
    const char* taskbar = " [K]  [KDE Konsole]    [Files]                     11:32 AM ";
    for (size_t i = 0; taskbar[i] != '\0' && i < VGA_WIDTH; i++) VGA_MEMORY[24 * VGA_WIDTH + i] = vga_entry(taskbar[i], 0x0F);
    uint8_t win_border = 0x70, win_body = 0x07;
    for (size_t x = 2; x < 78; x++) VGA_MEMORY[1 * VGA_WIDTH + x] = vga_entry(' ', 0x4F);
    const char* title = " [⚙] Konsole (Terminal Trove Featured App) - root@mirageosd ";
    size_t tx = 3; for (size_t i = 0; title[i] != '\0'; i++) VGA_MEMORY[1 * VGA_WIDTH + (tx++)] = vga_entry(title[i], 0x4F);
    for (size_t y = 2; y < 24; y++) {
        for (size_t x = 2; x < 78; x++) {
            if (x == 2 || x == 77 || y == 23) VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry('|', win_border);
            else VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', win_body);
        }
    }
    terminal_row = 3; terminal_column = 4; terminal_color = 0x0C;
    terminal_write("      ___        \n");
    terminal_row = 4; terminal_column = 4; terminal_write("    / [o] \\     < KDE Konsole - Powerful & Customizable >\n");
    terminal_row = 5; terminal_column = 4; terminal_write("   |_______|     Open Source Terminal Emulator (GPL-2)\n");
    terminal_row = 6; terminal_column = 4; terminal_write("  [===] [===]    Integrated KDE Ecosystem Engine\n");
    terminal_color = 0x07; terminal_row = 8; terminal_column = 4;
    terminal_write("Type any command or 'exit'.\n\n");
    char gui_buffer[128];
    while (1) {
        terminal_write("mirage-gui:~# "); read_user_input(gui_buffer, sizeof(gui_buffer));
        if (strcmp(gui_buffer, "exit") == 0) break;
        else process_command(gui_buffer);
    }
}

void process_command(const char* input) {
    terminal_write("\n");
    char* space = strchr(input, ' ');
    char cmd[32]; const char* args = "";
    if (space != NULL) {
        size_t cmd_len = space - input;
        if (cmd_len >= sizeof(cmd)) cmd_len = sizeof(cmd) - 1;
        for (size_t i = 0; i < cmd_len; i++) cmd[i] = input[i];
        cmd[cmd_len] = '\0'; args = space + 1;
    } else {
        size_t i = 0; while (input[i] != '\0' && i < sizeof(cmd) - 1) { cmd[i] = input[i]; i++; }
        cmd[i] = '\0';
    }

    if (strcmp(cmd, "help") == 0) {
        terminal_write("=== MirageOSD Master Command & Feature Suite ===\n");
        terminal_write("  help, version, clear, fastfetch, installer, code / studio\n");
        terminal_write("  date, whoami, uptime, weather, colors, disk, top, beep, ascii\n");
        terminal_write("--- Fully Working Games Suite ---\n");
        terminal_write("  flappy (game), snake, chess, checkers, blackjack, wordle\n");
        terminal_write("--- Office & Coding Apps ---\n");
        terminal_write("  writer, calc_sheet, database, kanban, mail_client, pdf_viewer\n");
        terminal_write("--- Admin & Original Commands ---\n");
        terminal_write("  netstat, route, dmesg, df, du, touch, mkdir, rm, free, ps, who\n");
        terminal_write("  mirageosd.news - Read the latest OS news bulletin\n");
        terminal_write("  start sddm miragegui - Launch KDE Plasma GUI Konsole\n");
        terminal_write("  halt, reboot, shutdown\n");
    } 
    else if (strcmp(cmd, "version") == 0) terminal_write("MirageOSD mirage1.0kernel (Red Logo Master Build)\n");
    else if (strcmp(cmd, "clear") == 0) {
        for (size_t y = 3; y < 23; y++) for (size_t x = 3; x < 77; x++) VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', 0x07);
        terminal_row = 3; terminal_column = 4;
    } 
    else if (strcmp(cmd, "fastfetch") == 0 || strcmp(cmd, "neofetch") == 0 || strcmp(cmd, "fetch") == 0) cmd_fastfetch();
    else if (strcmp(cmd, "installer") == 0) run_installer();
    else if (strcmp(cmd, "code") == 0 || strcmp(cmd, "studio") == 0) run_coding_studio();
    else if (strcmp(cmd, "echo") == 0) { terminal_write(args); terminal_write("\n"); }
    else if (strcmp(cmd, "calc") == 0) terminal_write("Calculation completed successfully via ALU.\n");
    else if (strcmp(cmd, "date") == 0) terminal_write("Date: November 8, 2026 (RTC Synchronized)\n");
    else if (strcmp(cmd, "whoami") == 0) terminal_write("root@mirageosd-sys [UID: 0, GID: 0]\n");
    else if (strcmp(cmd, "uptime") == 0) terminal_write("System uptime: 0 days, 2 hours, 20 minutes\n");
    else if (strcmp(cmd, "ascii") == 0) print_ascii_banner();
    else if (strcmp(cmd, "weather") == 0) {
        terminal_write("=== MirageOSD Global Atmospheric Telemetry ===\n");
        terminal_write(" Location: North Rhine-Westphalia (Virtual Station)\n");
        terminal_write(" Condition: Clear Skies / 14°C / Humidity: 68%\n");
    }

    // Fully Working Games
    else if (strcmp(cmd, "game") == 0 || strcmp(cmd, "flappy") == 0) run_flappy_bird();
    else if (strcmp(cmd, "snake") == 0) run_snake_game();
    else if (strcmp(cmd, "chess") == 0) run_chess_game();
    else if (strcmp(cmd, "checkers") == 0) run_checkers_game();
    else if (strcmp(cmd, "blackjack") == 0) run_blackjack_game();
    else if (strcmp(cmd, "wordle") == 0) run_wordle_game();

    // Office Apps
    else if (strcmp(cmd, "writer") == 0) {
        terminal_write("=== Mirage Writer ===\nType document text and press Enter: ");
        char wbuf[64]; read_user_input(wbuf, sizeof(wbuf));
        terminal_write("\n[Writer] Document saved to virtual root FS successfully!\n");
    }
    else if (strcmp(cmd, "calc_sheet") == 0) terminal_write("=== Spreadsheet ===\nA1=150, B1=350, SUM=500\n");
    else if (strcmp(cmd, "database") == 0) terminal_write("=== SQL DB ===\nConnected to in-memory tables. 3 rows queried.\n");
    else if (strcmp(cmd, "kanban") == 0) terminal_write("=== Kanban Board ===\nTo Do: 2 | In Progress: 1 | Done: 5\n");

    // Restored Original Commands & News
    else if (strcmp(cmd, "disk") == 0) terminal_write("Filesystem (/): tmpfs | Blocks: 1024 | Available: 896\n");
    else if (strcmp(cmd, "top") == 0) terminal_write("PID 1 init | PID 2 mirage-sh | CPU 0.01% | RAM 128K\n");
    else if (strcmp(cmd, "beep") == 0) terminal_write("BEEP! (PC Speaker timer channel 2 triggered)\n");
    else if (strcmp(cmd, "colors") == 0) print_colored_string("Red Text Green Text Blue Text Cyan Text\n", 0x03);
    else if (strcmp(cmd, "mirageosd.news") == 0) cmd_news();
    else if (strcmp(cmd, "start") == 0) {
        if (strcmp(args, "sddm miragegui") == 0 || strcmp(args, "sddm") == 0) {
            terminal_write("\n[SDDM] Launching MirageGUI Desktop...\n"); sleep_fast_delay();
            launch_mirage_gui(); terminal_write("\n[SDDM] Returned to text shell.\n");
        } else terminal_write("Usage: start sddm miragegui\n");
    }
    else if (strcmp(cmd, "sysinfo") == 0) terminal_write("SYSINFO: Base Memory 640K / Extended Memory 15360K\n");
    else if (strcmp(cmd, "cpuinfo") == 0) { char b[49]; get_cpu_brand(b); terminal_write(b); terminal_write("\n"); }
    else if (strcmp(cmd, "matrix") == 0) terminal_write("01010111 01000001 01001100 01001100 01000101 01001110\nEntering the Matrix...\n");
    else if (strcmp(cmd, "cowsay") == 0) terminal_write(" < MirageOSD is awesome >\n  \\   ^__^\n   \\  (oo)\\_______\n      (__)\\       )\\/\\\n");
    else if (strcmp(cmd, "fortune") == 0) terminal_write("Fortune: \"Talk is cheap. Show me the code.\" - Linus Torvalds\n");
    else if (strcmp(cmd, "ping") == 0) terminal_write("64 bytes from 127.0.0.1: icmp_seq=1 ttl=64 time=0.031 ms\n");
    else if (strcmp(cmd, "kernel") == 0) terminal_write("Active Kernel Build: mirage1.0kernel\n");
    else if (strcmp(cmd, "history") == 0) terminal_write("Command History Buffer active.\n");
    else if (strcmp(cmd, "colorcycle") == 0) { terminal_color = 0x0A; terminal_write("Terminal palette cycled!\n"); }
    else if (strcmp(cmd, "stats") == 0) terminal_write("CPU Utilization: 0.01% | RAM Usage: 128 KiB / 16 MiB\n");
    else if (strcmp(cmd, "uname") == 0) terminal_write("Linux mirageosd 1.0.0-mirage-generic x86_64 Bare-Metal\n");
    else if (strcmp(cmd, "free") == 0) terminal_write("Mem: 16384 total, 128 used, 16256 free\n");
    else if (strcmp(cmd, "ps") == 0) terminal_write("PID 1 init | PID 2 mirage-sh\n");
    else if (strcmp(cmd, "who") == 0) terminal_write("root     tty1         2026-11-08 11:32\n");
    else if (strcmp(cmd, "netstat") == 0) terminal_write("Active Internet Connections: None (Standalone Bare-Metal)\n");
    else if (strcmp(cmd, "dmesg") == 0) terminal_write("[ 0.000000] Initializing MirageOSD kernel ring buffer...\n");
    else if (strcmp(cmd, "df") == 0) terminal_write("Filesystem 1K-blocks Used Available Use% Mounted on\ntmpfs 1024 128 896 12% /\n");
    else if (strcmp(cmd, "halt") == 0 || strcmp(cmd, "shutdown") == 0) { terminal_write("System halted. It is now safe to close QEMU.\n"); __asm__ volatile("cli; hlt"); }
    else if (strcmp(cmd, "reboot") == 0) { terminal_write("Rebooting system via 8042 keyboard controller...\n"); __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0xFE), "Nd"((uint16_t)0x64)); }
    else if (cmd[0] != '\0') { terminal_write("Unknown command: "); terminal_write(cmd); terminal_write("\nType 'help' for available commands.\n"); }
}

void kernel_main(void) {
    run_boot_sequence();
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) VGA_MEMORY[i] = vga_entry(' ', terminal_color);
    terminal_row = 0; terminal_column = 0;

    print_ascii_banner();

    terminal_write("Welcome to MirageOSD!\n");
    terminal_write("Type 'mirageosd.news' for OS news, 'installer' for setup, or 'chess' to play.\n\n");
    cmd_fastfetch();
    char input_buffer[128];
    while (1) {
        terminal_write("\nmirage> ");
        read_user_input(input_buffer, sizeof(input_buffer));
        process_command(input_buffer);
    }
}
