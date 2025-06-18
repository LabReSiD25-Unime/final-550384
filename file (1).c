#include "http_utils.h"
#include <regex.h>

// Struttura per simulare un record utente
typedef struct {
    int id;
    char name[256];
    char email[256];
    int active;
} user_record_t;

// Array statico per simulare un database
static user_record_t users_db[100];
static int users_count = 0;
static int next_id = 1;

// Funzione per inizializzare alcuni dati di esempio
void init_sample_data() {
    // Aggiungi alcuni utenti di esempio
    strcpy(users_db[0].name, "Mario Rossi");
    strcpy(users_db[0].email, "mario.rossi@email.com");
    users_db[0].id = next_id++;
    users_db[0].active = 1;
    users_count++;
    
    strcpy(users_db[1].name, "Giulia Bianchi");
    strcpy(users_db[1].email, "giulia.bianchi@email.com");
    users_db[1].id = next_id++;
    users_db[1].active = 1;
    users_count++;
    
    strcpy(users_db[2].name, "Luca Verdi");
    strcpy(users_db[2].email, "luca.verdi@email.com");
    users_db[2].id = next_id++;
    users_db[2].active = 0;
    users_count++;
}

// Funzione per estrarre l'ID dall'URL
int extract_id_from_path(const char *path) {
    // Cerca pattern come /users/123
    const char *last_slash = strrchr(path, '/');
    if (last_slash && last_slash != path) {
        int id = atoi(last_slash + 1);
        return (id > 0) ? id : -1;
    }
    return -1;
}

// Funzione per trovare un utente per ID
int find_user_by_id(int id) {
    for (int i = 0; i < users_count; i++) {
        if (users_db[i].id == id) {
            return i;
        }
    }
    return -1;
}

// CREATE - Aggiunge un nuovo record
void crud_create(const http_request_t *request, http_response_t *response) {
    printf("=== OPERAZIONE CREATE ===\n");
    printf("Metodo: %s\n", request->method_str);
    printf("Path: %s\n", request->path);
    
    if (users_count >= 100) {
        printf("❌ Errore: Database pieno!\n");
        set_response_status(response, HTTP_INTERNAL_SERVER_ERROR);
        set_response_json(response, "{\"error\": \"Database pieno\"}");
        return;
    }
    
    // Simula il parsing del JSON dal body
    if (request->body && request->body_length > 0) {
        printf("Body ricevuto: %s\n", request->body);
        
        // Simula l'estrazione dei dati (in un caso reale useresti un parser JSON)
        user_record_t new_user;
        new_user.id = next_id++;
        snprintf(new_user.name, sizeof(new_user.name), "Nuovo Utente %d", new_user.id);
        snprintf(new_user.email, sizeof(new_user.email), "user%d@example.com", new_user.id);
        new_user.active = 1;
        
        // Aggiungi al "database"
        users_db[users_count] = new_user;
        users_count++;
        
        printf("✅ Utente creato con successo!\n");
        printf("   ID: %d\n", new_user.id);
        printf("   Nome: %s\n", new_user.name);
        printf("   Email: %s\n", new_user.email);
        printf("   Attivo: %s\n", new_user.active ? "Sì" : "No");
        
        // Prepara risposta
        char json_response[1024];
        snprintf(json_response, sizeof(json_response),
                "{\"id\": %d, \"name\": \"%s\", \"email\": \"%s\", \"active\": %s}",
                new_user.id, new_user.name, new_user.email, 
                new_user.active ? "true" : "false");
        
        set_response_status(response, HTTP_CREATED);
        set_response_json(response, json_response);
    } else {
        printf("❌ Errore: Nessun body nella richiesta\n");
        set_response_status(response, HTTP_BAD_REQUEST);
        set_response_json(response, "{\"error\": \"Body richiesto per la creazione\"}");
    }
    printf("========================\n\n");
}

