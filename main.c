//
//  main.c
//  lutctrl
//
//  Created by Ed Cole on 11/05/2018.
//  Copyright © 2018 Ed Cole. All rights reserved.
//

#ifndef DEBUG
#define DEBUG 0 // dont change this - pass it via -DDEBUG=1 at compile
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "unp.h"

#define MYNAME "lutctrl"
#define VERSION "2.0.1" // added -c option

#define D_SERVER_NAME "192.168.1.246"
#define D_PORT 4534
#define SOC_BUFF_SZ 1024

char *myname = MYNAME;
char *version = VERSION;

// Default server
static char d_server_name[] = D_SERVER_NAME;
static int port_no = D_PORT;
char *server_name=NULL;

// Commands file
char *cmds_filename = NULL;
static FILE *cmds_file;
struct cmd {  char *text;        // linked list of commands
    struct cmd *next;
};
struct cmd *cmds = NULL;
struct cmd *cmdn = NULL;     // next or current list pointer
int linelength = 256;           // for parsing command file
int debug = DEBUG;


void Writen(int fd, void *ptr, size_t nbytes);
ssize_t Readline(int fd, void *ptr, size_t maxlen);

//************   errorUsage
void
errorUsage()
{
    printf("USAGE: %s [-v] [-s server] [-p port] [-c COMMAND] <commands_file>\n",myname);
    printf("    -v           - Prints version number and exits\n");
    printf("    -c COMMAND   - COMMAND:= quoted Lutron command e.g. \"#OUTPUT,67,1,0.00\"\n");
    printf("  If no commands supplied then they are read on stdin\n");
    
}//errorUsage

//************   error
void
error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}//error

/************
 main
 ************/
int
main(int argc, char *argv[])
{
    int i,n, sockfd, port;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int opt;
    char buffer[SOC_BUFF_SZ];
    
    while ((opt = getopt(argc, argv, "vp:s:f:c:")) != -1){
        switch (opt){
                
            case 'c': // Lutron Command on command line
                cmds=malloc(sizeof(struct cmd));
                cmds->next = NULL;
                cmds->text=malloc(sizeof(optarg));
                strcpy(cmds->text,optarg);
                printf(">>: %s\n",cmds->text);
                break;
                
            case 's': // Alternate server
                server_name=malloc(sizeof(optarg));
                strcpy(server_name,optarg);
                break;
                
            case 'p': // Alternate listening port
                port_no = atoi(optarg);
                break;
                
            case 'v':
                printf("%s: Version %s\n",myname,version);
                exit(EXIT_SUCCESS);
                break;
                
            default:
                errorUsage();
                exit(EXIT_FAILURE);
                
        }//switch
    }//while
    
    if (optind == argc){ // no filename suplied
        // cmds_filename is initiated to NULL above
        // so no need to reasign here
    }else{
        cmds_filename = argv[optind];
    }
    
    
    if (server_name==NULL) server_name=d_server_name;
    
    if( cmds_filename != NULL){
        if ( (cmds_file = fopen(cmds_filename, "r")) == NULL ){
            printf("Unable to open commands file %s\n",cmds_filename);
            errorUsage();
            exit(EXIT_FAILURE);
        }//if
    }else{
        cmds_file = stdin;
    }
    
    // printf("Commands in %s\n",cmds_filename);
    if(cmds == NULL){ // no commands yet, so look in file or stdin
        char *line = (char *)malloc(sizeof(char) * linelength ); // raw line
        char *cline = (char *)malloc(sizeof(char) * linelength ); // clean line
        
        if ((line == NULL) || (cline==NULL)) {
            printf("Error allocating memory for line buffers.");
            return(-1);
        }
        
        while (fgets(line,linelength,cmds_file) != NULL){
            n=0;
            i=0;
            while( line[n] != '\n'){
                if (line[n]==';'){ // truncate on ';' char
                    line[n]='\n';
                }else{
                    if(isspace(line[n])==0){ // skip whitespace
                        cline[i++]=line[n];
                    }//if isspace
                    n++;
                }//else
            }
            cline[i]='\0'; // terminate string with NULL
            if(cline[0]!='\0'){ // we have a command so add to list
                if (cmds == NULL){ //fist command
                    cmds=malloc(sizeof(struct cmd));
                    cmds->text = malloc(strlen(cline)+1);
                    memcpy(cmds->text,cline,strlen(cline)+1);
                    cmds->next=NULL;
                    cmdn=cmds;
                }else{ //add next command to list
                    cmdn->next=malloc(sizeof(struct cmd));
                    cmdn=cmdn->next;
                    cmdn->text = malloc(strlen(cline)+1);
                    memcpy(cmdn->text,cline,strlen(cline)+1);
                    cmdn->next=NULL;
                }//else
            }//if - cline
        }//while fgets
        fclose(cmds_file);
        // printf("Commands read\n");
    }// if (cmds == null)
    
    // now lets connect to server and send commands
    server = gethostbyname(server_name);
    port = port_no;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR creating socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(  (char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(port);
    // printf("connecting to %s:%i\n",server_name,port_no);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        error("ERROR connecting");
    }
    n=1;
    while (cmds != NULL){ // there are some commands
        Writen(sockfd,cmds->text,strlen(cmds->text));
        Writen(sockfd,"\n",1);
        cmds=cmds->next;
        // wait for ack of some sort
        Readline(sockfd,buffer,SOC_BUFF_SZ);
    }//while cmds
    //  Readline(sockfd,buffer,SOC_BUFF_SZ);
    close(sockfd);
    exit(0);
}//main

