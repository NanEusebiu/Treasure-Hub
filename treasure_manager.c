#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

typedef struct
{
    int id;
    char username[50];
    float latitude;
    float longitude;
    char clue[1000];
    int value;
} Treasure;

void create_symlink(const char *hunt_id)
{
    char path[256];
    strcpy(path, hunt_id);
    strcat(path, "/log");

    char symlinkk[256];
    strcpy(symlinkk, "logged_hunt-<");
    strcat(symlinkk, hunt_id);
    strcat(symlinkk, ">");

    if(symlink(path, symlinkk) == -1) 
    {
        if(errno == EEXIST) 
        {
            perror("Link existent\n");
        } 
        else 
        {
            perror("Eroare la creare link\n ");
        }
    } 
    else 
    {
        printf("symlink: %s -> %s\n", symlinkk, path);
    }
}

void logg(const char *hunt_id, const char *message) 
{
    char log[256];
    strcpy(log, hunt_id);
    strcat(log, "/log");

    int fd = open(log, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(fd == -1) 
    {
        perror("Eroare la deschidere log\n");
        exit(-1);
    }

    time_t tim = time(NULL);
    char *t = ctime(&tim);
    if(t != NULL) 
    {
        t[strlen(t) - 1] = '\0';
    }

    char mesaj[1024];
    strcpy(mesaj, "[");
    strcat(mesaj, t);
    strcat(mesaj, "] ");
    strcat(mesaj, message);
    strcat(mesaj, "\n");

    if(write(fd, mesaj, strlen(mesaj)) == -1) 
    {
        perror("Eroare la scriere in log\n");
    }

    close(fd);
    create_symlink(hunt_id);
}


void add(const char *hunt_id) 
{
    struct stat st = {0};
    if(stat(hunt_id, &st) == -1) 
    {
        if(mkdir(hunt_id, 0755) == -1) 
        {
            perror("Eroare la creare director\n");
            exit(-1);
        }
    }
    
    Treasure treasure;
    char buffer[1024]; 
    size_t n;

    write(0,"id: ",5);
    if((n = read(0,buffer,sizeof(buffer))) == -1)
    {
        perror("Eroare la citire id\n");
        exit(-1);
    }
    buffer[n-1] = '\0';
    treasure.id = atoi(buffer);

    write(0,"username: ",11);
    if((n = read(0, buffer, sizeof(buffer))) == -1) 
    {
        perror("Eroare la citire username\n");
        exit(-1);
    }
    buffer[n - 1] = '\0';
    strncpy(treasure.username, buffer, sizeof(treasure.username) - 1);
    treasure.username[sizeof(treasure.username) - 1] = '\0';

    write(0,"latitude: ", 11);
    if((n = read(0, buffer, sizeof(buffer))) == -1) 
    {
        perror("Eroare la citire latitude\n");
        exit(-1);
    }
    buffer[n - 1] = '\0';
    treasure.latitude = atof(buffer);

    write(0,"longitude: ",12);
    if((n = read(0, buffer, sizeof(buffer))) == -1) 
    {
        perror("Eroare la citire longitude\n");
        exit(-1);
    }
    buffer[n - 1] = '\0';
    treasure.longitude = atof(buffer);

    write(0,"clue: ",7);
    if((n = read(0, buffer, sizeof(buffer))) == -1)
     {
        perror("Eroare la citire clue\n");
        exit(-1);
    }
    buffer[n - 1] = '\0';
    strncpy(treasure.clue, buffer, sizeof(treasure.clue));
    treasure.clue[sizeof(treasure.clue) - 1] = '\0';

    write(0,"value: ",8);
    if((n = read(0, buffer, sizeof(buffer))) == -1) 
    {
        perror("Eroare la citire value\n");
        exit(-1);
    }
    buffer[n - 1] = '\0';
    treasure.value = atoi(buffer);

    char file_path[256];
    strcpy(file_path, hunt_id);
    strcat(file_path, "/treasures.dat");

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if( fd == -1) 
    {
        perror("Eroare la deschidere\n");
        exit(-1);
    }

    if(write(fd, &treasure, sizeof(Treasure)) != sizeof(Treasure)) 
    {
        perror("Eroare la scriere fisier\n");
        close(fd);
        exit(-1);
    }

    close(fd);
    write(0,"Comoara adaugata\n",18);
    logg(hunt_id, "Adaugat\n");
}


void list(const char *hunt_id)
{
    char file_path[256];
    strcpy(file_path, hunt_id);
    strcat(file_path, "/treasures.dat");

    struct stat file_stat;
    if (stat(file_path, &file_stat) == -1) 
    {
        perror("Eroare la obținerea informațiilor despre fișier");
        exit(-1);
    }

    printf("Nume: %s\n", hunt_id);
    printf("Size: %ld\n", file_stat.st_size);
    printf("Ultima modificare: %s", ctime(&file_stat.st_mtime));

    
    int fd = open(file_path, O_RDONLY);
    if(fd == -1) 
    {
        perror("Eroare la deschidere fisier\n");
        exit(-1);
    }

    Treasure treasure;
    while(read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) 
    {
        printf("%d  %s  %.2f  %.2f  %d  %s\n", treasure.id, treasure.username, treasure.latitude, treasure.longitude, treasure.value, treasure.clue);     
    }
    close(fd);

    char log[1024];
    strcpy(log, hunt_id);
    strcat(log, "/log");

    int lfd = open(log, O_RDONLY);
    if (lfd == -1) 
    {
        perror("Eroare la deschidere log\n");
        return;
    }

    char buffer[1024];
    size_t n;
    while((n = read(lfd, buffer, sizeof(buffer) - 1)) > 0) 
    {
        buffer[n-1] = '\0';
        printf("%s", buffer);
    }
    if(n == -1) 
    {
        perror("Eroare la citire log\n");
        close(lfd);
        exit(-1);
    }
    close(lfd);

    logg(hunt_id, "Afisare\n");
}


void view(const char *hunt_id, int id)
{
    char file_path[256];
    strcpy(file_path, hunt_id);
    strcat(file_path, "/treasures.dat");

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        perror("Eroare la deschidere fisier\n");
        exit(-1);
    }

    Treasure treasure;
    while(read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) 
    {
        if (treasure.id == id)
        {
            printf("id: %d \n", id);
            printf("username: %s\n", treasure.username);
            printf("latitude: %.2f\n", treasure.latitude);
            printf("longitude: %.2f\n", treasure.longitude);
            printf("clue: %s\n", treasure.clue);
            printf("value: %d\n", treasure.value);

            logg(hunt_id, "Vazuta\n");
            close(fd);
            return;
        }
    }

    printf("Comoara negasita\n");
    close(fd);
}

