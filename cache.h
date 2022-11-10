#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "csapp.h"
typedef struct MultipleArg MultipleArg;
typedef struct CachedData CachedData;
typedef struct Cachelist Cachelist;

typedef struct _Cachelist
{
    CachedData *head;
    // CachedData *tail; // tail would not be used in sunny's code
} Cachelist;

typedef struct _CachedData
{
    char *c_key[MAXBUF];
    char *c_val;
    CachedData *next;
} CachedData;

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

void insertcache(Cachelist *list, CachedData *my_data)
{
    CachedData *p = list->head;
    list->head = my_data;
    list->head->next = p;
    return;
};

CachedData *findcache(Cachelist list, char* hostname)
{
    CachedData *cur_cache = list.head;

    while (cur_cache != NULL)
    {
        if (!strcmp(cur_cache->c_key, hostname))
            return cur_cache;
            
        else
            cur_cache = cur_cache->next;
    }
    
    return NULL;
};