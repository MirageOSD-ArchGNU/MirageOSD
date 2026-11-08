#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void launch_kde_gui(void) {
    printf("\n[MirageOSD] Switching display to DRM/KMS Framebuffer...\n");
    printf("[MirageOSD] Initializing SDDM Display Manager via systemd...\n\n");
    
    int status = system("sudo systemctl start sddm");
    if (status != 0) {
        system("sudo sddm &");
    }
}

int main(void) {
    char input[256];

    printf("=======================================\n");
    printf(" Welcome to MirageOSD Base Shell v1.0 \n");
    printf(" Type 'help' for command list        \n");
    printf("=======================================\n\n");

    while (1) {
        printf("mirage> ");
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start sddm miragegui") == 0 || strcmp(input, "startgui") == 0) {
            launch_kde_gui();
        } else if (strcmp(input, "fastfetch") == 0 || strcmp(input, "fetch") == 0) {
            system("fastfetch --config ~/.config/fastfetch/mirage_config.jsonc");
        } else if (strcmp(input, "clear") == 0) {
            system("clear");
        } else if (strcmp(input, "reboot") == 0) {
            printf("[MirageOSD] Rebooting system...\n");
            system("sudo reboot");
        } else if (strcmp(input, "poweroff") == 0 || strcmp(input, "shutdown") == 0) {
            printf("[MirageOSD] Shutting down...\n");
            system("sudo poweroff");
        } else if (strcmp(input, "help") == 0) {
            printf("Available MirageOSD commands:\n");
            printf("  start sddm miragegui - Launch KDE Plasma desktop\n");
            printf("  fastfetch            - Display system info with custom ASCII logo\n");
            printf("  clear                - Clear the text screen\n");
            printf("  reboot               - Restart the machine\n");
            printf("  poweroff             - Shutdown the machine\n");
            printf("  exit                 - Exit shell back to login\n");
        } else if (strcmp(input, "exit") == 0) {
            break;
        } else if (strlen(input) > 0) {
            printf("mirage: command not found: %s\n", input);
        }
    }
    return 0;
}