// READ - Legge uno o più record
void crud_read(const http_request_t *request, http_response_t *response) {
    printf("=== OPERAZIONE READ ===\n");
    printf("Metodo: %s\n", request->method_str);
    printf("Path: %s\n", request->path);
    
    int requested_id = extract_id_from_path(request->path);
    
    if (requested_id > 0) {
        // Leggi un singolo utente
        printf("Richiesto utente con ID: %d\n", requested_id);
        
        int index = find_user_by_id(requested_id);
        if (index >= 0) {
            printf("✅ Utente trovato:\n");
            printf("   ID: %d\n", users_db[index].id);
            printf("   Nome: %s\n", users_db[index].name);
            printf("   Email: %s\n", users_db[index].email);
            printf("   Attivo: %s\n", users_db[index].active ? "Sì" : "No");
            
            char json_response[1024];
            snprintf(json_response, sizeof(json_response),
                    "{\"id\": %d, \"name\": \"%s\", \"email\": \"%s\", \"active\": %s}",
                    users_db[index].id, users_db[index].name, users_db[index].email,
                    users_db[index].active ? "true" : "false");
            
            set_response_status(response, HTTP_OK);
            set_response_json(response, json_response);
        } else {
            printf("❌ Utente non trovato\n");
            set_response_status(response, HTTP_NOT_FOUND);
            set_response_json(response, "{\"error\": \"Utente non trovato\"}");
        }
    } else {
        // Leggi tutti gli utenti
        printf("Richiesta lista di tutti gli utenti\n");
        printf("✅ Trovati %d utenti:\n", users_count);
        
        for (int i = 0; i < users_count; i++) {
            printf("   [%d] ID: %d, Nome: %s, Email: %s, Attivo: %s\n",
                   i + 1, users_db[i].id, users_db[i].name, users_db[i].email,
                   users_db[i].active ? "Sì" : "No");
        }
        
        // Costruisci JSON array (semplificato)
        char json_response[4096] = "{\"users\": [";
        for (int i = 0; i < users_count; i++) {
            char user_json[512];
            snprintf(user_json, sizeof(user_json),
                    "%s{\"id\": %d, \"name\": \"%s\", \"email\": \"%s\", \"active\": %s}",
                    (i > 0) ? ", " : "",
                    users_db[i].id, users_db[i].name, users_db[i].email,
                    users_db[i].active ? "true" : "false");
            strcat(json_response, user_json);
        }
        strcat(json_response, "]}");
        
        set_response_status(response, HTTP_OK);
        set_response_json(response, json_response);
    }
    printf("======================\n\n");
}

// UPDATE - Aggiorna un record esistente
void crud_update(const http_request_t *request, http_response_t *response) {
    printf("=== OPERAZIONE UPDATE ===\n");
    printf("Metodo: %s\n", request->method_str);
    printf("Path: %s\n", request->path);
    
    int requested_id = extract_id_from_path(request->path);
    
    if (requested_id <= 0) {
        printf("❌ Errore: ID non specificato nell'URL\n");
        set_response_status(response, HTTP_BAD_REQUEST);
        set_response_json(response, "{\"error\": \"ID richiesto per l'aggiornamento\"}");
        printf("========================\n\n");
        return;
    }
    
    printf("Richiesto aggiornamento utente con ID: %d\n", requested_id);
    
    int index = find_user_by_id(requested_id);
    if (index < 0) {
        printf("❌ Utente non trovato\n");
        set_response_status(response, HTTP_NOT_FOUND);
        set_response_json(response, "{\"error\": \"Utente non trovato\"}");
        printf("========================\n\n");
        return;
    }
    
    printf("Utente trovato - Dati attuali:\n");
    printf("   Nome: %s\n", users_db[index].name);
    printf("   Email: %s\n", users_db[index].email);
    printf("   Attivo: %s\n", users_db[index].active ? "Sì" : "No");
    
    if (request->body && request->body_length > 0) {
        printf("Body ricevuto: %s\n", request->body);
        
        // Simula l'aggiornamento (in un caso reale useresti un parser JSON)
        snprintf(users_db[index].name, sizeof(users_db[index].name), 
                "Utente Aggiornato %d", requested_id);
        snprintf(users_db[index].email, sizeof(users_db[index].email), 
                "updated%d@example.com", requested_id);
        users_db[index].active = !users_db[index].active; // Inverte lo stato
        
        printf("✅ Utente aggiornato con successo!\n");
        printf("   Nuovi dati:\n");
        printf("   Nome: %s\n", users_db[index].name);
        printf("   Email: %s\n", users_db[index].email);
        printf("   Attivo: %s\n", users_db[index].active ? "Sì" : "No");
        
        char json_response[1024];
        snprintf(json_response, sizeof(json_response),
                "{\"id\": %d, \"name\": \"%s\", \"email\": \"%s\", \"active\": %s}",
                users_db[index].id, users_db[index].name, users_db[index].email,
                users_db[index].active ? "true" : "false");
        
        set_response_status(response, HTTP_OK);
        set_response_json(response, json_response);
    } else {
        printf("❌ Errore: Nessun body nella richiesta\n");
        set_response_status(response, HTTP_BAD_REQUEST);
        set_response_json(response, "{\"error\": \"Body richiesto per l'aggiornamento\"}");
    }
    printf("========================\n\n");
}

