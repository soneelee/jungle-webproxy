#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400


//curl -v --proxy http://localhost:15214 http://localhost:15213/home.html
//curl -v --proxy http://localhost:15214 http://localhost:15213/home.html
//curl -v --proxy http://localhost:15214 http://localhost:15213/home.html

void doit(int fd);
// void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *end_host, char *end_port);
void get_filetype(char *filename, char *filetype);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void make_packet(char *rqheader, char *filename, char *end_host, char *end_port);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
static const char *Connection = "Connection: close\r\n";
static const char *Proxy_Connection = "Proxy-Connection: close\r\n";



int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  listenfd = Open_listenfd(argv[1]); // 듣기 소켓 오픈
  /* 무한 서버 루프 */
  while (1)
  {
    clientlen = sizeof(clientaddr);
    /* 연결 요청 접수 */
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    /* Transaction 수행 */
    printf("나 이제 doit 실행한다...????");
    doit(connfd); // line:netp:tiny:doit
    /* 서버쪽 연결 종료 */
    Close(connfd); // line:netp:tiny:close
    printf("힝... 서버 닫힘 ㅠㅠ");
  }
}

/* doit : 한 개의 HTTP transaction 처리 */
void doit(int first_fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE], end_host[MAXLINE], end_port[MAXLINE], rqheader[MAXLINE];
  int end_fd;
  size_t n;
  rio_t rio;
  rio_t rio2;

  /* Request Line과 헤더를 읽기 */
  Rio_readinitb(&rio, first_fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  /* GET 메소드 외에는 지원하지 않음 - 다른 요청이 오면 에러 띄우고 종료 */
  if (!strcasecmp(method, "HEAD"))
    end_fd = parse_uri(uri, filename, end_host, end_port); // 파싱을 하면 예린이의 포트번호를 알아내야 함
  else if (!strcasecmp(method, "GET"))
    end_fd = parse_uri(uri, filename, end_host, end_port); // 파싱을 하면 예린이의 포트번호를 알아내야 함
  else
  {
    clienterror(first_fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  /* URI를 parsing 하고, 요청받은 것이 static contents인지 판단하는 플래그 설정 */
  // is_static = parse_uri(uri, filename, cgiargs);


   if (end_fd <0){
    printf("파싱해서... end_host, end_port로 열었는데 에러나는듯...ㅠㅠ");
    return;
  }

  make_packet(rqheader,filename, end_host, end_port);

  /*송이 -> 예린 연결*/
  // end_fd = Open_clientfd(end_host, end_port);
  printf("Debug: This is request header \n");
  printf("%s \n", rqheader);
  printf("Debug: endport is.. %s \n",end_port );
  printf("Debug: endhost is.. %s \n", end_host);
  printf("Debug: strlen(rqheader) is... %d \n",strlen(rqheader));
  printf("Debug: endfd is... %d \n",end_fd);  


  Rio_readinitb(&rio2, end_fd);
  // Rio_readlineb(&rio2, rqheader, MAXLINE);
  Rio_writen(end_fd, rqheader, strlen(rqheader));
  printf("다썼당!!! \n");
  // 송이 -> 예린 전달 완료!!!


  while ((n = Rio_readlineb(&rio2, buf, MAXLINE)) != 0)
  {                                                                    // 송이는 예린이가 준 답장 읽음
    printf("나는 송이, %d바이트를 받고 중선이한테 그대~로 넘겨줌", n); // 답장을 중선에게 주겠음
    Rio_writen(first_fd, buf, n); //송이 -> 중선
    //모든 과정이 완료됨! 중선->송이->예린->송이->중선
  }
}


void make_packet(char *rqheader, char *filename, char *end_host, char *end_port)
{
  char filetype[MAXLINE];
  get_filetype(filename, filetype);
sprintf(rqheader, "GET %s HTTP/1.0\r\n", filename);
sprintf(rqheader, "%sHOST: %s\r\n",rqheader, end_host);
sprintf(rqheader,"%s%s",rqheader,Proxy_Connection); //proxy-connection       
sprintf(rqheader,"%s%s",rqheader,user_agent_hdr); //user_agent_hdr    
sprintf(rqheader,"%s%s",rqheader,Connection);    //connection
sprintf(rqheader, "%sContent-type: %s\r\n\r\n",rqheader, filetype);
//TBD: 추가적인 요청헤더를 HTTP요청에 담아보낸다면, 그대로 서버에 전달
//(중선해석): 크롬(클라이언트) -> proxy할 때, 뭔가 추가적인 정보를 줘도 그거 그대~로 다받아서 전달시키라는 뜻인 것 같음
}



/* parse_uri: HTTP URI를 분석하기 */
int parse_uri(char *uri, char *filename, char *end_host, char *end_port)
{
  char *p, *temp, *temp2;
    char temp_uri[300];
    char *buf;
    buf = uri; 
    p = strchr(buf, '/');
    strcpy(temp_uri, p+2);
    p = strchr(temp_uri, ':');
  
    if (p == '\0'){//포트번호 없으면 80 냠~
        p = strchr(temp_uri, '/');
        strcpy(end_host, p);
        *p = '\0';
        strcpy(filename, temp_uri);
        strcpy(end_port, "80");
    }
    else {//포트번호 있으면.... 해당 포트번호 읽어오기
    *p = '\0';
    temp = strchr(p+1, '/');
    strcpy(filename, temp);
    *temp = '\0';
    strcpy(end_port,p+1);
    strcpy(end_host, temp_uri);
    }
    return Open_clientfd(end_host, end_port);
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}





/* serve_static : 서버의 local file을 body로 가진 HTTP response를 클라이언트에게 전달 */
/* Tiny는 5개의 정적 컨텐츠 파일을 지원함 : HTML, unformatted text file, GIF, PNG, JPEG */
// void serve_static(int fd, char *filename, int filesize)
// {
//   int srcfd;
//   // char *srcp, filetype[MAXLINE], buf[MAXBUF];
//   char *srcp, filetype[MAXLINE], buf[MAXBUF];

//   /* 파일 타입을 결정하기 */
//   get_filetype(filename, filetype);

//   /* 클라이언트에 response line과 header를 보내기 */
//   sprintf(buf, "HTTP/1.0 200 OK\r\n");
//   sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
//   sprintf(buf, "%sConnection: close\r\n", buf);
//   sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
//   sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
//   Rio_writen(fd, buf, strlen(buf));
//   printf("Response headers:\n");
//   printf("%s", buf);

//   /* 클라이언트에 response body 보내기 */
//   srcfd = Open(filename, O_RDONLY, 0); /* read를 위해 filename을 open하고 file descriptor를 얻어옴 */
//   // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); /* 요청한 파일을 가상메모리 영억으로 mapping */
//   // Close(srcfd);                                              /* mapping 후 파일을 닫는다 - 메모리 누수를 막기 위해 */
//   // Rio_writen(fd, srcp, filesize);                             /* 파일을 클라이언트에 전달 */
//   // Munmap(srcp, filesize);                                     /* mapping된 가상메모리를 free 한다 - 메모리 누수를 막기 위해 */

//   srcp = (char *)malloc(filesize);
//   Rio_readn(srcfd, srcp, filesize);
//   Close(srcfd);
//   Rio_writen(fd, srcp, filesize);
//   free(srcp);
// }

/* read_requesthdrs: 요청 헤더를 읽고 무시하기 */
// void read_requesthdrs(rio_t *rp)
// {
//   char buf[MAXLINE];

//   Rio_readlineb(rp, buf, MAXLINE);
//   while (strcmp(buf, "\r\n"))
//   {
//     Rio_readlineb(rp, buf, MAXLINE);
//     printf("%s", buf);
//   }
//   return;
// }


/*
 * get_filetype - Derive file type from filename
 */

/* serve_dynamic : child process를 fork하고, CGI program을 child context에서 실행함 */
// void serve_dynamic(int fd, char *filename, char *cgiargs)
// {
//   char buf[MAXLINE], *emptylist[] = {NULL};

//   /* 연결 성공을 알리는 response line을 보내기 */
//   sprintf(buf, "HTTP/1.0 200 OK\r\n");
//   Rio_writen(fd, buf, strlen(buf));
//   sprintf(buf, "Server: Tiny Web Server\r\n");
//   Rio_writen(fd, buf, strlen(buf));

//   if (Fork() == 0)
//   { /* Child process를 fork 하기 */
//     /* 실제 서버는 모든 CGI 환경 변수를 여기서 설정하나, Tiny에서는 생략함 */
//     setenv("QUERY_STRING", cgiargs, 1);   /* URI의 CGI argument를 이용해 QUERY_STRING 환경변수 초기화 */
//     Dup2(fd, STDOUT_FILENO);              /* child의 stdout을 file descriptor로 redirect */
//     Execve(filename, emptylist, environ); /* CGI program을 실행시킴 */
//   }
//   Wait(NULL); /* Parent process는 child process의 종료를 기다림 */
// }

/* clienterror : 에러 메시지를 클라이언트에게 전송 */