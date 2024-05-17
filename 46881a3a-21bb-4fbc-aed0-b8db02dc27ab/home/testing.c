#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>

#define MAX_USERNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 50
#define MAX_TYPE_LENGTH 20
#define MAX_BUF_SIZE 1024

// Estrutura para armazenar informações de um utilizador
typedef struct {
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    char type[MAX_TYPE_LENGTH];
} User;

// Variáveis globais para armazenar os utilizadores
User *users;
int num_users = 0;

// Funções de manipulação de clientes TCP e UDP
void *tcp_handler(void *arg);
void *udp_handler(void *arg);

// Função de tratamento de erros
void erro(char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s {PORTO_TURMAS} {PORTO_CONFIG} {ficheiro configuração}\n", argv[0]);
        exit(1);
    }

    // Obter portos e arquivo de configuração da linha de comando
    int PORTO_TURMAS = atoi(argv[1]);
    int PORTO_CONFIG = atoi(argv[2]);
    char *config_file = argv[3];

    // Abrir arquivo de configuração
    FILE *file = fopen(config_file, "r");
    if (file == NULL) {
        erro("Erro ao abrir o arquivo de configuração");
    }

    // Alocação dinâmica para armazenar os usuários
    int capacity = 1;
    users = malloc(capacity * sizeof(User));
    if (users == NULL) {
        erro("Erro ao alocar memória para usuários");
    }

    // Ler usuários do arquivo de configuração
    while (fscanf(file, "%49[^;];%49[^;];%19s\n", users[num_users].username, users[num_users].password, users[num_users].type) != EOF) {
        num_users++;
        if (num_users == capacity) {
            capacity += 1;
            users = realloc(users, capacity * sizeof(User));
            if (users == NULL) {
                erro("Erro ao realocar memória para usuários");
            }
        }
    }
    fclose(file);

    // Configurar socket TCP (PORTO_TURMAS)
    int sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_tcp < 0) {
        erro("Erro ao abrir socket TCP");
    }

    // Configurar socket UDP (PORTO_CONFIG)
    int sockfd_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd_udp < 0) {
        erro("Erro ao abrir socket UDP");
    }

    // Estruturas de endereço para sockets TCP e UDP
    struct sockaddr_in serv_addr_tcp, serv_addr_udp;
    memset(&serv_addr_tcp, 0, sizeof(serv_addr_tcp));
    memset(&serv_addr_udp, 0, sizeof(serv_addr_udp));
    serv_addr_tcp.sin_family = AF_INET;
    serv_addr_tcp.sin_addr.s_addr = INADDR_ANY;
    serv_addr_tcp.sin_port = htons(PORTO_TURMAS);
    serv_addr_udp.sin_family = AF_INET;
    serv_addr_udp.sin_addr.s_addr = INADDR_ANY;
    serv_addr_udp.sin_port = htons(PORTO_CONFIG);

    // Associar sockets aos endereços
    if (bind(sockfd_tcp, (struct sockaddr *) &serv_addr_tcp, sizeof(serv_addr_tcp)) < 0) {
        erro("Erro ao fazer bind para PORTO_TURMAS");
    }
    if (bind(sockfd_udp, (struct sockaddr *)&serv_addr_udp, sizeof(serv_addr_udp)) < 0) {
        erro("Erro ao fazer bind para PORTO_CONFIG");
    }

    // Ouvir por conexões TCP
    if (listen(sockfd_tcp, 5) < 0) {
        erro("Erro ao ouvir em PORTO_TURMAS");
    }

    printf("Aguardando conexões em PORTO_TURMAS e PORTO_CONFIG...\n");

    // Loop principal
    while (1) {
        // Tratamento de conexões TCP
        struct sockaddr_in cli_addr_tcp;
        socklen_t clilen_tcp = sizeof(cli_addr_tcp);
        int newsockfd_tcp = accept(sockfd_tcp, (struct sockaddr *) &cli_addr_tcp, &clilen_tcp);
        if (newsockfd_tcp < 0) {
            erro("Erro ao aceitar conexão TCP");
        }

        // Criar thread para manipular cliente TCP
        pthread_t tcp_tid;
        if (pthread_create(&tcp_tid, NULL, tcp_handler, &newsockfd_tcp) != 0) {
            erro("Erro ao criar thread TCP");
        }
        pthread_detach(tcp_tid);

        // Tratamento de conexões UDP
        struct sockaddr_in cli_addr_udp;
        socklen_t clilen_udp = sizeof(cli_addr_udp);
        int sockfd_udp_copy = sockfd_udp; // Copiar socket UDP para thread
        pthread_t udp_tid;
        if (pthread_create(&udp_tid, NULL, udp_handler, &sockfd_udp_copy) != 0) {
            erro("Erro ao criar thread UDP");
        }
        pthread_detach(udp_tid);
    }

    // Fechar sockets e liberar memória (não alcançado no loop infinito)
    close(sockfd_tcp);
    close(sockfd_udp);
    free(users);
    return 0;
}

// Thread para manipular conexões TCP
void *tcp_handler(void *arg) {
    int client_socket = *((int *)arg);

    // Lógica para manipular cliente TCP
    // Implemente a autenticação e o processamento do cliente aqui
    close(client_socket);
    pthread_exit(NULL);
}

// Thread para manipular conexões UDP
void *udp_handler(void *arg) {
    int sockfd_udp = *((int *)arg);

    // Lógica para manipular cliente UDP
    // Implemente a autenticação e o processamento do cliente aqui
    pthread_exit(NULL);
}

