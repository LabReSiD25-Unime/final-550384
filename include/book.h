#ifndef BOOK_H
#define BOOK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hiredis/hiredis.h"
#include <pthread.h>
#include <semaphore.h>
#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379

typedef struct {
    int id;
    char title[256];
    char author[256];
    double price;
} Book;

typedef struct {
    redisContext **connections;
    int size;
    int current;
    pthread_mutex_t mutex;
} redis_pool_t;

redisContext* connect_redis();
int init_redis_pool(int pool_size, redis_pool_t * redis_pool);
redisContext* get_redis_connection();

int save_book(redisContext *c, const Book *book);
Book* load_book(redisContext *c, int book_id);
int update_book_price(redisContext *c, int book_id, double new_price);
int book_exists(redisContext *c, int book_id);
int delete_book(redisContext *c, int book_id);
char* get_book_field(redisContext *c, int book_id, const char *field) ;
void print_book(const Book *book) ;

#endif