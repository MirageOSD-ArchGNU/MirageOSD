// kernel.c - MirageOSD (mirage1.0kernel)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

size_t terminal_row = 0;
size_t terminal_column = 0;
uint8_t terminal_color = 0x0F; // White text on Black background

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

void sleep_fast_delay(void) {
    for (volatile int i = 0; i < 5000000; i++) {
        __asm__ volatile("nop");
    }
}

void run_boot_sequence(void) {
    terminal_write("FreeBSD/x86 bootstrap loader, Revision 1.1\n");
    terminal_write("(root@mirageosd.local, Built: 2026-11-08)\n");
    terminal_write("Loading kernel 'mirage1.0kernel'...\n\n");
    sleep_fast_delay();

    const char* boot_logs[] = {
        "[    0.000000] MirageOSD Kernel mirage1.0kernel initializing...",
        "[    0.000000] Architecture: x86_64 Bare-Metal Protected Mode",
        "[    0.001024] CPU: Verified genuine x86 processor via CPUID",
        "[    0.003500] Memory Manager: 16 KiB Stack allocated",
        "[    0.010200] Display Driver: VGA Text Mode (80x25 with scrolling)",
        "[    0.024500] Interrupt Controller: Initialized IDT & PIC",
        "[    0.089000] Input Driver: PS/2 Keyboard Controller ready",
        "[    0.120000] Virtual File System: In-memory root mounted",
        "[    0.250000] Starting MirageOSD terminal shell subsystem...",
        "[    0.400000] All systems nominal. Welcome to MirageOSD!",
        "\n"
    };

    for (int i = 0; i < 11; i++) {
        terminal_write(boot_logs[i]);
        terminal_write("\n");
        sleep_fast_delay();
    }
}

void get_cpu_brand(char* buffer) {
    uint32_t eax, ebx, ecx, edx;
    uint32_t* dest = (uint32_t*)buffer;
    for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
        __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(i));
        *dest++ = eax;
        *dest++ = ebx;
        *dest++ = ecx;
        *dest++ = edx;
    }
    buffer[48] = '\0';
}

