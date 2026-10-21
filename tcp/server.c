#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

int sfd, csfd, r;
struct sockaddr_in soc, csoc;


void read_client(char *prompt, char *dest){
       char c;
       int n = 0;

       send(csfd, prompt, strlen(prompt), 0);
       while(1){
	    r = recv(csfd, &c, 1, 0);
            if(r <= 0){
                close(csfd);
                exit(0);
            }
            if(c == '\n')
                   break;
            if(c != '\r' && n < 49)
                    dest[n++] = c;
        }
        dest[n] = 0;
} 

void client(){
	alarm(60); //timeout
	char user[50], pass[50], ts[30];
        int i;
        time_t t;
        FILE *f;
	send(csfd, "Ubuntu 20.04.6 LTS\r\n", 20, 0);
	for(i = 0; i < 3; i++){
                read_client("login: ", user);
                read_client("password: ", pass);

                t = time(NULL);
                strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&t));

                f = fopen("log.txt", "a");
		if(f != NULL){
			fprintf(f, "%s | %s | %s | %s\n", ts, inet_ntoa(csoc.sin_addr), user, pass);
               		fclose(f);
		}
                send(csfd, "Incorrect login\n", 16, 0);	
	}
        close(csfd);
}

int main(){
	
        int clen = sizeof(struct sockaddr_in);
        signal(SIGCHLD, SIG_IGN); //for the kernet to clean the finished child processes
        sfd = socket(AF_INET, SOCK_STREAM, 0);

        soc.sin_family = AF_INET;
        soc.sin_port = htons(2323);
        soc.sin_addr.s_addr = inet_addr("0.0.0.0");
	
	int opt = 1; //to free the port right after
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if(bind(sfd, (struct sockaddr*)&soc, sizeof(struct sockaddr_in)) < 0){
                perror("[Server] Bind error");
                return 1;
        }
        listen(sfd, 5);
	
        while(1){
                csfd = accept(sfd, (struct sockaddr*)&csoc, &clen);
                if(fork() == 0){
                        close(sfd);
                        client();
                        exit(0);
                }
                close(csfd);
        }

        close(sfd);
}
