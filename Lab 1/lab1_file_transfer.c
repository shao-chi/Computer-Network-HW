#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    if (strcmp(argv[1], "tcp") == 0)
    {
        if (strcmp(argv[2], "send") == 0)
        {
            int sockfd, portno, n;
            struct sockaddr_in serv_addr;
            struct hostent *server;
            char buffer[256];

            if (argc < 5)
            {
                fprintf(stderr, "usage %s hostname post\n", argv[0]);
                exit(0);
            }

            portno = atoi(argv[4]);
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0)
                error("ERROR opening socket");

            server = gethostbyname(argv[3]);
            if (server == NULL)
            {
                fprintf(stderr, "ERROR, no such host\n");
                exit(0);
            }

            bzero((char *) &serv_addr, sizeof(sevr_addr));
            serv_addr.sin_family = AF_INET;
            bcopy((char *) server -> h_addr, (char *) &serv_addr.sin_addr.s_addr, server -> h_length);
            serv_addr.sin_port =  htons(portno);

            if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
                error("ERROR connecting");

            FILE *fptc = fopen(argv[5], "r");

            fseek(fptc, 0, SEEK_END);
            int size = ftell(fptc);
            fseek(fptc, 0, SEEK_SET);

            int bag = size/sizeof(buffer) + 1;
            int i = 1, rate = 0, pac = bag/20;
            int s;
            int hour, min, sec;

            time_t time;
            struct tm *p;
            time(&timep);
            p = gmtime(&timep);
            while()
        }
    }
}