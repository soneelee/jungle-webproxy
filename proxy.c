#include <stdio.h>
#include <pthread.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define NTHREADS 4
#define SBUFSIZE 16

//curl -v --proxy http://localhost:15214 http://localhost:15213/home.html

void echo_cnt(int connfd); 
void *thread(void *vargp);
void doit(int connfd);
void read_requesthdrs(rio_t *rp);
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
static sem_t mutex;

typedef struct _MultipleArg
{
  int connfd;
  char hostname[MAXLINE], port[MAXLINE];
  struct MultipleArg *next;
  struct MultipleArg *prev;
} MultipleArg;


int main(int argc, char **argv)
{
  int listenfd, *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  struct MultipleArg *multiple_arg;
  pthread_t tid;

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
    
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);

    /* 연결 요청 접수 */
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // ??????
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    /* 스레드 생성 */
    Pthread_create(&tid, NULL, thread, connfdp);
  }
}

void *thread(void *vargp)
{
  int connfd = *((int *)vargp);
  Pthread_detach(pthread_self()); 
  Free(vargp);

  doit(connfd);

  Close(connfd);
  return NULL; 
}


/* doit : 한 개의 HTTP transaction 처리 */
void doit(int connfd)
{
  int is_static;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE], end_host[MAXLINE], end_port[MAXLINE], rqheader[MAXLINE];
  int end_fd;
  size_t n;
  rio_t rio, rio2;

  /* Request Line과 헤더를 읽기 */
  Rio_readinitb(&rio, connfd);
  
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  /* GET 메소드 외에는 지원하지 않음 - 다른 요청이 오면 에러 띄우고 종료 */
  if (!strcasecmp(method, "HEAD"))
    parse_uri(uri, filename, end_host, end_port); // 파싱을 해서 예린이의 호스트네임, 포트번호 알아냄
  else if (!strcasecmp(method, "GET"))
    parse_uri(uri, filename, end_host, end_port);
  else
  {
    clienterror(connfd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  make_packet(rqheader,filename, end_host, end_port);

  /*송이 -> 예린 연결*/
  end_fd = Open_clientfd(end_host, end_port);
  
  if (end_fd <0){
    printf("파싱해서... end_host, end_port로 열었는데 에러나는듯...ㅠㅠ");
    return;
  }

  printf("Debug: This is request header \n");
  printf("%s \n", rqheader);
  printf("Debug: endport is.. %s \n", end_port);
  printf("Debug: endhost is.. %s \n", end_host);
  printf("Debug: strlen(rqheader) is... %d \n",strlen(rqheader));
  printf("Debug: endfd is... %d \n",end_fd);  

  Rio_writen(end_fd, rqheader, strlen(rqheader));
  printf("다썼당!!! \n");
  /*송이 -> 예린 전달 완료*/


  /* 캐시 생성 */

  

  /* 예린 -> 송이 -> 중선 */
  Rio_readinitb(&rio2, end_fd);
 
  while ((n = Rio_readlineb(&rio2, buf, MAXLINE)) != 0) // 송이는 예린이가 준 답장 읽음
  {                                                                    
    printf("나는 송이, %d바이트를 받고 중선이한테 그대~로 넘겨줌", n); // 답장을 중선에게 주겠음
    Rio_writen(connfd, buf, n); //송이 -> 중선
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
  return 1;
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

// void *thread(void *vargp)
// {
  
//   MultipleArg *multiple_arg = (MultipleArg *)Malloc(sizeof(MultipleArg));
//   multiple_arg->connfd = *((int *)vargp);

//   Pthread_detach(pthread_self()); 
//   Free(multiple_arg);

//   doit(multiple_arg);

//   Close(multiple_arg->connfd);

//   return NULL; 
// }