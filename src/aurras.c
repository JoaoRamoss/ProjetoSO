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
    if (mkfifo("tmp/client-server-fifo", 0600) == -1) {
        perror("Erro ao criar pipe client-server");
        return 1;
    }
    
}