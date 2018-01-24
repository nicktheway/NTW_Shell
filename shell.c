/**
 * File: shell.c
 * Description:
 *  A simple unix shell made for the Operating Systems class'
 *  project, 7th semester, ECE AUTH.
 * 
 * Compile command example:
 *  gcc -O3 shell.c -o shell
 * or
 *  gcc -O3 shell.c -o shell -D __FORCE_EXIT_ON_ERROR
 * 
 * Run command example:
 *  ./shell (for the interactive mode)
 * or
 *  ./shell <file_with_commands> (for the batch mode)
 * 
 * Author:
 *  Nikolaos Katomeris, AEM 8551, ngkatomer@auth.gr
 * 
 * Date:
 *  ~January 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 512
#define MAX_ARG_SIZE 420
#define MAX_ARGS 10
#define MAX_COMMANDS 10

char whitespace[] = " \t\r\n\v";
const char delim_dand[] = "&&";
const char delim_semicolon[] = ";";

/**
 * Enumerator for the various delimiters
 */ 
enum E_delimiter_type{
  EDT_none,
  EDT_double_and, // &&
  EDT_semicolon   // ;
};

/**
 * Special types 
 * (special commands are executed by the main-parent process)
 */  
enum E_special_type{
  EST_none, // Not a special type (executed on a child process)
  EST_QUIT, // quit command for terminating the program
  EST_CD    // cd command
};

/**
 * The type <struct command> includes:
 * -String array for the command arguments.
 * -Two E_delimiter_type enums for the previous and the next delimiters.
 * -An E_special_type enum for the command's special type.
 * -An int for storing the number of the command's arguments.
 */ 
struct command{
  char (*args)[MAX_ARG_SIZE];
  enum E_delimiter_type p_del;
  enum E_delimiter_type n_del;
  enum E_special_type special; // Special commands are executed by the parent.
  int arg_num;
};

int read_line(char *buf, int max_line_size, FILE* fd);
int my_fork();
int is_whitespace(const char* s);
void parseBuffer(const char* buffer, struct command** commands, int* command_num);
enum E_special_type special_type(char *s);


int main(int argc, char** argv)
{
  //Buffer for the input.
  static char buffer[BUFFER_SIZE];
  //Number of commands
  int command_num;
  //The commands
  struct command* commands;

  int i, empty_file = 1;
  FILE* fd;

  if (argc != 1 && argc != 2){
    printf("Usage: %s <optional_script_file>\n", argv[0]);
    return 1;
  }
  if (argc == 2){
    fd = fopen(argv[1], "a+");
    if (fd == NULL){
      fprintf(stderr, "Couldn't open file: %s\nEntering Interactive Mode...\n", argv[1]);
      fd = stdin;
    }
  }
  else fd = stdin;

  commands = (struct command*) malloc(MAX_COMMANDS*sizeof(struct command));
  for (i = 0; i < MAX_COMMANDS; i++){
    commands[i].args = malloc(MAX_ARGS*sizeof(char[MAX_ARG_SIZE]));
  }

  while (read_line(buffer, BUFFER_SIZE, fd) >= 0){
    int i, j, status;
    
    //Extract the commands from the buffer
    parseBuffer(buffer, &commands, &command_num);
    
    // Iterate through all the commands in the line
    for (i = 0; i < command_num; i++){
      empty_file = 0;
      //DEBUG:
      //printf("Command %d with p_del: %d\tn_del: %d and type: %d\n", i, commands[i].p_del, commands[i].n_del, commands[i].special);

      // Check the command type (if it's a special type it will be executed by the parent process (no child))
      // Supported special commands are: CD and QUIT
      // QUIT should be called with no extra arguments!
      // CD needs one argument (the destination folder)
      if (commands[i].special == EST_QUIT && commands[i].arg_num == 1){
        goto exit_label;
      }
      else if (commands[i].special == EST_CD && commands[i].arg_num == 2){
        if (chdir(commands[i].args[1]) < 0){
          fprintf(stderr, "Cannot cd to %s\n", commands[i].args[1]);
          status = 1;
        }
      }
      else if (commands[i].special == EST_none){
        // Pointer for execvp arguments.
        char **arg_ptr = (char**)malloc((commands[i].arg_num+1)*sizeof(char*));
        for (j = 0; j < commands[i].arg_num; j++){
          arg_ptr[j] = commands[i].args[j];
        }
        // NULL after the last argument, needed by execvp to work
        arg_ptr[commands[i].arg_num] = NULL;
        
        // Make a child process to execute the command
        if (my_fork() == 0){
          // Execute the command
          int ret = execvp(arg_ptr[0], arg_ptr);
          
          // Free any heap allocated memory
          for (i = 0; i < MAX_COMMANDS; i++)
            free(commands[i].args);
          free(commands);
          free(arg_ptr);

          // Exit successfully or not
          if(ret == -1) exit(-1);
          exit(0);
        }
        free(arg_ptr);
        // Wait for the child to finish and get its status
        wait(&status);
      }
      // CD/QUIT had wrong arguments, so they failed
      else status = -1;
      
      // If status isn't zero the command failed to execute
      // Then providing there is a next command, it should not execute if p_del == &&.
      // And the next -of the next- shouldn't execute as well if p_dell==&& etc.
      // In the __FORCE_EXIT_ON_ERROR or batch mode the program exits.
      if (status){
        fprintf(stderr, "Command: \"%s\" failed to run\n", commands[i].args[0]);

        // If __FORCE_EXTI_ON_ERROR is defined:
        //  In case there isn't a ";" after the failed command, exit
        int exit_flag = 1;
        for (int ff = command_num-1; ff >= i; ff--){
          if (commands[ff].n_del == EDT_semicolon){
            exit_flag = 0;
            break;
          }
        }
        #ifdef __FORCE_EXIT_ON_ERROR
        if(exit_flag) exit(1);
        if (argc == 2) exit(1);
        #endif

        if (argc == 2 && exit_flag) goto exit_label;
        while (i < command_num+1 && commands[i+1].p_del == EDT_double_and)
          i++;
      }
    }
  }
  if (empty_file && argc == 2)
     fprintf(stderr, "The specified file is empty.\n");

  exit_label:
  fclose(fd);
  for (i = 0; i < MAX_COMMANDS; i++)
    free(commands[i].args);
  free(commands);

  return 0;
}

