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
//-----------------------------------------------------Alias linked list
struct alias_list
{
    char alias_name[50];
    char command[128];
    struct alias_list* next;
};
struct alias_list* head=NULL;
/* Given a reference (pointer to pointer) to the head
  of a list and an int, push a new node on the front
  of the list. */
void deleteCommand(char*);
void push( char* alias_name, char* command)
{
    /* allocate node */
    deleteCommand(alias_name);
    struct alias_list* new_node =
            (struct alias_list*) malloc(sizeof(struct alias_list));

    /* put in the key  */
    strcpy(new_node->alias_name,alias_name);

    strcpy(new_node->command,command);

    /* link the old list off the new node */
    new_node->next = head;

    /* move the head to point to the new node */
    head= new_node;
}

/* Checks whether the value x is present in linked list */
int search(char** alias_name,char* command)
{
    struct alias_list* current = head;  // Initialize current
    while (current != NULL)
    {
        if (strcmp(current->alias_name,alias_name[0])==0){
            strcpy(command,current->command);
            int i;
            if(alias_name[1]!=NULL){


              for (i = 0; command[i]!='\n'; i++);
              command[i]='\0';

              for (size_t j = 1; alias_name[j]!=NULL; j++) {
                strcat(command," ");
                strcat(command,alias_name[j]);
              }

              strcat(command,"\n");
            }
            return 1;
        }
        current = current->next;
    }
    return 0;
}
void deleteCommand(char* alias_name)
{
    // Store head node
    struct alias_list* temp = head, *prev;

    // If head node itself holds the key to be deleted
    if (temp != NULL && strcmp(temp->alias_name,alias_name)==0 )
    {
        head = temp->next;   // Changed head
        free(temp);               // free old head
        return;
    }

    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && strcmp(temp->alias_name,alias_name)!=0 )
    {
        prev = temp;
        temp = temp->next;
    }

    // If key was not present in linked list
    if (temp == NULL) return;

    // Unlink the node from linked list
    prev->next = temp->next;

    free(temp);  // Free memory
}
void print_alias_list(){
  struct alias_list* temp_head=head;
  while(temp_head!=NULL){
    printf("Alias name: %s  ---  CommandName: %s \n",temp_head->alias_name,temp_head->command);
    temp_head=temp_head->next;
  }
}
void alias_list(char *input){
  char command[128];
  char alias_name[50];
  int cond=0;
  int j=0;
  int i;

    for (i = 0; i<strlen(input); i++) {
      if(input[i]=='"'){
        cond++;
        continue;
      }
      if(cond==1){
        command[j]=input[i];
        j++;
      }
      if(cond==2){
        command[j]='\n'; // we put this because we will send these string into setup function for getting arguments array.
        command[j+1]='\0';
        break;
      }
    }
    if(strlen(command)==0){
      perror("Invalid input. Please enter command inside double quotes.");
      return;
    }
    int test=1;
    for (size_t i = 0; i <strlen(command); i++) {
      if(command[i]==' ')
        test++;
    }
    if(test==strlen(command)){
      perror("Invalid input. Please enter command inside double quotess");
      return;
    }

    if(cond!=2){
      perror("Invalid input. There must be only 2 double quotes" );
      return;
    }
    j=0;
    cond=0;
    for (i = 0; i<strlen(input)-1; i++) {
      if(input[i]=='"'){
        cond++;
        continue;
      }
      if(input[i]=='\n')
        break;
      if(cond>=2){
        if(input[i]==' '&&cond==2){
          cond++;
          continue;
        }else if(cond==2){
          perror("You must put white spaces after this \" sign");
          return;
        }

        alias_name[j]=input[i];
        j++;
      }
    }
    alias_name[j]='\0';

    //printf("%s---------------%s\n",alias_name,command );
    if(strcmp(alias_name,"alias")==0||strcmp(alias_name,"unalias")==0||strlen(alias_name)==0||strstr(alias_name," ")!=NULL){ //ERROR checks of alias name
      perror("please enter valid alias name");
      return;
    }
    if(strcmp(command,"alias\n")==0||strcmp(command,"unalias\n")==0){ //ERROR checks of alias name
      perror("please enter valid command name");
      return;
    }

  push(alias_name,command);
}
void check_file_signs(char *args[]){
  cond_input=0;
  cond_output=0;
  for (size_t i = 0; args[i]!=0; i++) {
    if(strstr(args[i],"<")!=NULL)
      cond_input=1;
    if(strstr(args[i],">")!=NULL)
      cond_output=1;
  }
}
void checkPaths(char *arg,char location[128]){

  const char* paths =getenv("PATH");
  int j=0;

  for (int i = 0;i<strlen(paths); i++) {

    if(paths[i]==':'){
      strcat(location,"/");
      strcat(location,arg);


      if( access( location, F_OK ) != -1 ) {

        return;
      }
      j=0;
      strcpy(location,"");

    }else {
      location[j]=paths[i];
      j++;
      location[j]='\0';

    }
  }
  strcpy(location,"");
  return;
}
int wait_childpid(pid_t childpid,int background){

  if(background==0){
    deletePid(&pid_head,childpid);
    push_pid(&pid_head_fg,childpid);
    if(waitpid(childpid,NULL,0)>0){

      deletePid(&pid_head_fg,childpid);
    }
    return 1;
  }
  else{
    deletePid(&pid_head_fg,childpid);
    push_pid(&pid_head,childpid);
    if(waitpid(childpid,NULL,WNOHANG)>0){
      printf("background process is finished and it's id is=%d\n",childpid);
      deletePid(&pid_head,childpid);
    }
    return 0;
  }

}
int wait_background_childs(struct childpid_list **head,int background){
  struct childpid_list* temp=*head;
  int count=0;
  while(temp!=NULL){
    wait_childpid(temp->childpid,background);
    temp=temp->next;
    count++;
  }
  return count;

}
int set_io_files(char* input_file,char* output_file,int file_cond){
  int fd=0;

  //printf("---------%s\n",input_file);
  //printf("---------%s\n",output_file);

  if(strcmp(input_file,"")!=0 ){

    fd = open(input_file, CREATE_FLAGS_READ, CREATE_MODE);

    if (fd == -1) {
      perror("Failed to open my.file");
      exit(EXIT_FAILURE);
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
      perror("Failed to redirect standard output");
      return 1;
    }

    if (close(fd) == -1) {
      perror("Failed to close the file");
      return 1;
    }

  }
  if(strcmp(output_file,"")!=0){
    //printf("----------------------%d\n",file_cond );
    if(file_cond==0||file_cond==2)
      fd = open(output_file, CREATE_FLAGS_TRUNC, CREATE_MODE);
    else
      fd = open(output_file, CREATE_FLAGS_APPEND, CREATE_MODE);

    if (fd == -1) {
      perror("Failed to open my.filee");
      exit(EXIT_FAILURE);
    }
    if(file_cond==2){
      if (dup2(fd, STDERR_FILENO) == -1) {
        perror("Failed to redirect standard output");
        return 1;
      }
    }else{
      if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("Failed to redirect standard output");
        return 1;
      }
    }

    if (close(fd) == -1) {
      perror("Failed to close the file");
      return 1;
    }
  }
}
void set_background(char *args[],int *background){
  int i;
  for (i = 0; args[i]!=NULL; i++) {

  }
  if(strcmp(args[i-1],"&")==0){
    *background=1;
    args[i-1]=NULL;
  }else{
    *background=0;
  }
  return;
}

