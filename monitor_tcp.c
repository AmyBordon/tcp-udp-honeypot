#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "monitor_tcp.h"
#include "monitor_util.h"

#define MAX_IP 5000
#define QUIET 30              // quiet seconds = ended attack
#define UPDATE_EVERY 15

struct entry_tcp {
        char ip[16];
        char last_user[50];
        char last_type[32];     // type of current episode
        int tries;
        int same_user;
        int diff_user;
        int active;             // 1 = open episode
        int attack_count;       // tries in this episode
        int seen_cycle;
        time_t attack_start;
        time_t last_update;
        time_t last_seen;
};

static struct entry_tcp tab_tcp[MAX_IP];

static struct entry_tcp *find_tcp(char *ip){
        int i;
        for(i = 0; i < MAX_IP; i++){
                if(strcmp(tab_tcp[i].ip, ip) == 0)
                        return &tab_tcp[i];
                if(tab_tcp[i].ip[0] == 0){
                        strcpy(tab_tcp[i].ip, ip);
                        return &tab_tcp[i];
                }
        }
        return NULL;
}

static void end_episode(struct entry_tcp *e){
        char msg[200];
        snprintf(msg, sizeof(msg), "%s ended for IP: %s (%ld s, %d attempts)",
                 e->last_type, e->ip, (long)(e->last_seen - e->attack_start), e->attack_count);
        emit("ENDED", msg);

        e->active = 0;
        e->last_type[0] = 0;
}

void process_tcp(char *line){
        char ip[20], user[50], msg[200];
        const char *label;
        struct entry_tcp *e;
        struct tm tm = {0};
        time_t ts;
        int was_active;

        if(strptime(line, "%Y-%m-%d %H:%M:%S", &tm) == NULL)
                return;
        tm.tm_isdst = -1;
        ts = mktime(&tm);

        if(sscanf(line, "%*s %*s | %19s | %49s", ip, user) < 2)
                return;
        e = find_tcp(ip);
        if(e == NULL)
                return;

        // long pause: shut down the episode and reset the counters
        if(e->tries > 0 && ts - e->last_seen > QUIET){
                if(e->active)
                        end_episode(e);
                e->tries = 0;
                e->same_user = 0;
                e->diff_user = 0;
        }

        if(e->tries > 0){
                if(strcmp(e->last_user, user) == 0) e->same_user++;
                else e->diff_user++;
        }
        e->tries++;
        strcpy(e->last_user, user);
        if(e->active)
                e->attack_count++;

        if(e->tries >= 10){
                label = (e->same_user >= e->diff_user) ? "brute-force" : "credential stuffing";

                if(strcmp(e->last_type, label) != 0){
                        was_active = e->active;
                        if(was_active)
                                end_episode(e);         // tipul s-a schimbat
                        strcpy(e->last_type, label);
                        e->active = 1;
                        e->attack_start = ts;
                        e->last_update = ts;
                        e->attack_count = was_active ? 1 : e->tries;
                      

                        snprintf(msg, sizeof(msg), "%s detected by IP: %s", label, ip);
                        emit("ALERT", msg);
                }
        }
        e->last_seen = ts;
}

// called at every cycle: UPDATE and ENDED
void check_tcp(void){
        int i;
        char msg[200];
        time_t now = time(NULL);

        for(i = 0; i < MAX_IP && tab_tcp[i].ip[0] != 0; i++){
                struct entry_tcp *e = &tab_tcp[i];
   		int fresh = e->seen_cycle;
                e->seen_cycle = 0;              // recet after every cycle

                if(!e->active)
                        continue;
                if(now - e->last_seen > QUIET){
                        end_episode(e);
                } else if(now - e->last_update >= UPDATE_EVERY
                          && fresh>0){     
                        snprintf(msg, sizeof(msg), "%s still active from IP: %s (%ld s, %d attempts)",
                                 e->last_type, e->ip, (long)(e->last_seen - e->attack_start), e->attack_count);
                        emit("UPDATE", msg);
                        e->last_update = now;
                        
                }
        }
}
