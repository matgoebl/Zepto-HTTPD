#include <stdio.h>      //  Zepto-HTTPD v0.7 - (c)2010-2013 Matthias Goebl
#include <stdlib.h>     //  Usage: zhttpd [ PORT [ HTMLROOT [ INDEXPAGE ] ] ]
#include <string.h>     //  Default: zhttpd 8888 . /index.html
#include <errno.h>      //  Zepto- (symbol z) is a prefix in the metric system denoting *10^-21.
#include <fcntl.h>      //  This is a very minimalistic HTTP/1.0 partitial implementation in just
#include <netdb.h>      //  100 lines of code (100 characters wide) written in C for any POSIX OS.
#include <sys/ioctl.h>  //  Is results in a < 10kB executable that only depends on libc.
#include <sys/socket.h> //  Licensed under the Apache License, Version 2.0.
#include <sys/types.h>  //  Version 0.5 also implements PUT&DELETE(??d..), but lacks the AJAX demo.
#include <sys/wait.h>   //  It serves text and html files from the file system. 
#include <unistd.h>     //  It supports simple cgi-scripts (also nph).
#include <ctype.h>      //  Tricks: Double-Fork, Env-as-AssocArray, HTTP/1.0, 
#include <arpa/inet.h>  //  Features: CGI+params, put/delete, auth, hmtl/txt/jpg/bin, log,
#define DIE(msg...) do {fprintf(stderr,msg);fprintf(stderr," - terminated!\n");exit(1);} while (0)
#define DBG(msg...) do {if(getenv("DEBUG")){fprintf(stderr,msg);fprintf(stderr,"\n");}} while (0)
char *indexpage="/index.html";
void handle_client (FILE *fd, char *remote_addr) {
 char method[17], request[8193], *query="\0", envbuf[8192],response[8192],*p,*q,*r;FILE *pgfd=NULL;
 int ret, size, status = 503; sprintf(response,"HTTP Error\r\n");
 ret = fscanf (fd, "%16[^ \t]%*[ \t] %8192[^ \t]%*[^\n]\n", method, request);
 if (ret == 2) {
  DBG ("Request from %s: '%s' '%s'.", remote_addr, method, request);
  setenv ("REMOTE_ADDR", remote_addr, 1); setenv ("REQUEST_URI", request, 1);  // CGI environment
  query = strchr (request, '?'); if (query) { *query = '\0'; query++; } else query="";
  setenv ("PATH_INFO", request, 1); setenv ("QUERY_STRING", query, 1);
  strcpy(envbuf,"ZHTTP_");
  while ( fgets (&envbuf[6], sizeof (envbuf)-6, fd) != NULL ) { // put http header into environment
   p=strchr(envbuf,'\r'); if(p) *p='\0'; if (strlen (envbuf) <= 6) break;
   p=envbuf; while(*p!='\0' && *p!=':') {*p=toupper(*p);*p=*p>='A' && *p<='Z'?*p:'_';p++;}
   p=strchr(envbuf,':'); if(p) {*p='\0'; if(*(++p)==' ') p++; setenv(envbuf,p,1); }
   DBG ("Header: %s=%s.", envbuf,p);
  }
  strcpy(envbuf,"CGIPARAM_"); p=query; // put CGI args into environment
  while ( *p!='\0' ) {
   q=&envbuf[9]; // yes, this is too compact and therefore ugly and hard to read
   while(*p!='\0'&&*p!='='&&*p!='&')
     {*q=toupper(*p);*q=(*q>='A'&&*q<='Z')||(*q>='0'&&*q<='9')?*q:'_';p++;q++;}
   if(*p=='\0') break;
   *q++='\0'; p++; r=q; while ( *p!='\0' && *p!='&' ) {*q++=*p++;}
   p++; *q++='\0'; setenv(envbuf,r,1);
  }
  if(getenv("ZAUTH"))
   if(!getenv("ZHTTP_AUTHORIZATION")||strcmp(getenv("ZAUTH"),getenv("ZHTTP_AUTHORIZATION"))!=0)
    {fprintf(fd,"HTTP/1.0 401 No Auth\r\nWWW-Authenticate: Basic realm=\"auth\"\r\n\r\n");return;}
  if (strcmp (request, "/") == 0) strcpy (request, indexpage);
  if (strcasecmp (query, "?METHOD=DELETE") == 0) {strcpy(method,"DELETE");}
  if (strstr (request, "..")==NULL && request[0]=='/' && request[1]!='/' && request[1]!='.' ) {
   if (strcasecmp (method, "GET") == 0) {
    pgfd = strncmp (request,"/cgi/",5) == 0 ? popen (&request[1], "r") : fopen (&request[1], "r");
    status = 404; if (pgfd != NULL) { response[0] = '\0'; status = 200; }
   } else if (strncmp (request, "/data/", 6) == 0 ) {
    if((!strcasecmp(method,"PUT")||!strcasecmp(method,"POST"))&&getenv("ZHTTP_CONTENT_LENGTH")){
     pgfd = fopen (&request[1], "w"); size=atoi(getenv("ZHTTP_CONTENT_LENGTH"));
     if (pgfd != NULL) {
      while(size--)
       { ret=fgetc(fd); if(ret==EOF) break; ret=fputc(ret,pgfd); if(ret==EOF) break; }
      if(++size==0) { sprintf(response,"Ok.\r\n"); status = 200; }
      fclose(pgfd); pgfd=NULL; system("./.put_handler &");
     }
    } else if (strcasecmp (method, "DELETE") == 0 ) {
      ret = unlink (&request[1]);
      if (ret == 0) { sprintf(response,"Ok.\r\n"); status = 200; }
 }}}}
 DBG ("Reply: Status %i with Content:\n%s", status, response);
 if (!strstr (request, ".nph") && fd != NULL)
  fprintf (fd, "HTTP/1.0 %i %s\r\nServer: zeptohttpd/0.7\r\nContent-Type: %s\r\n\r\n%s",
   status, status == 200 ? "OK" : "Error", strstr (request, ".htm") ? "text/html" :
                strstr (request, ".txt") ? "text/plain" : strstr (request, ".jpg") ? "image/jpeg" :
                "application/octet-stream", response);
 if (pgfd != NULL) { // we have an open pipe to a cgi executable
   while ((ret = fread (response, 1, sizeof (response), pgfd)) > 0)
     fwrite (response, 1, ret, fd);
   strncmp (request,"/cgi/",5) == 0 ? pclose (pgfd) : fclose (pgfd);
 }
 printf ("%s %03i %s %s %s\n", remote_addr, status, method, request, query);
}
int main (int argc, char **argv) {
 struct sockaddr_in server_addr, client_addr; socklen_t addr_len;
 char client_info[NI_MAXHOST]; FILE *client_fd;
 int server_sock, client_sock, pid, yes = 1, port = argc >= 2 ? atoi (argv[1]) : 8888;
 if (argc >= 3) if (chdir (argv[2]) < 0) DIE (strerror (errno)); if (argc >= 4) indexpage=argv[3];
 server_sock = socket (AF_INET, SOCK_STREAM, 0); if (server_sock < 0) DIE (strerror (errno));
 server_addr.sin_family = AF_INET; server_addr.sin_addr.s_addr = htonl (INADDR_ANY);
 server_addr.sin_port = htons (port);
 if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))<0) DIE(strerror(errno));
 if(bind(server_sock,(struct sockaddr*)&server_addr,sizeof(server_addr))<0) DIE(strerror(errno));
 listen (server_sock, 10);
 while (1) {
   DBG ("\n\n*** Zepto-HTTPD v0.7 - (c)2010-13 Matthias Goebl ***\nListening at TCP Port %i.",port);
   addr_len = sizeof (client_addr);
   client_sock = accept (server_sock, (struct sockaddr *) &client_addr, &addr_len);
   if (client_sock == -1) continue;
   getnameinfo ((struct sockaddr *) &client_addr, sizeof (client_addr), client_info,
    sizeof (client_info), NULL, 0, NI_NUMERICHOST);
   client_fd = fdopen (client_sock, "r+"); DBG ("Connect from %s.", client_info);
   if (!(pid=fork())) { if (!fork ()) handle_client (client_fd, client_info); exit (0); }
   waitpid(pid, NULL, 0); // after a double-fork init handles our childs
   fclose (client_fd); DBG ("Disconnected from %s.", client_info);
}}
