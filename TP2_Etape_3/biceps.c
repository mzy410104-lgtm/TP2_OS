#include "gescom.h"

int main(int N,char *P[]){
    signal(SIGINT,SIG_IGN);
    read_history(".biceps_history");
    char *line;
    char *prompt;
    majCommand();

    while (1){
        prompt = make_prompt();
        line = readline(prompt);
        free(prompt);

        /*Si le EOF*/
        if (line == NULL){
            printf("\nSortie par EOF(Ctrl + D)\n");
            break;
        }

        /*Si le command n'est pas null*/
        if (strlen(line) >0){
            add_history(line);
            char *ptr = line;
            char *command_segment;
            printf("Le command %s est saisie",line);

            while ((command_segment = strsep(&ptr,";")) != NULL){
                if (*command_segment == '\0') continue;                  
                int count = analyseCom(command_segment);
                if (count > 0){
                    printf("Il y a %d mots\n",count);
                    printf("le command est %s\n",Mots[0]);
                    for (int i = 1; i < NMots; i++){
                        printf("le parametre[%d] est %s\n",i,Mots[i]);
                    }
                    if (!execComInt(count,Mots)){
                        execComext(Mots);
                    }
                }
            }
        }
        free(line);
    }
    return 0;
}