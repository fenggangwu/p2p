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
#include <sys/stat.h>
#include <libgen.h>

/* format: ./client host cmd (cmd2)*/
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
  char buf[BUFSIZ]; /*for file transfer*/
  char localaddr[256];

  int nfound, bytesread, bytesent, n, size;

  char *tok;

  struct stat st;

  FILE *fp;

  //  printf("peer: fork successed.\n");

  if((argc !=4) && (argc != 3)) {
    (void) fprintf(stderr,"usage: %s host cmd (cmd2)\n",
		   argv[0]);
    exit(1);
  }

  
  /* if(argc == 4){ */
  /*   printf("in child process exec: <%s> <%s> <%s><%s>\n", 	  */
  /* 	   argv[0], argv[1], argv[2],  */
  /* 	   argv[3]); */

  /* }else { */
  /*   printf("in child process exec: <%s> <%s> <%s>\n", */
  /* 	   argv[0], argv[1], argv[2]); */
  /* } */
  
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

  FD_ZERO(&mask);
  FD_SET(sock, &mask);
  /* this messenger will not listen to keyboard input*/
  //  FD_SET(fileno(stdin), &mask); 

  for(;;) {
    rmask = mask;
    nfound = select(FD_SETSIZE, &rmask, (fd_set *)0, (fd_set *)0, &timeout);
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
      /* timer expired */
      //      printf("Please type something!\n");
      continue;
    }
    if (FD_ISSET(sock,&rmask)) {
      /* data from network */
      bytesread = read(sock, bufread, sizeof(bufread));
      bufread[bytesread] = '\0';
      //      printf("%s: got %d bytes (%d, %d): %s\n", argv[0], bytesread, (int)sizeof(bufread), (int)strlen(bufread), bufread);
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

	      /* printf("sending msg to <%s>: <%s>...\n",  */
	      /* 	     argv[1], bufwrite); */

	      if(write(sock, bufwrite, strlen(bufwrite)) != 
		 strlen(bufwrite)){
		perror("reg");
		exit(-1);
	      }

	      /* printf("sent msg to <%s>: <%s>\n",  */
	      /* 	     argv[1], bufwrite); */

	      break;
	    }else if(!strcmp(argv[2], "get")){/* get filename */
	      /* msg format: "get lastip originip filename "*/
	      sprintf(bufwrite, "get%s%s%s%s%s%s",
		      DELIMITER, localaddr,
		      DELIMITER, localaddr,
		      DELIMITER, argv[3]);
	      /* printf("sending msg to <%s>: <%s>...\n",  */
	      /* 	     argv[1], bufwrite); */

	      if(write(sock, bufwrite, strlen(bufwrite)) != 
		 strlen(bufwrite)){
		perror("get");
		exit(-1);
	      }

	      /* printf("sent msg to <%s>: <%s>\n",  */
	      /* 	     argv[1], bufwrite); */
	      break;
	    }else if(!strcmp(argv[2], "fwd")){

	      sprintf(bufwrite, "get%s%s%s", 
		      DELIMITER, localaddr, argv[3]);
	      /* printf("fwding msg to <%s>: <%s>...\n",  */
	      /* 	     argv[1], bufwrite); */

	      if(write(sock, bufwrite, strlen(bufwrite)) != 
		 strlen(bufwrite)){
		perror("fwd");
		exit(-1);
	      }

	      /* printf("fwded msg to <%s>: <%s>\n",  */
	      /* 	     argv[1], bufwrite); */

	      break;

	    }else if(!strcmp(argv[2], "push")){
	      if(access(argv[3], F_OK)){
		printf("file %s does not exist\n", argv[3]);
		break;
	      }

	      stat(argv[3], &st);
	      size = st.st_size;
	      /* msg format: push file.txt size_in_byte*/
	      sprintf(bufwrite, "push%s%s%s%d",
		      DELIMITER, basename(argv[3]),
		      DELIMITER, size);
	      
	      /* printf("sending msg to <%s>: <%s>...\n",  */
	      /* 	     argv[1], bufwrite); */

	      if(write(sock, bufwrite, strlen(bufwrite)) != 
		 strlen(bufwrite)){
		perror("push");
		exit(-1);
	      }

	      /* printf("sent msg to <%s>: <%s>\n",  */
	      /* 	     argv[1], bufwrite); */


	      /*file transfer starts */

	      /* printf("Opening <%s>...\n", argv[3]); */
	      if(!(fp = fopen(argv[3], "r"))){
		perror("fopen");
		exit(-1);
	      }
	      else{
		//todo read the file and return it to the requester
		/* printf("open <%s> sucessfully\n", argv[3]); */

		bytesent = 0;
		bzero(buf, sizeof(buf));
		while((n=fread(buf, sizeof(char), BUFSIZ, fp)) > 0){
		  if(write(sock, buf, n) != n){
		    perror("write");
		    exit(-1);
		  }
		  bzero(buf, sizeof(buf));
		  bytesent += n;
		}

		printf("file %s (%d bytes) sent\n", 
		       argv[3], bytesent);

		if(fclose(fp)){
		  perror("fclose");
		  //		    exit(-1);
		}
	      }



	      /*file transfer ends */


	      break;
	    }else if(!strcmp(argv[2], "quit")){
	      if(argc == 4){
		sprintf(bufwrite, "quit%s%s%s%s",
			DELIMITER, localaddr,
			DELIMITER, argv[3]);
	      }else{
		sprintf(bufwrite, "quit%s%s", DELIMITER, localaddr);
	      }

	      /* printf("sending msg to <%s>: <%s>...\n",  */
	      /* 	     argv[1], bufwrite); */

	      if(write(sock, bufwrite, strlen(bufwrite)) != 
		 strlen(bufwrite)){
		perror("reg");
		exit(-1);
	      }

	      /* printf("sent msg to <%s>: <%s>\n",  */
	      /* 	     argv[1], bufwrite); */
	      break;

	    }else if(!strcmp(argv[2], "nbr")){
	      bzero(bufwrite, sizeof(bufwrite));
	      sprintf(bufwrite, "nbr%s%s",
		      DELIMITER, argv[3]);

	      if(write(sock, bufwrite, strlen(bufwrite)) != 
		 strlen(bufwrite)){
		perror("nbr");
		exit(-1);
	      }
	  
	      break;


	    }else{/* unrecognized arg[3] msg */
	      (void) fprintf(stderr,"invalid arg[3] format <%s>\n",
			     argv[2]);
	      exit(1);
	    }
	  }
	}else{
	  ; /*ignore the message comming other than "ip XX.XXX.XXX.XXX"*/
	}
      }
    }
  }
  if(close(sock)){
    perror("sock");
    exit(-1);
  }
  fflush(stdout);
  return 0;
} /* main - client.c */
