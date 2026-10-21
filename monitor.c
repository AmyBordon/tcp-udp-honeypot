#include <stdio.h>
#include <unistd.h>
#include "monitor_tcp.h"
#include "monitor_udp.h"

void monitor(){
        char line[300];
        FILE *ftcp = fopen("tcp/log.txt", "a+");
        FILE *fudp = fopen("udp/log.txt", "a+");

        if(ftcp == NULL || fudp == NULL){
                perror("[Monitor] log");
               return;
        }
        while(1){
                while(fgets(line, sizeof(line), ftcp))
                        process_tcp(line);
                clearerr(ftcp); //reset EOF

                while(fgets(line, sizeof(line), fudp))
                        process_udp(line);
                clearerr(fudp);

                check_tcp();
                check_udp();
                sleep(10);
        }
}

int main(){
        monitor();
        return 0;
}
