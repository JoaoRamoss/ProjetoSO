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

void parseArgs (int argc, char *argv[], char *res) {
    for (int i = 1; i < argc; i++) {
        strcat(res, argv[i]);
        strcat(res, " ");
    }
}

int main (int argc, char *argv[]) {
    int server_client_fifo;
    int client_server_fifo;
    int processing_fifo;

    if ((client_server_fifo = open("tmp/client-server-fifo", O_WRONLY)) < 0) {
            perror("Erro ao abrir o pipe do cliente");
            return -1;
    }

    if ((server_client_fifo = open("tmp/server-client-fifo", O_RDONLY)) == -1) {
        perror("Erro a abrir pipe de servidor");
        return -1;
    }

    if ((processing_fifo = open("tmp/processing-fifo", O_RDONLY)) == -1) {
        perror("Erro a abrir pipe de processing");
        return -1;
    }

    if (argc == 1) {
        printf("Utilização:\n");
        printf("\"bin/aurras status\" -> Recebe o estado dos processos do servidor.\n");
        printf("\"bin/aurras transform <ficheiro> <nome resultante> <filtros>\" -> transforma o ficheiro de áudio.\n");
        return 0;
    }

    if (!strcmp(argv[1], "status")) { 
        write(client_server_fifo, argv[1], strlen(argv[1]));
        close(client_server_fifo);

        int leitura = 0;
        char buffer[BUF_SIZE];
        while ((leitura = read(server_client_fifo, buffer, BUF_SIZE)) > 0) {
            write(1, buffer, leitura);
            if (strstr(buffer, "\0")) break;
        }
        close(server_client_fifo);
    }

    if (!strcmp(argv[1], "transform")) {
        char aux[BUF_SIZE];
        parseArgs(argc, argv, aux);
        write(client_server_fifo, aux, strlen(aux));
        close(client_server_fifo);
        int leitura = 0;
        char buffer[BUF_SIZE];
        while ((leitura = read(processing_fifo, buffer, BUF_SIZE)) > 0) {
            write(1, buffer, leitura);
            if (strstr(buffer, "Processing...")) break;
        }
        close(processing_fifo);
    }
}