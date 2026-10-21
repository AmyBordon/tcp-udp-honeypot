#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "monitor_udp.h"
#include "monitor_util.h"

#define MAX_IP 5000
#define WINDOW 10              
#define QUIET 30               
#define UPDATE_EVERY 15        
#define MIN_PACKETS 100

struct entry_udp {
        char ip[16];
        char last_hex[129];
        char last_type[32];     
        int packets;            // packets in currrent window
        int same_payload;
        int diff_payload;
        int active;             
        int attack_count;       // packets in episode
        int seen_cycle;         // lines received from last check
        time_t window_start;
        time_t attack_start;
        time_t last_update;
        time_t last_seen;
};
static struct entry_udp tab_udp[MAX_IP];

static struct entry_udp *find_udp(char *ip){
        int i;
        for(i = 0; i < MAX_IP; i++){
                if(strcmp(tab_udp[i].ip, ip) == 0)
                        return &tab_udp[i];
                if(tab_udp[i].ip[0] == 0){
                        strcpy(tab_udp[i].ip, ip);
                        return &tab_udp[i];
                }
        }
        return NULL;
}

static int hexbyte(char *hex, int n){
        char t[3] = {hex[2*n], hex[2*n+1], 0};
        unsigned int v = 0;
        sscanf(t, "%x", &v);
        return v;
}

static const char *classify(char *hex){
        int n = strlen(hex) / 2;     // nr bytes
        int p, qtype;

        // NTP monlist: mod 7 si cod de cerere 0x2a
        if(n >= 4 && (hexbyte(hex, 0) & 0x07) == 7 && hexbyte(hex, 3) == 0x2a)
                return "NTP monlist";

        // DNS ANY: cerere (QR = 0), o intrebare, tip 255
        if(n >= 17 && (hexbyte(hex, 2) & 0x80) == 0 && hexbyte(hex, 5) == 1){
                p = 12;
                while(p < n && hexbyte(hex, p) != 0 && hexbyte(hex, p) <= 63)
                        p += hexbyte(hex, p) + 1;
                if(p + 2 < n && hexbyte(hex, p) == 0){
                        qtype = hexbyte(hex, p + 1) * 256 + hexbyte(hex, p + 2);
                        if(qtype == 255)
                                return "DNS ANY";
                }
        }
        return "unknown";
}

static void end_episode(struct entry_udp *e){
        char msg[200];
        snprintf(msg, sizeof(msg), "%s ended for IP: %s (%ld s, %d packets)",
                 e->last_type, e->ip, (long)(e->last_seen - e->attack_start), e->attack_count);
        emit("ENDED", msg);

        e->active = 0;
        e->last_type[0] = 0;
}

void process_udp(char *line){
        char ip[20], hex[129], label[32], msg[200];
        const char *type;
        struct entry_udp *e;
        struct tm tm = {0};
        time_t ts;
        int was_active;

        if(strptime(line, "%Y-%m-%d %H:%M:%S", &tm) == NULL)
                return;
        tm.tm_isdst = -1;
        ts = mktime(&tm);

        if(sscanf(line, "%*s %*s | %19s | %*d | %128s", ip, hex) < 2)
                return;
        e = find_udp(ip);
        if(e == NULL)
                return;

        // new window
        if(e->packets == 0 || ts - e->window_start > WINDOW){
                e->packets = 0;
                e->same_payload = 0;
                e->diff_payload = 0;
                e->window_start = ts;
        }
        // pauza lunga: inchidem episodul daca nu l-a inchis deja check_udp
        if(e->active && ts - e->last_seen > QUIET)
                end_episode(e);

        if(e->packets > 0){
                if(strcmp(e->last_hex, hex) == 0)
		       	e->same_payload++;
                else 
			e->diff_payload++;
        }
        e->packets++;
        strcpy(e->last_hex, hex);
        if(e->active)
                e->attack_count++;

        if(e->packets >= MIN_PACKETS){
                type = classify(e->last_hex);

                if(e->same_payload > e->diff_payload && strcmp(type, "unknown") != 0)
                        snprintf(label, sizeof(label), "%s amplification", type);
                else
                        snprintf(label, sizeof(label), "flood");

                if(strcmp(e->last_type, label) != 0){
                        was_active = e->active;
                        if(was_active)
                                end_episode(e); // tipul s-a schimbat
                        strcpy(e->last_type, label);
                        e->active = 1;
                        e->attack_start = ts;
                        e->last_update = ts;
                        e->attack_count = was_active ? 1 : e->packets;
                      

                        snprintf(msg, sizeof(msg), "%s detected by IP: %s", label, ip);
                        emit("ALERT", msg);
                }
        }
        e->last_seen = ts;
	e->seen_cycle++;
}


void check_udp(void){
        int i;
        char msg[200];
        time_t now = time(NULL);

        for(i = 0; i < MAX_IP && tab_udp[i].ip[0] != 0; i++){
                struct entry_udp *e = &tab_udp[i];
	
		int fresh = e->seen_cycle;
                e->seen_cycle = 0;              // resetam la fiecare ciclu

                if(!e->active)
                        continue;
                if(now - e->last_seen > QUIET){
                        end_episode(e);
                } else if(now - e->last_update >= UPDATE_EVERY
                          && fresh>0){      
                        snprintf(msg, sizeof(msg), "%s still active from IP: %s (%ld s, %d packets)",
                                 e->last_type, e->ip, (long)(e->last_seen - e->attack_start), e->attack_count);
                        emit("UPDATE", msg);
                        e->last_update = now;
                      
                }
        }
}
