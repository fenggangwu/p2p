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
  //  struct servent *servp;
  struct sockaddr_in server, remote;

  char *ip;
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

  char portstr[6]; /* max port num: 65535*/

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

  if (argc != 3) {
    (void) fprintf(stderr,"usage: %s sharedir central-server-host \n",argv[0]);
    exit(1);
  }
  if ((request_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket");
    exit(1);
  }

  /* if (isdigit(argv[2][0])) { */
  /*   static struct servent s; */
  /*   servp = &s; */
  /*   s.s_port = htons((u_short)atoi(argv[2])); */
  /* } else if ((servp = getservbyname(argv[2], "tcp")) == 0) { */
  /*   fprintf(stderr,"%s: unknown service\n", argv[2]); */
  /*   exit(1); */
  /*} */
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

  /* if(!getcwd(sharedir, 256)){ */
  /*   perror(sharedir); */
  /*   exit(-1); */
  /* } */

  /* if ((pid = fork()) < 0){ */
  /*   perror("fork"); */
  /*   exit(1); */
  /* } */

  /* printf("%s %s pid=%d\n", argv[2], argv[3], pid); */

  /* if (pid == 0){ */
  /*   printf("This is the child process\n"); */
  /*   if (execl("client", "client",  argv[2], argv[3], NULL) < 0){ */
  /*     perror("execl"); */
  /*     exit(-1); */
  /*   } */
  /* } */
  
  /* printf("This is the parent process, %s, %s\n", argv[1], argv[2]); */

  FD_ZERO(&mask);
  FD_SET(request_sock, &mask);
  FD_SET(fileno(stdin), &mask);
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
      printf("the input msg is: <%s>\n", bufread);

      /* TODO parse the input command and process */
      errno = 0;

      if(!strncmp(bufread, "get", 3)){
	if((nmatch = sscanf(bufread, "get %s", filename)) == 1){
	  printf("get <%s>\n", filename);
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
	    printf("peer: fork successed.\n");
	  }
	}else if(errno !=0){
	  perror("scanf");
	}else{
	  fprintf(stderr, "No matching characters\n");
	}
      }else if(!strncmp(bufread, "share", 5)){
	if((nmatch = sscanf(bufread, "share %s %s", filename, otherhost)) == 2){
	  printf("share <%s> <%s>\n", filename, otherhost);
	  printf("before fork\n");
	  if((pid = fork())<0){
	    perror("fork");
	    exit(-1);
	  }
	  printf("after fork\n");
	  if(pid ==0){
	    strcpy(path, sharedir);
	    strcat(path, filename);
	    if(execl("messenger", "messenger",
		     otherhost,
		     "push", path, NULL)<0){
	      perror("execl");
	      exit(-1);
	    }
	  }
	  
	}else if(errno !=0){
	  perror("scanf");
	}else{
	  fprintf(stderr, "No matching characters\n");
	}
      }else if(!strncmp(bufread, "list", 4)){
	ndir = scandir(".", &namelist, 0, alphasort);
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
	    strcat(nbrlist, " ");
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
	      peerlistinsert(peerlistp, P2PSERV, ip);
	      //peerlistprint(peerlistp);
	    }
	    /* if(write(fd, "close", 5) !=5){ */
	    /*   perror("write"); */
	    /*   exit(-1); */
	    /* } */
	  }else if (!strcmp(tok, "get")){ /*"get host filename"*/
	    if ((tok = strtok(NULL, DELIMITER))){
	      ip = tok;
	      if ((tok = strtok(NULL, DELIMITER))){
		//filename = tok;
		strcpy(filename, tok);
		printf("get from %s, file=<%s>\n", 
		       ip, filename);

		/*try to read the file in sharedir*/
		strcpy(path, sharedir);
		strcat(path, filename);

		if(access(path, F_OK)){
		  /*if does not exist*/
		  for(ptr = peerlistp->head; ptr; ptr = ptr->next){
		    pid = fork();
		    if (pid == 0){
		      sprintf(portstr, "%hu", ptr->port);
		      printf("in child process pid = %d, <%s>, <%s>\n",
			     pid, portstr, inet_ntoa(ptr->addr));
		      if (execl("client", "client",
				portstr, inet_ntoa(ptr->addr), bufread,
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
		    if(!execl("messenger","messenger", 
			      ip, "push", filename, NULL)){
		      perror("execl");
		      exit(-1);
		    }
		  }
		}
	      }
	    }
	  }else if (!strcmp(tok, "push")){
	    if ((tok = strtok(NULL, DELIMITER))){
	      strcpy(filename, tok);
	      if ((tok = strtok(NULL, DELIMITER))){
		size = atoi(tok);
		strcpy(path, sharedir);
		strcat(path, filename);
		printf("try to create file <%s>\n", path);
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

		printf("received file <%s>()%d byte\n", 
		       filename, byterecv);

		if(fclose(fp)){
		  perror("fclose");
		  exit(-1);
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
} /* main - server.c */

