#ifndef HTTP_H
#define HTTP_H

#include <curl/curl.h>
#include <stddef.h>
#include <yyjson.h>

typedef struct {
    char *data;
    size_t size;
} http_response_t;

typedef struct {
    CURL *curl;
    struct curl_slist *headers;
    int delay;
} http_client_t;

void http_init(void);

void http_cleanup(void);

http_client_t *http_client_new(void);

void http_client_destroy(http_client_t *client);

void http_client_add_header(http_client_t *client, const char *header);

yyjson_doc *http_get_json(http_client_t *client, const char *url);

#endif
