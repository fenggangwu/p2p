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


/* format: ./client remoteport remortip mysvrport */
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
  char *msg = NULL; /*store the msg to be sent*/

  int nfound, bytesread;
  char mysvrip[BUFSIZ];
  unsigned short mysvrport;
  char *tok;


  if ((argc != 5) && (argc != 4)) {
    (void) fprintf(stderr,"usage: %s remoteport remoteip mysvrport (msg)\n",
		   argv[0]);
    exit(1);
  }

  if(argc == 5){
    msg = argv[4];
  }

  printf("in child process exec: %s %s %s %s %s\n", 
	 argv[0], argv[1], argv[2], argv[3], argv[4]);

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

  if (isdigit(argv[3][0])){ 
    mysvrport = (unsigned short)atoi(argv[3]);
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
  FD_SET(fileno(stdin), &mask);
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
    if (FD_ISSET(fileno(stdin), &rmask)) {
      /* data from keyboard */
      if (!fgets(bufread, sizeof bufread, stdin)) {
	if (ferror(stdin)) {
	  perror("stdin");
	  exit(1);
	}
	exit(0);
      }

      //     printf("the input msg is: <%s>\n", bufread);

      /*TODO eliminate tailing \n*/
      bufread[strlen(bufread)-1] = '\0';
      //      printf("the input msg is: <%s>\n", bufread);
      if (write(sock, bufread, strlen(bufread)) < 0) {
	perror("write");
	exit(1);
      }
    }
    if (FD_ISSET(sock,&rmask)) {
      /* data from network */
      bytesread = read(sock, bufread, sizeof(bufread));
      bufread[bytesread] = '\0';
      printf("%s: got %d bytes (%d, %d): %s\n", argv[0], bytesread, (int)sizeof(bufread), (int)strlen(bufread), bufread);
      if((tok = strtok(bufread, DELIMITER))){
	if(!strcmp(tok, "ip")){ /* msg format: "ip|xxx.xxx.xxx.xxx" */
	  if ((tok = strtok(NULL, DELIMITER))){
	    sprintf(mysvrip, "%s", tok);
	    
	    if(msg){
	      /* forward the msg to */
	      if(write(sock, msg, strlen(msg)) != 
		 strlen(msg)){
		perror("fwdmsg");
		exit(-1);
	      }
	    }else{
	      /* msg format: "reg port xxx.xxx.xxx.xxx" */
	      sprintf(bufwrite, "reg%s%hu%s%s", 
		      DELIMITER, mysvrport, DELIMITER, mysvrip);
	      if(write(sock, bufwrite, strlen(bufwrite)) != 
		 strlen(bufwrite)){
		perror("reg");
		exit(-1);
	      }
	    }
	  }
	}else{
	  ;
	}
      }
    }
  }
} /* main - client.c */
