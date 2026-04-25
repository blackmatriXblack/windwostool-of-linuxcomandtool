/*
 * WSL Command Bridge & Enumerator
 * Host OS: Windows
 * Target OS: Linux (via WSL)
 * Method: Uses Windows '_popen' to bridge into Linux Kernel execution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// Buffer size for reading large output from Linux
#define BUFFER_SIZE 4096
#define MAX_CMD_STORE 5000 // We will store up to 5000 commands in memory

// Global array to store command names
char *command_list[MAX_CMD_STORE];
int total_commands = 0;

void SetupConsole() {
    // Enable ANSI escape codes for Windows 10/11 Console to support colors
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

// This function bridges Windows C -> WSL Linux
// It runs a bash script to find ALL executables in the standard Linux paths
void FetchLinuxCommands() {
    printf("\n\033[1;33m[SYSTEM] Connecting to WSL Linux Kernel...\033[0m\n");
    printf("\033[1;33m[SYSTEM] Scanning /bin, /usr/bin, /sbin for executables...\033[0m\n");

    // The command sends a bash script into WSL:
    // 1. ls various bin directories
    // 2. sort them
    // 3. uniq (remove duplicates)
    const char *wsl_query = "wsl -e bash -c \"ls /bin /usr/bin /sbin /usr/sbin | sort | uniq\"";

    // _popen opens a pipe to the command. It's like opening a file, but it's a running process.
    FILE *pipe = _popen(wsl_query, "r");

    if (!pipe) {
        printf("\033[1;31mError: Could not connect to WSL. Is Linux installed?\033[0m\n");
        return;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        // Remove newline
        buffer[strcspn(buffer, "\n")] = 0;

        // Store simple names only (ignore empty lines)
        if (strlen(buffer) > 0 && total_commands < MAX_CMD_STORE) {
            command_list[total_commands] = strdup(buffer);
            total_commands++;
        }
    }

    _pclose(pipe);
    printf("\033[1;32m[SUCCESS] Connection Established. Found %d Linux commands.\033[0m\n", total_commands);
}

void DisplayCommands() {
    printf("\n\033[1;36m=== AVAILABLE LINUX COMMANDS (Inside WSL) ===\033[0m\n");

    int cols = 0;
    for (int i = 0; i < total_commands; i++) {
        // Print in green
        printf("\033[32m%-20s\033[0m", command_list[i]);
        cols++;
        if (cols >= 4) { // 4 columns per row
            printf("\n");
            cols = 0;
        }
    }
    printf("\n\033[1;36m=============================================\033[0m\n");
}

void RunLinuxCommand(char *cmd) {
    char full_cmd[1024];

    // Check if the command exists in our list (Optional security check)
    // Construct the bridge command: "wsl <command>"
    snprintf(full_cmd, sizeof(full_cmd), "wsl %s", cmd);

    printf("\033[1;30m[Executing in Linux Kernel via WSL Bridge...]\033[0m\n\n");

    // Execute using system()
    int status = system(full_cmd);

    if (status != 0) {
        printf("\n\033[1;31m[Error] Linux command returned exit code %d\033[0m\n", status);
    }
}

int main() {
    SetupConsole(); // Enable colors

    printf("\033[1;37m====================================================\n");
    printf("      WINDOWS-LINUX HYBRID COMMANDER v3.0           \n");
    printf("====================================================\033[0m\n");

    // 1. Fetch commands from Linux
    FetchLinuxCommands();

    if (total_commands == 0) {
        printf("No commands found. Please ensure WSL is running.\n");
        system("pause");
        return 1;
    }

    // 2. Interactive Loop
    char input[512];
    while (1) {
        printf("\n\033[1;33mCommands available: %d\033[0m\n", total_commands);
        printf("Type 'list' to show all commands.\n");
        printf("Type 'exit' to quit.\n");
        printf("\033[1;32mlinux@windows $ \033[0m");

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0) break;

        if (strcmp(input, "list") == 0) {
            DisplayCommands();
        }
        else if (strlen(input) > 0) {
            // Run arbitrary Linux command
            RunLinuxCommand(input);
        }
    }

    // Clean up memory
    for (int i = 0; i < total_commands; i++) {
        free(command_list[i]);
    }

    return 0;
}