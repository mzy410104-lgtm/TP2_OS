#ifndef _GESCOM_H_
#define _GESCOM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>


#define MAXMS 50 //maximum de mot de size


extern char *Mots[MAXMS];
extern int NMots;

/*Stucture de le command interieur*/
typedef struct{
    char *nom;
    int( *fonction)(int,char **);
}ComInt;

/*le declare des fonctions*/
/*free le Mots*/
void freeMots();

/*les command int*/
int Sortie(int n,char *P[]);
int do_cd(int n,char *P[]);
int do_pwd(int n,char *P[]);
int do_ver(int n,char *P[]);
int do_beuip(int n, char *P[]);
int do_mess(int n, char *P[]);

/*ajouter les command int*/
void ajouteCom(char *nom,int( *f)(int,char**));
void majCommand(void);

/*executer les commands*/
/*executer les commands int*/
int execComInt(int N,char *P[]);
/*executer les commands ext*/
int execComext(char **P);

/*Copier le command*/
char *copyString(char *s);
/*analyse le command*/
int analyseCom(char *s);


char *make_prompt();

#endif