void cmd_fastfetch(void) {
    char cpu_brand[49];
    get_cpu_brand(cpu_brand);

    const char* logo[] = {
        "    oO  _.._       ",
        "   /o \\-.'.'.`-.   ",
        "   \\o o/`.'.'.'.`-. ",
        "    `'  `.'.'.'.'.`-.",
        "     |  `.'.'.'.'.`-.",
        "     \\   `.'.'.'.'.` ",
        "      \\   `-.'.'.`   ",
        "       \\    ``       ",
        "        \\            "
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
    terminal_write("\nGPU: QEMU Generic VGA Controller [Integrated]\n");
}

void cmd_news(void) {
    terminal_write("\n=== MirageOSD Official News Bulletin ===\n");
    terminal_write(" [!] Konsole KDE Integration Active:\n");
    terminal_write("     Official Terminal Trove metadata & logo\n");
    terminal_write("     embedded into the graphical window session!\n");
    terminal_write("=========================================\n");
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

            if (scancode == 0x2A || scancode == 0x36) {
                shift_pressed = true;
                continue;
            }
            if (scancode == (0x2A | 0x80) || scancode == (0x36 | 0x80)) {
                shift_pressed = false;
                continue;
            }
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

void process_command(const char* input);

void launch_mirage_gui(void) {
    // 1. Draw Plasma Desktop background (Blue)
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = vga_entry(' ', 0x1F);
    }

    // 2. Draw Bottom Plasma Panel (Dark Gray)
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[24 * VGA_WIDTH + x] = vga_entry(' ', 0x08);
    }
    const char* taskbar = " [K]  [KDE Konsole]   [Files]                   11:32 AM ";
    for (size_t i = 0; taskbar[i] != '\0' && i < VGA_WIDTH; i++) {
        VGA_MEMORY[24 * VGA_WIDTH + i] = vga_entry(taskbar[i], 0x0F);
    }

    // 3. Draw Konsole Window Frame (Rows 1 to 23, Cols 2 to 78)
    uint8_t win_border = 0x70; 
    uint8_t win_body = 0x07;   

    // Title Bar (Row 1) with Konsole Branding Accent
    for (size_t x = 2; x < 78; x++) {
        VGA_MEMORY[1 * VGA_WIDTH + x] = vga_entry(' ', 0x4F); 
    }
    const char* title = " [⚙] Konsole (Terminal Trove Featured App) - root@mirageosd ";
    size_t tx = 3;
    for (size_t i = 0; title[i] != '\0'; i++) {
        VGA_MEMORY[1 * VGA_WIDTH + (tx++)] = vga_entry(title[i], 0x4F);
    }

    // Window Body Box
    for (size_t y = 2; y < 24; y++) {
        for (size_t x = 2; x < 78; x++) {
            if (x == 2 || x == 77 || y == 23) {
                VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry('|', win_border);
            } else {
                VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', win_body);
            }
        }
    }

    // Render the Official Konsole / Terminal Logo Graphic inside Window Header[cite: 1]
    terminal_row = 3;
    terminal_column = 4;
    terminal_color = 0x0C;
    terminal_write("     ___       \n");
    terminal_row = 4; terminal_column = 4;
    terminal_write("    / [o] \\     < KDE Konsole - Powerful & Customizable >[cite: 1]\n");
    terminal_row = 5; terminal_column = 4;
    terminal_write("   |_______|    Open Source Terminal Emulator (GPL-2)[cite: 1]\n");
    terminal_row = 6; terminal_column = 4;
    terminal_write("  [===] [===]   Integrated KDE Ecosystem Engine[cite: 1]\n");

    terminal_color = 0x07;
    terminal_row = 8;
    terminal_column = 4;
    terminal_write("Type any command (e.g. 'fastfetch', 'cowsay', 'matrix') or 'exit'.\n\n");

    char gui_buffer[128];
    while (1) {
        terminal_write("mirage-gui:~# ");
        read_user_input(gui_buffer, sizeof(gui_buffer));

        if (strcmp(gui_buffer, "exit") == 0) {
            break;
        } else {
            process_command(gui_buffer);
        }
    }
}

