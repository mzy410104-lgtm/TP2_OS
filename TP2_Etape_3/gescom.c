#include "gescom.h"
#include "creme.h"

#define NBMAXC 10

char *Mots[MAXMS];
int NMots = 0;

static ComInt TabComint[NBMAXC];
static int NbComInt = 0;

pid_t beuip_server_pid = -1;

/*Free le Mots*/
void freeMots(){
    for (int i = 0; i < NMots; i++){
        free(Mots[i]);
        Mots[i] = NULL;
    }
    NMots = 0;
}

/*la fonction de command exit*/
int Sortie(int n,char *P[]){
    write_history(".biceps_history");
    exit(0);
}

/*la fonction de command cd*/
int do_cd(int n,char *P[]){
    char *path;
    if (n < 2){
        path = getenv("HOME");
    }else{
        path = P[1];
    }
    if (chdir(path) != 0){
        perror("cd");
    }
    return 1;
}

/*la fonction de command pwd*/
int do_pwd(int n,char *P[]){
    char cwd[1024];
    if (getcwd(cwd,sizeof(cwd)) != NULL){
        printf("%s\n",cwd);
    }else{
        perror("pwd");
    }
    return 1;
}

/*la fonction de command ver*/
int do_ver(int n,char *P[]){
    printf("biceps version 1.0\n");
    return 1;
}

int do_beuip(int n, char *P[]) {
    if (n < 2) {
        printf("Usage: beuip start <pseudo> | beuip stop\n");
        return 1;
    }
    
    if (strcmp(P[1], "start") == 0 && n >= 3) {
        if (beuip_server_pid > 0) {
            printf("Le serveur tourne déjà (PID: %d)!\n", beuip_server_pid);
        } else {
            beuip_server_pid = fork();
            if (beuip_server_pid == 0) {
                start_beuip_server(P[2]); 
                exit(0);
            }
        }
    } else if (strcmp(P[1], "stop") == 0) {
        if (beuip_server_pid > 0) {
            kill(beuip_server_pid, SIGTERM); 
            waitpid(beuip_server_pid, NULL, 0);
            beuip_server_pid = -1;
            printf("Serveur BEUIP arrêté.\n");
        } else {
            printf("Aucun serveur BEUIP en cours.\n");
        }
    } else {
        printf("Commande beuip non reconnue.\n");
    }
    return 1;
}

int do_mess(int n, char *P[]) {
    if (n < 2) {
        printf("Usage: mess liste | mess a <pseudo> <msg> | mess tous <msg>\n");
        return 1;
    }
    
    if (strcmp(P[1], "liste") == 0) {
        send_local_command('3', NULL, NULL);
    } else if (strcmp(P[1], "a") == 0 && n >= 4) {
        send_local_command('4', P[2], P[3]);
    } else if (strcmp(P[1], "tous") == 0 && n >= 3) {
        send_local_command('5', P[2], NULL);
    } else {
        printf("Commande mess non reconnue ou paramètres manquants.\n");
    }
    return 1;
}

/*ajouter le command dans le tableau*/
void ajouteCom(char *nom,int( *f)(int,char**)){
    if (NbComInt < NBMAXC){
        TabComint[NbComInt].nom = nom;
        TabComint[NbComInt].fonction = f;
        NbComInt ++;
    }else{
        fprintf(stderr,"Error : le tableau de commande interne est pleine");
        exit(1);
    }
}

/*initialiser les commands interieur*/
void majCommand(void){
    ajouteCom("exit",Sortie);
    ajouteCom("cd",do_cd);
    ajouteCom("pwd",do_pwd);
    ajouteCom("ver",do_ver);
    ajouteCom("beuip", do_beuip);
    ajouteCom("mess", do_mess);
}

/*Executer les command int*/
int execComInt(int N,char *P[]){
    for (int i = 0; i < NbComInt; i++){
        if (strcmp(P[0],TabComint[i].nom) == 0){
            TabComint[i].fonction(N,P);
            return 1;//C'est le fonction inter
        }
    }
    return 0;//C'est pas le fonction inter
}

/*Executer les command ext*/
int execComext(char **P){
    int pid,status;
    pid = fork();
    if (pid == 0){//le processus de fils
        execvp(P[0],P);
        perror(P[0]);
        exit(1);
    }else if (pid > 0){//le pocessus de pere
        waitpid(pid,&status,0);
    }else{
        perror("fork");
    }
    return 0;
}

/*Copier le command*/
char *copyString(char *s){
    if (s == NULL) return NULL;
    char *copy = malloc(strlen(s) + 1);
    if (copy){
        strcpy(copy,s);
    }
    return copy;
}

/*analyse le command*/
int analyseCom(char *s){
    char *token;
    char *delimiter = " \t\n";

    NMots = 0;
    while ((token = strsep(&s,delimiter)) != NULL){
        if (*token != '\0'){
            if (NMots < (MAXMS -1)){
                Mots[NMots++] = token;
            }
        }
    }
    Mots[NMots] = NULL;
    return NMots;
}

char *make_prompt(){
  char hostname[256];
  char *user = getenv("USER");//get the name of user
  
  if(gethostname(hostname,sizeof(hostname))!= 0){
    strcpy(hostname,"unknown");//the name of computer
  }

  char symbol = (getuid() == 0) ? '#' : '$';//user normal est '$',user root est '#'

  /*Creer le prompt*/
  char *prompt = malloc(strlen(hostname) + strlen(user) + symbol +5);
  if (prompt != 0){
    sprintf(prompt,"%s@%s %c ",user,hostname,symbol);
  }
  return prompt;
}
