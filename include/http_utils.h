#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Costanti
#define MAX_METHOD_LEN 16
#define MAX_URI_LEN 2048
#define MAX_VERSION_LEN 16
#define MAX_HEADER_NAME_LEN 256
#define MAX_HEADER_VALUE_LEN 1024
#define MAX_HEADERS 50
#define MAX_QUERY_PARAMS 50
#define MAX_PARAM_NAME_LEN 256
#define MAX_PARAM_VALUE_LEN 1024
#define MAX_STATUS_MESSAGE_LEN 256

// Struct per gli header HTTP
typedef struct {
    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];
} http_header_t;


typedef enum {
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NO_CONTENT = 204,
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_FOUND = 302,
    HTTP_NOT_MODIFIED = 304,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_CONFLICT = 409,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503
} http_status_t;


// Struct per la risposta HTTP
typedef struct {
    char version[MAX_VERSION_LEN];
    http_status_t status_code;
    char status_message[MAX_STATUS_MESSAGE_LEN];
    
    http_header_t headers[MAX_HEADERS];
    int header_count;
    
    char *body;
    size_t body_length;
    
    // Buffer per la risposta completa
    char *raw_response;
    size_t raw_response_size;
} http_response_t;



// Enum per i metodi HTTP
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_PATCH,
    HTTP_TRACE,
    HTTP_CONNECT,
    HTTP_UNKNOWN
} http_method_t;

// Struct per i parametri query
typedef struct {
    char name[MAX_PARAM_NAME_LEN];
    char value[MAX_PARAM_VALUE_LEN];
} query_param_t;


// Struct principale per la richiesta HTTP
typedef struct {
    http_method_t method;
    char method_str[MAX_METHOD_LEN];
    char uri[MAX_URI_LEN];
    char path[MAX_URI_LEN];
    char query_string[MAX_URI_LEN];
    char version[MAX_VERSION_LEN];
    
    http_header_t headers[MAX_HEADERS];
    int header_count;
    
    query_param_t query_params[MAX_QUERY_PARAMS];
    int query_param_count;
    
    char *body;
    size_t body_length;
    size_t content_length;
} http_request_t;

// Funzioni di utility
http_method_t string_to_method(const char *method_str);
const char* method_to_string(http_method_t method);
void trim_whitespace(char *str);
int parse_request_line(const char *line, http_request_t *request);
int parse_header_line(const char *line, http_request_t *request);
int parse_query_string(const char *query_string, http_request_t *request);
void url_decode(char *dst, const char *src);
int hex_to_int(char c);

// Funzioni principali
http_request_t* create_http_request();
void free_http_request(http_request_t *request);
int parse_http_request(const char *raw_request, http_request_t *request);
void print_http_request(const http_request_t *request);

// Funzioni di accesso
const char* get_header_value(const http_request_t *request, const char *header_name);
const char* get_query_param(const http_request_t *request, const char *param_name);
int has_header(const http_request_t *request, const char *header_name);
int has_query_param(const http_request_t *request, const char *param_name);

http_response_t* create_http_response();


#endif