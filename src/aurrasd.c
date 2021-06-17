#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <poll.h>
#define BUF_SIZE 1024

typedef struct queue {
    char **line; //Comando em espera.
    int filled; //Inidice da cauda.
    int pos; //Proximo processo em execução.
} Queue;

//Variaveis globais necessárias para a execução do programa.
char *dir; //Diretoria dos executáveis dos filtros.
char *alto_f, *baixo_f, *eco_f, *rapido_f, *lento_f; //Contem nome dos executaveis de cada filtro.
int nProcesses = 0; //N de processos ativos
char *inProcess[1024]; //Processos em execução
int alto, baixo, eco, rapido, lento, alto_cur, baixo_cur, eco_cur, rapido_cur, lento_cur; //N maximo e atual de filtros em uso

int check_disponibilidade (char *comando);

Queue *initQueue () {
    Queue *fila = calloc(1, sizeof(struct queue));
    fila->line = (char **) calloc(100, sizeof(char *));
    fila->pos = 0;
    fila->filled = -1;
    return fila;
}
void freeSlots(char *arg);

int canQ (Queue *q) {
    if (q->filled >= 0 && q->pos <= q->filled) {
        if (check_disponibilidade(q->line[q->pos])) return 1;
    }
    return 0;
}


//Handler do SIGINT (terminação graceful)
void sigint_handler (int signum) {
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0) wait(NULL); //Termina todos os filhos.

    //Remove os ficheiros temporários criados durante a execução.
    write(1, "\nA terminar servidor.\n", strlen("\nA terminar servidor.\n"));
    if (unlink("tmp/server-client-fifo") == -1) {
        perror("[server-client-fifo] Erro ao eliminar ficheiro temporário");
        _exit(-1);
    }
    if (unlink("tmp/client-server-fifo") == -1) {
        perror("[client-server-fifo] Erro ao eliminar ficheiro temporário");
        _exit(-1);
    }
    if (unlink("tmp/processing-fifo") == -1) {
        perror("[processing-fifo] Erro ao eliminar ficheiro temporário");
        _exit(-1);
    }
    _exit(0);
}

void sigterm_handler (int signum) {
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0) wait(NULL); //Termina todos os filhos.

    //Remove os ficheiros temporários criados durante a execução.
    write(1, "\nA terminar servidor.\n", strlen("\nA terminar servidor.\n"));
    if (unlink("tmp/server-client-fifo") == -1) {
        perror("[server-client-fifo] Erro ao eliminar ficheiro temporário");
        _exit(-1);
    }
    if (unlink("tmp/client-server-fifo") == -1) {
        perror("[client-server-fifo] Erro ao eliminar ficheiro temporário");
        _exit(-1);
    }
    if (unlink("tmp/processing-fifo") == -1) {
        perror("[processing-fifo] Erro ao eliminar ficheiro temporário");
        _exit(-1);
    }
    _exit(0);
}

//Handler do sinal SIGCHLD
void sigchld_handler(int signum) {
    char *tok = strdup(inProcess[--nProcesses]); //Guarda o comando (Alterações necessárias aqui)
    strsep(&tok, " ");
    strsep(&tok, " ");
    strsep(&tok, " ");
    char *resto = strsep(&tok, "\n");
    freeSlots(resto); //Coloca os filtros como novamente disponíveis.
    free(tok);
}

//Atualiza informação sobre filtros em uso.
 void updateSlots(char *arg) {
    char *dup = strdup(arg);
    char *tok;
    //Dependendo do comando inserido, atualiza as variáveis correspondentes aos filtros em uso.
    while((tok = strsep(&dup, " "))) {
        if (!strcmp(tok, "alto")) alto_cur++;
        if (!strcmp(tok, "baixo")) baixo_cur++;
        if (!strcmp(tok, "eco")) eco_cur++;
        if (!strcmp(tok, "rapido")) rapido_cur++;
        if (!strcmp(tok, "lento")) lento_cur++;
    }
    free(dup);
}

//Atualiza informação sobre os filtros em uso
void freeSlots(char *arg) {
    char *dup = strdup(arg);
    char *tok;
    //Funcionamento igual ao updateSlots() mas decrementa em vez de incrementar.
    while((tok = strsep(&dup, " "))) {
        if (!strcmp(tok, "alto")) alto_cur--;
        if (!strcmp(tok, "baixo")) baixo_cur--;
        if (!strcmp(tok, "eco")) eco_cur--;
        if (!strcmp(tok, "rapido")) rapido_cur--;
        if (!strcmp(tok, "lento")) lento_cur--;
    }
    free(dup);
}

