#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> 

typedef struct {
    int id;
    char username[50];
    float latitude;
    float longitude;
    char clue[1000];
    int value;
} Treasure;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char file_path[256];
    snprintf(file_path, sizeof(file_path), "./%s/treasures.dat", argv[1]);

    // Verifică dacă fișierul există
    struct stat st;
    if (stat(file_path, &st) != 0) {
        fprintf(stderr, "Error: treasures.dat not found for hunt %s\n", argv[1]);
        return 1;
    }

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Error opening treasures file");
        return 1;
    }

    // Restul codului pentru calcularea scorurilor
    Treasure treasure;
    int scores[256] = {0}; // Scorurile utilizatorilor (indexate după ID-ul utilizatorului)
    char usernames[256][50] = {{0}}; // Numele utilizatorilor

    while (fread(&treasure, sizeof(Treasure), 1, file) == 1) {
        scores[treasure.id] += treasure.value;
        strcpy(usernames[treasure.id], treasure.username);
    }
    fclose(file);

    for (int i = 0; i < 256; i++) {
        if (scores[i] > 0) {
            printf("User: %s, Score: %d\n", usernames[i], scores[i]);
        }
    }

    return 0;
}
