#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "const.h"


/* format: ./client remoteport remortip cmd (cmd2)*/
int main(argc, argv)
int argc;
char *argv[];
{
  struct hostent *hostp;
  //  struct servent *servp;
  struct sockaddr_in server;
  int sock;
  static struct timeval timeout = { 5, 0 }; /* five seconds */
  fd_set rmask, /*xmask,*/ mask;
  char bufread[BUFSIZ];
  char bufwrite[BUFSIZ];

  char localaddr[256];

  int nfound, bytesread;

  char *tok;

  printf("peer: fork successed.\n");

  if((argc !=4) && (argc != 3)) {
    (void) fprintf(stderr,"usage: %s host cmd (cmd2)\n",
		   argv[0]);
    exit(1);
  }

  
  if(argc == 4){
    printf("in child process exec: <%s> <%s> <%s><%s>\n", 	 
	   argv[0], argv[1], argv[2], 
	   argv[3]);

  }else {
    printf("in child process exec: <%s> <%s> <%s>\n",
	   argv[0], argv[1], argv[2]);
  }
  
  if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket");
    exit(1);
  }

  if ((hostp = gethostbyname(argv[1])) == 0) {
    fprintf(stderr,"%s: unknown host\n",argv[1]);
    exit(1);
  }
  memset((void *) &server, 0, sizeof server);
  server.sin_family = AF_INET;
  memcpy((void *) &server.sin_addr, hostp->h_addr, hostp->h_length);
  server.sin_port = htons(P2PSERV);
  if (connect(sock, (struct sockaddr *)&server, sizeof server) < 0) {
    (void) close(sock);
    perror("connect");
    exit(1);
  }

  printf("Client connected\n");

  FD_ZERO(&mask);
  FD_SET(sock, &mask);
  /* this messenger will not listen to keyboard input*/
  //  FD_SET(fileno(stdin), &mask); 

  for(;;) {
    rmask = mask;
    nfound = select(FD_SETSIZE, &rmask, (fd_set *)0, (fd_set *)0, &timeout);
    if (nfound < 0) {
      if (errno == EINTR) {
	printf("interrupted system call\n");
	continue;
      }
      /* something is very wrong! */
      perror("select");
      exit(1);
    }
    if (nfound == 0) {
      /* timer expired */
      //      printf("Please type something!\n");
      continue;
    }
    if (FD_ISSET(sock,&rmask)) {
      /* data from network */
      bytesread = read(sock, bufread, sizeof(bufread));
      bufread[bytesread] = '\0';
      printf("%s: got %d bytes (%d, %d): %s\n", argv[0], bytesread, (int)sizeof(bufread), (int)strlen(bufread), bufread);
      if(!strncmp(bufread,"close", 5)) /*if the server tear the conn */
	break;

      if((tok = strtok(bufread, DELIMITER))){
	if(!strcmp(tok, "ip")){ /* msg format: "ip xxx.xxx.xxx.xxx" */
	  if ((tok = strtok(NULL, DELIMITER))){
	    sprintf(localaddr, "%s", tok);
		    
	    bzero(bufwrite, sizeof(bufwrite));
	    if(!strcmp(argv[2], "reg")){/*reg myport*/
	      /* msg format: "reg xxx.xxx.xxx.xxx" */
	      sprintf(bufwrite, "reg%s%s", DELIMITER, localaddr);
	    }else if(!strcmp(argv[2], "get")){/* get filename */
	      /* msg format: "get xxx.xxx.xxx.xxx filename "*/
	      sprintf(bufwrite, "get%s%s%s%s",
		      DELIMITER, localaddr,
		      DELIMITER, argv[3]);
	    }else if(!strcmp(argv[2], "fwd")){
	      strcpy(bufwrite, "get");
	      strcat(bufwrite, &argv[2][3]); /*replace "fwd" with "get" */
	    }else if(!strcmp(argv[2], "push")){
	      sprintf(bufwrite, "push%s%s%s%s",
		      DELIMITER, localaddr,
		      DELIMITER, argv[3]);
	    }else if(!strcmp(argv[2], "quit")){
	      if(argc == 4){
		sprintf(bufwrite, "quit%s%s%s%s",
			DELIMITER, localaddr,
			DELIMITER, argv[3]);
	      }else{
		sprintf(bufwrite, "quit%s%s", DELIMITER, localaddr);
	      }
	    }else{/* unrecognized arg[3] msg */
	      (void) fprintf(stderr,"invalid arg[3] format <%s>\n",
			     argv[2]);
	      exit(1);
	    }

	    printf("sending msg to <%s>: <%s>...\n", 
		   argv[1], bufwrite);

	    if(write(sock, bufwrite, strlen(bufwrite)) != 
		 strlen(bufwrite)){
		perror("reg");
		exit(-1);
	    }

	    printf("sent msg to <%s>: <%s>\n", 
		   argv[1], bufwrite);
	  }
	}else{
	  ; /*ignore the message comming other than "ip XX.XXX.XXX.XXX"*/
	}
      }
    }
  }
  return 0;
} /* main - client.c */
