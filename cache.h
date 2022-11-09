#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "csapp.h"
typedef struct MultipleArg MultipleArg;
typedef struct CachedData CachedData;
typedef struct Cachelist Cachelist;

struct Cachelist
{
    CachedData *head;
    // CachedData *tail; // tail would not be used in sunny's code
};

struct CachedData
{
    char *c_key[MAXBUF];
    char *c_val;
    CachedData *next;
};

struct MultipleArg
{
    int connfd;
    char hostname[MAXLINE], port[MAXLINE];
    Cachelist Cachelist;
};

void initcachelist()
{
    Cachelist cachelist;
    cachelist.head->next = NULL;
};

void insertcache(CachedData my_data){
    // TBD
};

void *findcache(Cachelist list)
{
    if (list.head == NULL)
    {
        return NULL;
    }
    else
    {
        // 쭉쭉내려가면서 해당 파일명 찾기
    }
    return NULL;
};