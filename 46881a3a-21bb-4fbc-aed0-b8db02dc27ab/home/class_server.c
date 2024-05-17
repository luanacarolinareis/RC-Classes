/**************************************************************
 * SERVIDOR à escuta de novos clientes, nos portos 5000 e 6000
 *
 * 5000: PORTO_CONFIG
 * 6000: PORTO_TURMAS
 *
 **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define MAX_USERNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 50
#define MAX_TYPE_LENGTH 20
#define MAX_BUF_SIZE 512

// Estrutura para armazenar informações de um utilizador
typedef struct {
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    char type[MAX_TYPE_LENGTH];
} User;

// Estrutura para armazenar informações de uma turma
typedef struct {
    char name[MAX_BUF_SIZE];
    int max_capacity;
    char multicast[MAX_BUF_SIZE];
    int num_students;
    char (*students)[MAX_USERNAME_LENGTH];
} Class;

// Variáveis globais para armazenar o username e o tipo do utilizador autenticado
char current_username[MAX_USERNAME_LENGTH];
char current_type[MAX_TYPE_LENGTH];
int aux = 0;

// Variáveis globais para armazenar os utilizadores e turmas
User *users;
Class *classes;
int num_users = 0;
int num_classes = 0;

// Variáveis globais para os descritores de arquivo dos 'sockets'
int sockfd_tcp, newsockfd_tcp, sockfd_udp;

// Protótipos de funções gerais
void auth_tcp(int);
void auth_udp(int, struct sockaddr *, socklen_t);
void process_client(int);
void process_admins(int, struct sockaddr *, socklen_t);
void erro(char *);
void cleanup();
void sigint_handler();

// Funções específicas para administradores
void add_user(char *, char *, char *);
void del_user(char *);
void list_users();
void quit_server();

// Funções específicas para alunos e professores
void list_classes(int);

// Funções específicas para alunos
void list_subscribed(int, char *);
void subscribe_class(int, char *, char *);

// Funções específicas para professores
void create_class(char *, int);
void send_content(char *, char *);

int main(int argc, char *argv[]) {
    // class_server {PORTO_TURMAS} {PORTO_CONFIG} {ficheiro de configuração}
    if (argc != 4)
        erro(strcat(argv[0], " {PORTO_TURMAS} {PORTO_CONFIG} {ficheiro configuração}"));

    // Porto e fichero de configuração obtidos através da linha de comando
    char *endptr;
    long PORTO_TURMAS = strtol(argv[1], &endptr, 10);
    long PORTO_CONFIG = strtol(argv[2], &endptr, 10);
    char *config_file = argv[3];

    // Variáveis para lidar com clientes TCP e UDP
    int maxfd;
    struct sockaddr_in serv_addr_tcp, serv_addr_udp;

    fd_set readfds;
    int activity;
    socklen_t clilen_tcp;
    pid_t pid;

    // Abrir o ficheiro de configuração em modo de leitura
    FILE *file = fopen(config_file, "r");
    if (file == NULL)
        erro("ao abrir o ficheiro de configuração");

    // Alocação inicial de memória para os utilizadores e para turmas
    int capacity = 1;
    users = malloc(capacity * sizeof(User));
    if (users == NULL) {
        printf("Erro: ao alocar memória\n");
        exit(1);
    }

    // Ler os utilizadores registados, a partir do ficheiro de texto
    while (fscanf(file, "%49[^;];%49[^;];%19s\n", users[num_users].username, users[num_users].password, users[num_users].type) != EOF) {
        num_users++;
        // Verificar se é necessário realocar memória para mais utilizadores
        if (num_users == capacity) {
            capacity += 1;
            users = realloc(users, capacity * sizeof(User));
            if (users == NULL) {
                printf("Erro: ao realocar memória\n");
                exit(1);
            }
        }
    }
    // Após terminar a leitura do ficheiro, libertar recursos
    fclose(file);

    // Turmas iniciais para teste
    create_class("rc", 17);
    create_class("so", 20);
    create_class("bd", 15);
    create_class("aed", 10);
    create_class("atd", 25);

    // Configurar o socket para 'PORTO_TURMAS' (TCP)
    if ((sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("ao abrir socket para 'PORTO_TURMAS'");

    // Configurar o socket para 'PORTO_CONFIG' (UDP)
    if ((sockfd_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        erro("ao abrir socket para 'PORTO_CONFIG'");

    // Preenchimento da socket address structure para TCP
    memset(&serv_addr_tcp, 0, sizeof(serv_addr_tcp));
    serv_addr_tcp.sin_family = AF_INET;
    serv_addr_tcp.sin_addr.s_addr = INADDR_ANY;
    serv_addr_tcp.sin_port = htons(PORTO_TURMAS);

    // Preenchimento da socket address structure para UDP
    memset(&serv_addr_udp, 0, sizeof(serv_addr_udp));
    serv_addr_udp.sin_family = AF_INET;
    serv_addr_udp.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr_udp.sin_port = htons(PORTO_CONFIG);

    // Associa o 'socket' à informação de endereço para TCP
    if (bind(sockfd_tcp, (struct sockaddr *) &serv_addr_tcp, sizeof(serv_addr_tcp)) < 0)
        erro("ao fazer bind para 'PORTO_TURMAS'");

    // Associa o 'socket' à informação de endereço para UDP
    if (bind(sockfd_udp, (struct sockaddr *)&serv_addr_udp, sizeof(serv_addr_udp)) < 0)
        erro("ao fazer bind para 'PORTO_CONFIG'");

    // Iniciar escuta para clientes TCP em 'PORTO_TURMAS'
    if (listen(sockfd_tcp, 5) < 0)
        erro("ao ouvir em 'PORTO_TURMAS'");

    // O servidor aceita conexões nos portos 'PORTO_TURMAS' e 'PORTO_CONFIG'
    printf("A aguardar conexões em 'PORTO_TURMAS' e 'PORTO_CONFIG'...\n");

    while (1) {
        // CTRL + C para encerrar o servidor
        signal(SIGINT, sigint_handler);

        FD_ZERO(&readfds);
        FD_SET(sockfd_tcp, &readfds);
        FD_SET(sockfd_udp, &readfds);
        maxfd = (sockfd_tcp > sockfd_udp) ? sockfd_tcp : sockfd_udp;

        // Esperar por atividade em qualquer um dos 'sockets'
        activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0)
            erro("ao selecionar socket");

        // Atividade detectada em socket TCP
        if (FD_ISSET(sockfd_tcp, &readfds)) {
            clilen_tcp = sizeof(struct sockaddr_in);
            newsockfd_tcp = accept(sockfd_tcp, (struct sockaddr *) &serv_addr_tcp, &clilen_tcp);
            if (newsockfd_tcp < 0)
                erro("ao aceitar conexão TCP");
            printf("\n\033[1;32mConexão aceite em 'PORTO_TURMAS'!\033[0m\n");
            pid = fork();
            if (pid < 0)
                erro("no fork");
            else if (pid == 0) {
                close(sockfd_tcp);
                aux = 0;
                auth_tcp(newsockfd_tcp);
                if (strcmp(current_type, "aluno") == 0 || strcmp(current_type, "professor") == 0) {
                    process_client(newsockfd_tcp);
                }
                exit(0);
            } else {
                close(newsockfd_tcp);
            }
        }

        // Atividade detectada em socket UDP
        if (FD_ISSET(sockfd_udp, &readfds)) {
            printf("\nMensagem UDP recebida em 'PORTO_CONFIG'!\n");
            struct sockaddr_in cli_addr;
            socklen_t clilen_udp = sizeof(cli_addr);
            aux = 0;
            auth_udp(sockfd_udp, (struct sockaddr *) &cli_addr, clilen_udp);
            if (strcmp(current_type, "administrador") == 0) {
                process_admins(sockfd_udp, (struct sockaddr *) &cli_addr, clilen_udp);
            }
        }
    }
}

// Função para autenticar um cliente TCP
void auth_tcp(int client_socket) {
    char buffer[MAX_BUF_SIZE];
    int n;

    // Enquanto o cliente não estiver autenticado...
    while(aux != 1) {
        memset(buffer, 0, MAX_BUF_SIZE);

        // Receber o username do cliente atual
        n = read(client_socket, buffer, MAX_BUF_SIZE);
        if (n <= 0) {
            printf("Erro: ao receber o username do cliente\n");
            close(client_socket);
            return;
        }
        buffer[n] = '\0';
        strncpy(current_username, buffer, MAX_USERNAME_LENGTH);

        // Receber a password do cliente atual
        n = read(client_socket, buffer, MAX_BUF_SIZE);
        if (n <= 0) {
            printf("Erro: ao receber a password do cliente\n");
            close(client_socket);
            return;
        }
        buffer[n] = '\0';

        // Verificar se as credenciais do cliente são válidas
        printf("(TCP CLIENT) LOGIN %s %s\n", current_username, buffer);

        for (int i = 0; i < num_users; i++) {
            if (strcmp(users[i].username, current_username) == 0 && strcmp(users[i].password, buffer) == 0) {
                strcpy(current_type, users[i].type);
                if (strcmp(users[i].type, "administrador") != 0) {
                    aux = 1;
                    write(client_socket, "OK", strlen("OK"));
                }
                else
                    write(client_socket, "ADMIN", strlen("ADMIN"));
                break;
            }
            if ((i == (num_users - 1)) && aux == 0)
                write(client_socket, "REJECTED", strlen("REJECTED"));
        }

    }
}

// Função para autenticar um cliente UDP
void auth_udp(int client_socket, struct sockaddr *client_addr, socklen_t client_len) {
    char buffer[MAX_BUF_SIZE];
    int n;

    // Receber o username do cliente atual
    n = recvfrom(client_socket, buffer, MAX_BUF_SIZE, 0, client_addr, &client_len);
    if (n < 0) {
        printf("Erro: falha ao receber o username do cliente\n");
        close(client_socket);
        return;
    }
    buffer[n] = '\0';
    strncpy(current_username, buffer, MAX_USERNAME_LENGTH);

    // Receber a password do cliente atual
    n = recvfrom(client_socket, buffer, MAX_BUF_SIZE, 0, client_addr, &client_len);
    if (n < 0) {
        printf("Erro: falha ao receber a password do cliente\n");
        close(client_socket);
        return;
    }
    buffer[n] = '\0';

    // Verificar se as credenciais do cliente são válidas
    printf("(UDP CLIENT) LOGIN %s %s\n", current_username, buffer);
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, current_username) == 0 && strcmp(users[i].password, buffer) == 0) {
            aux = 1;
            strcpy(current_type, users[i].type);
            sendto(client_socket, "OK", strlen("OK"), 0, client_addr, client_len);
            break;
        }
        if ((i == (num_users - 1)) && aux == 0)
            sendto(client_socket, "REJECTED", strlen("REJECTED"), 0, client_addr, client_len);
    }
}

// Função para processar um cliente TCP aluno ou professor
void process_client(int client_socket) {
    char buffer[MAX_BUF_SIZE];
    int n;
    write(client_socket, current_type, strlen(current_type));

    do {
        memset(buffer, 0, MAX_BUF_SIZE);
        n = read(client_socket, buffer, MAX_BUF_SIZE);
        if (n <= 0) {
            printf("Erro: ao receber dados do cliente\n");
            close(client_socket);
            return;
        }
        buffer[n] = '\0';
        if (strlen(buffer) > 0) {
            printf("(TCP CLIENT) %s\n", buffer);

            // Aluno ou professor pede para sair
            if (strcmp(buffer, "EXIT") == 0) {
                break;
            }

            // Aluno ou professor pede para listar turmas disponíveis
            else if (strcmp(buffer, "LIST_CLASSES") == 0) {
                list_classes(client_socket);
            }

            // Aluno pede para listar turmas subscritas
            else if (strcmp(buffer, "LIST_SUBSCRIBED") == 0) {
                list_subscribed(client_socket, current_username);
            }

            // Aluno pede para subscrever uma turma
            else if (strstr(buffer, "SUBSCRIBE_CLASS") != NULL) {
                char class_name[MAX_BUF_SIZE];
                char *space = strchr(buffer, ' ');
                if (space != NULL) {
                    strcpy(class_name, space + 1);
                    class_name[strlen(class_name)] = '\0';
                }
                subscribe_class(client_socket, current_username, class_name);
            }

            // Professor pede para criar uma turma
            else if (strstr(buffer, "CREATE_CLASS") != NULL) {
                char class_name[MAX_BUF_SIZE];
                int max_capacity;
                char* token;
                token = strtok(NULL, " ");

                // O próximo token é o nome da turma
                if (token != NULL) strcpy(class_name, token);
                token = strtok(NULL, " ");

                // O próximo token é a capacidade máxima da turma
                if (token != NULL) max_capacity = atoi(token);

                create_class(class_name, max_capacity);
            }

            // Professor pede para enviar conteúdo para uma turma
            else if (strstr(buffer, "SEND") != NULL) {
                char class_name[MAX_BUF_SIZE];
                char content[MAX_BUF_SIZE];
                char* token;
                token = strtok(NULL, " ");

                // O próximo token é o nome da turma
                if (token != NULL) strcpy(class_name, token);
                token = strtok(NULL, " ");

                // O próximo token é o conteúdo a enviar
                if (token != NULL) strcpy(content, token);

                send_content(content, class_name);
            }
        }
    } while (1);
}

// Função para processar um cliente UDP administrador
void process_admins(int sockfd, struct sockaddr *client_addr, socklen_t client_len) {
    char buffer[MAX_BUF_SIZE];
    int n;
    sendto(sockfd, current_type, strlen(current_type), 0, client_addr, client_len);

    do {
        memset(buffer, 0, MAX_BUF_SIZE);
        n = recvfrom(sockfd, buffer, MAX_BUF_SIZE, 0, client_addr, &client_len);
        if (n < 0) {
            printf("Erro: ao receber dados do cliente\n");
            return;
        }
        buffer[n] = '\0';
        if (strlen(buffer) > 0) {
            printf("(UDP CLIENT) %s\n", buffer);

            // Administrador pede para sair
            if (strcmp(buffer, "EXIT") == 0) {
                break;
            }

            // Administrador pede para adicionar um utilizador
            else if (strstr(buffer, "ADD_USER") != NULL) {
                char username[MAX_BUF_SIZE];
                char password[MAX_BUF_SIZE];
                char type[MAX_BUF_SIZE];
                char *token;
                token = strtok(NULL, " ");

                // O próximo token é o username
                if (token != NULL) strcpy(username, token);
                token = strtok(NULL, " ");

                // O próximo token é a password
                if (token != NULL) strcpy(password, token);
                token = strtok(NULL, " ");

                // O próximo token é o tipo de utilizador
                if (token != NULL) strcpy(type, token);

                add_user(username, password, type);
            }

            // Administrador pede para remover um utilizador
            else if (strstr(buffer, "DEL") != NULL) {
                char username[MAX_BUF_SIZE];
                char *space = strchr(buffer, ' ');
                if (space != NULL) {
                    strcpy(username, space + 1);
                }
                del_user(username);
            }

            // Administrador pede para listar utilizadores
            else if (strcmp(buffer, "LIST") == 0) {
                list_users();
            }

            // Administrador pede para encerrar o servidor
            else if (strcmp(buffer, "QUIT_SERVER") == 0) {
                quit_server();
            }
        }
    } while (1);
}

// Função de erro genérica
void erro(char *msg){
    printf("Erro: %s\n", msg);
    exit(1);
}

// Função de limpeza de recursos ao encerrar o servidor
void cleanup() {
    close(sockfd_tcp);
    close(newsockfd_tcp);
    close(sockfd_udp);
}

// Função de manipulação de sinal para SIGINT (CTRL + C)
void sigint_handler() {
    cleanup();
    printf("A encerrar o servidor...\n");
    exit(0);
}

// Administrador: adicionar utilizador
void add_user(char *username, char *password, char *type) {
    // Verificar se o utilizador já existe
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            printf("Utilizador já existe!\n");
            return;
        }
    }

    // Adicionar o utilizador à lista de utilizadores
    User user;
    strcpy(user.username, username);
    strcpy(user.password, password);
    strcpy(user.type, type);
    if (users != NULL)
        users = realloc(users, (num_users + 1) * sizeof(User));
    if (users == NULL) {
        printf("Erro: ao realocar memória\n");
        exit(1);
    }
    users[num_users] = user;
    num_users++;
    printf("Utilizador '%s' adicionado com sucesso!\n", username);
}

// Administrador: remover utilizador
void del_user(char *username) {
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            for (int j = i; j < num_users - 1; j++) {
                strcpy(users[j].username, users[j + 1].username);
                strcpy(users[j].password, users[j + 1].password);
                strcpy(users[j].type, users[j + 1].type);
            }
            num_users--;
            users = realloc(users, num_users * sizeof(User));
            if (users == NULL && num_users > 0) {
                printf("Erro: ao realocar memória\n");
                exit(1);
            }
            printf("Utilizador '%s' removido com sucesso!\n", username);
            return;
        }
    }
    printf("Utilizador '%s' não encontrado!\n", username);
}

// Administrador: listar utilizadores
void list_users() {
    if (num_users == 0) {
        printf("Nenhum utilizador registado!\n");
        return;
    }
    printf("----- Utilizadores registados -----\n\n");
    for (int i = 0; i < num_users; i++)
        printf("Username: %-10s |       Tipo: %s\n", users[i].username, users[i].type);
}

// Administrador: encerrar o servidor
void quit_server() {
    sigint_handler();
}

// Aluno e professor: listar turmas disponíveis
void list_classes(int client_socket) {
    char buffer[MAX_BUF_SIZE * 2];
    int offset = 0;

    if (num_classes == 0) {
        snprintf(buffer, sizeof(buffer),"Nenhuma turma disponível!\n");
        write(client_socket, buffer, strlen(buffer));
        return;
    }
    // Iniciar com o cabeçalho das turmas disponíveis
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "----- Turmas disponíveis -----\n\n");

    for (int i = 0; i < num_classes; i++) {
        if (classes != NULL)
            // Acrescentar informações de cada turma
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Nome: %-10s |     Capacidade máxima: %d\n", classes[i].name, classes[i].max_capacity);
    }

    // Enviar a string buffer completa para o cliente
    write(client_socket, buffer, strlen(buffer));
}

// Aluno: listar turmas subscritas
void list_subscribed(int client_socket, char *username) {
    char buffer[MAX_BUF_SIZE * 2];
    int offset = 0;
    int aux = 0;

    if (num_classes == 0) {
        snprintf(buffer, sizeof(buffer),"Nenhuma turma disponível!\n");
        write(client_socket, buffer, strlen(buffer));
        return;
    }
    // Iniciar com o cabeçalho das turmas disponíveis
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "----- Turmas subscritas por %s -----\n\n", username);

    for (int i = 0; i < num_classes; i++) {
        for (int j = 0; j < classes[i].num_students; j++) {
            if (classes[i].students[j] != NULL && strcmp(classes[i].students[j], username) == 0) {
                // Acrescentar informações de cada turma subscrita
                aux = 1;
                offset += snprintf(buffer + offset, sizeof(buffer) - offset, "CLASS %s/%s\n", classes[i].name, classes[i].multicast);
            }
        }
    }
    if (aux == 0) snprintf(buffer + offset, sizeof(buffer) - offset,"Nenhuma turma subscrita!\n");
    write(client_socket, buffer, strlen(buffer));
}

// Aluno: subscrever uma turma
void subscribe_class(int client_socket, char *username, char *class_name) {
    char buffer[MAX_BUF_SIZE + 20];
    for (int i = 0; i < num_classes; i++) {
        printf("comparando %s com %s", classes[i].name, class_name);
        if (strcmp(classes[i].name, class_name) == 0) {
            // Verificar se o utilizador já está inscrito nesta turma
            for (int j = 0; j < classes[i].num_students; j++) {
                if (classes[i].students[j] != NULL && strcmp(classes[i].students[j], username) == 0) {
                    snprintf(buffer, sizeof(buffer),"REJECTED (ALREADY SUBSCRIBED)\n");
                    write(client_socket, buffer, strlen(buffer));
                    return;
                }
            }
            // Inscrever o utilizador na turma, se houver capacidade
            if (classes[i].num_students < classes[i].max_capacity) {
                strcpy(classes[i].students[classes[i].num_students], username);
                classes[i].num_students++;
                snprintf(buffer, sizeof(buffer),"ACCEPTED <%s>", classes[i].multicast);
                write(client_socket, buffer, strlen(buffer));
            } else {
                snprintf(buffer, sizeof(buffer),"REJECTED (IS FULL)\n");
                write(client_socket, buffer, strlen(buffer));
            }
            return;
        }
    }
    snprintf(buffer, sizeof(buffer),"REJECTED (NOT FOUND)\n");
    write(client_socket, buffer, strlen(buffer));
    return;
}

// Professor: criar uma turma
void create_class(char *name, int max_capacity) {
    // Verificar se a turma já existe
    for (int i = 0; i < num_classes; i++) {
        if (strcmp(classes[i].name, name) == 0) {
            printf("A turma %s já existe!\n", classes[i].name);
            return;
        }
    }

    // Adicionar a turma à lista de turmas
    Class class;
    strcpy(class.name, name);
    class.max_capacity = max_capacity;
    char multicast[20];
    snprintf(multicast, sizeof(multicast),"224.0.0.%d", num_classes + 1);
    strcpy(class.multicast, multicast);
    class.num_students = 0;

    // Se ainda não existirem turmas, alocar memória para a primeira turma
    if (num_classes == 0) {
        classes = malloc(sizeof(Class));
        if (classes == NULL) {
            printf("Erro: ao alocar memória\n");
            exit(1);
        }
    } else {
        classes = realloc(classes, (num_classes + 1) * sizeof(Class));
        if (classes == NULL) {
            printf("Erro: ao realocar memória\n");
            exit(1);
        }
    }
    classes[num_classes] = class;
    classes[num_classes].students = malloc(max_capacity * sizeof(char[MAX_USERNAME_LENGTH]));
    num_classes++;
    printf("Turma '%s' criada com sucesso!\n", name);
}

// Professor: enviar conteúdo para uma turma
void send_content(char *content, char *class_name) {
    if (num_classes == 0) {
        printf("Nenhuma turma disponível!\n");
        return;
    }
    for (int i = 0; i < num_classes; i++) {
        if (strcmp(classes[i].name, class_name) == 0) {
            printf("A enviar conteúdo para a turma '%s'!\n", class_name);
            printf("%s\n", content);

            // Completar lógica para enviar o conteúdo para os membros da turma, via multicast
            // ...
            return;
        }
    }
    printf("Turma '%s' não encontrada!\n", class_name);
}