/**
 * Reads line from the fd (stdin or file)
 */ 
int read_line(char *buf, int max_line_size, FILE* fd)
{
  if (isatty(fileno(fd)) && fd == stdin)
    fprintf(stdout, "\e[1;36mKatomeris_8551_$ \e[0m");
  if(fgets(buf, max_line_size, fd)==NULL)
    return -1;
  if (buf[0] == 0)
    return -1;
  
  return 0;
}

// Just an error checking fork.
int my_fork()
{
  int pid;
  
  pid = fork();
  if (pid == -1)
    perror("fork");
  return pid;
}

/**
 * Checks if the '\0' pointed by <s> string has only whitespace characters.
 */
int is_whitespace(const char* s){
  int i = 0;
  while(s[i] != '\0'){
    if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r' && s[i] != '\v')
      return 0;
    i++;
  }
  return 1;
}

/**
 * Extracts the commands from a string buffer.
 */ 
void parseBuffer(const char* buffer, struct command** commands, int* command_num)
{
  int command_number = 0;
  enum E_delimiter_type temp_del = EDT_none;
  const char* sub1, *sub2;
  const char* subbuffer = buffer;

  char command[BUFFER_SIZE + 1];
  (*commands)[0].p_del = EDT_none;
  while(1){
    if (command_number >= MAX_COMMANDS){
      printf("Too many commands.\nCurrently defined MAX number of commads per line is: %d\n", MAX_COMMANDS);
      break;
    }
    strcpy(command, subbuffer);
    
    sub1 = strstr(subbuffer, delim_dand);
    sub2 = strstr(subbuffer, delim_semicolon);
    
    if (sub1 != NULL && sub2 != NULL){
      command[(sub1 > sub2) ? (int)(sub2-subbuffer) : (int)(sub1-subbuffer)] = ' ';
      command[(sub1 > sub2) ? (int)(sub2-subbuffer) + 1 : (int)(sub1-subbuffer)+ 1] = '\0';
      subbuffer = (sub1 > sub2) ? sub2 + strlen(delim_semicolon): sub1 + strlen(delim_dand);
      (*commands)[command_number].n_del = (sub1 > sub2) ? EDT_semicolon : EDT_double_and;
    }
    else if(sub1 != NULL && sub2 == NULL){
      command[(int) (sub1 - subbuffer)] = ' ';
      command[(int) (sub1 - subbuffer)+1] = '\0';
      subbuffer = sub1 + strlen(delim_dand);
      (*commands)[command_number].n_del = EDT_double_and;
    }
    else if(sub1 == NULL && sub2 != NULL){
      command[(int) (sub2 - subbuffer)] = ' ';
      command[(int) (sub2 - subbuffer) + 1] = '\0';
      subbuffer = sub2 + strlen(delim_semicolon);
      (*commands)[command_number].n_del = EDT_semicolon;
    }
    //printf("com1 = %s\n", command);
    if (command_number > 0){
      (*commands)[command_number].p_del = temp_del;
    }
    
    // Ignore empty commands
    if(is_whitespace(command)){
      temp_del = (*commands)[command_number].n_del;
      if (*subbuffer == '\n') break;
      subbuffer = subbuffer + 1;
      continue;
    }
    // For the next command's p_del.
    temp_del = (*commands)[command_number].n_del;

    //---GET ARGUMENTS---
    char * subcom = command;
    char arg[MAX_ARG_SIZE];
    int argc = 0;
    while(1){
      if (argc >= MAX_ARGS){
        printf("One command has too many arguments.\nCurrently defined MAX number of arguments is: %d\n", MAX_ARGS);
        break;
      }
      strcpy(arg, subcom);
      char* arg_end = strpbrk(subcom, whitespace);
      if (arg_end == NULL) {
        break;
      }
      // Ignore whitespace at start
      if (subcom == arg_end) {
        subcom = subcom + 1;
        continue;
      }
      
      arg[(int)(arg_end-subcom)] = '\0';
      subcom = arg_end;

      if (argc == 0){
        (*commands)[command_number].special = special_type(arg);
      }

      strcpy((*commands)[command_number].args[argc], arg);

      argc++;
      (*commands)[command_number].arg_num = argc;
    }

    command_number++;

    if (sub1 == NULL && sub2 == NULL) {
      (*commands)[command_number - 1].n_del = EDT_none;
      break;
    }
  }
  *command_num = command_number;
}

/**
 * Checks if a command is "special" (only 'cd' and 'quit' are supported).
 * Special commands are the commands that should be executed by the parent(main loop) process.
 */ 
enum E_special_type special_type(char *s)
{
  if (strlen(s) < 4){}
  else if (s[0] == 'q' && s[1] == 'u' && s[2] == 'i' && s[3] == 't')
  {
    return EST_QUIT;
  }

  if (strlen(s) < 2) return 0;
  else if (s[0] == 'c' && s[1] == 'd')
  {
    return EST_CD;
  }
  return EST_none;
}
