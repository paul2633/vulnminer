#include <curl/curl.h>
#include <stdio.h>

#include "http.h"
#include "utils.h"

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

void http_client_init(http_client_t *client) {
    client->curl = curl_easy_init();
    EXIT_IF(client->curl == NULL, "curl_easy_init");

    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, write_callback));
    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_FOLLOWLOCATION, 1L));
    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_ACCEPT_ENCODING, ""));
    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_TIMEOUT, 16L));
}

void http_client_destroy(http_client_t *client) {
    curl_easy_cleanup(client->curl);
    curl_slist_free_all(client->headers);

    client->curl = NULL;
    client->headers = NULL;
}

void http_client_add_header(http_client_t *client, const char *header) {
    client->headers = curl_slist_append(client->headers, header);
    EXIT_IF(client->headers == NULL, "curl_slist_append");

    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, client->headers));
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
