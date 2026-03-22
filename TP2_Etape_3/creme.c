#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "creme.h"

#define PORT 9998
#define LOCALHOST "127.0.0.1"
#define BCAST_IP "192.168.88.255"

struct User {
    struct in_addr ip;
    char pseudo[50];
} table[255];
int nb_users = 0;

void add_or_update_user(struct in_addr ip, const char *pseudo) {
    for (int i = 0; i < nb_users; i++) {
        if (table[i].ip.s_addr == ip.s_addr) {
            strcpy(table[i].pseudo, pseudo); 
            return;
        }
    }
    if (nb_users < 255) {
        table[nb_users].ip = ip;
        strncpy(table[nb_users].pseudo, pseudo, 49);
        table[nb_users].pseudo[49] = '\0';
        nb_users++;
        printf("Nouvel utilisateur ajouté: %s (%s)\n", pseudo, inet_ntoa(ip));
    }
}

void remove_user(struct in_addr ip) {
    for (int i = 0; i < nb_users; i++) {
        if (table[i].ip.s_addr == ip.s_addr) {
            table[i] = table[nb_users - 1]; 
            nb_users--;
            return;
        }
    }
}


volatile int server_running = 1;
void handle_stop_signal(int sig) {
    server_running = 0;
}

void start_beuip_server(const char *pseudo) {
    int sid;
    struct sockaddr_in servAddr, cliAddr, bcastAddr;
    char buffer[1024], out_msg[1024];
    int broadcast_enable = 1;

    struct sigaction sa;
    sa.sa_handler = handle_stop_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    sid = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sid, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(PORT);
    bind(sid, (struct sockaddr *)&servAddr, sizeof(servAddr));

    memset(&bcastAddr, 0, sizeof(bcastAddr));
    bcastAddr.sin_family = AF_INET;
    bcastAddr.sin_port = htons(PORT);
    bcastAddr.sin_addr.s_addr = inet_addr(BCAST_IP);
    
    sprintf(out_msg, "1BEUIP%s", pseudo); /* 统一使用 pseudo */
    sendto(sid, out_msg, strlen(out_msg), 0, (struct sockaddr *)&bcastAddr, sizeof(bcastAddr));

    printf("[TRACE] Serveur en arrière-plan démarré. Attente des messages...\n");

    while (server_running) {
        fd_set readfds;
        struct timeval tv;
        
        FD_ZERO(&readfds);
        FD_SET(sid, &readfds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 500000;

        int ret = select(sid + 1, &readfds, NULL, NULL, &tv);
        
        if (ret < 0 || !server_running) {
            break;
        }

        if (ret > 0 && FD_ISSET(sid, &readfds)) {
            socklen_t addrLen = sizeof(cliAddr);
            int n = recvfrom(sid, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cliAddr, &addrLen);
            
            if (n >= 6 && strncmp(buffer + 1, "BEUIP", 5) == 0) {
                buffer[n] = '\0';
                char code = buffer[0];
                char *payload = buffer + 6;
                int is_local = (cliAddr.sin_addr.s_addr == inet_addr(LOCALHOST));

                switch (code) {
                    case '1': 
                        if (strcmp(payload, pseudo) != 0) { /* 统一使用 pseudo */
                            add_or_update_user(cliAddr.sin_addr, payload);
                            sprintf(out_msg, "2BEUIP%s", pseudo);
                            sendto(sid, out_msg, strlen(out_msg), 0, (struct sockaddr *)&cliAddr, addrLen);
                        }
                        break;
                    case '2': 
                        add_or_update_user(cliAddr.sin_addr, payload);
                        break;
                    case '3': 
                        if (is_local) {
                            printf("\n--- Utilisateurs BEUIP en ligne (%d) ---\n", nb_users);
                            for (int i = 0; i < nb_users; i++) {
                                printf(" - %s (%s)\n", table[i].pseudo, inet_ntoa(table[i].ip));
                            }
                        }
                        break;
                    case '4': 
                        if (is_local) {
                            char *target_pseudo = payload;
                            char *msg = payload + strlen(target_pseudo) + 1;
                            for (int i = 0; i < nb_users; i++) {
                                if (strcmp(table[i].pseudo, target_pseudo) == 0) {
                                    sprintf(out_msg, "9BEUIP%s", msg);
                                    struct sockaddr_in targetAddr;
                                    memset(&targetAddr, 0, sizeof(targetAddr));
                                    targetAddr.sin_family = AF_INET;
                                    targetAddr.sin_port = htons(PORT); /* 强制修正端口 */
                                    targetAddr.sin_addr = table[i].ip;
                                    sendto(sid, out_msg, strlen(out_msg), 0, (struct sockaddr *)&targetAddr, sizeof(targetAddr));
                                    break;
                                }
                            }
                        }
                        break;
                    case '5': 
                        if (is_local) {
                            sprintf(out_msg, "9BEUIP%s", payload);
                            struct sockaddr_in targetAddr;
                            memset(&targetAddr, 0, sizeof(targetAddr));
                            targetAddr.sin_family = AF_INET;
                            targetAddr.sin_port = htons(PORT); /* 强制修正端口 */
                            for (int i = 0; i < nb_users; i++) {
                                targetAddr.sin_addr = table[i].ip;
                                sendto(sid, out_msg, strlen(out_msg), 0, (struct sockaddr *)&targetAddr, sizeof(targetAddr));
                            }
                        }
                        break;
                    case '9': 
                        {
                            char sender[50] = "Inconnu";
                            for (int i = 0; i < nb_users; i++) {
                                if (table[i].ip.s_addr == cliAddr.sin_addr.s_addr) {
                                    strcpy(sender, table[i].pseudo);
                                    break;
                                }
                            }
                            printf("\n[Message de %s]: %s\n", sender, payload);
                        }
                        break;
                    case '0': 
                        remove_user(cliAddr.sin_addr);
                        break;
                }
            }
        }
    }

    printf("\n[TRACE] Signal d'arrêt reçu. Déconnexion en cours...\n");
    sprintf(out_msg, "0BEUIP%s", pseudo); /* 统一使用 pseudo */
    sendto(sid, out_msg, strlen(out_msg), 0, (struct sockaddr *)&bcastAddr, sizeof(bcastAddr));
    
    close(sid);
    exit(0);
}

void send_local_command(char code, const char *param1, const char *param2) {//clibeuip
    int sid = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in servAddr;
    char buffer[1024];
    int msg_len = 0;

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(PORT);
    servAddr.sin_addr.s_addr = inet_addr(LOCALHOST);

    if (code == '3') {
        msg_len = sprintf(buffer, "3BEUIP");
    } else if (code == '4') {
        msg_len = sprintf(buffer, "4BEUIP%s", param1);
        msg_len++; 
        strcpy(buffer + msg_len, param2);
        msg_len += strlen(param2);
    } else if (code == '5') {
        msg_len = sprintf(buffer, "5BEUIP%s", param1);
    } else if (code == '0'){
        strcpy(buffer, "0BEUIP");
        msg_len = 6;
        printf("Signal de déconnexion envoyé au serveur.\n");
    } else {
        fprintf(stderr, "Code inconnu. Utilisez 3, 4, 5 ou 0.\n");
        exit(4);
    }
    

    if ((sendto(sid, buffer, msg_len, 0, (struct sockaddr *)&servAddr, sizeof(servAddr))) == -1){
        perror("sendto");
        exit(5);
    }
    
    close(sid);
}