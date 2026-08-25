#include <curl/curl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "http.h"
#include "utils.h"

#define HTTP_MAX_TIMEOUT 30

#define CURL_OK(expr)                                                                                                                                          \
    do {                                                                                                                                                       \
        CURLcode err = (expr);                                                                                                                                 \
        EXIT_IF(err != CURLE_OK, curl_easy_strerror(err));                                                                                                     \
    } while (0)

void http_init(void) { CURL_OK(curl_global_init(CURL_GLOBAL_DEFAULT)); }

void http_cleanup(void) { curl_global_cleanup(); }

static size_t write_callback(const void *ptr, size_t size, size_t nmemb, void *userdata) {
    FILE *stream = userdata;
    return fwrite(ptr, size, nmemb, stream);
}

static void http_client_configure_curl(http_client_t *client) {
    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, write_callback));
    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_FOLLOWLOCATION, 1L));
    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_ACCEPT_ENCODING, ""));
    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_TIMEOUT, HTTP_MAX_TIMEOUT));

    if (client->headers != NULL)
        CURL_OK(curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, client->headers));
}

http_client_t *http_client_new(void) {
    http_client_t *client = calloc(1, sizeof(*client));
    EXIT_IF(client == NULL, "calloc");

    client->curl = curl_easy_init();
    EXIT_IF(client->curl == NULL, "curl_easy_init");

    http_client_configure_curl(client);
    EXIT_IF(pthread_mutex_init(&client->lock, NULL) != 0, "pthread_mutex_init");

    return client;
}

void http_client_destroy(http_client_t *client) {
    curl_easy_cleanup(client->curl);
    curl_slist_free_all(client->headers);
    pthread_mutex_destroy(&client->lock);

    client->curl = NULL;
    client->headers = NULL;

    free(client);
}

void http_client_add_header(http_client_t *client, const char *header) {
    client->headers = curl_slist_append(client->headers, header);
    EXIT_IF(client->headers == NULL, "curl_slist_append");

    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, client->headers));
}

void http_client_reset(http_client_t *client) {
    curl_easy_cleanup(client->curl);

    client->curl = curl_easy_init();
    EXIT_IF(client->curl == NULL, "curl_easy_init");

    http_client_configure_curl(client);
}

CURLcode http_get(http_client_t *client, const char *url, http_response_t *response, long *status) {
    FILE *stream = open_memstream(&response->data, &response->size);
    EXIT_IF(stream == NULL, "open_memstream");

    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_URL, url));
    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, stream));

    CURLcode err = curl_easy_perform(client->curl);

    if (err == CURLE_OK)
        CURL_OK(curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, status));

    EXIT_IF(fclose(stream) == EOF, "fclose");

    return err;
}
