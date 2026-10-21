#include <stdio.h>
#include <string.h>
#include <time.h>
#include "monitor_util.h"

#define ALERT_FILE "alerts.txt"
#define SHOW_UPDATE 1

#define RED    "\033[31m"
#define GREEN  "\033[32m"
#define ORANGE "\033[38;5;208m"
#define RESET  "\033[0m"

static const char *color_of(const char *tag){
        if(strcmp(tag, "ALERT") == 0)  return RED;
        if(strcmp(tag, "UPDATE") == 0) return GREEN;
        if(strcmp(tag, "ENDED") == 0)  return ORANGE;
        return RESET;
}

void emit(const char *tag, const char *msg){
        char when[30], line[400];
        time_t t = time(NULL);
        FILE *f;

        strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", localtime(&t));
        snprintf(line, sizeof(line), "[%s] %s | %s\n", tag, when, msg);

        
        if(strcmp(tag, "UPDATE") != 0 || SHOW_UPDATE){
                printf("%s[%s]%s %s | %s\n", color_of(tag), tag, RESET, when, msg);
                fflush(stdout);
        }
      
        f = fopen(ALERT_FILE, "a");
        if(f != NULL){
                fputs(line, f);
                fclose(f);
        }
}
