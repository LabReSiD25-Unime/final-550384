#include "http_utils.h"

http_method_t string_to_method(const char *method_str) {
    if (strcasecmp(method_str, "GET") == 0) return HTTP_GET;
    if (strcasecmp(method_str, "POST") == 0) return HTTP_POST;
    if (strcasecmp(method_str, "PUT") == 0) return HTTP_PUT;
    if (strcasecmp(method_str, "DELETE") == 0) return HTTP_DELETE;
    if (strcasecmp(method_str, "HEAD") == 0) return HTTP_HEAD;
    if (strcasecmp(method_str, "OPTIONS") == 0) return HTTP_OPTIONS;
    if (strcasecmp(method_str, "PATCH") == 0) return HTTP_PATCH;
    if (strcasecmp(method_str, "TRACE") == 0) return HTTP_TRACE;
    if (strcasecmp(method_str, "CONNECT") == 0) return HTTP_CONNECT;
    return HTTP_UNKNOWN;
}

const char* method_to_string(http_method_t method) {
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE";
        case HTTP_HEAD: return "HEAD";
        case HTTP_OPTIONS: return "OPTIONS";
        case HTTP_PATCH: return "PATCH";
        case HTTP_TRACE: return "TRACE";
        case HTTP_CONNECT: return "CONNECT";
        case HTTP_UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

void trim_whitespace(char *str) {
    char *end;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
}

int hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

void url_decode(char *dst, const char *src) {
    char *d = dst;
    const char *s = src;
    
    while (*s) {
        if (*s == '%' && s[1] && s[2]) {
            int high = hex_to_int(s[1]);
            int low = hex_to_int(s[2]);
            if (high >= 0 && low >= 0) {
                *d++ = (char)(high * 16 + low);
                s += 3;
            } else {
                *d++ = *s++;
            }
        } else if (*s == '+') {
            *d++ = ' ';
            s++;
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';
}

int parse_request_line(const char *line, http_request_t *request) {
    char method[MAX_METHOD_LEN];
    char uri[MAX_URI_LEN];
    char version[MAX_VERSION_LEN];
    
    if (sscanf(line, "%15s %2047s %15s", method, uri, version) != 3) {
        return -1;
    }
    
    // Parsing del metodo
    strncpy(request->method_str, method, MAX_METHOD_LEN - 1);
    request->method_str[MAX_METHOD_LEN - 1] = '\0';
    request->method = string_to_method(method);
    
    // Parsing dell'URI
    strncpy(request->uri, uri, MAX_URI_LEN - 1);
    request->uri[MAX_URI_LEN - 1] = '\0';
    
    // Separazione path e query string
    char *query_start = strchr(uri, '?');
    if (query_start) {
        *query_start = '\0';
        strncpy(request->path, uri, MAX_URI_LEN - 1);
        request->path[MAX_URI_LEN - 1] = '\0';
        
        strncpy(request->query_string, query_start + 1, MAX_URI_LEN - 1);
        request->query_string[MAX_URI_LEN - 1] = '\0';
        
        parse_query_string(request->query_string, request);
        *query_start = '?'; // Ripristina l'URI originale
    } else {
        strncpy(request->path, uri, MAX_URI_LEN - 1);
        request->path[MAX_URI_LEN - 1] = '\0';
        request->query_string[0] = '\0';
    }
    
    // Parsing della versione
    strncpy(request->version, version, MAX_VERSION_LEN - 1);
    request->version[MAX_VERSION_LEN - 1] = '\0';
    
    return 0;
}

int parse_header_line(const char *line, http_request_t *request) {
    if (request->header_count >= MAX_HEADERS) {
        return -1;
    }
    
    char *colon = strchr(line, ':');
    if (!colon) {
        return -1;
    }
    
    size_t name_len = colon - line;
    if (name_len >= MAX_HEADER_NAME_LEN) {
        return -1;
    }
    
    // Parsing del nome dell'header
    strncpy(request->headers[request->header_count].name, line, name_len);
    request->headers[request->header_count].name[name_len] = '\0';
    trim_whitespace(request->headers[request->header_count].name);
    
    // Parsing del valore dell'header
    strncpy(request->headers[request->header_count].value, colon + 1, MAX_HEADER_VALUE_LEN - 1);
    request->headers[request->header_count].value[MAX_HEADER_VALUE_LEN - 1] = '\0';
    trim_whitespace(request->headers[request->header_count].value);
    
    // Aggiorna content_length se è Content-Length
    if (strcasecmp(request->headers[request->header_count].name, "Content-Length") == 0) {
        request->content_length = strtoul(request->headers[request->header_count].value, NULL, 10);
    }
    
    request->header_count++;
    return 0;
}

int parse_query_string(const char *query_string, http_request_t *request) {
    if (!query_string || strlen(query_string) == 0) {
        return 0;
    }
    
    char *query_copy = strdup(query_string);
    if (!query_copy) {
        return -1;
    }
    
    char *param = strtok(query_copy, "&");
    while (param && request->query_param_count < MAX_QUERY_PARAMS) {
        char *equals = strchr(param, '=');
        if (equals) {
            *equals = '\0';
            
            url_decode(request->query_params[request->query_param_count].name, param);
            url_decode(request->query_params[request->query_param_count].value, equals + 1);
        } else {
            url_decode(request->query_params[request->query_param_count].name, param);
            request->query_params[request->query_param_count].value[0] = '\0';
        }
        
        request->query_param_count++;
        param = strtok(NULL, "&");
    }
    
    free(query_copy);
    return 0;
}

http_request_t* create_http_request() {
    http_request_t *request = calloc(1, sizeof(http_request_t));
    if (!request) {
        return NULL;
    }
    
    request->method = HTTP_UNKNOWN;
    request->header_count = 0;
    request->query_param_count = 0;
    request->body = NULL;
    request->body_length = 0;
    request->content_length = 0;
    
    return request;
}

void free_http_request(http_request_t *request) {
    if (request) {
        if (request->body) {
            free(request->body);
        }
        free(request);
    }
}

int parse_http_request(const char *raw_request, http_request_t *request) {
    if (!raw_request || !request) {
        return -1;
    }
    
    // Trova il separatore tra headers e body
    const char *header_end = strstr(raw_request, "\r\n\r\n");
    if (!header_end) {
        // Prova con solo \n se non trova \r\n\r\n
        header_end = strstr(raw_request, "\n\n");
        if (!header_end) {
            return -1;
        }
    }
    
    // Calcola la lunghezza della parte headers
    size_t header_length = header_end - raw_request;
    
    // Crea una copia solo della parte headers
    char *headers_copy = malloc(header_length + 1);
    if (!headers_copy) {
        return -1;
    }
    
    strncpy(headers_copy, raw_request, header_length);
    headers_copy[header_length] = '\0';
    
    // Parsing delle linee degli headers
    char *line = strtok(headers_copy, "\r\n");
    if (!line) {
        // Prova con solo \n
        free(headers_copy);
        headers_copy = malloc(header_length + 1);
        strncpy(headers_copy, raw_request, header_length);
        headers_copy[header_length] = '\0';
        line = strtok(headers_copy, "\n");
        if (!line) {
            free(headers_copy);
            return -1;
        }
    }
    
    // Parsing della request line
    if (parse_request_line(line, request) != 0) {
        free(headers_copy);
        return -1;
    }
    
    // Parsing degli headers
    while ((line = strtok(NULL, "\r\n")) != NULL || (line = strtok(NULL, "\n")) != NULL) {
        if (strlen(line) == 0) {
            break; // Fine degli headers
        }
        if (parse_header_line(line, request) != 0) {
            // Non considerare un errore critico un header malformato
            continue;
        }
    }
    
    free(headers_copy);
    
    // Parsing del body se presente
    const char *body_start = strstr(raw_request, "\r\n\r\n");
    if (body_start) {
        body_start += 4; // Salta "\r\n\r\n"
    } else {
        body_start = strstr(raw_request, "\n\n");
        if (body_start) {
            body_start += 2; // Salta "\n\n"
        }
    }
    
    if (body_start) {
        size_t total_length = strlen(raw_request);
        size_t body_start_offset = body_start - raw_request;
        size_t remaining = total_length - body_start_offset;
        
        if (request->content_length > 0) {
            // Usa Content-Length se specificato
            size_t body_size = (remaining < request->content_length) ? remaining : request->content_length;
            request->body = malloc(body_size + 1);
            if (request->body) {
                memcpy(request->body, body_start, body_size);
                request->body[body_size] = '\0';
                request->body_length = body_size;
            }
        } else if (remaining > 0) {
            // Se non c'è Content-Length ma c'è del contenuto, prendilo tutto
            request->body = malloc(remaining + 1);
            if (request->body) {
                memcpy(request->body, body_start, remaining);
                request->body[remaining] = '\0';
                request->body_length = remaining;
                request->content_length = remaining;
            }
        }
    }
    
    return 0;
}

const char* get_header_value(const http_request_t *request, const char *header_name) {
    if (!request || !header_name) {
        return NULL;
    }
    
    for (int i = 0; i < request->header_count; i++) {
        if (strcasecmp(request->headers[i].name, header_name) == 0) {
            return request->headers[i].value;
        }
    }
    
    return NULL;
}

const char* get_query_param(const http_request_t *request, const char *param_name) {
    if (!request || !param_name) {
        return NULL;
    }
    
    for (int i = 0; i < request->query_param_count; i++) {
        if (strcmp(request->query_params[i].name, param_name) == 0) {
            return request->query_params[i].value;
        }
    }
    
    return NULL;
}

int has_header(const http_request_t *request, const char *header_name) {
    return get_header_value(request, header_name) != NULL;
}

int has_query_param(const http_request_t *request, const char *param_name) {
    return get_query_param(request, param_name) != NULL;
}

void print_http_request(const http_request_t *request) {
    if (!request) {
        printf("Request is NULL\n");
        return;
    }
    
    printf("=== HTTP REQUEST ===\n");
    printf("Method: %s (%s)\n", request->method_str, method_to_string(request->method));
    printf("URI: %s\n", request->uri);
    printf("Path: %s\n", request->path);
    printf("Query String: %s\n", request->query_string);
    printf("Version: %s\n", request->version);
    printf("Content Length: %zu\n", request->content_length);
    
    printf("\nHeaders (%d):\n", request->header_count);
    for (int i = 0; i < request->header_count; i++) {
        printf("  %s: %s\n", request->headers[i].name, request->headers[i].value);
    }
    
    printf("\nQuery Parameters (%d):\n", request->query_param_count);
    for (int i = 0; i < request->query_param_count; i++) {
        printf("  %s = %s\n", request->query_params[i].name, request->query_params[i].value);
    }
    
    if (request->body && request->body_length > 0) {
        printf("\nBody (%zu bytes):\n", request->body_length);
        printf("%.*s\n", (int)request->body_length, request->body);
    }
    
    printf("==================\n");
}