// DELETE - Elimina un record
void crud_delete(const http_request_t *request, http_response_t *response) {
    printf("=== OPERAZIONE DELETE ===\n");
    printf("Metodo: %s\n", request->method_str);
    printf("Path: %s\n", request->path);
    
    int requested_id = extract_id_from_path(request->path);
    
    if (requested_id <= 0) {
        printf("❌ Errore: ID non specificato nell'URL\n");
        set_response_status(response, HTTP_BAD_REQUEST);
        set_response_json(response, "{\"error\": \"ID richiesto per l'eliminazione\"}");
        printf("========================\n\n");
        return;
    }
    
    printf("Richiesta eliminazione utente con ID: %d\n", requested_id);
    
    int index = find_user_by_id(requested_id);
    if (index < 0) {
        printf("❌ Utente non trovato\n");
        set_response_status(response, HTTP_NOT_FOUND);
        set_response_json(response, "{\"error\": \"Utente non trovato\"}");
        printf("========================\n\n");
        return;
    }
    
    printf("Utente trovato:\n");
    printf("   ID: %d\n", users_db[index].id);
    printf("   Nome: %s\n", users_db[index].name);
    printf("   Email: %s\n", users_db[index].email);
    
    // Simula l'eliminazione spostando gli elementi
    for (int i = index; i < users_count - 1; i++) {
        users_db[i] = users_db[i + 1];
    }
    users_count--;
    
    printf("✅ Utente eliminato con successo!\n");
    printf("   Utenti rimanenti: %d\n", users_count);
    
    set_response_status(response, HTTP_NO_CONTENT);
    set_response_text(response, "");
    
    printf("========================\n\n");
}

// Funzione principale per il routing delle operazioni CRUD
void handle_crud_request(const http_request_t *request, http_response_t *response) {
    printf("🔄 Gestione richiesta CRUD\n");
    printf("URL: %s\n", request->path);
    printf("Metodo: %s\n", request->method_str);
    
    // Verifica se il path inizia con /users
    if (strncmp(request->path, "/users", 6) != 0) {
        printf("❌ Endpoint non supportato: %s\n", request->path);
        set_response_status(response, HTTP_NOT_FOUND);
        set_response_json(response, "{\"error\": \"Endpoint non trovato\"}");
        return;
    }
    
    // Routing basato sul metodo HTTP
    switch (request->method) {
        case HTTP_POST:
            crud_create(request, response);
            break;
            
        case HTTP_GET:
            crud_read(request, response);
            break;
            
        case HTTP_PUT:
        case HTTP_PATCH:
            crud_update(request, response);
            break;
            
        case HTTP_DELETE:
            crud_delete(request, response);
            break;
            
        default:
            printf("❌ Metodo HTTP non supportato: %s\n", request->method_str);
            set_response_status(response, HTTP_METHOD_NOT_ALLOWED);
            set_response_json(response, "{\"error\": \"Metodo non supportato\"}");
            add_response_header(response, "Allow", "GET, POST, PUT, PATCH, DELETE");
            break;
    }
}

