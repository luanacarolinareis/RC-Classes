/*************************************************
 * CLIENTE liga ao servidor (definido em argv[1])
 * no porto especificado (em argv[2])
 *************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>

#define MAX_TYPE_LENGTH 20
#define MAX_BUFFER_SIZE 1024
#define MAX_CLASS_NAME 50
#define MAX_TEXT_SIZE 500

// Protótipos de funções
void erro(char *);
char* strupper(char *);
char* strlower(char *);

int main(int argc, char *argv[]) {
    // class_client {endereço do servidor} {PORTO_TURMAS}
    if (argc < 3)
        erro(strcat(argv[0], " {endereço do servidor} {PORTO_TURMAS}"));

    int sockfd, opt = 0, max, c;
    ssize_t n;
    struct sockaddr_in serv_addr;
    char buffer[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];
    char client_type[MAX_TYPE_LENGTH];
    char class_name[MAX_CLASS_NAME];
    char text[MAX_TEXT_SIZE];

    // Endereço e porto do servidor obtidos através da linha de comando
    char *server_ip = argv[1];
    char *endptr;
    long PORTO_TURMAS = strtol(argv[2], &endptr, 10);

    if ((gethostbyname(server_ip)) == 0)
        erro("não foi possível obter endereço");

    // Criar socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        erro("ao abrir socket");

    // Configurar o endereço do servidor
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    serv_addr.sin_port = htons(PORTO_TURMAS);

    // Conectar ao servidor
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        erro("ao conectar");

    do {
        // Enviar dados de 'login' para autenticação do cliente
        do {
            printf("\nUsername: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0';
            if (strlen(buffer) > 0) n = send(sockfd, buffer, strlen(buffer), 0);
            if (n < 0)
                erro("ao enviar dados");
        } while(strlen(buffer) == 0);
        do {
            printf("Password: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0';
            if (strlen(buffer) > 0) n = send(sockfd, buffer, strlen(buffer), 0);
            if (n < 0)
                erro("ao enviar dados");
        } while(strlen(buffer) == 0);

        // Receber resposta de autenticação
        memset(response, 0, sizeof(response));
        n = recv(sockfd, response, sizeof(response), 0);
        if (n < 0)
            erro("ao receber dados");

        // Verificar resposta
        if (strcmp(response, "OK") == 0) {
            memset(client_type, 0, sizeof(client_type));
            n = recv(sockfd, client_type, sizeof(client_type), 0);
            if (n < 0)
                erro("ao receber dados");
            client_type[n] = '\0';

            printf("\033[1;32mAutenticação bem sucedida!\033[0m\n\n");
            do {
                // Listar opções disponíveis para o tipo de cliente autenticado
                printf("\n\033[1;34m----- OPÇÕES DISPONÍVEIS PARA %s -----\033[0m\n", strupper(client_type));
                if (strcmp(client_type, "ALUNO") == 0) {
                    printf("\n[ 1 ] LIST_CLASSES\n");
                    printf("[ 2 ] LIST_SUBSCRIBED\n");
                    printf("[ 3 ] SUBSCRIBE_CLASS {name}\n");
                    printf("[ 4 ] EXIT\n");
                } else if (strcmp(client_type, "PROFESSOR") == 0) {
                    printf("\n[ 1 ] LIST_CLASSES\n");
                    printf("[ 2 ] CREATE_CLASS {name} {size}\n");
                    printf("[ 3 ] SEND {name} {text that server will send to subscribers}\n");
                    printf("[ 4 ] EXIT\n");
                }
                do {
                    printf("\nEscolha uma opção: ");
                    scanf("%d", &opt);
                } while (opt < 1 || opt > 4);

                while ((c = getchar()) != '\n' && c != EOF);
                switch (opt) {
                    case 1:
                        n = send(sockfd, "LIST_CLASSES", strlen("LIST_CLASSES"), 0);
                        if (n < 0)
                            erro("ao enviar dados");
                        break;
                    case 2:
                        if (strcmp(client_type, "ALUNO") == 0) {
                            n = send(sockfd, "LIST_SUBSCRIBED", strlen("LIST_SUBSCRIBED"), 0);
                            if (n < 0)
                                erro("ao enviar dados");
                        } else if (strcmp(client_type, "PROFESSOR") == 0) {
                            do {
                                printf("Nome da turma: ");
                                fgets(class_name, sizeof(class_name), stdin);
                                class_name[strcspn(class_name, "\n")] = '\0';
                            } while(strlen(class_name) == 0);

                            do {
                                printf("Capacidade máxima: ");
                                scanf("%d", &max);
                            } while (max < 1);

                            strlower(class_name);
                            sprintf(buffer, "CREATE_CLASS %s %d", class_name, max);
                            if (strlen(buffer) > 0) n = send(sockfd, buffer, strlen(buffer), 0);
                            if (n < 0)
                                erro("ao enviar dados");
                        }
                        break;
                    case 3:
                        if (strcmp(client_type, "ALUNO") == 0) {
                            do {
                                printf("Nome da turma: ");
                                fgets(class_name, sizeof(class_name), stdin);
                                class_name[strcspn(class_name, "\n")] = '\0';
                            } while(strlen(class_name) == 0);

                            strlower(class_name);
                            sprintf(buffer, "SUBSCRIBE_CLASS %s", class_name);
                            if (strlen(buffer) > 0) n = send(sockfd, buffer, strlen(buffer), 0);
                            if (n < 0)
                                erro("ao enviar dados");
                        } else if (strcmp(client_type, "PROFESSOR") == 0) {
                            do {
                                printf("Nome da turma: ");
                                fgets(class_name, sizeof(class_name), stdin);
                                class_name[strcspn(class_name, "\n")] = '\0';
                            } while(strlen(class_name) == 0);

                            do {
                                printf("Texto a enviar: ");
                                fgets(text, sizeof(text), stdin);
                                text[strcspn(text, "\n")] = '\0';
                            } while(strlen(text) == 0);

                            strlower(class_name);
                            sprintf(buffer, "SEND %s %s", class_name, text);
                            if (strlen(buffer) > 0) n = send(sockfd, buffer, strlen(buffer), 0);
                            if (n < 0)
                                erro("ao enviar dados");
                        }
                        break;
                    case 4:
                        n = send(sockfd, "EXIT", strlen("EXIT"), 0);
                        if (n < 0)
                            erro("ao enviar dados");
                        printf("A terminar sessão...\n\n");
                        close(sockfd);
                        return 0;
                    default:
                        break;
                }

                // Receber resposta à operação escolhida
                memset(response, 0, sizeof(response));
                n = recv(sockfd, response, sizeof(response), 0);
                if (n < 0)
                    erro("ao receber dados");
                printf("\n%s\n", response);

            } while ((opt < 1 || opt > 4) || opt != 4);

        } else if (strcmp(response, "REJECTED") == 0) {
            printf("\033[1;31mAutenticação falhou. Username ou password incorretos!\033[0m\n");
        } else if (strcmp(response, "ADMIN") == 0) {
            printf("\033[1;31mAutenticação falhou. Impossível autenticar administradores por TCP!\033[0m\n");
        }
    } while (strcmp(response, "REJECTED") == 0 || strcmp(response, "ADMIN") == 0);

    // Fechar a conexão
    close(sockfd);
    return 0;
}

// Função de erro genérica
void erro(char *msg){
    printf("Erro: %s\n", msg);
    exit(1);
}

// Função para converter 'string' para maiúsculas
char* strupper(char* str) {
    for (int i = 0; str[i] != '\0'; i++)
        str[i] = toupper(str[i]);
    return str;
}

// Função para converter 'string' para maiúsculas
char* strlower(char* str) {
    for (int i = 0; str[i] != '\0'; i++)
        str[i] = tolower(str[i]);
    return str;
}
