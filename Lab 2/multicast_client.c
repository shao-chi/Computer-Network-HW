/* Receiver/client multicast Datagram example. */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
struct sockaddr_in localSock;
struct ip_mreq group;
int sd;
int datalen;
char databuf[1024];
char media_file[15] = "recv_music.mp3";

int main(int argc, char *argv[])
{
/* Create a datagram socket on which to receive. */
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0)
	{
		perror("Opening datagram socket error");
		exit(1);
	}
	else
	printf("Opening datagram socket....OK.\n");
		 
	/* Enable SO_REUSEADDR to allow multiple instances of this */
	/* application to receive copies of the multicast datagrams. */
	{
		int reuse = 1;
		if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
		{
			perror("Setting SO_REUSEADDR error");
			close(sd);
			exit(1);
		}
		else
			printf("Setting SO_REUSEADDR...OK.\n");
	}
	 
	/* Bind to the proper port number with the IP address */
	/* specified as INADDR_ANY. */
	memset((char *) &localSock, 0, sizeof(localSock));
	localSock.sin_family = AF_INET;
	localSock.sin_port = htons(4321);
	localSock.sin_addr.s_addr = INADDR_ANY;
	if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock)))
	{
		perror("Binding datagram socket error");
		close(sd);
		exit(1);
	}
	else
		printf("Binding datagram socket...OK.\n");
	 
	/* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
	/* interface. Note that this IP_ADD_MEMBERSHIP option must be */
	/* called for each local interface over which the multicast */
	/* datagrams are to be received. */
	group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
	group.imr_interface.s_addr = inet_addr("192.168.1.104"); //改成自己local的ip address(linux的話在ifconfig裡面可以查)
	if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
	{
		perror("Adding multicast group error");
		close(sd);
		exit(1);
	}
	else
		printf("Adding multicast group...OK.\n");

	/* 多媒體檔案 */
	FILE *fpuc = fopen(media_file, "wb");
	fseek(fpuc, 0, SEEK_END);
	int size = ftell(fpuc);
	fseek(fpuc, 0, SEEK_SET);

	int bag;
	if(read(sd, &bag, sizeof(bag)) < 0)
	{
		perror("Reading bag number message error");
		close(sd);
		exit(1);
	}
	else
	{
		printf("Reading bag number message...OK.\n");
		// printf("The message from multicast server is: \"%s\"\n", databuf)
	}
	int total = bag;
	/* Read from the socket. */
	while (bag >= 0)
	{
        bzero(databuf, sizeof(databuf)); /* buffer初始化 */
		datalen = sizeof(databuf);
		if(read(sd, databuf, datalen) < 0)
		{
			perror("Reading datagram message error");
			close(sd);
			exit(1);
		}
		else
		{
			if (databuf == "finished")
			{
				printf("Finish~~~~~\n");
				break;
			}
			else
			{
				printf("Reading datagram message...OK.\n");
				// printf("The message from multicast server is: \"%s\"\n", databuf);
				fwrite(databuf, sizeof(char), sizeof(databuf), fpuc);
			}
		}
		bag = bag - 1;
	}	
	float loss = bag/total;
	printf("封包丟失率： %f\n", loss);

	return 0;
}
