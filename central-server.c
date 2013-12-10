#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "peerlist.h"
#include <stdlib.h>
#include <string.h>
#include "const.h"
#include <dirent.h>



int main(argc, argv)
int argc;
char *argv[];
{
  struct sockaddr_in server, remote;

  char *ip;


  int request_sock, new_sock;
  int nfound, fd, maxfd, bytesread, n;
  int pid;
  unsigned addrlen;
  fd_set rmask, mask;
  static struct timeval timeout = { 0, 500000 }; /* one half second */
  char bufread[BUFSIZ];
  char bufwrite[BUFSIZ];
  char msg[BUFSIZ];

  //  char portstr[6]; /* max port num: 65535*/

  char *tok;

  struct ipport* ptr;
  

  /* neighbor list */
  struct peerlist apeerlist, *peerlistp = &apeerlist;
  peerlistinit(peerlistp);


  if ((request_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket");
    exit(1);
  }


  bzero((void *) &server, sizeof server);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;

  server.sin_port = htons(P2PSERV);
  if (bind(request_sock, (struct sockaddr *)&server, sizeof server) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(request_sock, SOMAXCONN) < 0) {
    perror("listen");
    exit(1);
  }


  FD_ZERO(&mask);
  FD_SET(request_sock, &mask);

  maxfd = request_sock;
  for (;;) {
    rmask = mask;
    nfound = select(maxfd+1, &rmask, (fd_set *)0, (fd_set *)0, &timeout);
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
      /* timeout */
      //      printf("."); fflush(stdout);
      continue;
    }


    if (FD_ISSET(request_sock, &rmask)) {
      /* a new connection is available on the connetion socket */
      addrlen = sizeof(remote);
      new_sock = accept(request_sock,
			(struct sockaddr *)&remote, &addrlen);
      if (new_sock < 0) {
	perror("accept");
	exit(1);
      }
      printf("connection from host %s, port %d, socket %d\n",
	     inet_ntoa(remote.sin_addr), ntohs(remote.sin_port),
	     new_sock);

      /* echo IP. the host itself doesn't know its IP.*/
      sprintf(bufwrite, "ip%s%s", DELIMITER, inet_ntoa(remote.sin_addr));

      /* if (write(new_sock, "ip", 3) != 3) */
      /* 	perror("ip"); */

      if (write(new_sock, bufwrite, strlen(bufwrite)) != strlen(bufwrite))
	perror("echoip");
      printf("bufwrite <%s>, %d written\n", bufwrite, (int)strlen(bufwrite));

      FD_SET(new_sock, &mask);
      if (new_sock > maxfd)
	maxfd = new_sock;
      FD_CLR(request_sock, &rmask);
    }
    for (fd=0; fd <= maxfd ; fd++) {
      /* look for other sockets that have data available */
      if (FD_ISSET(fd, &rmask)) {
	/* process the data */
	bytesread = read(fd, bufread, sizeof bufread - 1);
	if (bytesread<0) {
	  perror("read");
	  /* fall through */
	}
	if (bytesread<=0) {
	  printf("server: end of file on %d\n",fd);
	  FD_CLR(fd, &mask);
	  if (close(fd)) perror("close");
	  continue;
	}

	strcpy(msg, bufread); /* make a copy, not to destroy the read buf*/

	addrlen = sizeof(remote);
	if(getsockname(fd, (struct sockaddr*)&remote, &addrlen)<0){
	  perror("getsockname");
	  exit(-1);
	}

	//ip = inet_ntoa(remote.sin_addr);
	//port = ntohs(remote.sin_port);

	

	msg[bytesread] = '\0';
	
	printf("\n\n%s: %d bytes from %d: %s\n",
	       argv[0], bytesread, fd, msg);

	if((tok = strtok(msg, DELIMITER))){
	  /* msg format: "reg host" */
	  if(!strcmp(tok, "reg")){
	    if ((tok = strtok(NULL, DELIMITER))){
	      ip = tok;
	      printf("reg from %s\n", ip);
	      ptr = peerlistrand(peerlistp);
	      n = peerlistinsert(peerlistp, P2PSERV, ip);
	      printf("insert nbr success, %d nbrs now\n", peerlistp->cnt);
	      if(ptr){/*if nbr exists, randomly pick one to reply */
		if((pid = fork())<0){
		  perror("fork");
		  exit(-1);
		}
		if(pid == 0){
		  /* msg format: nbr nbrip */
		  if(execl("messenger", "messenger", ip, 
			   "nbr", inet_ntoa(ptr->addr),  NULL)<0){
		    perror("execl");
		    exit(-1);
		  }
		}
	      }
	    }
	  }else if(!strcmp(tok, "quit")){
	    if ((tok = strtok(NULL, DELIMITER))){
	      ip = tok;
	      printf("quit from %s\n", ip);
	      n = peerlistdelete(peerlistp, P2PSERV, ip);
	      printf("delete nbr success, %d nbrs remains\n", n);
	    }
	  }else {
	    ;/* ignore illegal message here */
	  }
	}

	if (write(fd, bufread, bytesread)!=bytesread)
	  perror("echo");
      }
    }
  }
} /* main - server.c */

