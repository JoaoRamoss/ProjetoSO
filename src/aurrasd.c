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

char *dir;
char *alto_f, *baixo_f, *eco_f, *rapido_f, *lento_f; //Contem nome dos executaveis de cada filtro.
int alto, baixo, eco, rapido, lento, alto_cur, baixo_cur, eco_cur, rapido_cur, lento_cur;


void sigint_handler (int signum) {
    write(1, "\nA terminar servidor.\n", strlen("\nA terminar servidor.\n"));
    exit(0);
}

void sigalarm_handler(int signum) {
    alarm(1);
}

//Retorna 1 se tivermos filtros disponiveis para executar a transformação.
int check_disponibilidade (char *comando) {
    char *found;
    for (int i = 0; i < 3; i++) {
        found = strsep(&comando, " ");
    }
    found = strsep(&comando, " ");
    do {
        if (!strcmp(found, "alto")) if (alto_cur >= alto) return 0;
        if (!strcmp(found, "baixo")) if (baixo_cur >= baixo) return 0;
        if (!strcmp(found, "eco")) if (eco_cur >= eco) return 0;
        if (!strcmp(found, "rapido")) if (rapido_cur >= rapido) return 0;
        if (!strcmp(found, "lento")) if (lento_cur >= lento) return 0;
    } while ((found = strsep(&comando, " ")) != NULL);
    return 1;
}

int countSpaces (char *text) {
    int count = 0;
    for (int i = 0; text[i]; i++) if (text[i] == ' ') count++;
    return count;
}


char *assignExec(char *nome) {
    if (!strcmp(nome, "alto")) return strdup(alto_f);
    if (!strcmp(nome, "baixo")) return strdup(baixo_f);
    if (!strcmp(nome, "eco")) return strdup(eco_f);
    if (!strcmp(nome, "rapido")) return strdup(rapido_f);
    if (!strcmp(nome, "lento")) return strdup(lento_f);
    return NULL;
}

char **setArgs(char *input, char *output, char *remaining) {
    char **ret = (char **) calloc(100, sizeof(char *));
    char *dirAux = strdup(dir);
    char *let = strsep(&dirAux, "/");
    let = strsep(&dirAux, "\0");
    int idx = 0;
    char *token;
    char *resto = strdup(remaining);
    token = strsep(&resto, " ");
    char aux[100];
    strcpy(aux, let);
    char *one = assignExec(token);
    strcat(aux, one);
    char *firstArg = malloc(30); 
    firstArg[0] = 0;
    strcat(firstArg, "./");
    strcat(firstArg, aux);
    ret[idx++] = strdup(firstArg);
    ret[idx++] = strdup("<");
    char helper[50];
    strcat(helper, "../");
    strcat(helper, input);
    ret[idx++] = strdup(helper);

    if (countSpaces(resto) == 0) {
        char helper2[50];
        strcat(helper2, "../");
        strcat(helper2, output);
        ret[idx++] = strdup(">");
        ret[idx++] = strdup(helper2);
    }
    else {
        while((token = strsep(&resto, " ")) != NULL) {
            if (token != NULL) { 
                char *aux2 = assignExec(token);
                if (aux2 != NULL) {
                    ret[idx++] = strdup("|");
                    char auxiliar[30];
                    strcpy(auxiliar, let);
                    strcat(auxiliar, aux2);
                    ret[idx++] = strdup(auxiliar);
                }
            }
        }
        char helper2[50];
        strcat(helper2, "../");
        strcat(helper2, output);
        ret[idx++] = strdup(">");
        ret[idx++] = strdup(helper2);
    }
    ret[idx] = NULL;
    free(firstArg);
    free(dirAux);
    return ret;
}

