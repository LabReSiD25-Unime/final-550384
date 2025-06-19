#include "book.h"

extern redis_pool_t *redis_pool;

// Connessione a Redis
redisContext* connect_redis() {
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Errore connessione: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Impossibile allocare contesto redis\n");
        }
        return NULL;
    }
    return c;
}

// Salva un libro usando HSET
int save_book(redisContext *c, const Book *book) {
    redisReply *reply;
    char key[64];
    
    // Crea la chiave del libro
    snprintf(key, sizeof(key), "book:%d", book->id);
    
    // Usa HSET per salvare tutti i campi
    reply = redisCommand(c, "HSET %s id %d title %s author %s price %.2f",
                        key, book->id, book->title, book->author, book->price);
    
    if (reply == NULL) {
        printf("Errore nel comando HSET\n");
        return -1;
    }
    
    printf("Libro salvato: %s\n", key);
    freeReplyObject(reply);
    return 0;
}

// Carica un libro usando HGETALL
Book* load_book(redisContext *c, int book_id) {
    redisReply *reply;
    char key[64];
    Book *book = NULL;
    
    snprintf(key, sizeof(key), "book:%d", book_id);
    
    // Recupera tutti i campi del libro
    reply = redisCommand(c, "HGETALL %s", key);
    
    if (reply == NULL) {
        printf("Errore nel comando HGETALL\n");
        return NULL;
    }
    
    if (reply->type == REDIS_REPLY_ARRAY && reply->elements > 0) {
        book = malloc(sizeof(Book));
        if (book == NULL) {
            printf("Errore allocazione memoria\n");
            freeReplyObject(reply);
            return NULL;
        }
        
        // Parsing della risposta (field1, value1, field2, value2, ...)
        for (size_t i = 0; i < reply->elements; i += 2) {
            char *field = reply->element[i]->str;
            char *value = reply->element[i + 1]->str;
            
            if (strcmp(field, "id") == 0) {
                book->id = atoi(value);
            } else if (strcmp(field, "title") == 0) {
                strncpy(book->title, value, sizeof(book->title) - 1);
                book->title[sizeof(book->title) - 1] = '\0';
            } else if (strcmp(field, "author") == 0) {
                strncpy(book->author, value, sizeof(book->author) - 1);
                book->author[sizeof(book->author) - 1] = '\0';
            } else if (strcmp(field, "price") == 0) {
                book->price = atof(value);
            }
        }
    }
    
    freeReplyObject(reply);
    return book;
}

// Aggiorna il prezzo di un libro usando HSET
int update_book_price(redisContext *c, int book_id, double new_price) {
    redisReply *reply;
    char key[64];
    
    snprintf(key, sizeof(key), "book:%d", book_id);
    
    reply = redisCommand(c, "HSET %s price %.2f", key, new_price);
    
    if (reply == NULL) {
        printf("Errore aggiornamento prezzo\n");
        return -1;
    }
    
    printf("Prezzo aggiornato per %s: %.2f\n", key, new_price);
    freeReplyObject(reply);
    return 0;
}

// Verifica se un libro esiste
int book_exists(redisContext *c, int book_id) {
    redisReply *reply;
    char key[64];
    int exists = 0;
    
    snprintf(key, sizeof(key), "book:%d", book_id);
    
    reply = redisCommand(c, "EXISTS %s", key);
    
    if (reply != NULL) {
        exists = reply->integer;
        freeReplyObject(reply);
    }
    
    return exists;
}

// Elimina un libro
int delete_book(redisContext *c, int book_id) {
    redisReply *reply;
    char key[64];
    
    snprintf(key, sizeof(key), "book:%d", book_id);
    
    reply = redisCommand(c, "DEL %s", key);
    
    if (reply == NULL) {
        printf("Errore eliminazione libro\n");
        return -1;
    }
    
    printf("Libro eliminato: %s\n", key);
    freeReplyObject(reply);
    return 0;
}

// Ottieni un singolo campo usando HGET
char* get_book_field(redisContext *c, int book_id, const char *field) {
    redisReply *reply;
    char key[64];
    char *value = NULL;
    
    snprintf(key, sizeof(key), "book:%d", book_id);
    
    reply = redisCommand(c, "HGET %s %s", key, field);
    
    if (reply != NULL && reply->type == REDIS_REPLY_STRING) {
        value = strdup(reply->str);  // Copia la stringa
    }
    
    if (reply) {
        freeReplyObject(reply);
    }
    
    return value;  // Ricorda di fare free() del risultato
}

