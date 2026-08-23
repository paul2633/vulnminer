#ifndef HTTP_H
#define HTTP_H

#include <curl/curl.h>
#include <stddef.h>

typedef struct {
    char *data;
    size_t size;
} http_response_t;

typedef struct {
    CURL *curl;
    struct curl_slist *headers;
} http_client_t;

void http_init(void);

void http_cleanup(void);

void http_client_init(http_client_t *client);

void http_client_destroy(http_client_t *client);

void http_client_add_header(http_client_t *client, const char *header);

CURLcode http_get(http_client_t *client, const char *url, http_response_t *response, long *status);

#endif