int main (int argc, char *argv[]) {
    //Declaração de variaveis
    int bytesRead = 0;
    char **inProcess = (char **) malloc(sizeof(char *) * 100);
    int nProcesses = 0;
    alto = baixo = eco = rapido = lento = alto_cur = baixo_cur = eco_cur = rapido_cur = lento_cur = 0; //Guarda numero de capacidade de filtros

    //No caso de existir um pipe com esse nome cria um novo e substitui.
    struct stat stats;
    //Cria server fifo
    if (stat("tmp/server-client-fifo", &stats) < 0) {
        if (errno != ENOENT) {
            perror("Falha no stat");
            return -1;
        }
    }
    else {
        if (unlink("tmp/server-client-fifo") < 0) {
            perror("Falha no unlink");
            return -1;
        }
    }
    if (mkfifo("tmp/server-client-fifo", 0600) == -1) {
        perror("Erro a abrir o pipe server-client");
        return 1;
    }
    //Cria client fifo
    if (stat("tmp/client-server-fifo", &stats) < 0) {
        if (errno != ENOENT) {
            perror("Falha no stat");
            return -1;
        }
    }
    else {
        if (unlink("tmp/client-server-fifo") < 0) {
            perror("Falha no unlink");
            return -1;
        }
    }
    if (mkfifo("tmp/client-server-fifo", 0600) == -1) {
        perror("Erro a abrir o pipe client-server");
        return 1;
    }
    //Inicialização do servidor

    signal(SIGINT, sigint_handler);
    signal(SIGALRM, sigalarm_handler);

    if (argc == 3) {
        char buffer[1024];
        int fd_config;
        if ((fd_config = open(argv[1], O_RDONLY)) == -1) {
            perror("Erro ao abrir o ficheiro de configuração");
            return 1;
        }
        while ((bytesRead = read(fd_config, buffer, BUF_SIZE) > 0)) {
            char *token = strtok(buffer, "\n");
            do {
                char *aux = strdup(token);
                char *found = strsep(&aux, " ");
                if (!strcmp(found, "alto")) {
                    alto_f = strdup(strsep(&aux, " "));
                    alto = atoi(strsep(&aux, "\n"));
                }
                else if (!strcmp(found, "baixo")) {
                    baixo_f = strdup(strsep(&aux, " "));
                    baixo = atoi(strsep(&aux, "\n"));
                }
                else if (!strcmp(found, "eco")) {
                    eco_f = strdup(strsep(&aux, " "));
                    eco = atoi(strsep(&aux, "\n"));
                }
                else if (!strcmp(found, "rapido")) {
                    rapido_f = strdup(strsep(&aux, " "));
                    rapido = atoi(strsep(&aux, "\n"));
                }
                else {
                    lento_f = strdup(strsep(&aux, " "));
                    lento = atoi(strsep(&aux, "\n"));
                }
            } while ((token = strtok(NULL, "\n")));
        }
        dir = strcat(strdup(argv[2]), "/");
    }
    //Fim da inicialização do servidor.

    int leitura = 0;
    char comando[BUF_SIZE];
    int server_client_fifo;
    int client_server_fifo;
    if ((client_server_fifo = open("tmp/client-server-fifo", O_RDONLY)) == -1) {
        perror("Erro a abrir pipe de cliente");
        return -1;
    }
    if ((server_client_fifo = open("tmp/server-client-fifo", O_WRONLY)) == -1) {
        perror("Erro a abrir pipe de servidor");
        return -1;
    }

    //Vai lendo comandos vindos do cliente
    while (1) {
        alarm(1);
        pause();
        if ((leitura = read(client_server_fifo, comando, BUF_SIZE)) > 0) {
            if (!strcmp(comando, "status")) {
                char mensagem[5000];
                char res[5000];
                res[0] = 0;
                sprintf(mensagem, "Status: \n");
                strcat(res, mensagem);
                sprintf(mensagem, "filter alto: %d/%d (current/max)\n", alto_cur, alto);
                strcat(res, mensagem);
                sprintf(mensagem, "filter baixo: %d/%d (current/max)\n", baixo_cur, baixo);
                strcat(res, mensagem);
                sprintf(mensagem, "filter eco: %d/%d (current/max)\n", eco_cur, eco);
                strcat(res, mensagem);
                sprintf(mensagem, "filter rapido: %d/%d (current/max)\n", rapido_cur, rapido);
                strcat(res, mensagem);
                sprintf(mensagem, "filter lento: %d/%d (current/max)\n", lento_cur, lento);
                strcat(res, mensagem);
                sprintf(mensagem, "pid: %d\n", getpid());
                strcat(res, mensagem);
                strcat(res, "\0");
                write(server_client_fifo, res, strlen(res));
            }
            else {
                int pid = -1;
                write(server_client_fifo, "Pending...\n", strlen("Pending...\n"));
                if(check_disponibilidade(strdup(comando))) {
                    write(server_client_fifo, "Processing...\n", strlen("Processing...\n"));
                    int fd[2];
                    if (pipe(fd) < 0) {
                        perror("[aurrasd]: Erro em abertura de pipe anónimo");
                        return -1;
                    }
                    if (!(pid = fork())) {
                        close(fd[0]);
                        char *args = strdup(comando);
                        write(fd[1], args, strlen(args));
                        close(fd[1]);
                        nProcesses++;
                        char *found;
                        found = strsep(&args, " ");
                        char *input = strsep(&args, " ");
                        char *output = strsep(&args, " ");
                        char *resto = strsep(&args, "\n");
                        char **argumentos = setArgs(input, output, resto);
                        char *agr2[] = {"./bin/aurrasd-filters/aurrasd-gain-double","aurrasd-gain-double", "<", "samples/sample-1-so.m4a", ">", "output.m4a", NULL};
                        for (int i = 0; argumentos[i] != NULL; i++) {
                            printf("%s ", argumentos[i]);
                        }
                        printf("\n");
                    
                        if (execv(*agr2, agr2) == -1) {
                            perror("Erro em execvp");
                            return -1;
                        }
                        _exit(0);
                    }
                    else if (pid > 0){
                        int status = wait(&status);
                        if (WIFEXITED(status)) {
                        close(fd[1]);
                        char help[BUF_SIZE];
                        printf("Terminated\n");
                        while (read(fd[0], help, BUF_SIZE) > 0);
                        close(fd[0]);
                        inProcess[nProcesses-1] = strdup(help);
                        }
                    }
                }
            }
        }
    }

}