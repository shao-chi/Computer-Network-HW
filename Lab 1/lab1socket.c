#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <time.h>

#define ERR_EXIT(m) \
        do \
        { \
                perror(m); \
                exit(EXIT_FAILURE); \
        } while(0)

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
	if(strcmp(argv[1], "tcp") == 0)
	{
		if(strcmp(argv[2], "send") == 0)
		{
			int sockfd, portno, n;
			struct sockaddr_in serv_addr;
			struct hostent *server;

		    char buffer[64];
		    if (argc < 5)
			{
		       	fprintf(stderr,"usage %s hostname port\n", argv[0]);
		       	exit(0);
			}
		    portno = atoi(argv[4]);
		    sockfd = socket(AF_INET, SOCK_STREAM, 0);
		    if (sockfd < 0) 
		        error("ERROR opening socket");
		    server = gethostbyname(argv[3]);
		    if (server == NULL)
			{
		        fprintf(stderr,"ERROR, no such host\n");
		        exit(0);
			}
		    bzero((char *) &serv_addr, sizeof(serv_addr));
		    serv_addr.sin_family = AF_INET;
		    bcopy((char *)server->h_addr, 
		    (char *)&serv_addr.sin_addr.s_addr, server->h_length);
		    serv_addr.sin_port = htons(portno);
		    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		        error("ERROR connecting");
		      
		    FILE *fptc = fopen(argv[5], "r");
 
		    fseek(fptc, 0, SEEK_END);
			int size = ftell(fptc);
			fseek(fptc, 0, SEEK_SET);
				
			int bag = size/sizeof(buffer) + 1;
			int i = 1, rate = 0, pac = bag/20;
			int s;
			int hour, min, sec;
			
			time_t timep;
			struct tm *p;
			time(&timep);
			p = gmtime(&timep);
			while(!feof(fptc))
			{
				n = fread(buffer, sizeof(char), sizeof(buffer), fptc);
				send(sockfd,buffer, n, 0);
				rate++;
				if(rate % pac == 0)
				{
					s= (int)time(NULL);
					s += 8*60*60;
					sec = s % 60;
					min = (s/60) % 60;
					hour = (s/3600) % 24;
					if(i <= 20)
						printf("%3d%% %d/%d/%d %d:%d:%d\n", 5*i, (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, hour, min, sec);
					if(i == 20)
						continue;
					i++;
				}
			}
		    if (n < 0) 
		        error("ERROR writing to socket");
		    bzero(buffer, sizeof(buffer));

		    printf("%s\n",buffer);
		    close(sockfd);
		 
		    return 0;	
		}
		if(strcmp(argv[2], "recv") == 0)
		{
			int sockfd, newsockfd, portno, ip;
		    socklen_t clilen;
		    char buffer[64];
		    struct sockaddr_in serv_addr, cli_addr;
		    int n;
		    if (argc < 4)
			{
	        	fprintf(stderr,"ERROR, no port provided\n");
	        	exit(1);
			}
		    sockfd = socket(AF_INET, SOCK_STREAM, 0);
		    if (sockfd < 0)
				error("ERROR opening socket");
		    bzero((char *) &serv_addr, sizeof(serv_addr));
		    ip = atoi(argv[3]);
		    portno = atoi(argv[4]);
		    serv_addr.sin_family = AF_INET;
		    serv_addr.sin_addr.s_addr = INADDR_ANY;
		    serv_addr.sin_port = htons(portno);
		    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	        	error("ERROR on binding");
		    listen(sockfd,5);
		    clilen = sizeof(cli_addr);
		    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		    if (newsockfd < 0)
	        	error("ERROR on accept");
			
			FILE *fpts = fopen(argv[5], "w");
			while(1)
			{
				n = recv(newsockfd, buffer, sizeof(buffer), 0);
				int k = fwrite(buffer, sizeof(char), n, fpts);
				if(n == 0)
					break;
			}
		
		    bzero(buffer,sizeof(buffer));
			printf("I got the file!!\n");
			
			close(newsockfd);
			close(sockfd);

		     return 0;	
		}
	}

	if(strcmp(argv[1], "udp") == 0)
	{
		if(strcmp(argv[2], "send") == 0)
		{
			int sock, portno;
		    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		        ERR_EXIT("socket");
			portno = atoi(argv[4]);
		    struct sockaddr_in servaddr;
		    memset(&servaddr, 0, sizeof(servaddr));
		    servaddr.sin_family = AF_INET;
		    servaddr.sin_port = htons(portno);
		    servaddr.sin_addr.s_addr = inet_addr(argv[3]);
		
		    int sumpac = 0, i;
		    char sendbuf[12000] = {0};
			
			FILE *fpuc = fopen(argv[5], "r");
			while (!feof(fpuc))
		    {
				int num = fread(sendbuf, sizeof(char), sizeof(sendbuf), fpuc);
				sendto(sock, sendbuf, num, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
		        memset(sendbuf, 0, sizeof(sendbuf));
		        sumpac++;
		        printf("Sending NO.%d packet\n", sumpac);
				sleep(0.1);
		    }

			char buffer[5];
			int l = sizeof(buffer);
			for(i = 0; i < l; i++)
			    buffer[i] = '0';
			while(sumpac != 0)
			{
				buffer[--l] = sumpac%10 + '0';
				sumpac/=10;
			}
			while(sendto(sock, buffer, 5, 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
			{
				printf("Sending error\n");
			}
			printf("Sending file finished!\n");	  
			close(sock);
		
		    return 0;
		}
		if(strcmp(argv[2], "recv") == 0)
		{
			int sock, portno, ip;
		    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		        ERR_EXIT("socket error");
	        ip = atoi(argv[3]);
			portno = atoi(argv[4]);
		    struct sockaddr_in servaddr;
		    memset(&servaddr, 0, sizeof(servaddr));
		    servaddr.sin_family = AF_INET;
		    servaddr.sin_port = htons(portno);
		    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		
		    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		        ERR_EXIT("bind error");
		
		    char recvbuf[12000] = {0};
		    struct sockaddr_in peeraddr;
		    socklen_t peerlen = sizeof(peeraddr);
		    
			FILE *fpus = fopen(argv[5], "w");
			int sumpac = 0, lospac = 0, n, k;
			double losrate;
		    while (1)
		    {
		        memset(recvbuf, 0, sizeof(recvbuf));
		        n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&peeraddr, &peerlen);
				k = fwrite(recvbuf, sizeof(char), n, fpus);
				sumpac++;
				if(n < sizeof(recvbuf))
					break;
		    }
		    printf("Got the file\n");

			char buffer[5];
			while(read(sock,buffer,5) < 0)
			{
				printf("Receiving error\n");
			}
			double send = (buffer[0]-'0')*10000 + (buffer[1]-'0')*1000 + (buffer[2]-'0')*100 + (buffer[3]-'0')*10 + (buffer[4]-'0');
			double lostrate = ((send-sumpac)/send)*100;
			int sum = send - sumpac;
			if(sum == 0)
				printf("Total missed %d packet\n", sum);
			else
				printf("Total missed %d packets\n", sum);
			printf("lost rate = %lf%%\n", lostrate);
		    close(sock);
		
		    return 0;	
		}
	}
}
