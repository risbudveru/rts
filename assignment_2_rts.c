#define _GNU_SOURCE
#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include<readline/readline.h> 
#include<readline/history.h>  
#include<fcntl.h>
#include<errno.h>
#include<limits.h>

#define MAXCOM 100000 // max number of letters to be supported 
#define MAXLIST 10000 // max number of commands to be supported 
#define BUFFERSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
#define INITSIZE 256

// Clearing the shell using escape sequences 
#define clear() printf("\033[H\033[J")

static int log = 0;
int shl_cd(char **args);
int shl_help(char **args);
int shl_exit(char **args);

void viewoutlog(void) {

    char filename[1000], c;
    FILE *fp = fopen("output.log", "r");
    if (fp == NULL) {   
        printf("Cannot open file \n");
        return;
    }
    c = fgetc(fp);
    
    while (c != EOF)
    {
        printf ("%c", c);
        c = fgetc(fp);
    }
    

}

void viewcmdlog(void) {

    char filename[1000], c;
    FILE *fp = fopen("commands.log", "r");
    if (fp == NULL) {   
        printf("Cannot open file \n");
        return;
    }
    c = fgetc(fp);
    
    while (c != EOF)
    {
        printf ("%c", c);
        c = fgetc(fp);
    }
    

}

int threePipe(char **arg1, char **arg2, char **arg3) {
  int i, status;
  int pipes[4];
  pipe(pipes); 
  pipe(pipes + 2); 
 
  if (fork() == 0)
    {

      dup2(pipes[1], 1);

      close(pipes[0]);
      close(pipes[1]);
      close(pipes[2]);
      close(pipes[3]);

      execvp(*arg1, arg1);
    }
  else
    {

      if (fork() == 0)
	{
	  
	  dup2(pipes[0], 0);

	  dup2(pipes[3], 1);

	  close(pipes[0]);
	  close(pipes[1]);
	  close(pipes[2]);
	  close(pipes[3]);

	  execvp(*arg2, arg2);
	}
      else
	{

	  if (fork() == 0)
	    {

	      dup2(pipes[2], 0);

	      close(pipes[0]);
	      close(pipes[1]);
	      close(pipes[2]);
	      close(pipes[3]);

	      execvp(*arg3, arg3);
	    }
	}
    }
  
  close(pipes[0]);
  close(pipes[1]);
  close(pipes[2]);
  close(pipes[3]);

  for (i = 0; i < 3; i++)
    wait(&status);
  return 1;
}


int shell_execute_pipe(char **cmd1, char **cmd2) {

    int pipeRW[2];
    pid_t p1, p2, wpid;
    int status;
    if(pipe(pipeRW) < 0) {
        puts("PIPE1 ERROR");
        exit(EXIT_FAILURE);
    }
    p1 = fork();
    if(p1 < 0) {
    
        puts("UNABLE TO FORM FIRST CHILD");
        exit(EXIT_FAILURE);
    }
    if(p1 == 0) {
        
        close(pipeRW[0]); //end 0 for reading, end 1 for writing
        dup2(pipeRW[1], STDOUT_FILENO);
        close(pipeRW[1]);

        if(execvp(cmd1[0], cmd1 )< 0) {
            puts("UNABLE TO EXECUTE COMMAND 1");
            exit(EXIT_FAILURE);
        }
    }
    else {
        p2 = fork();

        if(p2 < 0) {
        
            puts("PIPE2 ERROR");
            exit(EXIT_FAILURE);
        }
        
        if(p2 == 0) {
            close(pipeRW[1]);
            dup2(pipeRW[0], STDIN_FILENO);
            close(pipeRW[0]);
        
            if(execvp(cmd2[0], cmd2 )< 0) {
                puts("UNABLE TO EXECUTE COMMAND 2");
                exit(EXIT_FAILURE);
            }
        }
        else {
            wait(NULL);
        }

   }
   return 1;

}

int shl_launch(char **args) {
    
    pid_t pid, wpid;
    int status;
    if(strcmp(args[0], "changedir") == 0) {
        chdir(args[1]);
        puts("DIRECTORY CHANGED");
        return 1;
    }
    pid = fork();
    if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
        perror("ERROR");
        exit(0);
    }
    exit(EXIT_FAILURE);
    } 
    else if (pid < 0) {
        // Error forking
        perror("ERROR");
        exit(0);
    } 
    else {
        // Parent process
        do {
        wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}


char *builtin_str[] = {"cd", "help", "exit"};

int (*builtin_func[]) (char **) = {&shl_cd, &shl_help, &shl_exit};

int shl_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

int shl_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "expected argument to \"cd\"\n");
    } 
    else {
        if (chdir(args[1]) != 0) {
        perror("ERROR");
        }
    }
    return 1;
}

