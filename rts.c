#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include<readline/readline.h> 
#include<readline/history.h>  
#include <fcntl.h>

#define MAXCOM 100000 // max number of letters to be supported 
#define MAXLIST 10000 // max number of commands to be supported 

#define BUFFERSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
#define INITSIZE 256
// Clearing the shell using escape sequences 
#define clear() printf("\033[H\033[J")
static int log = 0;
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

int threePipe(char **cat_args, char **grep_args, char **cut_args) {
  int i, status;
  //char *cat_args[] = {"ls", "-l", NULL};
  //char *grep_args[] = {"grep", "evidence", NULL};
  //char *cut_args[] = {"more", NULL, NULL , NULL};

  // make 2 pipes (cat to grep and grep to cut); each has 2 fds

  int pipes[4];
  pipe(pipes); // sets up 1st pipe
  pipe(pipes + 2); // sets up 2nd pipe

  // we now have 4 fds:
  // pipes[0] = read end of cat->grep pipe (read by grep)
  // pipes[1] = write end of cat->grep pipe (written by cat)
  // pipes[2] = read end of grep->cut pipe (read by cut)
  // pipes[3] = write end of grep->cut pipe (written by grep)

  // Note that the code in each if is basically identical, so you
  // could set up a loop to handle it.  The differences are in the
  // indicies into pipes used for the dup2 system call
  // and that the 1st and last only deal with the end of one pipe.

  // fork the first child (to execute cat)
  
  if (fork() == 0)
    {
      // replace cat's stdout with write part of 1st pipe

      dup2(pipes[1], 1);

      // close all pipes (very important!); end we're using was safely copied

      close(pipes[0]);
      close(pipes[1]);
      close(pipes[2]);
      close(pipes[3]);

      execvp(*cat_args, cat_args);
    }
  else
    {
      // fork second child (to execute grep)

      if (fork() == 0)
	{
	  // replace grep's stdin with read end of 1st pipe
	  
	  dup2(pipes[0], 0);

	  // replace grep's stdout with write end of 2nd pipe

	  dup2(pipes[3], 1);

	  // close all ends of pipes

	  close(pipes[0]);
	  close(pipes[1]);
	  close(pipes[2]);
	  close(pipes[3]);

	  execvp(*grep_args, grep_args);
	}
      else
	{
	  // fork third child (to execute cut)

	  if (fork() == 0)
	    {
	      // replace cut's stdin with input read of 2nd pipe

	      dup2(pipes[2], 0);

	      // close all ends of pipes

	      close(pipes[0]);
	      close(pipes[1]);
	      close(pipes[2]);
	      close(pipes[3]);

	      execvp(*cut_args, cut_args);
	    }
	}
    }
      
  // only the parent gets here and waits for 3 children to finish
  
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

int shell_execute_pipes(char **cmd1, char **cmd2, char **cmd3) {
    int pipe1[2];
    int pipe2[2];
    pid_t p1, p2, p3;
    if(pipe(pipe1) < 0) {
        puts("PIPE1 ERROR");
        exit(EXIT_FAILURE);
    }
    p1 = fork();
    if(p1 < 0) {
        puts("UNABLE TO CREATE CHILD 1");
        exit(EXIT_FAILURE);
    }
    if(p1 == 0) {
        dup2(pipe1[1], 1);
        close(pipe1[0]);
        close(pipe1[1]);

        if(execv(cmd1[0], cmd1) < 0) {
            puts("UNABLE TO EXECUTE COMMAND 1");
            exit(EXIT_FAILURE);
        }
    }
    else {
        if(pipe(pipe2) < 0) {
            puts("PIPE2 ERROR");
            exit(EXIT_FAILURE);
        }
        p2 = fork();
        if(p2 < 0) {
            puts("UNABLE TO CREATE CHILD 2");
            exit(EXIT_FAILURE);
        }
        if(p2 == 0) {
            dup2(pipe1[0], 0);
            dup2(pipe2[1], 1);
            close(pipe1[0]);
            close(pipe1[1]);
            close(pipe2[0]);
            close(pipe2[1]);

            if(execv(cmd2[0], cmd2) < 0) {
                puts("UNABLE TO EXECUTE COMMAND 2");
            }

        }
        else {
            close(pipe1[0]);
            close(pipe1[1]);
            
            p3 = fork();
            if(p3 < 0) {
                puts("UNABLE TO CREATE CHILD 3");
                exit(EXIT_FAILURE);
            }
            if(p3 == 0) {
                dup2(pipe2[0], 0);
                close(pipe2[0]);
                close(pipe2[1]);

                if(execvp(cmd3[0], cmd3) < 0) {
                    puts("UNABLE TO EXECUTE COMMAND 3");
                    exit(EXIT_FAILURE);    
                }
            }
            else {
               wait(NULL);
            }
        }
    }
    return 1;
}

int temp(char **cmd1, char **cmd2, char **cmd3) {

    int fd[2];             // sort <===> head
    int fd1[2];            //  du  <===> sort

    pipe(fd);

    switch (fork()) {
        case 0:            // Are we in sort?
             pipe(fd1);    // If yes, let's make a new pipe!

             switch (fork()) {
                 case 0:   // Are we in du?
                     dup2(fd1[1], STDOUT_FILENO);
                     close(fd1[0]);
                     close(fd1[1]);
                     //execl("/usr/bin/du", "du", (whatever directory), NULL);
                     execvp(cmd1[0], cmd1);
                     //exit(1);

                 default:
                     /* If not in du, we're in sort! in the middle!
                        Let's set up both input and output properly.
                        We have to deal with both pipes */
                     dup2(fd1[0], STDIN_FILENO);
                     dup2(fd[1], STDOUT_FILENO);
                     close(fd1[0]);
                     close(fd1[1]);
                     //execl("/usr/bin/sort", "sort (flags if needed)", (char *) 0);
                     execvp(cmd2[0], cmd2);
                     //exit(2);
             }

            //exit(3);

        default:            // If we're not in sort, we're in head
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            close(fd[1]);
            //execl("/usr/bin/head", "head (flags if needed)", (char *) 0);
            execvp(cmd3[0], cmd3);
            wait(NULL);
            //exit(4);

    }   
    return 1;
}

int lsh_launch(char **args) {
    
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
    }
    exit(EXIT_FAILURE);
    } 
    else if (pid < 0) {
        // Error forking
        perror("ERROR");
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
char *builtin_str[] = {"cd", "help", "exit"};

int (*builtin_func[]) (char **) = {&lsh_cd, &lsh_help, &lsh_exit};

int lsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int lsh_cd(char **args)
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

int lsh_help(char **args)
{
    int i;

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
        do {
        using_history(); 
        printf("ownShell> ");
        line = shell_readline();
        
        if(line[0] == 'l' && line[1] == 'o' && line[2] == 'g')  {    
            puts("LOGGING");
            fp = fopen("commands.log", "a+");
            log = 1;
        }
        if(log == 1)
            fprintf(fp, "%s\n", line);
        
        if(line[0] == 'u' && line[1] == 'n' && line[2] == 'l' && line[3] == 'o' && line[4] == 'g') {
            fclose(fp);
            log = 0;
            puts("UNLOGGED");
            char filename[100], c;

            fp = fopen("commands.log", "r");
            if (fp == NULL)
            {
                printf("Cannot open file \n");
                break;
            }

            c = fgetc(fp);
            while (c != EOF)
            {
                printf ("%c", c);
                c = fgetc(fp);
            }

            fp = fopen("commands.log", "wb");
            fclose(fp);
        }
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
            if(p == 2) {
                //status = temp(arg1, arg2, arg3);//status = shell_execute_pipes(arg1, arg2, arg3);
                status = threePipe(arg1, arg2, arg3);
            }
            if(strcmp(args[0], "exit") == 0) {
                while(1) {
                    puts("COMMAND LINE INTERPRETER EXITED");
                    sleep(5);
                    clear();
                }
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

    return EXIT_SUCCESS;
}
