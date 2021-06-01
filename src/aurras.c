#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#define BUF_SIZE 1024

int main (int argc, char *argv[]) {
    int server_client_fifo;
    int client_server_fifo;

    if (!strcmp(argv[1], "status")) {
        if ((client_server_fifo = open("tmp/client-server-fifo", O_WRONLY)) < 0) {
            perror("Erro ao abrir o pipe do cliente");
            return -1;
        }
        write(client_server_fifo, argv[1], strlen(argv[1]));
    }
    if ((server_client_fifo = open("tmp/server-client-fifo", O_RDONLY)) == -1) {
        perror("Erro a abrir pipe de servidor");
        return -1;
    }
    int leitura = 0;
    char buffer[BUF_SIZE];
    while ((leitura = read(server_client_fifo, buffer, BUF_SIZE)) > 0) {
        write(1, buffer, leitura);
        if (strstr(buffer, "\0")) break;
    }

}