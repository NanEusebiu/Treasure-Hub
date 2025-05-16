#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

int monitor_pipe[2];
pid_t monitor_pid = -1;
int monitor_running = 0;
int monitor_terminated = 0;

void write_command_to_file(const char *cmd)
{
    int fd = open("command.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        perror("Nu s a deschis command.txt");
        return;
    }
    write(fd, cmd, strlen(cmd));
    write(fd, "\n", 1);
    close(fd);
}

void start_monitor()
{
    if (monitor_running)
    {
        write(1, "Monitorul ruleaza deja\n", 23);
        return;
    }

    if (pipe(monitor_pipe) == -1) // Creează pipe-ul
    {
        perror("Eroare la crearea pipe-ului");
        exit(1);
    }

    monitor_pid = fork();
    if (monitor_pid == 0)
    {
        // Procesul copil (monitor)
        close(monitor_pipe[0]); // Închide capătul de citire al pipe-ului
        dup2(monitor_pipe[1], STDOUT_FILENO); // Redirectează stdout către pipe
        close(monitor_pipe[1]); // Închide capătul de scriere al pipe-ului
        execl("./monitor", "./monitor", NULL);
        perror("Eroare la execl");
        exit(-1);
    }
    else if (monitor_pid > 0)
    {
        // Procesul părinte
        close(monitor_pipe[1]); // Închide capătul de scriere al pipe-ului
        monitor_running = 1;
        monitor_terminated = 0;
        char msg[64];
        snprintf(msg, sizeof(msg), "Monitor PID: %d\n", monitor_pid);
        write(1, msg, strlen(msg));
    }
    else
    {
        perror("Nu s-a putut crea procesul monitor");
    }
}

void read_from_monitor()
{
    char buffer[1024];
    ssize_t bytes_read = read(monitor_pipe[0], buffer, sizeof(buffer) - 1);
    if (bytes_read > 0)
    {
        buffer[bytes_read] = '\0'; // Termină șirul
        write(1, buffer, bytes_read); // Afișează rezultatele în terminal
    }
}

void stop_monitor()
{
    if (!monitor_running)
    {
        write(0, "Monitorul nu merge\n", 18);
        return;
    }
    kill(monitor_pid, SIGTERM);
    monitor_running = 0;
}

void send_command(const char *cmd_string, int signal)
{
    if (!monitor_running) 
    {
        write(0, "Porneste monitorul\n", 20);
        return;
    }
    write_command_to_file(cmd_string);
    kill(monitor_pid, signal);
}

void list_hunts()
{
    send_command("list_hunts", SIGUSR1);
    read_from_monitor();

}

void list_treasures()
{
    char hunt_id[256];
    write(0, "hunt ID:", 8);
    if (read(0, hunt_id, sizeof(hunt_id)) <= 0)
    {
        write(0, "Incearca alt hunt ID\n", 22);
        return;
    }
    hunt_id[strcspn(hunt_id, "\n")] = '\0';
    char cmd[256];
    strcpy(cmd, "list_treasures ");
    strcat(cmd, hunt_id);
    send_command(cmd, SIGUSR2);
    read_from_monitor();
}

void view_treasure()
{
    char hunt_id[256];
    char treasure_id[256];

    write(0, "hunt ID: ", 9);
    if (read(0, hunt_id, sizeof(hunt_id)) <= 0)
    {
        write(0, "Incearca alt ID de hunt\n", 25);
        return;
    }
    hunt_id[strcspn(hunt_id, "\n")] = '\0';

    write(0, "treasure ID: ", 13);
    if (read(0, treasure_id, sizeof(treasure_id)) <= 0)
    {
        write(0, "Incearca alt ID de treasure\n", 29);
        return;
    }
    treasure_id[strcspn(treasure_id, "\n")] = '\0';

    char cmd[256];
    strcpy(cmd, "view_treasure ");
    strcat(cmd, hunt_id);
    strcat(cmd, " ");
    strcat(cmd, treasure_id);
    send_command(cmd, SIGUSR2);
}

void sigchld_handler(int sig)
{
    if (sig == SIGCHLD) 
    {
        int status;
        pid_t child_pid = waitpid(monitor_pid, &status, WNOHANG);
        if (child_pid == monitor_pid)
        {
            monitor_running = 0;
            monitor_terminated = 1;
            write(0,"Proces monitor terminat\n", 25);
            if (WIFEXITED(status))
            {
                char msg[64];
                snprintf(msg, sizeof(msg), "Status: %d\n", WEXITSTATUS(status));
                write(0, msg, strlen(msg));    
            } 
            else
                if (WIFSIGNALED(status))
                {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "Semnal de sfrasit: %d\n", WTERMSIG(status));
                    write(0, msg, strlen(msg));
                }
        }
    }
}

void calculate_score()
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
            pid_t pid = fork();
            if (pid == 0)
            {
                // Proces copil
                execl("./calculate_score", "./calculate_score", entry->d_name, NULL);
                perror("Error executing calculate_scores");
                exit(1);
            }
        }
    }
    closedir(dir);

    // Așteaptă toate procesele copil
    while (wait(NULL) > 0);
}

int main()
{
    char buffer[256];
    int option = 1;
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    while (option)
    {
        ssize_t n = read(0, buffer, sizeof(buffer));
        if (n == -1) 
        {
            perror("citire\n");
            exit(1);
        }
        if (n > 0 && buffer[n - 1] == '\n') 
        {
            buffer[n - 1] = '\0';
        }

        if (strcmp(buffer, "--start_monitor") == 0) 
        {
            write(0, "Ai pornit monitorul\n", 20);
            start_monitor();
        }else 
            if (strcmp(buffer, "--list_hunts") == 0)
            {
                write(0, "List hunts\n", 11);
                if (!monitor_running) 
                {
                write(0, "Monitorul nu merge\n", 17);
                }
                else
                {
                list_hunts();
                }
            }
            else 
                if(strcmp(buffer, "--list_treasures") == 0)
                {
                    write(0, "List treasures din hunt\n", 25);
                    if(!monitor_running)
                    {
                        write(0, "Monitorul nu merge\n", 17);
                    }
                    else
                    {
                        list_treasures();
                    }
                }
                else
                    if(strcmp(buffer, "--view_treasure") == 0)
                    {
                        write(0, "view treasure\n", 15);
                        if (!monitor_running)
                        {
                            write(0, "Monitorul nu merge\n", 17);
                        }
                        else
                        {
                            view_treasure();
                        }
                    }
                    else
                        if(strcmp(buffer, "--stop_monitor") == 0)
                        {
                            write(0, "stop monitor\n", 13);
                            stop_monitor();
                        }
                        else
                            if(strcmp(buffer, "--exit") == 0)
                            {
                                write(0, "exit\n", 5);
                                if (monitor_running)
                                {
                                write(0, "Monitor merge opreste-l\n", 25);
                                }
                                else
                                {
                                option = 0;
                                }
                            }    
                            else
                                if(strcmp(buffer,"--caculate_score") == 0)
                                {
                                    write(0, "Calculeaza scorul\n", 19);
                                    if (!monitor_running)
                                    {
                                        write(0, "Monitorul nu merge\n", 17);
                                    }
                                    else
                                    {
                                        calculate_score();
                                    }
                                }
                                else
                                {
                                    write(0, "Comanda gresita\n", 17);
                                }
                            }

    if(monitor_running)
    {
        kill(monitor_pid, SIGTERM);
    }
    return 0;
}
