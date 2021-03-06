#include <stdio.h>      //  Zepto-HTTPD v0.9 - (c)2010-2015 Matthias.Goebl*goebl.net
#include <stdlib.h>     //  Usage: zhttpd [ PORT [ HTMLROOT [ INDEXPAGE [ DATADIR ]] ] ]
#include <string.h>     //  Defaults: zhttpd 8888 . /index.html /data/
#include <errno.h>      //  Zepto- (symbol z) is a prefix in the metric system denoting *10^-21.
#include <fcntl.h>      //  This is a very minimalistic HTTP/1.0 partitial implementation in just
#include <netdb.h>      //  100 lines of code (100 characters wide) written in C for any POSIX OS.
#include <sys/ioctl.h>  //  Is results in a < 10kB executable that only depends on libc.
#include <sys/socket.h> //  Licensed under the Apache License, Version 2.0.
#include <sys/types.h>  //  Version 0.5 also implements PUT&DELETE(??d..), but lacks the AJAX demo.
#include <sys/wait.h>   //  It serves text and html files from the file system. 
#include <unistd.h>     //  It supports simple cgi-scripts (also nph).
#include <ctype.h>      //  Tricks: Only HTTP/1.0, Env-as-AssocArray,
#include <arpa/inet.h>  //  Features: CGI+params, put/delete, auth, hmtl/txt/jpg/bin, log, dirlist
#include <dirent.h>     //
#include <sys/stat.h>   //
#define DIE(msg...) do {fprintf(stderr,msg);fprintf(stderr," - terminated!\n");exit(1);} while (0)
#define DBG(msg...) do {if(getenv("DEBUG")){fprintf(stderr,msg);fprintf(stderr,"\n");}} while (0)
char *indexpage="/index.html", *datadir="/data/";
void handle_client (FILE *fd, int fh, char *remote_addr) {
 char method[17],request[8193],*query="\0",envbuf[8192],response[8192],*p,*q,*r,fb[99];
 FILE *pgfd=NULL; int ret, size, status = 503; sprintf(response,"HTTP Error\r\n");
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
   DBG ("Header: %s=%s.", envbuf,p); }
  strcpy(envbuf,"CGIPARAM_"); p=query; // put CGI args into environment
  while ( *p!='\0' ) {
   q=&envbuf[9]; // yes, this is too compact and therefore ugly and hard to read
   while(*p!='\0'&&*p!='='&&*p!='&')
     {*q=toupper(*p);*q=(*q>='A'&&*q<='Z')||(*q>='0'&&*q<='9')?*q:'_';p++;q++;}
   if(*p=='\0') {break;} *q++='\0'; p++; r=q;
   while (*p!='\0' && *p!='&') { if(*p=='%') {char h[3]={p[1],p[2],0};p+=3;*q++=strtol(h,NULL,16);}
    else if(*p=='+') {*q++=' ';p++;} else *q++=*p++;} p++; *q++='\0'; setenv(envbuf,r,1); }
  if(getenv("ZAUTH"))
   if(!getenv("ZHTTP_AUTHORIZATION")||strcmp(getenv("ZAUTH"),getenv("ZHTTP_AUTHORIZATION"))!=0)
    {sprintf(fb,"HTTP/1.0 401 No Auth\r\nWWW-Authenticate: Basic realm=\"auth\"\r\n\r\n");
     write(fh,fb,strlen(fb));return;}
  if (strcmp (request, "/") == 0) strcpy (request, indexpage);
  if (strncasecmp (query, "?METHOD=DELETE", 14) == 0) {strcpy(method,"DELETE");}
  if (strstr (request, "..")==NULL && request[0]=='/' && request[1]!='/' && request[1]!='.' ) {
   if (strcasecmp (method, "GET") == 0) {
    if(request[strlen(request)-1]=='/'){DIR *dp;struct dirent *ep;struct stat st;
     if(!(chdir(&request[1])==0&&(dp=opendir("."))!=NULL)) goto err;
     sprintf(fb,"HTTP/1.0 200 Ok.\r\nContent-Type: text/plain\r\n\r\n");write(fh,fb,strlen(fb));
     if(dp!=NULL){while((ep=readdir(dp))!=NULL) {write(fh,ep->d_name,strlen(ep->d_name));
      if(stat(ep->d_name,&st)==0&&S_ISDIR(st.st_mode)) write(fh,"/",1);write(fh,"\n",1);}}
      closedir(dp);status=200;goto log;}
    pgfd = strncmp (request,"/cgi/",5) == 0 ? popen (&request[1], "r") : fopen (&request[1], "r");
    status = 404; if (pgfd != NULL) { response[0] = '\0'; status = 200; }
   } else if (strncmp (request, datadir, strlen(datadir)) == 0 ) {
    if((!strcasecmp(method,"PUT")||!strcasecmp(method,"POST"))&&getenv("ZHTTP_CONTENT_LENGTH")){
     pgfd = fopen (&request[1], "w"); size=atoi(getenv("ZHTTP_CONTENT_LENGTH"));
     if (pgfd != NULL) {
      while(size--)
       { ret=fgetc(fd); if(ret==EOF) break; ret=fputc(ret,pgfd); if(ret==EOF) break; }
      if(++size==0) { sprintf(response,"Ok.\r\n"); status = 200; }
      fclose(pgfd); pgfd=NULL; system("./.put_handler &"); }
    } else if (strcasecmp (method, "DELETE") == 0 ) {
      ret = unlink (&request[1]);
      if (ret == 0) { sprintf(response,"Ok.\r\n"); status = 200; }}}}}
 err: DBG ("Reply: Status %i with Content:\n%s", status, response);
 if (!strstr (request, ".nph") && fd != NULL)
  {sprintf (fb, "HTTP/1.0 %i %s\r\nServer: zeptohttpd/0.9\r\nContent-Type: %s\r\n\r\n%s",
   status, status == 200 ? "OK" : "Error", strstr (request, ".htm") ? "text/html" :
                strstr (request, ".txt") ? "text/plain" : strstr (request, ".jpg") ? "image/jpeg" :
                "application/octet-stream", response); write(fh,fb,strlen(fb));}
 if (pgfd != NULL) { // we have an open pipe to a cgi executable
   while ((ret = fread (response, 1, sizeof (response), pgfd)) > 0) write (fh, response, ret);
   strncmp (request,"/cgi/",5) == 0 ? pclose (pgfd) : fclose (pgfd); }
 log: printf ("%s %03i %s %s %s\n", remote_addr, status, method, request, query); }
