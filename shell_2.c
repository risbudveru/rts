#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include<readline/readline.h> 
#include<readline/history.h>  
#define MAXCOM 1000 // max number of letters to be supported 
#define MAXLIST 100 // max number of commands to be supported 
#define BUFFERSIZE 64
#define LSH_TOK_DELIM " |\t\r\n\a"

// Clearing the shell using escape sequences 
#define clear() printf("\033[H\033[J")

char *shell_readline(void)
{
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  getline(&line, &bufsize, stdin);
  return line;
}

int shell_pipe_parsed(char* str, char** strpiped) { 
    int i = 0; 
    for (; i < MAXLIST; i++) { 
        strpiped[i] = strsep(&str, "|"); 
        if (strpiped[i] == NULL) 
            break; 
    } 
  
    if(strpiped[2] == NULL) {
        if(strpiped[1] == NULL)
            return 0;
        else
            return 1;
    }
    else
        return 2;
}

char** shell_parse(char *line) {

    int buf = BUFFERSIZE;
    int position = 0;
    char **toks = malloc(sizeof(char*) * buf);
    char *tok;
   
    if(!toks) {
    
        perror("ALLOCATION FAILED WHILE PARSING\n");
        exit(EXIT_FAILURE);
    }
    
    tok = strtok(line, " \t\r\n\a");
    
    while(tok) {
        toks[position++] = tok;
        if(position >= buf) {
            buf += BUFFERSIZE;
            toks = realloc(toks, sizeof(char*) * buf);
            if(!toks) {
            
                perror("ALLOCATION FAILED WHILE REALLOCATION\n");
                exit(EXIT_FAILURE);
            }
        }
        tok = strtok(NULL, " \t\r\n\a"); 
    }     
    toks[position] = NULL;
    return toks;
}

void init_loop() {

    char *line;
    char **args;
    char* pipedArgs[MAXLIST];
    char** arg1;
    char** arg2; 
    int p;
    char **pipedCmd;
    int status = 0;
    do {
        
        printf("ownShell> ");
        line = shell_readline();
        p = shell_pipe_parsed(line, pipedArgs);
        printf("%d\n", p);
        /*if(p == 2) {
            shell_parse(pipedArgs[0], pipedCmd);
            shell_parse(pipedArgs[1], pipedCmd);
        }
        else if(p == 1)
            shell_parse(pipedArgs[0], pipedCmd);
        else
            shell_parse(line, args);*/
        if(p) {
            arg1 = shell_parse(pipedArgs[0]);
            arg2 = shell_parse(pipedArgs[1]);
        }
        else
            args = shell_parse(line);
        //status = shell_execute(args);
        //printf("%d\n",p);
        free(line);
        free(args);
    } while(status);
}

int main() {
    init_loop();
    return EXIT_SUCCESS;
}
