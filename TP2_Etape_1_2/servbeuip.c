#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9998
#define BCAST_IP "192.168.88.255"

struct User {
    struct in_addr ip;
    char pseudo[50];
} table[255];
int nb_users = 0;

void add_or_update_user(struct in_addr ip, const char *pseudo) {
    for (int i = 0; i < nb_users; i++) {
        if (table[i].ip.s_addr == ip.s_addr) {
            strncpy(table[i].pseudo, pseudo, 49); 
            table[i].pseudo[49] = '\0';
            return;
        }
    }
    if (nb_users < 255) {
        table[nb_users].ip = ip;
        strncpy(table[nb_users].pseudo, pseudo, 49);
        table[nb_users].pseudo[49] = '\0';
        nb_users++;
        printf("[TRACE] Nouvel utilisateur ajouté: %s (%s)\n", pseudo, inet_ntoa(ip));
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

int main(int argc, char *argv[]) {
    int sid;
    struct sockaddr_in servAddr, cliAddr, bcastAddr;
    socklen_t addrLen = sizeof(cliAddr);
    char buffer[1024];
    char out_msg[1024];
    int broadcast_enable = 1;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pseudo>\n", argv[0]);
        exit(1);
    }
    char *my_pseudo = argv[1];

    if ((sid = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sid, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("setsockopt SO_BROADCAST");
        exit(1);
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(PORT);

    if (bind(sid, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        perror("bind");
        exit(1);
    }

    printf("Serveur BEUIP démarré sur le port %d avec le pseudo '%s'.\n", PORT, my_pseudo);

    memset(&bcastAddr, 0, sizeof(bcastAddr));
    bcastAddr.sin_family = AF_INET;
    bcastAddr.sin_port = htons(PORT);
    bcastAddr.sin_addr.s_addr = inet_addr(BCAST_IP);

    sprintf(out_msg, "1BEUIP%s", my_pseudo); 
    sendto(sid, out_msg, strlen(out_msg), 0, (struct sockaddr *)&bcastAddr, sizeof(bcastAddr));

    while (1) {
        int n = recvfrom(sid, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cliAddr, &addrLen);
        if (n < 0) continue;
        buffer[n] = '\0';

        if (n < 6 || strncmp(buffer + 1, "BEUIP", 5) != 0) {
            continue; 
        }

        char code = buffer[0];
        char *payload = buffer + 6;
        int is_local = (cliAddr.sin_addr.s_addr == inet_addr("127.0.0.1"));

        switch (code) {
            case '1': 
                if (strcmp(payload, my_pseudo) != 0) {
                    add_or_update_user(cliAddr.sin_addr, payload); 
                    sprintf(out_msg, "2BEUIP%s", my_pseudo);
                    
                    struct sockaddr_in replyAddr = cliAddr;
                    replyAddr.sin_port = htons(PORT);
                    sendto(sid, out_msg, strlen(out_msg), 0, (struct sockaddr *)&replyAddr, sizeof(replyAddr));
                }
                break;

            case '2': 
                add_or_update_user(cliAddr.sin_addr, payload); 
                break;

            case '3':
                if (is_local) {
                    printf("Liste des utilisateurs (%d) \n", nb_users);
                    for (int i = 0; i < nb_users; i++) {
                        printf("%s\t%s\n", inet_ntoa(table[i].ip), table[i].pseudo); 
                    }
                }
                break;

            case '4': 
                if (is_local) {
                    char *target_pseudo = payload;
                    char *msg = payload + strlen(target_pseudo) + 1; 
                    
                    int found = 0;
                    for (int i = 0; i < nb_users; i++) {
                        if (strcmp(table[i].pseudo, target_pseudo) == 0) {
                            sprintf(out_msg, "9BEUIP%s", msg);
                            
                            struct sockaddr_in targetAddr;
                            memset(&targetAddr, 0, sizeof(targetAddr));
                            targetAddr.sin_family = AF_INET;
                            targetAddr.sin_port = htons(PORT); 
                            targetAddr.sin_addr = table[i].ip;
                            
                            sendto(sid, out_msg, strlen(out_msg), 0, (struct sockaddr *)&targetAddr, sizeof(targetAddr));
                            found = 1;
                            break;
                        }
                    }
                    if (!found) printf("Erreur: Pseudo '%s' introuvable.\n", target_pseudo);
                }
                break;

            case '9': 
            {
                char sender_pseudo[50] = "Inconnu";
                int found = 0;
                for (int i = 0; i < nb_users; i++) {
                    if (table[i].ip.s_addr == cliAddr.sin_addr.s_addr) {
                        strcpy(sender_pseudo, table[i].pseudo);
                        found = 1;
                        break;
                    }
                }
                if (found) {
                    printf("Message de %s: %s\n", sender_pseudo, payload); 
                } else {
                    printf("Erreur: Message d'une IP non enregistrée.\n"); 
                }
                break;
            }

            case '5': 
                if (is_local) {
                    sprintf(out_msg, "9BEUIP%s", payload); 

                    struct sockaddr_in targetAddr;
                    memset(&targetAddr, 0, sizeof(targetAddr));
                    targetAddr.sin_family = AF_INET;
                    targetAddr.sin_port = htons(PORT); 
                    
                    for (int i = 0; i < nb_users; i++) {
                        targetAddr.sin_addr = table[i].ip;
                        sendto(sid, out_msg, strlen(out_msg), 0, (struct sockaddr *)&targetAddr, sizeof(targetAddr)); 
                    }
                    printf("Message diffusé à %d utilisateurs.\n", nb_users);
                }
                break;

            case '0': 
                if (is_local) {
                    sprintf(out_msg, "0BEUIP%s", my_pseudo);
                    sendto(sid, out_msg, strlen(out_msg), 0, (struct sockaddr *)&bcastAddr, sizeof(bcastAddr));
                    printf("Arrêt du serveur BEUIP.\n");
                    close(sid);
                    exit(0);
                } else {
                    remove_user(cliAddr.sin_addr); 
                    printf("Utilisateur déconnecté: %s\n", payload);
                }
                break;
        }
    }
    return 0;
}