//Retorna 1 se tivermos filtros disponiveis para executar a transformação.
int check_disponibilidade (char *command) {
    char *comando = strdup(command);
    char *found;
    for (int i = 0; i < 3; i++) {
        found = strsep(&comando, " ");
    }
    found = strsep(&comando, " ");
    //Basta um filtro não estar disponível e a função retorna 0 (0 -> não há disponibilidade para execução do comando).
    do {
        if (!strcmp(found, "alto")) if (alto_cur >= alto) return 0;
        if (!strcmp(found, "baixo")) if (baixo_cur >= baixo) return 0;
        if (!strcmp(found, "eco")) if (eco_cur >= eco) return 0;
        if (!strcmp(found, "rapido")) if (rapido_cur >= rapido) return 0;
        if (!strcmp(found, "lento")) if (lento_cur >= lento) return 0;
    } while ((found = strsep(&comando, " ")) != NULL);
    free(comando);
    return 1;
}

//Conta numero de espaços num dado texto. (Serve para contar quantos filtros foram dados como argumento no comando).
int countSpaces (char *text) {
    int count = 0;
    for (int i = 0; text[i]; i++) if (text[i] == ' ') count++;
    return count;
}

//Dependendo do filtro pedido no comando, retorna uma string com a diretoria e nome do executável com o correspondente filtro.
char *assignExec(char *nome) {
    if (!strcmp(nome, "alto")) return strdup(alto_f);
    if (!strcmp(nome, "baixo")) return strdup(baixo_f);
    if (!strcmp(nome, "eco")) return strdup(eco_f);
    if (!strcmp(nome, "rapido")) return strdup(rapido_f);
    if (!strcmp(nome, "lento")) return strdup(lento_f);
    return NULL;
}

//Set dos argumentos a serem inseridos na função execvp();
char **setArgs(char *input, char *output, char *remaining) {
    char **ret = (char **) calloc(100, sizeof(char *));
    int current = 0;
    char *aux = assignExec(strsep(&remaining, " "));
    char res[50];
    res[0] = 0;
    strcat(res, dir);
    strcat(res, aux);
    ret[current++] = strdup(res);
    ret[current] = NULL;
    return ret;
}


int executaTransform (char *comando) {
    char *found;
    char *args = strdup(comando);
    found = strsep(&args, " ");
    char *input = strsep(&args, " "); //Guarda o nome e path do ficheiro de input.
    char *output = strsep(&args, " "); //Guarda nome e path do ficheiro de output.
    char *resto = strsep(&args, "\n"); //Guarda os filtros pedidos pelo utilizador.
    inProcess[nProcesses++] = strdup(comando);
    updateSlots(resto);
    char **argumentos = setArgs(input, output, resto); //Guarda os argumentos a serem fornecidos à função execvp().
    if (!fork()) {
        int exInput;
        if ((exInput = open(input, O_RDONLY)) < 0) { //Abre ficheiro de input.
            perror("[transform] Erro ao abrir ficheiro input");
            return -1;
        }
        dup2(exInput, 0); //Coloca o stdin no ficheiro de input.
        close(exInput);
        int outp;
        if ((outp = open(output, O_CREAT | O_TRUNC | O_WRONLY)) < 0) { //Cria ficheiro de output.
            perror("[transform] Erro ao criar ficheiro de output");
            return -1;
        }
        dup2(outp, 1); //Coloca o stdout no ficheiro de output.
        close(outp);
        if (execvp(*argumentos, argumentos) == -1) {
            perror("[transform] Erro em execvp");
            return -1;
        }
        _exit(0);
    }
    free(args);
    return 0;
}

