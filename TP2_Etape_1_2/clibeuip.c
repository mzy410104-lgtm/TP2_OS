#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9998
#define LOCALHOST "127.0.0.1"

int main(int argc,char* argv[] ){
    int sid;
    struct sockaddr_in servAddr;
    char buffer[1024];
    int msg_len = 0;
    
    if (argc < 2){
        fprintf(stderr,"Utilisation : \n ");
        fprintf(stderr, "  %s 3                  : Demander la liste des utilisateurs\n", argv[0]);
        fprintf(stderr, "  %s 4 <pseudo> <msg>   : Envoyer un message privé\n", argv[0]);
        fprintf(stderr, "  %s 5 <msg>            : Envoyer un message à tout le monde\n", argv[0]);
        fprintf(stderr, "  %s 0                  : Quitter le réseau\n", argv[0]);
        exit(1);
    }

    if ((sid = socket(AF_INET,SOCK_DGRAM,0)) == -1){
        perror("socket");
        exit(2);
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(PORT);
    servAddr.sin_addr.s_addr = inet_addr(LOCALHOST);

    char code = argv[1][0];

    switch (code)
    {
    case '3':strcpy(buffer,"3BEUIP");
        msg_len = 6;
        printf("Demande de liste envoye au serveur\n");
        break;
    case '4':
        if (argc != 4){
            fprintf(stderr,"Erreur : pseudo ou massage manque\n");
            exit(3);
        }
        msg_len = sprintf(buffer,"4BEUIP%s",argv[2]);
        msg_len ++;
        strcpy(buffer + msg_len, argv[3]);
        msg_len += strlen(argv[3]);
        printf("Demande d'envoi de message privé à '%s' envoyée.\n", argv[2]);
        break;
    case '5' : if (argc != 3) {
        fprintf(stderr, "Erreur: Message manquant.\n");
        exit(1);
        }
        msg_len = sprintf(buffer, "5BEUIP%s", argv[2]);
        printf("Demande de diffusion envoyée.\n");
        break;
    case '0' : strcpy(buffer, "0BEUIP");
        msg_len = 6;
        printf("Signal de déconnexion envoyé au serveur.\n");
        break;
    default:
        fprintf(stderr, "Code inconnu. Utilisez 3, 4, 5 ou 0.\n");
        exit(4);
        break;
    }

    if (sendto(sid, buffer, msg_len, 0, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        perror("sendto");
        exit(1);
    }
    close(sid);
    return 0;
}