// Funzione di test per dimostrare l'uso
void test_crud_operations() {
    printf("🧪 AVVIO TEST OPERAZIONI CRUD\n");
    printf("=====================================\n\n");
    
    // Inizializza dati di esempio
    init_sample_data();
    
    // Test CREATE
    printf("1️⃣ TEST CREATE\n");
    const char *create_request = 
        "POST /users HTTP/1.1\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 45\r\n"
        "\r\n"
        "{\"name\": \"Test User\", \"email\": \"test@example.com\"}";
    
    http_request_t *req1 = create_http_request();
    http_response_t *resp1 = create_http_response();
    
    if (parse_http_request(create_request, req1) == 0) {
        handle_crud_request(req1, resp1);
        printf("Risposta: %d %s\n", resp1->status_code, resp1->status_message);
    }
    
    free_http_request(req1);
    free_http_response(resp1);
    
    // Test READ (tutti gli utenti)
    printf("2️⃣ TEST READ (tutti)\n");
    const char *read_all_request = 
        "GET /users HTTP/1.1\r\n"
        "Accept: application/json\r\n"
        "\r\n";
    
    http_request_t *req2 = create_http_request();
    http_response_t *resp2 = create_http_response();
    
    if (parse_http_request(read_all_request, req2) == 0) {
        handle_crud_request(req2, resp2);
        printf("Risposta: %d %s\n", resp2->status_code, resp2->status_message);
    }
    
    free_http_request(req2);
    free_http_response(resp2);
    
    // Test READ (singolo utente)
    printf("3️⃣ TEST READ (singolo)\n");
    const char *read_single_request = 
        "GET /users/2 HTTP/1.1\r\n"
        "Accept: application/json\r\n"
        "\r\n";
    
    http_request_t *req3 = create_http_request();
    http_response_t *resp3 = create_http_response();
    
    if (parse_http_request(read_single_request, req3) == 0) {
        handle_crud_request(req3, resp3);
        printf("Risposta: %d %s\n", resp3->status_code, resp3->status_message);
    }
    
    free_http_request(req3);
    free_http_response(resp3);
    
    // Test UPDATE
    printf("4️⃣ TEST UPDATE\n");
    const char *update_request = 
        "PUT /users/1 HTTP/1.1\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 52\r\n"
        "\r\n"
        "{\"name\": \"Updated User\", \"email\": \"updated@example.com\"}";
    
    http_request_t *req4 = create_http_request();
    http_response_t *resp4 = create_http_response();
    
    if (parse_http_request(update_request, req4) == 0) {
        handle_crud_request(req4, resp4);
        printf("Risposta: %d %s\n", resp4->status_code, resp4->status_message);
    }
    
    free_http_request(req4);
    free_http_response(resp4);
    
    // Test DELETE
    printf("5️⃣ TEST DELETE\n");
    const char *delete_request = 
        "DELETE /users/3 HTTP/1.1\r\n"
        "\r\n";
    
    http_request_t *req5 = create_http_request();
    http_response_t *resp5 = create_http_response();
    
    if (parse_http_request(delete_request, req5) == 0) {
        handle_crud_request(req5, resp5);
        printf("Risposta: %d %s\n", resp5->status_code, resp5->status_message);
    }
    
    free_http_request(req5);
    free_http_response(resp5);
    
    // Test metodo non supportato
    printf("6️⃣ TEST METODO NON SUPPORTATO\n");
    const char *unsupported_request = 
        "OPTIONS /users HTTP/1.1\r\n"
        "\r\n";
    
    http_request_t *req6 = create_http_request();
    http_response_t *resp6 = create_http_response();
    
    if (parse_http_request(unsupported_request, req6) == 0) {
        handle_crud_request(req6, resp6);
        printf("Risposta: %d %s\n", resp6->status_code, resp6->status_message);
    }
    
    free_http_request(req6);
    free_http_response(resp6);
    
    printf("=====================================\n");
    printf("🏁 FINE TEST CRUD\n");
}

// Funzione main per testare il sistema
int main() {
    test_crud_operations();
    return 0;
}