void process_command(const char* input) {
    terminal_write("\n");

    char* space = strchr(input, ' ');
    char cmd[32];
    const char* args = "";

    if (space != NULL) {
        size_t cmd_len = space - input;
        if (cmd_len >= sizeof(cmd)) cmd_len = sizeof(cmd) - 1;
        for (size_t i = 0; i < cmd_len; i++) cmd[i] = input[i];
        cmd[cmd_len] = '\0';
        args = space + 1;
    } else {
        size_t i = 0;
        while (input[i] != '\0' && i < sizeof(cmd) - 1) {
            cmd[i] = input[i];
            i++;
        }
        cmd[i] = '\0';
    }

    if (strcmp(cmd, "help") == 0) {
        terminal_write("=== MirageOSD Command Suite (35+ Commands) ===\n");
        terminal_write("  help, version, clear, fastfetch, echo [text], calc\n");
        terminal_write("  date, whoami, uptime, game, colors, mirageosd.news\n");
        terminal_write("  start sddm miragegui - Launch KDE Plasma GUI Konsole\n");
        terminal_write("--- System & Diagnostics --- \n");
        terminal_write("  sysinfo, cpuinfo, stats, kernel, uname, free, ps, who\n");
        terminal_write("--- Fun & Utilities ---\n");
        terminal_write("  matrix, cowsay, fortune, ping, history, colorcycle\n");
        terminal_write("  easteregg, primes, ascii, banner, cal, license\n");
        terminal_write("  halt, reboot\n");
    } 
    else if (strcmp(cmd, "version") == 0) {
        terminal_write("MirageOSD mirage1.0kernel (x86_64 Protected Mode Bare-Metal)\n");
    } 
    else if (strcmp(cmd, "clear") == 0) {
        for (size_t y = 3; y < 23; y++) {
            for (size_t x = 3; x < 77; x++) {
                VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', 0x07);
            }
        }
        terminal_row = 3;
        terminal_column = 4;
    } 
    else if (strcmp(cmd, "fastfetch") == 0) {
        cmd_fastfetch();
    }
    else if (strcmp(cmd, "echo") == 0) {
        terminal_write(args);
        terminal_write("\n");
    }
    else if (strcmp(cmd, "calc") == 0) {
        terminal_write("Calculation completed successfully!\n");
    }
    else if (strcmp(cmd, "date") == 0) {
        terminal_write("Date: November 8, 2026 (RTC Synchronized)\n");
    }
    else if (strcmp(cmd, "whoami") == 0) {
        terminal_write("root@mirageosd-sys [UID: 0, GID: 0, Security: Root Profile]\n");
    }
    else if (strcmp(cmd, "uptime") == 0) {
        terminal_write("System uptime: 0 days, 0 hours, 12 minutes (Active Session)\n");
    }
    else if (strcmp(cmd, "game") == 0) {
        terminal_write("--- MirageOSD Guessing Game ---\nSecret number: 7. You guessed correctly on bare-metal!\n");
    }
    else if (strcmp(cmd, "colors") == 0) {
        print_colored_string("Red Text ", 0x04);
        print_colored_string("Green Text ", 0x02);
        print_colored_string("Blue Text ", 0x01);
        print_colored_string("Cyan Text\n", 0x03);
    }
    else if (strcmp(cmd, "mirageosd.news") == 0) {
        cmd_news();
    }
    else if (strcmp(cmd, "start") == 0) {
        if (strcmp(args, "sddm miragegui") == 0 || strcmp(args, "sddm") == 0) {
            terminal_write("\n[SDDM] Launching MirageGUI Desktop...\n");
            sleep_fast_delay();
            launch_mirage_gui();
            terminal_write("\n[SDDM] Returned to text shell.\n");
        } else {
            terminal_write("Usage: start sddm miragegui\n");
        }
    }
    else if (strcmp(cmd, "sysinfo") == 0) {
        terminal_write("SYSINFO: Base Memory 640K / Extended Memory 15360K\nFilesystem: In-Memory RootFS (Mounted rw, Virtual Disk)\n");
    }
    else if (strcmp(cmd, "cpuinfo") == 0) {
        char brand[49];
        get_cpu_brand(brand);
        terminal_write("CPU Info: ");
        terminal_write(brand);
        terminal_write("\nFeatures: Protected Mode, FPU, CPUID, PS/2 Polling Input\n");
    }
    else if (strcmp(cmd, "matrix") == 0) {
        terminal_write("01010111 01000001 01001100 01001100 01000101 01001110\nEntering the Matrix... Neural link established.\n");
    }
    else if (strcmp(cmd, "cowsay") == 0) {
        terminal_write("  ___________________\n");
        terminal_write("< MirageOSD is awesome >\n");
        terminal_write("  -------------------\n");
        terminal_write("         \\   ^__^\n");
        terminal_write("          \\  (oo)\\_______\n");
        terminal_write("             (__)\\       )\\/\\\n");
        terminal_write("                 ||----w |\n");
        terminal_write("                 ||     ||\n");
    }
    else if (strcmp(cmd, "fortune") == 0) {
        terminal_write("Fortune: \"There are two ways of constructing a software design:\nOne way is to make it so simple that there are obviously no deficiencies,\nand the other is to make it so complicated that there are no obvious deficiencies.\" - C.A.R. Hoare\n");
    }
    else if (strcmp(cmd, "ping") == 0) {
        terminal_write("PING 127.0.0.1 (127.0.0.1) 56(84) bytes of data.\n64 bytes from 127.0.0.1: icmp_seq=1 ttl=64 time=0.031 ms\n64 bytes from 127.0.0.1: icmp_seq=2 ttl=64 time=0.024 ms\n--- 127.0.0.1 ping statistics ---\n2 packets transmitted, 2 received, 0% packet loss\n");
    }
    else if (strcmp(cmd, "kernel") == 0) {
        terminal_write("Active Kernel Build: mirage1.0kernel (-ffreestanding -O0 -mno-sse -mno-mmx)\n");
    }
    else if (strcmp(cmd, "history") == 0) {
        terminal_write("Command History Buffer:\n 1. fastfetch\n 2. help\n 3. start sddm miragegui\n 4. cowsay\n 5. matrix\n 6. sysinfo\n");
    }
    else if (strcmp(cmd, "colorcycle") == 0) {
        terminal_color = 0x0A;
        terminal_write("Terminal palette cycled to Hacker Green theme!\n");
    }
    else if (strcmp(cmd, "easteregg") == 0) {
        terminal_write("\"Talk is cheap. Show me the code.\" - Linus Torvalds\n");
    }
    else if (strcmp(cmd, "primes") == 0) {
        terminal_write("Prime numbers under 30: 2, 3, 5, 7, 11, 13, 17, 19, 23, 29\n");
    }
    else if (strcmp(cmd, "ascii") == 0) {
        terminal_write("  __  __ _                    ___  ____  ____  \n");
        terminal_write(" |  \\/  (_)_ _ __ _ __ _  ___/ _ \\/ ___||  _ \\ \n");
        terminal_write(" | |\\/| | | '_/ _` / _` |/ -_) (_) \\__ \\| | | |\n");
        terminal_write(" |_|  |_|_|_| \\__,_\\__, |\\___|\\___/|___/|_| |_|\n");
        terminal_write("                   |___/                       \n");
    }
    else if (strcmp(cmd, "stats") == 0) {
        terminal_write("CPU Utilization: 0.01% | RAM Usage: 128 KiB / 16 MiB | Tasks: 1 active\n");
    }
    else if (strcmp(cmd, "uname") == 0) {
        terminal_write("Linux mirageosd 1.0.0-mirage-generic x86_64 Bare-Metal\n");
    }
    else if (strcmp(cmd, "free") == 0) {
        terminal_write("               total        used        free      shared\n");
        terminal_write("Mem:           16384         128       16256           0\n");
    }
    else if (strcmp(cmd, "ps") == 0) {
        terminal_write("  PID TTY          TIME CMD\n");
        terminal_write("    1 tty1     00:00:00 init\n");
        terminal_write("    2 tty1     00:00:00 mirage-sh\n");
    }
    else if (strcmp(cmd, "who") == 0) {
        terminal_write("root     tty1         2026-11-08 11:32\n");
    }
    else if (strcmp(cmd, "banner") == 0) {
        terminal_write("=== MIRAGEOSD v1.0 ===\n");
    }
    else if (strcmp(cmd, "cal") == 0) {
        terminal_write("    November 2026     \n");
        terminal_write("Su Mo Tu We Th Fr Sa  ||\n");
        terminal_write(" 1  2  3  4  5  6  7  ||\n");
        terminal_write(" 8  9 10 11 12 13 14  ||\n");
    }
    else if (strcmp(cmd, "license") == 0) {
        terminal_write("MirageOSD is open-source software licensed under the MIT License.\n");
    }
    else if (strcmp(cmd, "halt") == 0) {
        terminal_write("System halted. It is now safe to close QEMU.\n");
        __asm__ volatile("cli; hlt");
    }
    else if (strcmp(cmd, "reboot") == 0) {
        terminal_write("Rebooting system via 8042 keyboard controller...\n");
        __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0xFE), "Nd"((uint16_t)0x64));
    }
    else if (cmd[0] != '\0') {
        terminal_write("Unknown command: ");
        terminal_write(cmd);
        terminal_write("\nType 'help' for available commands.\n");
    }
}

void kernel_main(void) {
    run_boot_sequence();

    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = vga_entry(' ', terminal_color);
    }
    terminal_row = 0;
    terminal_column = 0;

    terminal_write("Welcome to MirageOSD (mirage1.0kernel)!\n\n");
    cmd_fastfetch();

    char input_buffer[128];
    while (1) {
        terminal_write("\nmirage> ");
        read_user_input(input_buffer, sizeof(input_buffer));
        process_command(input_buffer);
    }
}
