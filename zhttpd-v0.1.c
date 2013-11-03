#include <stdio.h>      //  Zepto-HTTPD v0.1 - (c)2010 Matthias Goebl
#include <stdlib.h>     //  Usage: zhttpd [ PORT(default:8888) [ HTMLROOT(default:working dir) ] ]
#include <string.h>     //  
#include <errno.h>      //  Zepto- (symbol z) is a prefix in the metric system denoting *10^-21.
#include <fcntl.h>      //  This is a very minimalistic HTTP/1.0 partitial implementation in just
#include <netdb.h>      //  99 lines of code (99 characters wide) written in C for any POSIX OS.
#include <sys/ioctl.h>  //  Is results in a < 10kB executable that only depends on libc.
#include <sys/socket.h> //  It internally serves /date /counter /counter/increment, demonstrating
#include <sys/types.h>  //  a slim http interface. On /index.html it provides an ajax javascript
#include <time.h>       //  application using the latter. Additionally it serves text and html
#include <unistd.h>     //  files from the file system. It supports simple .cgi scripts.

#define DIE(msg...) do {fprintf(stderr,msg);fprintf(stderr," - terminated!\n");exit(1);} while (0)
#define DBG(msg...) do {if(!getenv("QUIET")){fprintf(stderr,msg);fprintf(stderr,"\n");}} while (0)
#define HTML "<html><body><span id='counter'>000</span><script type='text/javascript'>\n"\
" window.onload = function() { http = new XMLHttpRequest(); update(); }; "\
" function update() { http.open('GET', '/counter', true); http.onreadystatechange = function (){"\
"    if (http.readyState == 4) { document.getElementById('counter').firstChild.data ="\
"       http.responseText; }; }; http.send(null); setTimeout(update,250); }; "\
" function increment() {var cmd=new XMLHttpRequest(); cmd.open('GET','/counter/increment',true);"\
"   cmd.send(null); }\n"\
"</script><input type=button onclick='increment()' value='+'></body></html>"
int counter = 1;

void handle_client (FILE *fd) {
 char method[17], request[8193], header[8192], response[8192] = "Error!\r\n";
 int ret, status = 503; FILE * pgfd = NULL;
 ret = fscanf (fd, "%16[^ \t]%*[ \t] %8192[^ \t]%*[^\n]\n", method, request);
 if (ret == 2) {
   DBG ("Request: '%s' '%s'.", method, request);
   setenv ("REQUEST", request, 1);  // $REQUEST has the full request for out pseudo-"CGI"s
   if (strchr (request, '?')) *strchr (request, '?') = '\0';
   while ( fgets (header, sizeof (header), fd) != NULL ) {
     if (strlen (header) && header[ strlen(header)-1 ] == '\n') header[ strlen(header)-1 ] = '\0';
     if (strlen (header) && header[ strlen(header)-1 ] == '\r') header[ strlen(header)-1 ] = '\0';
     if (strlen (header) == 0) break;
     DBG ("Header: %s.", header);
   }
   if (strcasecmp (method, "GET") == 0) {
     status = 200;
     if (strcmp (request, "/") == 0) strcpy (request, "/index.html");
     if (strcmp (request, "/date") == 0) {
       snprintf (response, sizeof (response), "%lli\r\n", (long long int) time (NULL));
     } else if (strcmp (request, "/counter") == 0) {
       snprintf (response, sizeof (response), "%i\r\n", counter);
     } else if (strcmp (request, "/counter/increment") == 0) {
       counter++; snprintf (response, sizeof (response), "%i\r\n", counter);
     } else if (strcmp (request, "/index.html") == 0) {
       snprintf (response, sizeof (response), "%s", HTML);
     } else if (strstr (request, "..") == NULL && request[0] == '/' && request[1] != '/') {
       pgfd = strstr (request, ".cgi") ? popen (&request[1], "r") : fopen (&request[1], "r");
       if (pgfd != NULL) response[0] = '\0'; else status = 404;
     } else {
       status = 404;
     }
   }
 }
 DBG ("Reply: Status %i with Content:\n%s", status, response);
 fprintf (fd, "HTTP/1.0 %i %s\r\nServer: zeptohttpd/0.1\r\nContent-Type: text/%s\r\n\r\n%s",
  status, status == 200 ? "OK" : "Error", strstr (request, ".htm") ? "html" : "plain", response);
 if (pgfd != NULL) { // ".cgi" anywhere in the request opens a pipe to a "CGI" executable
   while ((ret = fread (response, 1, sizeof (response), pgfd)) > 0)
     fwrite (response, 1, ret, fd);
   strstr (request, ".cgi") ? pclose (pgfd) : fclose (pgfd);
 }
}

int main (int argc, char **argv) {
 int server_sock, client_sock, yes = 1, port = argc > 1 ? atoi (argv[1]) : 8888;
 struct sockaddr_in server_addr, client_addr; socklen_t addr_len;
 char client_info[NI_MAXHOST]; FILE *client_fd;
 if (argc > 2) if (chdir (argv[2]) < 0) DIE (strerror (errno));

 server_sock = socket (AF_INET, SOCK_STREAM, 0);
 if (server_sock < 0) DIE (strerror (errno));
 server_addr.sin_family = AF_INET;
 server_addr.sin_addr.s_addr = htonl (INADDR_ANY);
 server_addr.sin_port = htons (port);
 if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))<0) DIE(strerror(errno));
 if(bind(server_sock,(struct sockaddr*)&server_addr,sizeof(server_addr))<0) DIE(strerror(errno));
 listen (server_sock, 10);

 while (1) {
   DBG ("\n\n*** Zepto-HTTPD v0.1 - (c)2010 Matthias Goebl ***\nListening at TCP Port %i.",port);
   addr_len = sizeof (client_addr);
   client_sock = accept (server_sock, (struct sockaddr *) &client_addr, &addr_len);
   if (client_sock == -1) continue;
   getnameinfo ((struct sockaddr *) &client_addr, sizeof (client_addr), client_info,
    sizeof (client_info), NULL, 0, NI_NUMERICHOST);
   DBG ("Connect from %s.", client_info);
   client_fd = fdopen (client_sock, "r+");
   handle_client (client_fd);
   fclose (client_fd);
   DBG ("Disconnected from %s.", client_info);
 }
 return (0);
}
