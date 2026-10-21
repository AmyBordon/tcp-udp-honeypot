#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>


int sfd, r;
struct sockaddr_in soc, csoc;

int main(){
        socklen_t clen = sizeof(struct sockaddr_in);
        sfd = socket(AF_INET, SOCK_DGRAM, 0);

        soc.sin_family = AF_INET;
        soc.sin_port = htons(5353); //53 - DNS
        soc.sin_addr.s_addr = inet_addr("0.0.0.0");

        if(bind(sfd, (struct sockaddr*)&soc, sizeof(struct sockaddr_in)) < 0){
                perror("[Server] Bind error");
                return 1;
        }

        char buf[2048], ts[30];
        time_t t;
        FILE *f;
	f = fopen("log.txt", "a");
        while(1){
                r = recvfrom(sfd, buf, sizeof(buf)-1, 0, (struct sockaddr*)&csoc, &clen);

                if(r < 0)
                        continue;
                buf[r] = 0;

                t = time(NULL);
                strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&t));
                 
                if(f != NULL){
                        char hex[129]; // 64 bytes, first chunck of hex
                        int i;
                        for(i = 0; i < r && i < 64; i++)
                                sprintf(hex + i*2, "%02x", (unsigned char)buf[i]); // writes buf[i] as 2 hex digits
                        hex[i*2] = 0;           
                        fprintf(f, "%s | %s | %d | %s\n", ts, inet_ntoa(csoc.sin_addr), r , hex);

                   
			fflush(f);
                }
                clen = sizeof(struct sockaddr_in);
        }
}

