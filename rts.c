#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include<readline/readline.h> 
#include<readline/history.h>  
#define MAXCOM 100000 // max number of letters to be supported 
#define MAXLIST 10000 // max number of commands to be supported 
#define BUFFERSIZE 128
#define LSH_TOK_DELIM " \t\r\n\a"
#define INITSIZE 2048
// Clearing the shell using escape sequences 
#define clear() printf("\033[H\033[J")

int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

int shell_execute_pipe(char **cmd1, char **cmd2) {

    int pipeRW[2];
    pid_t p1, p2, wpid;
    int status;
    if(pipe(pipeRW) < 0) {
        puts("PIPE1 ERROR");
        return -1;
    }
    p1 = fork();
    if(p1 < 0) {
    
        puts("UNABLE TO FORM FIRST CHILD");
        return - 1;
    }
    if(p1 == 0) {
        
        close(pipeRW[0]); //end 0 for reading, end 1 for writing
        dup2(pipeRW[1], STDOUT_FILENO);
        close(pipeRW[1]);

        if(execvp(cmd1[0], cmd1 )< 0) {
            puts("UNABLE TO EXECUTE COMMAND 1");
            return -1;
        }
    }
    else {
        p2 = fork();

        if(p2 < 0) {
        
            puts("PIPE2 ERROR");
            return -1;
        }
        
        if(p2 == 0) {
            close(pipeRW[1]);
            dup2(pipeRW[0], STDIN_FILENO);
            close(pipeRW[0]);
        
            if(execvp(cmd2[0], cmd2 )< 0) {
                puts("UNABLE TO EXECUTE COMMAND 2");
                return -1;
            }
        }
        else {
            wait(NULL);
        }

   }
   return 1;

}

int shell_execute_pipes(char **cmd1, char **cmd2, char **cmd3) {
    int pipe1[2];
    int pipe2[2];
    pid_t p1, p2, p3;
    if(pipe(pipe1) < 0) {
        puts("PIPE1 ERROR");
        return -1;
    }
    p1 = fork();
    if(pid < 0) {
        puts("UNABLE TO CREATE CHILD 1");
        return -1;
    }
    if(p1 == 0) {
        close(pipe1[0]);
        dup2(pipe1[1], 1);
        close(pipe1[1]);

        if(execvp(cmd1[0], cmd1) < 0) {
            puts("UNABLE TO EXECUTE COMMAND 1");
            return -1;
        }
    }
    else {
        if(pipe(pipe2) < 0) {
            puts("PIPE2 ERROR");
            return -1;
        }
        p2 = fork();
        if(p2 < 0) {
            puts("UNABLE TO CREATE CHILD 2");
            return -1;
        }
        if(p2 == 0) {
            dup2(pipe1[0], 0);
            dup2(pipe2[1], 1);
            close(pipe1[0]);
            close(pipe1[1]);
            close(pipe2[0]);
            close(pipe2[1]);

            if(execvp(cmd2[0], cmd2) < 0) {
                puts("UNABLE TO EXECUTE COMMAND 2");
            }

        }
        else {
            close(pipe1[0]);
            close(pipe1[1]);
            
            p3 = fork();
            if(p3 == 0) {
                dup2(pipe2[0], 0);
                close(pipe2[0]);
                close(pipe2[1]);

                if(execvp(cmd3[0], cmd3) < 0) {
                    puts("UNABLE TO EXECUTE COMMAND 3");
                    return -1;    
                }
            }
            else {
               wait(NULL);
            }
        }
    }
    return 1;
}

int lsh_launch(char **args) {
    
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
        perror("lsh");
    }
    exit(EXIT_FAILURE);
    } 
    else if (pid < 0) {
        // Error forking
        perror("lsh");
    } 
    else {
        // Parent process
        do {
        wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}


/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};

int lsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int lsh_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    } 
    else {
        if (chdir(args[1]) != 0) {
        perror("lsh");
        }
    }
    return 1;
}

int lsh_help(char **args)
{
    int i;
    printf("Stephen Brennan's LSH\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < lsh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int lsh_exit(char **args)
{
    return 0;
}

char *shell_readline(void)
{
    char *line = NULL;
    ssize_t bufsize = 0; // have getline allocate a buffer for us
    getline(&line, &bufsize, stdin);
    return line;
}

int shell_pipe_parsed(char* line, char** toks) { 
      
    int buf = BUFFERSIZE;
    int position = 0;
    char *tok;

    if(!toks) {

        perror("ALLOCATION FAILED WHILE PARSING\n");
        exit(EXIT_FAILURE);
    }

    tok = strtok(line, "|");

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
        tok = strtok(NULL, "|");
    }
    toks[position] = NULL;
    return (position - 1); 
}

int shell_execute(char **args) {
  
    int i;
    if (args[0] == NULL) {
        // An empty command was entered.
        return 1;
    }

    for (i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
    return lsh_launch(args);
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

    char *line  = malloc(sizeof(char*) * INITSIZE);
    char **args = malloc(sizeof(char*) * INITSIZE);
    char** pipedArgs = malloc(sizeof(char*) * 64);
    char** arg1 = malloc(sizeof(char*) * INITSIZE);
    char** arg2 = malloc(sizeof(char*) * INITSIZE);
    char** arg3 = malloc(sizeof(char*) * INITSIZE);
    int p;
    char **pipedCmd = malloc(sizeof(char*) * INITSIZE);
    int status = 0;
    do {
        
        printf("ownShell> ");
        line = shell_readline();
        p = shell_pipe_parsed(line, pipedArgs);
        //printf("PIPES : %d\n", p);
        
        if(p == 2) {
            arg1 = shell_parse(pipedArgs[0]);
            arg2 = shell_parse(pipedArgs[1]);
            arg3 = shell_parse(pipedArgs[2]);
        }

        if(p == 1) {
            //puts(pipedArgs[0]);
            arg1 = shell_parse(pipedArgs[0]);
            arg2 = shell_parse(pipedArgs[1]);
        }
        else
            args = shell_parse(line);
        
        if(p == 0)
            status = shell_execute(args);
        if(p == 1)
            status = shell_execute_pipe(arg1, arg2);
        if(p == 2)
            status = shell_execute_pipes(arg1, arg2, arg3);
        free(line);
        free(args);
    } while(status);
}

int main() {
    init_loop();
    return EXIT_SUCCESS;
}