int main (int argc, char **argv) {
 struct sockaddr_in server_addr, client_addr; socklen_t addr_len;
 char client_info[NI_MAXHOST]; FILE *client_fd; int server_sock, client_sock, yes = 1;
 int port = argc >= 2 ? atoi (argv[1]) : 8888; if (argc >= 5) datadir=argv[4];
 if (argc >= 3) if (chdir (argv[2]) < 0) DIE (strerror (errno)); if (argc >= 4) indexpage=argv[3];
 server_sock = socket (AF_INET, SOCK_STREAM, 0); if (server_sock < 0) DIE (strerror (errno));
 server_addr.sin_family = AF_INET; server_addr.sin_addr.s_addr = htonl (INADDR_ANY);
 server_addr.sin_port = htons (port);
 if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))<0) DIE(strerror(errno));
 if(bind(server_sock,(struct sockaddr*)&server_addr,sizeof(server_addr))<0) DIE(strerror(errno));
 listen (server_sock, 10); signal(SIGCHLD, SIG_IGN);
 while (1) {
   DBG("\n\n*** Zepto-HTTPD v0.9 - (c)2010-2015 Matthias.Goebl*goebl.net ***\nTCP Port %i.",port);
   addr_len = sizeof (client_addr);
   client_sock = accept (server_sock, (struct sockaddr *) &client_addr, &addr_len);
   if (client_sock == -1) continue;
   getnameinfo ((struct sockaddr *) &client_addr, sizeof (client_addr), client_info,
    sizeof (client_info), NULL, 0, NI_NUMERICHOST);
   client_fd = fdopen (client_sock, "r+"); DBG ("Connect from %s.", client_info);
   if (!fork()) { handle_client (client_fd, client_sock, client_info); exit (0); }
   fclose (client_fd); DBG ("Disconnected from %s.", client_info);
}}