// Stampa un libro
void print_book(const Book *book) {
    if (book) {
        printf("ID: %d\n", book->id);
        printf("Titolo: %s\n", book->title);
        printf("Autore: %s\n", book->author);
        printf("Prezzo: %.2f€\n", book->price);
        printf("---\n");
    }
}

// Inizializzazione del pool Redis
int init_redis_pool(int pool_size,redis_pool_t * redis_pool) {
    redis_pool->connections = malloc(sizeof(redisContext*) * pool_size);
    redis_pool->size = pool_size;
    redis_pool->current = 0;
    
    if (pthread_mutex_init(&redis_pool->mutex, NULL) != 0) {
        fprintf(stderr, "Errore nell'inizializzazione del mutex Redis\n");
        return -1;
    }
    
    for (int i = 0; i < pool_size; i++) {
        redis_pool->connections[i] = redisConnect(REDIS_HOST, REDIS_PORT);
        if (redis_pool->connections[i] == NULL || redis_pool->connections[i]->err) {
            if (redis_pool->connections[i]) {
                fprintf(stderr, "Errore connessione Redis: %s\n", 
                        redis_pool->connections[i]->errstr);
                redisFree(redis_pool->connections[i]);
            } else {
                fprintf(stderr, "Impossibile allocare il contesto Redis\n");
            }
            return -1;
        }
    }
    
    printf("Pool Redis inizializzato con %d connessioni\n", pool_size);
    return 0;
}

redisContext* get_redis_connection() {
// Ottenimento di una connessione Redis dal pool
    pthread_mutex_lock(&redis_pool->mutex);
    redisContext *ctx = redis_pool->connections[redis_pool->current];
    redis_pool->current = (redis_pool->current + 1) % redis_pool->size;
    pthread_mutex_unlock(&redis_pool->mutex);
    return ctx;
}



int parse_book_json(const char* json_string, Book* book) {
    if (!json_string || !book) return 0;
    
    // Crea una copia della stringa per manipolarla
    char* json_copy = strdup(json_string);
    if (!json_copy) return 0;
    // Estrai id_book
    double id_value = extract_numeric_value(json_copy, "id_book");
    book->id = (int)id_value;
        printf("Here\n");

    // Estrai title
    char* title_value = extract_string_value(json_copy, "title");
    if (title_value) {
        strncpy(book->title, title_value, sizeof(book->title) - 1);
        book->title[sizeof(book->title) - 1] = '\0';
        free(title_value);
    } else {
        book->title[0] = '\0';
    }
    
    // Estrai author
    char* author_value = extract_string_value(json_copy, "author");
    if (author_value) {
        strncpy(book->author, author_value, sizeof(book->author) - 1);
        book->author[sizeof(book->author) - 1] = '\0';
        free(author_value);
    } else {
        book->author[0] = '\0';
    }
    
    // Estrai price
    book->price = extract_numeric_value(json_copy, "price");
    
    free(json_copy);
    return 1; // Successo
}


char* book_trim_whitespace(char* str) {
    while (isspace(*str)) str++;
    return str;
}

// Funzione per estrarre il valore tra virgolette
char* extract_string_value(char* json, const char* key) {
    char search_pattern[512];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", key);
    
    char* key_pos = strstr(json, search_pattern);
    if (!key_pos) return NULL;
    
    // Trova i due punti dopo la chiave
    char* colon = strchr(key_pos, ':');
    if (!colon) return NULL;
    
    // Salta spazi dopo i due punti
    colon++;
    colon = book_trim_whitespace(colon);
    
    // Deve iniziare con virgolette per essere una stringa
    if (*colon != '"') return NULL;
    colon++; // Salta la prima virgoletta
    
    // Trova la virgoletta di chiusura
    char* end_quote = strchr(colon, '"');
    if (!end_quote) return NULL;
    
    // Alloca memoria per il risultato
    int len = end_quote - colon;
    char* result = malloc(len + 1);
    strncpy(result, colon, len);
    result[len] = '\0';
    
    return result;
}

// Funzione per estrarre un valore numerico
double extract_numeric_value(char* json, const char* key) {
    char search_pattern[512];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", key);
    
    char* key_pos = strstr(json, search_pattern);
    if (!key_pos) return 0;
    
    // Trova i due punti dopo la chiave
    char* colon = strchr(key_pos, ':');
    if (!colon) return 0;
    
    // Salta spazi dopo i due punti
    colon++;
    colon = book_trim_whitespace(colon);
    
    return atof(colon);
}