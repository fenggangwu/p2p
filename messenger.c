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


/* format: ./client remoteport remortip cmd cmd2 (cmd3)*/
int main(argc, argv)
int argc;
char *argv[];
{
  struct hostent *hostp;
  struct servent *servp;
  struct sockaddr_in server;
  int sock;
  static struct timeval timeout = { 5, 0 }; /* five seconds */
  fd_set rmask, /*xmask,*/ mask;
  char bufread[BUFSIZ];
  char bufwrite[BUFSIZ];

  char localaddr[256];

  int nfound, bytesread;

  char *tok;


  if((argc !=5) && (argc != 6)) {
    (void) fprintf(stderr,"usage: %s remoteport remoteip cmd cmd2 (cmd3)\n",
		   argv[0]);
    exit(1);
  }

  
  if(argc == 5){
    printf("in child process exec: <%s> <%s> <%s> <%s> <%s>\n", 	 
	   argv[0], argv[1], argv[2], 
	   argv[3], argv[4]);

  }else {
    printf("in child process exec: <%s> <%s> <%s> <%s> <%s> <%s>\n",
	   argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
  }
  
  if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket");
    exit(1);
  }

  if (isdigit(argv[1][0])) {
    static struct servent s;
    servp = &s;
    s.s_port = htons((u_short)atoi(argv[1]));
  } else if ((servp = getservbyname(argv[1], "tcp")) == 0) {
    fprintf(stderr,"%s: unknown service\n",argv[1]);
    exit(1);
  }

  if ((hostp = gethostbyname(argv[2])) == 0) {
    fprintf(stderr,"%s: unknown host\n",argv[2]);
    exit(1);
  }
  memset((void *) &server, 0, sizeof server);
  server.sin_family = AF_INET;
  memcpy((void *) &server.sin_addr, hostp->h_addr, hostp->h_length);
  server.sin_port = servp->s_port;
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
  for (;;) {
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
      if((tok = strtok(bufread, DELIMITER))){
	if(!strcmp(tok, "ip")){ /* msg format: "ip xxx.xxx.xxx.xxx" */
	  if ((tok = strtok(NULL, DELIMITER))){
	    sprintf(localaddr, "%s", tok);
		    
	    bzero(bufwrite, sizeof(bufwrite));
	    if(!strcmp(argv[3], "reg")){/*reg myport*/
	      /* msg format: "reg port xxx.xxx.xxx.xxx" */
	      sprintf(bufwrite, "reg%s%s%s%s", 
		      DELIMITER, argv[4], DELIMITER, localaddr);
	    }else if(!strcmp(argv[3], "get")){/* get myport filename */
	      sprintf(bufwrite, "get%s%s%s%s%s%s",
		      DELIMITER, argv[4], DELIMITER, localaddr,
		      DELIMITER, argv[5]);
	    }else if(!strcmp(argv[3], "fwd")){
	      strcpy(bufwrite, "get");
	      strcat(bufwrite, &argv[3][3]); /*replace "fwd" with "get" */
	    }else if(!strcmp(argv[3], "push")){
	      sprintf(bufwrite, "push%s%s%s%s%s%s",
		      DELIMITER, argv[4], DELIMITER, localaddr,
		      DELIMITER, argv[5]);
	    }else if(!strcmp(argv[3], "quit")){
	      sprintf(bufwrite, "quit%s%s%s%s%s%s",
		      DELIMITER, argv[4], DELIMITER, localaddr,
		      DELIMITER, argv[5]);
	    }else{/* unrecognized arg[3] msg */
	      (void) fprintf(stderr,"invalid arg[3] format <%s>\n",
			     argv[3]);
	      exit(1);
	    }

	    printf("sending msg to <%s> <%s>: <%s>...\n", 
		   argv[1], argv[2], bufwrite);

	    if(write(sock, bufwrite, strlen(bufwrite)) != 
		 strlen(bufwrite)){
		perror("reg");
		exit(-1);
	    }

	    printf("sent msg to <%s> <%s>: <%s>\n", 
		   argv[1], argv[2], bufwrite);
	  }
	}else{
	  ; /*ignore the message comming other than "ip XX.XXX.XXX.XXX"*/
	}
      }
    }
  }
} /* main - client.c */
