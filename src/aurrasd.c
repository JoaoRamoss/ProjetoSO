#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

int main (int argc, char *argv[]) {
    int bytesRead = 0;
    char alto_f[1024], baixo_f[1024], eco_f[1024], rapido_f[1024], lento_f[1024];
    int alto, baixo, eco, rapido, lento;
    alto = baixo = eco = rapido = lento = 0;

    if (mkfifo("tmp/server-client-fifo", 0600) == -1) {
        perror("Erro a abrir o pipe server-client");
        return 1;
    }
    if (argc == 3) {
        char buffer[1024];
        int fd_config;
        if ((fd_config = open(argv[1], O_RDONLY)) == -1) {
            perror("Erro ao abrir o ficheiro de configuração");
            return 1;
        }
        while ((bytesRead = read(fd_config, buffer, 1024)) > 0) {
            char *token = strtok(buffer, " ");
            if (!strcmp(token, "alto")) {
                strtok(NULL, " ");
                alto = atoi(strtok(NULL, "\n"));
            }
            else if (!strcmp(token, "baixo")) {
                strtok(NULL, " ");
                baixo = atoi(strtok(NULL, "\n"));
            }
            else if (!strcmp(token, "eco")) {
                strtok(NULL, " ");
                eco = atoi(strtok(NULL, "\n"));
            }
            else if (!strcmp(token, "rapido")) {
                strtok(NULL, " ");
                rapido = atoi(strtok(NULL, "\n"));
            }
            else {
                strtok(NULL, " ");
                lento = atoi(strtok(NULL, "\n"));
            }
        }
    }
}