int main (int argc, char *argv[]) {
    //Declaração de variaveis
    int bytesRead = 0;
    alto = baixo = eco = rapido = lento = alto_cur = baixo_cur = eco_cur = rapido_cur = lento_cur = 0; //Guarda numero de capacidade de filtros
       
    if (argc <= 2) {
        write(1, "Não foram inseridos argumentos suficientes (Esperado: 2).\n", strlen("Não foram inseridos argumentos suficientes (Esperado: 2).\n"));
        return -1;
    }
    else if (argc > 3) {
        write(1, "Foram introduzidos demasiados argumentos (Esperado: 2)\n", strlen("Foram introduzidos demasiados argumentos (Esperado: 2)\n"));
        return -1;
    }
    //Cria os named pipes necessários.
    if (mkfifo("tmp/server-client-fifo", 0600) == -1) {
        perror("Erro a abrir o pipe server-client");
        return 1;
    }

    if (mkfifo("tmp/client-server-fifo", 0600) == -1) {
        perror("Erro a abrir o pipe client-server");
        return 1;
    }

    if (mkfifo("tmp/processing-fifo", 0600) == -1) {
        perror("Erro na criação do pipe processing");
        return 1;
    }

    //Declaração dos handlers dos sinais.
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("[signal] erro da associação do signint_handler.");
        exit(-1);
    }
    if (signal(SIGCHLD, sigchld_handler) == SIG_ERR) {
        perror("[signal] erro da associação do sigchld_handler.");
        exit(-1);
    }
    if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
        perror("[signal] erro da associação do sigterm_handler.");
        exit(-1);
    }

    //Inicialização do servidor
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
                free(aux);
            } while ((token = strtok(NULL, "\n")));
        }
        dir = strcat(strdup(argv[2]), "/");
        write(1, "Servidor iniciado com sucesso!\n", strlen("Servidor iniciado com sucesso!\n"));
    }
    //Fim da inicialização do servidor.

    int leitura = 0;
    char comando[BUF_SIZE];
    int server_client_fifo;
    int client_server_fifo;
    int processing_fifo;
    Queue *q = initQueue();

    write(1, "A receber pedidos do cliente...\n", strlen("A receber pedidos do cliente...\n"));
    //Abre as respetivas extremidades de escrita/leitura dos named pipes.
    if ((client_server_fifo = open("tmp/client-server-fifo", O_RDONLY)) == -1) {
        perror("Erro a abrir pipe de cliente");
        return -1;
    }
    if ((server_client_fifo = open("tmp/server-client-fifo", O_WRONLY)) == -1) {
        perror("Erro a abrir pipe de servidor");
        return -1;
    }
    if ((processing_fifo = open("tmp/processing-fifo", O_WRONLY)) == -1) {
        perror("Erro a abrir pipe de processing");
        return -1;
    }

    //Setup da função poll()
    struct pollfd *pfd = calloc(1, sizeof(struct pollfd));
    pfd->fd = client_server_fifo;
    pfd->events = POLLIN;
    pfd->revents = POLLOUT;
    //Vai lendo comandos vindos do cliente
    while (1) {
        if (canQ(q) == 1) { //Verifica sempre se pode executar o que esta na fila.
            //Caso de poder executar a fila, usa mesmo código do transform que esta mais em baixo.
            char *comandoQ = strdup(q->line[q->pos]);
            q->pos++;
            if(check_disponibilidade(strdup(comandoQ))) { //Verifica se temos filtros suficientes para executar o comando
                write(processing_fifo, "Processing...\n", strlen("Processing...\n")); //informa o cliente que o pedido começou a ser processado.
                executaTransform(comandoQ);
            }
        }
        //Execução bloqueada até ser lida alguma coisa no pipe. Diminui utilização de CPU. 
        else 
            poll(pfd, 1, -1); //Verifica se o pipe está disponivel para leitura
        leitura = read(client_server_fifo, comando, BUF_SIZE);
        comando[leitura] = 0;
        //Handling de erro no caso de ocorrer algum problema na leitura.
        if (leitura == -1) {
            perror("[client-server-fifo] Erro na leitura");
            exit(1);
        }
        //Execução do comando "status"
        if (leitura > 0 && !strncmp(comando, "status", leitura)) {
            char mensagem[5000];
            char res[5000];
            res[0] = 0;
            for (int i = 0; i < nProcesses; i++) {
                sprintf(mensagem, "Task #%d: %s\n", i+1, inProcess[i]);
                strcat(res, mensagem);
            }
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
        //Execução do comando "transform"
        else if (leitura > 0 && !strncmp(comando, "transform", 9)) {
            write(processing_fifo, "Pending...\n", strlen("Pending...\n"));
            if(check_disponibilidade(strdup(comando))) { //Verifica se temos filtros suficientes para executar o comando
                write(processing_fifo, "Processing...\n", strlen("Processing...\n")); //informa o cliente que o pedido começou a ser processado.
                executaTransform(comando);
            }
            else {
                q->line[++q->filled] = strdup(comando);
            }
        }
    }
}
