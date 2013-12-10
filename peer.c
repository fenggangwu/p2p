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
#include <signal.h>


void
catch_alarm (int sig)
{
  puts("time out: file not found");
  signal (sig, catch_alarm);
}

int main(argc, argv)
int argc;
char *argv[];
{
  //  struct servent *servp;
  struct sockaddr_in server, remote;

  char *ip;
  char *lastip;
  char *originip;
  char *nbrip;
  //  unsigned short port; /* we use the macro P2PSERV in const.h */


  int request_sock, new_sock;
  int nfound, fd, maxfd, bytesread, nmatch, ndir, i;
  int size, n, byterecv;
  unsigned addrlen;
  fd_set rmask, mask;
  int pid;
  static struct timeval timeout = { 0, 500000 }; /* one half second */
  char bufread[BUFSIZ];
  char bufwrite[BUFSIZ];
  char msg[BUFSIZ];

  //  char portstr[6]; /* max port num: 65535*/

  char *tok;
  char filename[256];
  char otherhost[256];

  char *sharedir;
  char path[256];

  FILE *fp;

  struct ipport* ptr;
  struct dirent **namelist;

  char nbrlist[BUFSIZ];
  

  /* neighbor list */
  struct peerlist apeerlist, *peerlistp = &apeerlist;
  peerlistinit(peerlistp);

  signal (SIGALRM, catch_alarm);

  if (argc != 3) {
    (void) fprintf(stderr,"usage: %s sharedir central-server-host \n",argv[0]);
    exit(1);
  }
  if ((request_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket");
    exit(1);
  }

  bzero((void *) &server, sizeof server);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  /* server.sin_port = servp->s_port; */
  server.sin_port = htons(P2PSERV);
  if (bind(request_sock, (struct sockaddr *)&server, sizeof server) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(request_sock, SOMAXCONN) < 0) {
    perror("listen");
    exit(1);
  }

  sharedir = argv[1];

  printf("shared dir set to %s\n", sharedir);

  /* register to the central server*/
  if((pid = fork())<0){
    perror("fork");
    exit(-1);
  }
  if(pid ==0){
    if(execl("messenger", "messenger",
	     argv[2],
	     "reg", NULL)<0){
      perror("execl");
      exit(-1);
    }
  }

  FD_ZERO(&mask);
  FD_SET(request_sock, &mask);
  FD_SET(fileno(stdin), &mask);
  maxfd = request_sock;
  for (;;) {
    rmask = mask;
    nfound = select(maxfd+1, &rmask, (fd_set *)0, (fd_set *)0, &timeout);
    if (nfound < 0) {
      if (errno == EINTR) {
	//	printf("interrupted system call\n");
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

      /* eliminate tailing \n*/
      bufread[strlen(bufread)-1] = '\0';
      //      printf("the input msg is: <%s>\n", bufread);

      /* TODO parse the input command and process */
      errno = 0;

      if(!strncmp(bufread, "get", 3)){
	if((nmatch = sscanf(bufread, "get %s", filename)) == 1){
	  //  printf("get <%s>\n", filename);
	  for(ptr = peerlistp->head; ptr; ptr = ptr->next){
	    if((pid = fork()) < 0){
	      perror("fork");
	      exit(-1);
	    };
	    if(pid == 0){
	      if(execl("messenger", "messenger", 
		       inet_ntoa(ptr->addr), /* nbr ip */
		       "get", filename, NULL) 
		 <0){ /* msg to nbr */
		perror("execel");
		exit(-1);
	      }
	    }
	    /* if(pid) */
	    /*   printf("peer: parent, fork successed.\n"); */
	    /* else */
	    /*   printf("peer: child, fork successed.\n"); */
	  }
	  puts("schedule time, time out = 3 sec");
	  alarm(3);
	}else if(errno !=0){
	  perror("scanf");
	}else{
	  fprintf(stderr, "No matching characters\n");
	}
      }else if(!strncmp(bufread, "share", 5)){
	if((nmatch = sscanf(bufread, "share %s %s", filename, otherhost)) == 2){

	  strcpy(path, sharedir);
	  strcat(path, "/");
	  strcat(path, filename);

	  if(access(path, F_OK)){
	    printf("file %s does not exist\n", filename);
	  }else{
	    if((pid = fork())<0){
	      perror("fork");
	      exit(-1);
	    }
	    if(pid ==0){
	      if(execl("messenger", "messenger",
		       otherhost,
		       "push", path, NULL)<0){
		perror("execl");
		exit(-1);
	      }
	    }
	  }
	  
	}else if(errno !=0){
	  perror("scanf");
	}else{
	  fprintf(stderr, "No matching characters\n");
	}
      }else if(!strncmp(bufread, "list", 4)){
	ndir = scandir(sharedir, &namelist, 0, alphasort);
	if (ndir < 0)
	  perror("scandir");
	else {
	  while (ndir--) {
	    printf("%s\n", namelist[ndir]->d_name);
	    free(namelist[ndir]);
	  }
	  free(namelist);
	}
      }else if(!strncmp(bufread, "quit", 4)){
	if((pid = fork())<0){
	  perror("fork");
	  exit(-1);
	}
	if(pid ==0){
	  if(execl("messenger", "messenger",
		   argv[2],
		   "quit", NULL)<0){
	    perror("execl");
	    exit(-1);
	  }
	}

	strcpy(nbrlist, ""); /* init */
	for(i=1, ptr=peerlistp->head; i < peerlistp->cnt; 
	    i++, ptr=ptr->next){ /*stop before the last one*/
	  if((pid = fork())<0){
	    perror("fork");
	    exit(-1);
	  }
	  if(pid ==0){
	    if(execl("messenger", "messenger",
		     inet_ntoa(ptr->addr),
		     "quit", NULL)<0){
	      perror("execl");
	      exit(-1);
	    }
	  }
	  if(i > 1){
	    strcat(nbrlist, DELIMITER);
	  }
	  strcat(nbrlist, inet_ntoa(ptr->addr));
	}

	if((pid = fork())<0){
	  perror("fork");
	  exit(-1);
	}

	if(pid ==0){
	  if(execl("messenger", "messenger",
		   inet_ntoa(ptr->addr),
		   "quit", nbrlist, NULL)<0){
	    perror("execl");
	    exit(-1);
	  }
	}

	if(close(request_sock)){
	  perror("close");
	  exit(-1);
	}

	return 0;/*quit from the program*/

      }else if(!strncmp(bufread, "reg", 3)){
	if((nmatch = sscanf(bufread, "reg %s", otherhost)) == 1){
	  if((pid = fork())<0){
	    perror("fork");
	    exit(-1);
	  }
	  if(pid ==0){
	    if(execl("messenger", "messenger",
		     otherhost,
		     "reg", NULL)<0){
	      perror("execl");
	      exit(-1);
	    }
	  }
	  
	}else if(errno !=0){
	  perror("scanf");
	}else{
	  fprintf(stderr, "No matching characters\n");
	}
      }else{
	printf("invalid command: %s\n", bufread); /* ignore invalid command*/
      }
      FD_CLR(fileno(stdin), &rmask);
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
      /* printf("connection from host %s, port %d, socket %d\n", */
      /* 	     inet_ntoa(remote.sin_addr), ntohs(remote.sin_port), */
      /* 	     new_sock); */
      printf("\n\nconnection from host %s\n", inet_ntoa(remote.sin_addr));

      /* echo IP. the host itself doesn't know its IP.*/
      sprintf(bufwrite, "ip%s%s", DELIMITER, inet_ntoa(remote.sin_addr));

      /* if (write(new_sock, "ip", 3) != 3) */
      /* 	perror("ip"); */

      if (write(new_sock, bufwrite, strlen(bufwrite)) != strlen(bufwrite))
	perror("echoip");
      //      printf("bufwrite <%s>, %d written\n", bufwrite, (int)strlen(bufwrite));

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
	  //	  printf("server: end of file on %d\n",fd);
	  FD_CLR(fd, &mask);
	  if (close(fd)) perror("close");
	  continue;
	}

	strcpy(msg, bufread); /* make a copy, not to destroy the read buf*/

	addrlen = sizeof(remote);
	if(getsockname(fd, (struct sockaddr*)&remote, &addrlen)<0){
	  printf("fd = %d\n", fd);
	  //	  FD_CLR(fd, &mask);
	  //	  FD_CLR(fd, &rmask);
	  perror("getsockname");
	  exit(-1);
	  continue;
	}

	//ip = inet_ntoa(remote.sin_addr);
	//port = ntohs(remote.sin_port);

	

	msg[bytesread] = '\0';
	
	/* printf("\n\n%s: %d bytes from %d: %s\n", */
	/*        argv[0], bytesread, fd, msg); */

	if((tok = strtok(msg, DELIMITER))){
	  /* msg format: "reg host" */
	  if(!strcmp(tok, "reg")){
	    if ((tok = strtok(NULL, DELIMITER))){
	      ip = tok;
	      /* printf("reg from %s\n", ip); */
	      peerlistinsert(peerlistp, P2PSERV, ip);
	      //peerlistprint(peerlistp);
	    }
	    /* if(write(fd, "close", 5) !=5){ */
	    /*   perror("write"); */
	    /*   exit(-1); */
	    /* } */
	  }else if (!strcmp(tok, "get")){ 
	    /* msg format: "get lastip originip filename "*/
	    if ((tok = strtok(NULL, DELIMITER))){
	      lastip = tok;
	      if ((tok = strtok(NULL, DELIMITER))){
		originip = tok;
		if ((tok = strtok(NULL, DELIMITER))){
		  //filename = tok;
		  strcpy(filename, tok);
		  /* printf("get from <%s>, <%s> requests file=<%s>\n",  */
		  /* 	 lastip, originip, filename); */

		  /*try to read the file in sharedir*/
		  strcpy(path, sharedir);
		  strcat(path, "/");
		  strcat(path, filename);

		  if(access(path, F_OK)){
		    /*if does not exist*/
		    bzero(bufwrite, sizeof(bufwrite));
		    for(ptr = peerlistp->head; ptr; ptr = ptr->next){
		      /* here we skip the nbr where the msg came*/
		      if(!strcmp(lastip, inet_ntoa(ptr->addr)))
			continue;
		      if((pid = fork())<0){
			perror("fork");
			exit(-1);
		      }
		      if (pid == 0){
			bzero(bufwrite, sizeof(bufwrite));
			sprintf(bufwrite, "%s%s%s%s",
				 DELIMITER, originip,
				 DELIMITER, filename);
		      
			if (execl("messenger", "messenger",
				  inet_ntoa(ptr->addr), 
				  "fwd", bufwrite,
				  NULL) < 0){
			  perror("execl");
			  exit(-1);
			}
		      }
		    }
		  }else{
		    /* if file exists */
		    if((pid = fork())<0){
		      perror("fork");
		      exit(-1);
		    }
		    if(pid == 0){
		      strcpy(path, sharedir);
		      strcat(path, "/");
		      strcat(path, filename);
		      if(!execl("messenger","messenger", 
				originip, "push", path, NULL)){
			perror("execl");
			exit(-1);
		      }
		    }
		  }
		}
	      }
	    }
	  }else if (!strcmp(tok, "push")){
	    if ((tok = strtok(NULL, DELIMITER))){
	      strcpy(filename, tok);
	      if ((tok = strtok(NULL, DELIMITER))){
		alarm(0); /*file found, reset alarm*/
		size = atoi(tok);
		strcpy(path, sharedir);
		strcat(path, "/");
		strcat(path, filename);
		/* printf("try to create file <%s>\n", path); */
		if(!(fp = fopen(path, "w"))){
		  perror("fopen");
		  exit(-1);
		}

		bzero(bufread, sizeof(bufread));
		byterecv = 0;
		while((n = read(fd, bufread, sizeof(bufread)))>0){
		  if(n != fwrite(bufread, sizeof(char), n, fp)){
		    perror("fwrite");
		    exit(-1);
		  }
		    

		  bzero(bufread, sizeof(bufread));
		  byterecv += n;
		  if (byterecv >=  size) 
		    break;
		}

		printf("received file <%s> (%d) byte\n", 
		       filename, byterecv);

		if(fclose(fp)){
		  perror("fclose");
		  exit(-1);
		}
	      }
	    }
	  }else if (!strcmp(tok, "nbr")){
	    if ((tok = strtok(NULL, DELIMITER))){
	      nbrip = tok;
	      peerlistinsert(peerlistp, P2PSERV, nbrip);
	      if((pid = fork()) < 0){
		perror("fork");
		exit(-1);
	      };
	      if(pid == 0){
		/* printf("reg to <%s>\n", nbrip); */
		if(execl("messenger", "messenger", 
			 nbrip, /* nbr ip */
			 "reg", NULL) <0){ /* msg to nbr */
		  perror("execel");
		  exit(-1);
		}
	      }
	    }
	  }else if (!strcmp(tok, "quit")){
	    if ((tok = strtok(NULL, DELIMITER))){
	      /*delete the quiting peer*/
	      ip = tok;
	      peerlistdelete(peerlistp, P2PSERV, ip);

	      /* take over its nbrs if any*/
	      while((tok = strtok(NULL, DELIMITER))){
		peerlistinsert(peerlistp, P2PSERV, tok);
		
		if((pid = fork())<0){
		  perror("fork");
		  exit(-1);
		}
		if(pid ==0){
		  if(execl("messenger", "messenger",
			   tok, "reg", NULL)<0){
		    perror("execl");
		    exit(-1);
		  }
		}
	      }
	    }

	  }else {
	    ;/* ignore illegal message here */
	  }
	}

	/* if(handlemsg(buf, peerlistp)<0){ */
	/*   perror("handlemsg"); */
	/* } */
	/* echo it back */
	if (write(fd, bufread, bytesread)!=bytesread)
	  perror("echo");
      }
    }
  }
  if(close(request_sock)){
    perror("close");
    exit(-1);
  }
  return 0;
} /* main - server.c */

