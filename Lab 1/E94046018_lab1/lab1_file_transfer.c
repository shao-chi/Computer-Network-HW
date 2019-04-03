#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
    exit(0);
}

/* udp封包資訊 */
struct SendPack 
{ 
  int id; 
  char buf[64]; 
} pack; 

int main(int argc, char *argv[])
{
    /* tcp transfer */
    if (strcmp(argv[1], "tcp") == 0)
    {
        if (strcmp(argv[2], "send") == 0)
        {
            int sockfd, newsockfd, portno, ip;
            char buffer[64];
            struct sockaddr_in serv_addr, cli_addr;
            socklen_t clilen = sizeof(cli_addr);
            int n;

            if (argc < 4)
            {
                fprintf(stderr, "ERROR, no port provided\n");
                exit(1);
            }
            
            /* 建立socket */
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0)
                error("ERROR opening socket");
            
            /* 設定server地址 */
            bzero((char *) &serv_addr, sizeof(serv_addr));
            ip = atoi(argv[3]);
            portno = atoi(argv[4]);
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = INADDR_ANY;
            serv_addr.sin_port = htons(portno);

            /* 綁定server地址在socket上 */
            if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
                error("ERROR on binding");
            
            /* 監聽隊列 */
            listen(sockfd, 5);

            /* 連接client */
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0)
                error("ERROR on accept");

            /* 傳送檔案名稱 */
            send(newsockfd, argv[5], sizeof(buffer), 0);

            /* 打開檔案 */
            FILE *fptc = fopen(argv[5], "r");
            fseek(fptc, 0, SEEK_END);
            int size = ftell(fptc);
            fseek(fptc, 0, SEEK_SET);

            int bag = (int)(size/sizeof(buffer) + 1); /* 封包數量 */
            int i = 1; /* 目前是第i封包 */
            int t = 0, t_late; /* 進度比例是0.05的多少倍數 */
            int s, hour, min, sec; /* 時間 */

            printf("%d bag(s)\n", bag);

            /* 取得時間 */
            time_t timep;
            struct tm *p;
            time(&timep);
            p = gmtime(&timep);

            /* 檔案傳輸ing */
            while (!feof(fptc))
            {
                n = fread(buffer, sizeof(char), sizeof(buffer), fptc); /* 依buffer大小讀取檔案 -> 封包 */
                send(newsockfd, buffer, n, 0); /* 傳送封包 */
                float m = (float)((float)(i)/(float)(bag));
                t_late = (int)(m/0.05); /* 現在傳送比例之0.05倍數 */
                if (t_late > t) /* 每增加一個倍數print一次目前進度 */
                {
                    int count = t_late-t, j;
                    for(j = 0; j < count; j ++)
                    {
                        s = (int)time(NULL); /* 取得現在時間 */
                        s += 8*60*60;
                        sec = s % 60;
                        min = (s/60) % 60;
                        hour = (s/3600) % 24;
                        printf("%3d%% %d/%d/%d %d:%d:%d\n", 5*(t + j + 1), (1900 + p -> tm_year), (1 + p -> tm_mon), p -> tm_mday, hour, min, sec);
                    }
                    t = t_late;
                }
                i ++;
            }
            if (n < 0)
                error("ERROR writing to socket");
            bzero(buffer, sizeof(buffer)); /* buffer初始化 */

            /* 結束socket */
            close(newsockfd);
            close(sockfd);

            return 0;
        }

        if (strcmp(argv[2], "recv") == 0)
        {
            int sockfd, portno, n;
            struct sockaddr_in serv_addr;
            struct hostent *server;
            char buffer[64];
            if (argc < 5)
            {
                fprintf(stderr, "usage %s hostname post\n", argv[0]);
                exit(0);
            }

            portno = atoi(argv[4]);
            /* 建立socket */
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0)
                error("ERROR opening socket");

            /* 設定ip */
            server = gethostbyname(argv[3]);
            if (server == NULL)
            {
                fprintf(stderr, "ERROR, no such host\n");
                exit(0);
            }

            /* 設定server地址和port */
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            bcopy((char *) server -> h_addr, (char *) &serv_addr.sin_addr.s_addr, server -> h_length);
            serv_addr.sin_port =  htons(portno);

            /* 連接server */
            if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
                error("ERROR connecting");

            recv(sockfd, buffer, sizeof(buffer), 0); /* 接收檔案名稱 */
            printf("Received file name: %s\n", buffer);
            char tmp[64] = "tcp_recv_";
            strcat(tmp, buffer);
            FILE *fpts = fopen(tmp, "w"); /* 以"recv_"+原檔案名稱儲存 */
            printf("Saved as %s\n", tmp);
            bzero(buffer, sizeof(buffer)); /* buffer初始化 */

            while(1)
            {
                n = recv(sockfd, buffer, sizeof(buffer), 0); /* 接收封包 */
                fwrite(buffer, sizeof(char), n, fpts); /* 將封包存入檔案 */
                if (n == 0)
                    break;
            }

            bzero(buffer, sizeof(buffer));
            printf("TCP 傳輸成功~~\n");
            /* 結束socket */
            close(sockfd);

            return 0;
        }
    }

    /* udp 傳輸 */
    if (strcmp(argv[1], "udp") == 0)
    {
        if (strcmp(argv[2], "send") == 0)
        {
            int sock_s, portno;

            /* 建立socket */
            if ((sock_s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
                error("socket");
            portno = atoi(argv[4]);

            /* 設定server地址 */
            struct sockaddr_in servaddr;
            memset(&servaddr, 0, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(portno);
            servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

            /* 綁定server在socket上 */
            if (bind(sock_s, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		        ERR_EXIT("bind error");

            struct sockaddr_in peeraddr;
            socklen_t peerlen;

		    char sendbuf[64] = {0};
            memset(sendbuf, 0, sizeof(sendbuf)); /* buffer初始化 */
			
			FILE *fpuc = fopen(argv[5], "r"); /* 開啟檔案 */

            peerlen = sizeof(peeraddr);
            if (recvfrom(sock_s, sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&peeraddr, &peerlen) < 0)  
                error("recv Failed..."); 
            printf("%s\n", sendbuf);

            /* 傳送檔案名稱 */
            peerlen = sizeof(peeraddr);
            if (sendto(sock_s, argv[5], sizeof(sendbuf), 0, (struct sockaddr *)&peeraddr, peerlen) < 0)  
                error("Send File Name Failed...");  
            else
                printf("Send File Name: %s\n", argv[5]);

            memset(sendbuf, 0, sizeof(sendbuf)); /* buffer初始化 */

            fseek(fpuc, 0, SEEK_END);
            int size = ftell(fpuc);
            fseek(fpuc, 0, SEEK_SET);

            int bag = (int)(size/sizeof(pack.buf) + 1); /* 封包數量 */
            int i = 1; /* 目前是第i封包 */
            int t = 0, t_late; /* 進度比例是0.05的多少倍數 */
            int s, hour, min, sec; /* 時間 */
            printf("%d bag(s)\n", bag);

            /* 傳送封包數量 */
            peerlen = sizeof(peeraddr);
            if (sendto(sock_s, (int *)&bag, sizeof(bag), 0, (struct sockaddr *)&peeraddr, peerlen) < 0)  
                error("Send bag number Failed...");

            int send_id = 0, recv_id = 0; /* 目前傳送封包與完成封包id資訊 */

            /* 取得時間 */
            time_t timep;
            struct tm *p;
            time(&timep);
            p = gmtime(&timep);

            /* 傳送ing */
			while (!feof(fpuc))
		    {
                peerlen = sizeof(peeraddr);
                struct SendPack pack;
                if (recv_id == send_id) /* 確認前一封包是否完成傳送 */
                {
                    send_id ++;
                    /* 以buffer大小讀取檔案 -> 封包 */
				    int num = fread(pack.buf, sizeof(char), sizeof(pack.buf), fpuc);
                    /* 設定封包id */
                    pack.id = send_id;
                    /* 傳送封包 */
    				sendto(sock_s, (struct SendPack *) &pack, sizeof(pack), 0, (struct sockaddr *)&peeraddr, peerlen);
                    /* 接收已完成傳送封包之id */
		            recvfrom(sock_s, (int*) &recv_id, sizeof(recv_id), 0, (struct sockaddr *)&peeraddr, &peerlen);  

                    float m = (float)((float)(i)/(float)(bag));
                    t_late = (int)(m/0.05); /* 現在傳送比例之0.05倍數 */
                    if (t_late > t) /* 每增加一個倍數print一次目前進度 */
                    {
                        int count = t_late-t, j;
                        for(j = 0; j < count; j ++)
                        {
                            s = (int)time(NULL); /* 取得現在時間 */
                            s += 8*60*60;
                            sec = s % 60;
                            min = (s/60) % 60;
                            hour = (s/3600) % 24;
                            printf("%3d%% %d/%d/%d %d:%d:%d\n", 5*(t + j + 1), (1900 + p -> tm_year), (1 + p -> tm_mon), p -> tm_mday, hour, min, sec);
                        }
                    t = t_late;
                    }
                    i ++;
                }
                else /* 傳送失敗 */
                {
                    /* 再傳送一次封包 */
                    sendto(sock_s, (struct SendPack *) &pack, sizeof(pack), 0, (struct sockaddr *)&peeraddr, peerlen);
                    /* 再接受一次封包id */
                    recvfrom(sock_s, (int *) &recv_id, sizeof(recv_id), 0, (struct sockaddr *)&peeraddr, &peerlen);  
                }
		    }
            /* 結束socket */
            close(sock_s);
		    return 0;
        }

        if (strcmp(argv[2], "recv") == 0)
        {
            int sock_c, portno;

            /* 建立socket */
		    if ((sock_c = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		        ERR_EXIT("socket error");

            /* 設定server */
			portno = atoi(argv[4]);
		    struct sockaddr_in servaddr;
		    memset(&servaddr, 0, sizeof(servaddr));
		    servaddr.sin_family = AF_INET;
		    servaddr.sin_port = htons(portno);
		    servaddr.sin_addr.s_addr = inet_addr(argv[3]);

            char recvbuf[64];

            if (sendto(sock_c, "udp start~", sizeof("udp start~"), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)  
                error("send Failed...");  

            /* 接收檔案名稱 */
            if (recvfrom(sock_c, recvbuf, sizeof(recvbuf), 0, NULL, NULL) < 0)
                error("Failed to receive file name...");

            printf("Received file name: %s\n", recvbuf);
            char tmp[64] = "udp_recv_";
            strcat(tmp, recvbuf);
            FILE *fpus = fopen(tmp, "w"); /* 以"recv_"+原檔案名稱儲存 */
            printf("Saved as %s\n", tmp);
            bzero(recvbuf, sizeof(recvbuf)); /* buffer初始化 */

            /* 接收封包數量 */
            int bag;
            if (recvfrom(sock_c, (int *)&bag, sizeof(bag), 0, NULL, NULL) < 0)
                error("Failed to receive bag number...");
            printf("%d bags\n", bag);

            int id = 1; /* 預接收之封包id */
            /* 傳送ing */
            int n;
            while (1)
            {
                struct SendPack pack;
                n = recvfrom(sock_c, (struct SendPack *) &pack, sizeof(pack), 0, NULL, NULL);
                if (pack.id == id) /* 確認現在收到的封包是現在要存的封包 */
                {
                    id ++;
                    /* 回傳剛收到的封包id以確認封包傳送成功 */
                    sendto(sock_c, (int*) &pack.id, sizeof(pack.id), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
                    /* 寫入檔案 */
                    fwrite(pack.buf, sizeof(char), sizeof(pack.buf), fpus);
                }
                else if (pack.id < id) /* 若傳來的是已傳送過的封包 回傳client現已皆收到的封包id */
                {
                    /* 回傳已收到的封包id */
                    sendto(sock_c, (int*) &pack.id, sizeof(pack.id), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
                }
                if (id > bag)
                    break;
            }

            printf("UDP 傳輸成功！！！！\n");
            /* 結束socket */
            close(sock_c);

            return 0;
        }
    }
}