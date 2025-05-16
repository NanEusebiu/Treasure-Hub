#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

typedef struct
{
    int id;
    char username[50];
    float latitude;
    float longitude;
    char clue[1000];
    int value;
}Treasure;

void handle_list_hunts()
{
    DIR *dir = opendir("./");
    if (dir == NULL)
    {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.')
        {
            char msg[256];
            strcpy(msg, "Hunt: ");
            strcat(msg, entry->d_name);
            strcat(msg, "\n");
            write(1, msg, strlen(msg)); // Scrie în stdout (redirectat către pipe)
        }
    }
    closedir(dir);
}

void handle_list_treasures(const char *hunt_id)
{
    char msg[512];
    strcat(msg, "Monitor: Treasures in hunt ");
    strcat(msg, hunt_id);
    strcat(msg, "...\n");
    write(0, msg, strlen(msg));

    char hunt_dir[256];
    strcpy(hunt_dir, "./");
    strcat(hunt_dir, hunt_id);
    strcat(hunt_dir, "/treasures.dat");

    int fd = open(hunt_dir, O_RDONLY);
    if (fd == -1)
    {
        perror("Eroare la deschidere");
        return;
    }

    Treasure treasure;
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure))
    {
        printf("ID: %d, Username: %s, Latitude: %.2f, Longitude: %.2f, Clue: %s, Value: %d\n",
               treasure.id, treasure.username, treasure.latitude, treasure.longitude,
               treasure.clue, treasure.value);
    }
    close(fd);
}

void handle_view_treasure(const char *hunt_id, const char *treasure_id)
{
    char msg[512];
    strcpy(msg, "Monitor: Treasure ");
    strcat(msg, treasure_id);
    strcat(msg, " in hunt ");
    strcat(msg, hunt_id);
    strcat(msg, "...\n");
    write(0, msg, strlen(msg));

    char treasure_file_path[256];
    strcpy(treasure_file_path, "./");
    strcat(treasure_file_path, hunt_id);
    strcat(treasure_file_path, "/treasures.dat");

    int fd = open(treasure_file_path, O_RDONLY);
    if (fd == -1)
    {
        perror("Error deschidere\n");
        return;
    }

    Treasure treasure;
    int id = atoi(treasure_id);
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure))
    {
        if (treasure.id == id)
        {
            printf("ID: %d\nUsername: %s\nLatitude: %.2f\nLongitude: %.2f\nClue: %s\nValue: %d\n",
                   treasure.id, treasure.username, treasure.latitude, treasure.longitude,
                   treasure.clue, treasure.value);
            close(fd);
            return;
        }
    }

    write(0,"Treasure negasita\n", 18);
    close(fd);
}

void handle_signal(int sig) 
{
    if (sig == SIGUSR1 || sig == SIGUSR2)
    {
        write(0,"Monitor: semnal primit\n", 23);

        int fd = open("command.txt", O_RDONLY);
        if (fd == -1)
        {
            perror("Monitor: Eroare command.txt");
            return;
        }

        char command[256];
        ssize_t x = read(fd, command, sizeof(command) - 1);
        if (x > 0)
        {
            command[x] = '\0';
            command[strcspn(command, "\n")] = '\0';

            if (strncmp(command, "list_hunts", 10) == 0)
            {
                handle_list_hunts();
            }
            else
                if (strncmp(command, "list_treasures ", 15) == 0)
                {
                    char hunt_id[256];
                    char *token;
                    token = strtok(command + 14, " ");
                    if (token != NULL)
                    {
                        strncpy(hunt_id, token, sizeof(hunt_id) - 1);
                        hunt_id[sizeof(hunt_id) - 1] = '\0';
                    }
                    else
                    {
                        write(0, "Eroare: Hunt ID\n", 17);
                        return;
                    }
                    handle_list_treasures(token);
                }
                else
                    if (strncmp(command, "view_treasure ", 14) == 0)
                    {
                        char hunt_id[256], treasure_id[256];
                        char *token;
                        token = strtok(command + 14, " ");
                        if (token != NULL)
                        {
                            strncpy(hunt_id, token, sizeof(hunt_id) - 1);
                            hunt_id[sizeof(hunt_id) - 1] = '\0';
                        }
                        else
                        {
                            write(0, "Eroare: Hunt ID\n", 17);
                            return;
                        }

                        token = strtok(NULL, " ");
                        if (token != NULL)
                        {
                            strncpy(treasure_id, token, sizeof(treasure_id) - 1);
                            treasure_id[sizeof(treasure_id) - 1] = '\0'; // Asigură terminarea corectă a șirului
                        }
                        else
                        {
                            write(0, "Eroare: Treasure ID\n", 21);
                            return;
                        }
                        handle_view_treasure(hunt_id, treasure_id);
                    }
                    else
                    {
                        write(0,"Monitor: Comanda gresita\n", 25);
                    }
        }
        else
            if (x == -1)
            {
                perror("Monitor: Eroare citire command.txt");
            }
            close(fd);
    }
    else
        if (sig == SIGTERM)
        {
            write(0,"Monitor oprit\n",14);
        }
}

int main()
{
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("sigaction SIGUSR1");
        exit(1);
    }
    if (sigaction(SIGUSR2, &sa, NULL) == -1)
    {
        perror("sigaction SIGUSR2");
        exit(1);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("sigaction SIGTERM");
        exit(1);
    }

    while (1)
    {
        pause();
    }

    return 0;
}
