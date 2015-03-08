CFLAGS  += -I. -Wall -O2
LDFLAGS += -s

TARGETS = zhttpd

all:	$(TARGETS)

zhttpd: zhttpd.o
	$(CC) $(LDFLAGS) -o $@ $^
	wc zhttpd.c
	wc -c zhttpd

clean:
	-rm -f *.o $(TARGETS)
	-rm -f www/data/* test.txt

demo:	zhttpd
	-killall zhttpd 2>/dev/null
	@echo 'Please open http://127.0.0.1:8888 in your favorite browser.'
	./zhttpd 8888 www/ /demo.html

test:	zhttpd
	-killall zhttpd 2>/dev/null
	NODEBUG=1 ./zhttpd 8888 www/ /demo.html &
	echo 'Hello World!' > text.txt
	curl -s http://127.0.0.1:8888/data/text.txt -T text.txt | grep Ok
	curl -s http://127.0.0.1:8888/data/text.txt | grep Hello
	curl -s http://127.0.0.1:8888/cgi/ls.txt | grep text.txt
	curl -s http://127.0.0.1:8888/ | grep Zepto-HTTPD
	curl -s http://127.0.0.1:8888/cgi/env.txt | grep REQUEST_URI
	curl -s http://127.0.0.1:8888/cgi/set.txt'?'text=123
	curl -s http://127.0.0.1:8888/data/text.txt | grep 123
	curl -s http://127.0.0.1:8888/data/ | grep text.txt
	curl -s http://127.0.0.1:8888/data/text.txt -X DELETE | grep Ok
	-killall zhttpd 2>/dev/null
	-rm -f text.txt

authtest: zhttpd
	-killall zhttpd 2>/dev/null
	echo -n 'user:pass'|base64
	ZAUTH='Basic dXNlcjpwYXNz' DEBUG=1 ./zhttpd 8888 www/ &
	curl -s -v http://127.0.0.1:8888/cgi/env.txt 2>&1 | grep ' 401 '
	curl -s -u 'user:pass' http://127.0.0.1:8888/cgi/env.txt | grep AUTHORIZATION
	-killall zhttpd 2>/dev/null