void remove_hunt(const char *hunt_id) 
{
    char file_path[256];
    strcpy(file_path, hunt_id);
    strcat(file_path, "/treasures.dat");

    if(remove(file_path) == -1) 
    {
        perror("Eroare la stergere fisier\n");
        exit(-1);
    }

    char log[256];
    strcpy(log, hunt_id);
    strcat(log, "/log");

    if (remove(log) == -1) 
        {
            perror("Eroare la stergere log\n");
        }

    if(rmdir(hunt_id) == -1) 
    {
        perror("Eroare la stergere director\n");
        exit(-1);
    }

    write(0,"Hunt sters\n",12);
    logg(hunt_id, "Sters\n");
}

void remove_treasure(const char *hunt_id, int id)
{
    char file_path[256];
    strcpy(file_path, hunt_id);
    strcat(file_path, "/treasures.dat");

    char file_path2[256];
    strcpy(file_path2, hunt_id);
    strcat(file_path2, "/ceva.dat");

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        perror("Eroare la deschidere fisier orig");
        exit(-1);
    }

    int fd2 = open(file_path2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd2 == -1) 
    {
        perror("Eroare la crearea fisierului temporar");
        close(fd);
        exit(-1);
    }

    Treasure treasure;
    int gasit = 0;

    
    while(read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) 
    {
        if(treasure.id == id) 
        {
            gasit = 1;
            continue;
        }
        if(write(fd2, &treasure, sizeof(Treasure)) != sizeof(Treasure)) 
        {
            perror("Eroare la scriere în fisier2\n");
            close(fd);
            close(fd2);
            exit(-1);
        }
    }

    close(fd);
    close(fd2);

    if(gasit) 
    {
        if(rename(file_path2, file_path) == -1)
        {
            perror("Eroare la înlocuire fisier\n");
        } else 
        {
            write(0,"Comoara eliminata\n",19);
            logg(hunt_id, "Eliminata\n");
        }
    } 
    else 
    {
        write(0,"Comoara negasita\n",18);
        remove(file_path2);
        logg(hunt_id, "Negasita\n");
    }
}

int main(int argc, char ** argv)
{
    if(argc < 3) 
    {
        perror("nu e nr suf de arg\n");
        exit(-1);
    }
    if(strcmp(argv[1], "--add") == 0)
    {
        add(argv[2]);
    }
    else
    {
        if(strcmp(argv[1],"--list")==0)
        {
            list(argv[2]);
        }
        else
        {
            if(strcmp(argv[1],"--view")==0 && argc==4)
            {
                view(argv[2],atoi(argv[3]));
            }
            else
            {
                if(strcmp(argv[1],"--remove_treasure")==0 && argc==4)
                {
                    remove_treasure(argv[2],atoi(argv[3]));
                }
                else
                {
                    if(strcmp(argv[1],"--remove_hunt")==0 && argc==3)
                    {
                        remove_hunt(argv[2]);
                    }
                    else
                    {
                        return 1;
                    }
                }
            }
        }
    }
}
