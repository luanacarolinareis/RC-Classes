/*************************************************
 * CLIENTE envia pacotes UDP para o servidor,
 * no porto especificado (em argv[1])
 *************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_USERNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 50
#define MAX_TYPE_LENGTH 20

// Protótipo de função
void erro(char *);
char* strupper(char *);
char* strlower(char *);

int main(int argc, char *argv[]) {
    // class_client {endereço do servidor} {PORTO_CONFIG}
    if (argc < 3)
        erro(strcat(argv[0], " {endereço do servidor} {PORTO_TURMAS}"));

    // Porto do servidor obtidos através da linha de comando
    char *server_ip = argv[1];
    int PORTO_CONFIG = atoi(argv[2]);

    struct sockaddr_in si_outra;
    int sockfd, n, opt = 0, c;
    socklen_t slen = sizeof(si_outra);
    char buffer[MAX_BUFFER_SIZE];
    char client_type[MAX_TYPE_LENGTH];
    char response[MAX_BUFFER_SIZE];
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    char type[MAX_TYPE_LENGTH];

    // Cria um socket para envio de pacotes UDP
    if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        erro("ao criar socket");

    // Preenchimento da socket address structure
    memset((char *)&si_outra, 0, sizeof(si_outra));
    si_outra.sin_family = AF_INET;
    si_outra.sin_port = htons(PORTO_CONFIG);
    if (inet_aton(server_ip, &si_outra.sin_addr) == 0)
        erro("ao converter endereço");

    do {
        // Enviar dados de 'login' para autenticação do cliente
        do {
            printf("\nUsername: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0';
            if (strlen(buffer) > 0) n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &si_outra, slen);
            if (n < 0)
                erro("ao enviar dados");
        } while (strlen(buffer) == 0);
        do {
            printf("Password: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0';
            if (strlen(buffer) > 0) n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &si_outra, slen);
            if (n < 0)
                erro("ao enviar dados");
        } while (strlen(buffer) == 0);

        // Receber resposta de autenticação
        memset(response, 0, sizeof(response));
        n = recvfrom(sockfd, response, sizeof(response), 0, (struct sockaddr *) &si_outra, (socklen_t * ) & slen);
        if (n < 0)
            erro("ao receber dados");

        // Verificar resposta
        if (strcmp(response, "OK") == 0) {
            memset(client_type, 0, sizeof(client_type));
            n = recvfrom(sockfd, client_type, sizeof(client_type), 0, (struct sockaddr *) &si_outra, (socklen_t * ) & slen);
            if (n < 0)
                erro("ao receber dados");
            client_type[n] = '\0';

            printf("\033[1;32mAutenticação bem sucedida!\033[0m\n\n");
            do {
                // Listar opções disponíveis para o tipo de cliente autenticado
                printf("\n\033[1;34m----- OPÇÕES DISPONÍVEIS PARA %s -----\033[0m\n", strupper(client_type));
                if (strcmp(client_type, "ADMINISTRADOR") == 0) {
                    printf("\n[ 1 ] ADD_USER {username} {password} {administrador/aluno/professor}\n");
                    printf("[ 2 ] DEL {username}\n");
                    printf("[ 3 ] LIST\n");
                    printf("[ 4 ] EXIT\n");
                    printf("[ 5 ] QUIT_SERVER\n");
                }
                printf("\nEscolha uma opção: ");
                scanf("%d", &opt);
                while ((c = getchar()) != '\n' && c != EOF);
                switch (opt) {
                    case 1:
                        if(strcmp(client_type, "ADMINISTRADOR") == 0) {
                            do {
                                printf("Username a adicionar: ");
                                fgets(username, sizeof(username), stdin);
                                username[strcspn(username, "\n")] = '\0';
                            } while(strlen(username) == 0);
                            strlower(username);

                            do {
                                printf("Password a adicionar: ");
                                fgets(password, sizeof(password), stdin);
                                password[strcspn(password, "\n")] = '\0';
                            } while(strlen(password) == 0);
                            strlower(password);

                            do {
                                printf("Tipo a adicionar: ");
                                fgets(type, sizeof(type), stdin);
                                type[strcspn(type, "\n")] = '\0';
                                strlower(type);
                            } while(strcmp(strlower(type), "administrador") != 0 && strcmp(strlower(type), "aluno") != 0 && strcmp(strlower(type), "professor") != 0);

                            sprintf(buffer, "ADD_USER %s %s %s", username, password, type);
                            if (strlen(buffer) > 0) n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &si_outra, slen);
                            if (n < 0)
                                erro("ao enviar dados");
                        }
                        break;
                    case 2:
                        if(strcmp(client_type, "ADMINISTRADOR") == 0) {
                            do {
                                printf("Username a remover: ");
                                fgets(username, sizeof(username), stdin);
                                username[strcspn(username, "\n")] = '\0';
                            } while(strlen(username) == 0);
                            strlower(username);

                            sprintf(buffer, "DEL %s", username);
                            if (strlen(buffer) > 0) n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &si_outra, slen);
                            if (n < 0)
                                erro("ao enviar dados");
                        }
                        break;
                    case 3:
                        n = sendto(sockfd, "LIST", strlen("LIST"), 0, (struct sockaddr *) &si_outra, slen);
                        if (n < 0)
                            erro("ao enviar dados");
                        break;
                    case 4:
                        n = sendto(sockfd, "EXIT", strlen("EXIT"), 0, (struct sockaddr *) &si_outra, slen);
                        if (n < 0)
                            erro("ao enviar dados");
                        printf("A terminar sessão...\n\n");
                        close(sockfd);
                        return 0;
                    case 5:
                        n = sendto(sockfd, "QUIT_SERVER", strlen("QUIT_SERVER"), 0, (struct sockaddr *) &si_outra, slen);
                        if (n < 0)
                            erro("ao enviar dados");
                        printf("A terminar sessão e a encerrar servidor...\n\n");
                        close(sockfd);
                        return 0;
                    default:
                        n = sendto(sockfd, "INVALID", strlen("INVALID"), 0, (struct sockaddr *) &si_outra, slen);
                        printf("\033[1;31mOpção inválida!\033[0m\n");
                        break;
                }

                // Receber resposta à operação escolhida
                memset(response, 0, sizeof(response));
                n = recvfrom(sockfd, response, sizeof(response), 0, (struct sockaddr *) &si_outra, (socklen_t * ) & slen);
                if (n < 0)
                    erro("ao receber dados");
                printf("\n%s\n", response);

            } while((opt < 1 || opt > 5) || (opt != 4 && opt != 5));

        } else if (strcmp(response, "REJECTED") == 0) {
            printf("\033[1;31mAutenticação falhou. Username ou password incorretos!\033[0m\n\n");
        } else if (strcmp(response, "CLIENT") == 0) {
            printf("\033[1;31mAutenticação falhou. Impossível autenticar clientes aluno/professor por UDP!\033[0m\n\n");
        }
    } while(strcmp(response, "REJECTED") == 0 || strcmp(response, "CLIENT") == 0);

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

// Função para converter 'string' para minúsculas
char* strlower(char* str) {
    for (int i = 0; str[i] != '\0'; i++)
        str[i] = tolower(str[i]);
    return str;
}
