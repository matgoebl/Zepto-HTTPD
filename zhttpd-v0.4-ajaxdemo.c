#include <stdio.h>      //  Zepto-HTTPD v0.4-AJAXDEMO - (c)2010 Matthias Goebl
#include <stdlib.h>     //  Usage: zhttpd [ PORT(default:8888) [ HTMLROOT(default:working dir) ] ]
#include <string.h>     //  Set environment DEMO=1 for the AJAX demo.
#include <errno.h>      //  Zepto- (symbol z) is a prefix in the metric system denoting *10^-21.
#include <fcntl.h>      //  This is a very minimalistic HTTP/1.0 partitial implementation in just
#include <netdb.h>      //  99 lines of code (99 characters wide) written in C for any POSIX OS.
#include <sys/ioctl.h>  //  Is results in a < 10kB executable that only depends on libc.
#include <sys/socket.h> //  It internally serves /DEMO/{date,counter,counter/inc} demonstrating
#include <sys/types.h>  //  a slim http interface. On /DEMO.html it provides an ajax javascript
#include <sys/wait.h>   //  application using the latter. Additionally it serves text and html
#include <time.h>       //  files from the file system. It even supports simple ".cgi"-scripts.
#include <unistd.h>     //
#define DIE(msg...) do {fprintf(stderr,msg);fprintf(stderr," - terminated!\n");exit(1);} while (0)
#define DBG(msg...) do {if(getenv("DEBUG")){fprintf(stderr,msg);fprintf(stderr,"\n");}} while (0)
#define HTML "<html><body><span id='counter'>000</span><script type='text/javascript'>\n"\
" window.onload = function() { http = new XMLHttpRequest(); update(); }; "\
" function update() { http.open('GET','/DEMO/counter',true); http.onreadystatechange=function(){"\
"    if (http.readyState == 4) { document.getElementById('counter').firstChild.data ="\
"       http.responseText; }; }; http.send(null); setTimeout(update,250); }; "\
" function increment() {var cmd=new XMLHttpRequest(); cmd.open('GET','/DEMO/counter/inc',true);"\
"   cmd.send(null); }\n"\
"</script><input type=button onclick='increment()' value='+'></body></html>"
int counter = 1;  // only usefull without FORKing

void handle_client (FILE *fd, char *remote_addr) {
 char method[17], request[8193], *query, header[8192], response[8192]; FILE * pgfd = NULL;
 int ret, status = 503; sprintf(response,"Error!\r\n");
 ret = fscanf (fd, "%16[^ \t]%*[ \t] %8192[^ \t]%*[^\n]\n", method, request);
 if (ret == 2) {
   DBG ("Request from %s: '%s' '%s'.", remote_addr, method, request);
   setenv ("REMOTE_ADDR", remote_addr, 1); setenv ("REQUEST_URI", request, 1);  // CGI environment
   query = strchr (request, '?'); if (query) { *query = '\0'; query++; } else query="";
   setenv ("PATH_INFO", request, 1); setenv ("QUERY_STRING", query, 1);
   while ( fgets (header, sizeof (header), fd) != NULL ) {
     if (strlen (header) && header[ strlen(header)-1 ] == '\n') header[ strlen(header)-1 ] = '\0';
     if (strlen (header) && header[ strlen(header)-1 ] == '\r') header[ strlen(header)-1 ] = '\0';
     if (strlen (header) == 0) break; DBG ("Header: %s.", header);
   }
   if (strcasecmp (method, "GET") == 0) {
     status = 200;
     if (strcmp (request, "/") == 0) strcpy (request, getenv("DEMO")?"/DEMO.html":"/index.html");
     if (strcmp (request, "/DEMO/date") == 0) {
       snprintf (response, sizeof (response), "%lli\r\n", (long long int) time (NULL));
     } else if (strcmp (request, "/DEMO/counter") == 0) {
       snprintf (response, sizeof (response), "%i\r\n", counter);
     } else if (strcmp (request, "/DEMO/counter/inc") == 0) {
       counter++; snprintf (response, sizeof (response), "%i\r\n", counter);
     } else if (strcmp (request, "/DEMO.html") == 0) {
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
 if (!strstr (request, ".nph"))
  fprintf (fd, "HTTP/1.0 %i %s\r\nServer: zeptohttpd/0.3\r\nContent-Type: text/%s\r\n\r\n%s",
   status, status == 200 ? "OK" : "Error", strstr (request, ".htm") ? "html" : "plain", response);
 if (pgfd != NULL) { // ".cgi" anywhere in the request opens a pipe to a "CGI" executable
   while ((ret = fread (response, 1, sizeof (response), pgfd)) > 0)
     fwrite (response, 1, ret, fd);
   strstr (request, ".cgi") ? pclose (pgfd) : fclose (pgfd);
 }
 printf ("%s %03i %s %s %s\n", remote_addr, status, method, request, query);
}

int main (int argc, char **argv) {
 int server_sock, client_sock, pid, yes = 1, port = argc > 1 ? atoi (argv[1]) : 8888;
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
   DBG ("\n\n*** Zepto-HTTPD v0.3 - (c)2011 Matthias Goebl ***\nListening at TCP Port %i.",port);
   addr_len = sizeof (client_addr);
   client_sock = accept (server_sock, (struct sockaddr *) &client_addr, &addr_len);
   if (client_sock == -1) continue;
   getnameinfo ((struct sockaddr *) &client_addr, sizeof (client_addr), client_info,
    sizeof (client_info), NULL, 0, NI_NUMERICHOST);
   client_fd = fdopen (client_sock, "r+"); DBG ("Connect from %s.", client_info);
   if (getenv ("DEMO")) handle_client (client_fd, client_info);
     else { if (!(pid=fork())) { if (!fork ()) handle_client (client_fd, client_info); exit (0); }
            waitpid(pid, NULL, 0); } // after a double-fork init handles our childs
   fclose (client_fd); DBG ("Disconnected from %s.", client_info);
 }
 return (0);
}
