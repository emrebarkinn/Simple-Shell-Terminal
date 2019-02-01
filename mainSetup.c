#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define CREATE_FLAGS_APPEND (O_WRONLY | O_CREAT | O_APPEND)
#define CREATE_FLAGS_TRUNC (O_WRONLY | O_CREAT | O_TRUNC )
#define CREATE_FLAGS_READ (O_RDONLY)
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */
char user_input[MAX_LINE];
int cond_input=0;
int cond_output=0;
int pipe_count=0;

void setup(char inputBuffer[], char *args[],int *background,int cond,char* input)
{
  int length, /* # of characters in the command line */
  i,      /* loop index for accessing inputBuffer array */
  start,  /* index where beginning of next command parameter is */
  ct;     /* index of where to place the next parameter into args[] */

  ct = 0;

  /* read what the user enters on the command line */
  if(cond){
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);
    strcpy(user_input,inputBuffer);
  }
  else{
    length=strlen(input);
    strcpy(inputBuffer,input);
  }

  start = -1;
  if (length == 0)
  exit(0);            /* ^d was entered, end of user command stream */

  /* the signal interrupted the read system call */
  /* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
  if ( (length < 0) && (errno != EINTR) ) {
    perror("error reading the command");
    exit(-1);           /* terminate with error code of -1 */
  }

  //printf(">>%s<<",inputBuffer);
  for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

    switch (inputBuffer[i]){
      case ' ':
      case '\t' :               /* argument separators */
      if(start != -1){
        args[ct] = &inputBuffer[start];    /* set up pointer */
        ct++;
      }
      inputBuffer[i] = '\0'; /* add a null char; make a C string */
      start = -1;
      break;

      case '\n':                 /* should be the final char examined */
      if (start != -1){
        args[ct] = &inputBuffer[start];
        ct++;
      }
      inputBuffer[i] = '\0';
      args[ct] = NULL; /* no more arguments to this command */
      break;

      default :             /* some other character */
      if(inputBuffer[i]=='<')
        cond_input=1; //means input
      if(inputBuffer[i]=='>')
        cond_output=1; //means output
      if(inputBuffer[i]=='|')
        pipe_count++;

      if (start == -1)
      start = i;
      if (inputBuffer[i] == '&'){
        *background  = 1;
        inputBuffer[i-1] = '\0';
      }
    } /* end of switch */
  }    /* end of for */
  args[ct] = NULL; /* just in case the input line was > 80 */

  //for (i = 0; i <= ct; i++)
  //printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */
struct childpid_list
{
    pid_t childpid;
    struct childpid_list *next;
};
struct childpid_list* pid_head=NULL;
struct childpid_list* pid_head_fg=NULL;

int deletePid(struct childpid_list** ,pid_t);

void push_pid(struct childpid_list** head_ref, pid_t new_childpid)
{
    deletePid(head_ref,new_childpid);
    struct childpid_list* new_node = (struct childpid_list*) malloc(sizeof(struct childpid_list));
    new_node->childpid  = new_childpid;
    new_node->next = (*head_ref);
    (*head_ref)    = new_node;
}
int deletePid(struct childpid_list **head_ref,pid_t childpid)
{
    // Store head node
    struct childpid_list* temp = *head_ref, *prev;

    // If head node itself holds the key to be deleted
    if (temp != NULL && temp->childpid == childpid)
    {
        *head_ref = temp->next;   // Changed head
        free(temp);               // free old head
        return 1;
    }

    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && temp->childpid != childpid)
    {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) return 0;

    // Unlink the node from linked list
    prev->next = temp->next;

    free(temp);  // Free memory
    return 1;
}
void print_pid_list(struct childpid_list** head_ref){
  struct childpid_list* temp_head=*head_ref;
  while(temp_head!=NULL){
    printf("processes : %d \n",temp_head->childpid);
    temp_head=temp_head->next;
  }
}
