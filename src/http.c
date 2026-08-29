#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <yyjson.h>

#include "http.h"
#include "utils.h"

#define HTTP_MAX_DELAY 32

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
    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_TIMEOUT, HTTP_MAX_DELAY));

    if (client->headers != NULL)
        CURL_OK(curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, client->headers));
}

http_client_t *http_client_new(void) {
    http_client_t *client = calloc(1, sizeof(*client));
    EXIT_IF(client == NULL, "calloc");

    client->curl = curl_easy_init();
    EXIT_IF(client->curl == NULL, "curl_easy_init");

    http_client_configure_curl(client);

    return client;
}

void http_client_destroy(http_client_t *client) {
    curl_easy_cleanup(client->curl);
    curl_slist_free_all(client->headers);

    client->curl = NULL;
    client->headers = NULL;

    free(client);
}

void http_client_add_header(http_client_t *client, const char *header) {
    client->headers = curl_slist_append(client->headers, header);
    EXIT_IF(client->headers == NULL, "curl_slist_append");

    CURL_OK(curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, client->headers));
}

static void http_client_reset(http_client_t *client) {
    curl_easy_cleanup(client->curl);

    client->curl = curl_easy_init();
    EXIT_IF(client->curl == NULL, "curl_easy_init");

    http_client_configure_curl(client);
}

static CURLcode http_get(http_client_t *client, const char *url, http_response_t *response, long *status) {
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

yyjson_doc *http_get_json(http_client_t *client, const char *url) {
    while (true) {
        sleep(client->delay);

        http_response_t response = {0};
        long status = 0;

        CURLcode err = http_get(client, url, &response, &status);

        if (err == CURLE_OPERATION_TIMEDOUT) {
            client->delay = 0;
            http_client_reset(client);
            free(response.data);
            continue;
        }

        EXIT_IF(err != CURLE_OK, curl_easy_strerror(err));

        if (status == 403 || status == 429 || status >= 500) {
            client->delay = client->delay == 0 ? 1 : client->delay * 2 > HTTP_MAX_DELAY ? client->delay : client->delay * 2;
            http_client_reset(client);
            free(response.data);
            continue;
        }

        if (status == 404) {
            free(response.data);
            return NULL;
        }

        EXIT_IF(status < 200 || status >= 300, "HTTP error %ld", status);

        client->delay /= 2;
        yyjson_doc *doc = yyjson_read(response.data, response.size, 0);
        free(response.data);
        EXIT_IF(doc == NULL, "yyjson_read");

        return doc;
    }
}