int shl_help(char **args)
{
    int i;

    for (i = 0; i < shl_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int shl_exit(char **args)
{
    return 0;
}

char *shell_readline(void)
{
    char* line;
    ssize_t bufsize = 0; // have getline allocate a buffer for us
    getline(&line, &bufsize, stdin);
    add_history(line);
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
        return 1;
    }

    for (i = 0; i < shl_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
    return shl_launch(args);
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

    char *line = malloc(sizeof(char*) * INITSIZE);
    char **args = malloc(sizeof(char*) * INITSIZE);
    char** pipedArgs = malloc(sizeof(char*) * 64);
    char** arg1 = malloc(sizeof(char*) * INITSIZE);
    char** arg2 = malloc(sizeof(char*) * INITSIZE);
    char** arg3 = malloc(sizeof(char*) * INITSIZE);
    int p;
    char **pipedCmd = malloc(sizeof(char*) * INITSIZE);
    int status = 0;
    int log = 0;
    FILE *fp;
    int f, nf;
    FILE *f1;
    int l;
        do {
        using_history(); 
        printf("ownShell> ");
        line = shell_readline();
        
        if(line[0] == 'l' && line[1] == 'o' && line[2] == 'g')  {    
            log = 1;
            fp = fopen("commands.log", "a+");
            //f1 = fopen("output.log", "w");
            fprintf(f1, "output logger started\n");
            nf = fileno(f1);
            f = dup(STDOUT_FILENO);
            dup2(nf, STDOUT_FILENO);
            puts("LOGGING");
        }

        if(log == 1) {
            fprintf(fp, "%s\n", line);
            if(l > 0)
                dup2(l, 1);

        }

        if( line[0] == 'v' && 
            line[1] == 'i' && 
            line[2] == 'e' &&
            line[3] == 'w' && 
            line[4] == 'c' && 
            line[5] == 'm' &&
            line[6] == 'd' && 
            line[7] == 'l' && 
            line[8] == 'o' &&
            line[9] == 'g' ) 
        {
            puts("commands.log");
            viewcmdlog();
            puts("output.log");
            viewoutlog();
        }

        if(line[0] == 'u' && line[1] == 'n' && line[2] == 'l' && line[3] == 'o' && line[4] == 'g') {
            log = 0;
            puts("UNLOGGED");
            viewcmdlog();
            fp = fopen("commands.log", "w");
            fclose(fp);
            dup2(f, STDOUT_FILENO);
            fflush(stdout);
            close(f);
        }

        p = shell_pipe_parsed(line, pipedArgs);
        
        if(p == 2) {
            arg1 = shell_parse(pipedArgs[0]);
            arg2 = shell_parse(pipedArgs[1]);
            arg3 = shell_parse(pipedArgs[2]);
        }
        
        if(p == 1) {
            arg1 = shell_parse(pipedArgs[0]);
            arg2 = shell_parse(pipedArgs[1]);
        }
        else
            args = shell_parse(line);
        
                
        if(p == 0)
            status = shell_execute(args);
        if(p == 1)
            status = shell_execute_pipe(arg1, arg2);
        if(p == 2) {
            status = threePipe(arg1, arg2, arg3);
        }
        if(strcmp(args[0], "exit") == 0) {
            break;
        }
         
        free(line);
        free(args);
  } while(status);
}

int main() {
    char temp[50];
    fgets(temp, sizeof(temp), stdin);
    while(temp[0] != 'e' && temp[1] != 'n' && temp[2] != 't' && temp[3] != 'r' && temp[4] != 'y' ) {
            printf("COMMAND LINE INTERPRETER NOT STARTED !!\n");
            fgets(temp, sizeof(temp), stdin);
            sleep(5);
            clear();
        }
    
    init_loop();
    while(1) {
        puts("COMMAND LINE INTERPRETER EXITED");
        sleep(5);
        clear();
    }

    return EXIT_SUCCESS;
}
