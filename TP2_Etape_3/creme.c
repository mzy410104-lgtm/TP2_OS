#include "creme.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 9998
#define BCAST_IP "192.168.88.255"
#define LOCALHOST "127.0.0.1"

struct User { struct in_addr ip; char pseudo[50]; } table[255];
int nb_users = 0;
volatile sig_atomic_t server_running = 1;

void handle_stop_signal(int sig) { server_running = 0; }

void add_or_update_user(struct in_addr ip, const char *pseudo) {
    for (int i = 0; i < nb_users; i++) {
        if (table[i].ip.s_addr == ip.s_addr) {
            strncpy(table[i].pseudo, pseudo, 49); return;
        }
    }
    if (nb_users < 255) {
        table[nb_users].ip = ip;
        strncpy(table[nb_users].pseudo, pseudo, 49);
        table[nb_users].pseudo[49] = '\0'; nb_users++;
    }
}

void remove_user(struct in_addr ip) {
    for (int i = 0; i < nb_users; i++) {
        if (table[i].ip.s_addr == ip.s_addr) {
            table[i] = table[nb_users - 1]; nb_users--; return;
        }
    }
}

void handle_c1(int sid, struct sockaddr_in c, char *p, const char *me) {
    if (strcmp(p, me) == 0) return;
    add_or_update_user(c.sin_addr, p);
    char msg[1024]; sprintf(msg, "2BEUIP%s", me);
    c.sin_port = htons(PORT); 
    sendto(sid, msg, strlen(msg), 0, (struct sockaddr*)&c, sizeof(c));
}

void handle_c3(int local) {
    if (!local) return;
    printf("\n--- Utilisateurs BEUIP (%d) ---\n", nb_users);
    for(int i = 0; i < nb_users; i++)
        printf(" - %s (%s)\n", table[i].pseudo, inet_ntoa(table[i].ip));
}

void handle_c4(int sid, int local, char *p) {
    if (!local) return;
    char *target = p, *msg = p + strlen(target) + 1;
    for (int i = 0; i < nb_users; i++) {
        if (strcmp(table[i].pseudo, target) == 0) {
            char out[1024]; sprintf(out, "9BEUIP%s", msg);
            struct sockaddr_in tAddr; memset(&tAddr, 0, sizeof(tAddr));
            tAddr.sin_family = AF_INET; tAddr.sin_port = htons(PORT); 
            tAddr.sin_addr = table[i].ip;
            sendto(sid, out, strlen(out), 0, (struct sockaddr*)&tAddr, sizeof(tAddr));
            break;
        }
    }
}

void handle_c5(int sid, int local, char *p) {
    if (!local) return;
    char out[1024]; sprintf(out, "9BEUIP%s", p);
    struct sockaddr_in tAddr; memset(&tAddr, 0, sizeof(tAddr));
    tAddr.sin_family = AF_INET; tAddr.sin_port = htons(PORT);
    for (int i = 0; i < nb_users; i++) {
        tAddr.sin_addr = table[i].ip;
        sendto(sid, out, strlen(out), 0, (struct sockaddr*)&tAddr, sizeof(tAddr));
    }
}

void handle_c9(struct in_addr ip, char *p) {
    char sender[50] = "Inconnu";
    for (int i = 0; i < nb_users; i++) {
        if (table[i].ip.s_addr == ip.s_addr) { 
            strcpy(sender, table[i].pseudo); break; 
        }
    }
    printf("\n[Message de %s]: %s\n", sender, p);
}

/* Fonction de routage principale */
void route_msg(char code, char *p, int sid, struct sockaddr_in c, const char *me) {
    int loc = (c.sin_addr.s_addr == inet_addr(LOCALHOST));
    if (code == '1') handle_c1(sid, c, p, me);
    else if (code == '2') add_or_update_user(c.sin_addr, p);
    else if (code == '3') handle_c3(loc);
    else if (code == '4') handle_c4(sid, loc, p);
    else if (code == '5') handle_c5(sid, loc, p);
    else if (code == '9') handle_c9(c.sin_addr, p);
    else if (code == '0') remove_user(c.sin_addr);
}

/* --- Fonctions d'initialisation et serveur --- */
int init_socket() {
    int sid = socket(AF_INET, SOCK_DGRAM, 0), on = 1;
    setsockopt(sid, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    struct sockaddr_in sAddr; memset(&sAddr, 0, sizeof(sAddr));
    sAddr.sin_family = AF_INET; sAddr.sin_addr.s_addr = INADDR_ANY; 
    sAddr.sin_port = htons(PORT);
    bind(sid, (struct sockaddr *)&sAddr, sizeof(sAddr));
    return sid;
}

void send_bcast(int sid, const char *code, const char *pseudo) {
    struct sockaddr_in bAddr; memset(&bAddr, 0, sizeof(bAddr));
    bAddr.sin_family = AF_INET; bAddr.sin_port = htons(PORT); 
    bAddr.sin_addr.s_addr = inet_addr(BCAST_IP);
    char msg[1024]; sprintf(msg, "%sBEUIP%s", code, pseudo);
    sendto(sid, msg, strlen(msg), 0, (struct sockaddr*)&bAddr, sizeof(bAddr));
}

void start_beuip_server(const char *pseudo) {
    struct sigaction sa; sa.sa_handler = handle_stop_signal; 
    sigemptyset(&sa.sa_mask); sa.sa_flags = 0; sigaction(SIGTERM, &sa, NULL);
    int sid = init_socket(); send_bcast(sid, "1", pseudo);
    char buf[1024]; struct sockaddr_in cAddr;
    while (server_running) {
        fd_set fds; FD_ZERO(&fds); FD_SET(sid, &fds);
        struct timeval tv = {0, 500000}; /* Timeout 0.5s */
        if (select(sid + 1, &fds, NULL, NULL, &tv) > 0) {
            socklen_t len = sizeof(cAddr);
            int n = recvfrom(sid, buf, 1023, 0, (struct sockaddr *)&cAddr, &len);
            if (n >= 6 && !strncmp(buf + 1, "BEUIP", 5)) {
                buf[n] = '\0'; route_msg(buf[0], buf + 6, sid, cAddr, pseudo);
            }
        }
    }
    send_bcast(sid, "0", pseudo); close(sid); exit(0);
}

void send_local_command(char code, const char *p1, const char *p2) {
    int sid = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in sAddr; memset(&sAddr, 0, sizeof(sAddr));
    sAddr.sin_family = AF_INET; sAddr.sin_port = htons(PORT); 
    sAddr.sin_addr.s_addr = inet_addr(LOCALHOST);
    char msg[1024];
    if (code == '4') sprintf(msg, "4BEUIP%s%c%s", p1, '\0', p2);
    else if (code == '5') sprintf(msg, "5BEUIP%s", p1);
    else sprintf(msg, "%cBEUIP", code);
    int len = (code == '4') ? 6 + strlen(p1) + 1 + strlen(p2) : strlen(msg);
    sendto(sid, msg, len, 0, (struct sockaddr*)&sAddr, sizeof(sAddr));
    close(sid);
}