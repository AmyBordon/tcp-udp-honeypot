all: monitor tcp/server udp/server

monitor: monitor.c monitor_tcp.c monitor_udp.c monitor_util.c monitor_tcp.h monitor_udp.h monitor_util.h
	gcc monitor.c monitor_tcp.c monitor_udp.c monitor_util.c -o monitor

tcp/server: tcp/server.c
	gcc tcp/server.c -o tcp/server

udp/server: udp/server.c
	gcc udp/server.c -o udp/server

run: all
	cd tcp && ./server &
	cd udp && ./server &
	./monitor

stop:
	-pkill -x server
	-pkill -x monitor

fresh: stop
	rm -f tcp/log.txt udp/log.txt alerts.txt	

clean:
	rm -f monitor tcp/server udp/server alerts.txt
