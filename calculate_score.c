#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>

typedef struct {
    int id;
    char username[50];
    float latitude;
    float longitude;
    char clue[1000];
    int value;
} Treasure;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        perror("Nu sunt suf arg\n");
        return 1;
    }

    char file_path[256];
    strcpy(file_path,"./");
    strcat(file_path, argv[1]);
    strcat(file_path, "/treasures.dat");

    struct stat st;
    if(stat(file_path, &st) != 0)
    {
        perror("treasures.dat nu exista \n");
        return 1;
    }

    int fd = open(file_path,O_RDONLY);
    if(fd == -1)
    {
        perror("Eroare treasures.dat\n");
        return 1;
    }

    Treasure treasure;
    int scores[256] = {0};
    char usernames[256][50] = {{0}};

    while (read(fd,&treasure, sizeof(Treasure)) == sizeof(Treasure))
    {
        scores[treasure.id] += treasure.value;
        strcpy(usernames[treasure.id], treasure.username);
    }
    close(fd);

    for (int i = 0; i < 256; i++)
    {
        if (scores[i] > 0)
        {
            write(1, "User: ", 6);
            write(1, usernames[i], strlen(usernames[i]));
            write(1, ", Score: ", 9);
            printf("%d\n", scores[i]);
        }
    }

    return 0;
}
