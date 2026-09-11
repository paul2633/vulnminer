#include <curl/curl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include "http.h"
#include "utils.h"

#define HTTP_MAX_TIMEOUT 30

#define NVD_MAX_DELAY 8
#define GITHUB_TIME_BETWEEN_REQUESTS 1
#define MAX_ERRORS 3

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

char *nvd_download(http_client_t *client, const char *url, size_t *response_size) {
    pthread_mutex_lock(&client->lock);
    unsigned unexpected_errors_count = 0;

    while (true) {
        sleep(client->delay);
        http_response_t response = {0};

        long status = 0;
        CURLcode err = http_get(client, url, &response, &status);

        if (err != CURLE_OK) {
            EXIT_IF(++unexpected_errors_count == MAX_ERRORS, curl_easy_strerror(err));
            client->delay = 1;
            http_client_reset(client);
            free(response.data);
            continue;
        }

        if (status == 429) {
            client->delay = client->delay == 0 ? 1 : client->delay * 2 > NVD_MAX_DELAY ? client->delay : client->delay * 2;
            http_client_reset(client);
            free(response.data);
            continue;
        }

        if (status < 200 || status >= 300) {
            EXIT_IF(++unexpected_errors_count == MAX_ERRORS, "HTTP error %ld", status);
            client->delay = 1;
            http_client_reset(client);
            free(response.data);
            continue;
        }

        client->delay /= 2;
        pthread_mutex_unlock(&client->lock);

        *response_size = response.size;
        return response.data;
    }
}

typedef struct {
    long remaining;
    long reset;
} github_headers_t;

static size_t github_header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    github_headers_t *headers = userdata;
    size_t total = size * nitems;

    const char *remaining_header = "x-ratelimit-remaining:";
    const char *reset_header = "x-ratelimit-reset:";

    if (strncasecmp(buffer, remaining_header, strlen(remaining_header)) == 0) {
        headers->remaining = strtol(buffer + strlen(remaining_header), NULL, 10);
    } else if (strncasecmp(buffer, reset_header, strlen(reset_header)) == 0) {
        headers->reset = strtol(buffer + strlen(reset_header), NULL, 10);
    }

    return total;
}

char *github_download(http_client_t *client, const char *url, size_t *response_size) {
    pthread_mutex_lock(&client->lock);
    unsigned unexpected_errors_count = 0;

    sleep(GITHUB_TIME_BETWEEN_REQUESTS);
    while (true) {
        http_response_t response = {0};
        long status = 0;

        github_headers_t headers = {-1, -1};
        CURL_OK(curl_easy_setopt(client->curl, CURLOPT_HEADERFUNCTION, github_header_callback));
        CURL_OK(curl_easy_setopt(client->curl, CURLOPT_HEADERDATA, &headers));

        CURLcode err = http_get(client, url, &response, &status);

        CURL_OK(curl_easy_setopt(client->curl, CURLOPT_HEADERFUNCTION, NULL));
        CURL_OK(curl_easy_setopt(client->curl, CURLOPT_HEADERDATA, NULL));

        if (err != CURLE_OK) {
            free(response.data);
            if (++unexpected_errors_count == MAX_ERRORS) {
                pthread_mutex_unlock(&client->lock);
                return NULL;
            }

            http_client_reset(client);
            sleep(GITHUB_TIME_BETWEEN_REQUESTS);
            continue;
        }

        if (status == 403) {
            long remaining = headers.remaining;
            long reset = headers.reset;

            EXIT_IF(remaining != 0, "HTTP error %ld with %ld remaining requests", status, remaining);
            EXIT_IF(reset < 0, "HTTP error %ld with missing x-ratelimit-reset", status);

            time_t now = time(NULL);
            EXIT_IF(reset < now, "HTTP error %ld with invalid x-ratelimit-reset", status);

            free(response.data);
            sleep((unsigned)(reset - now));
            continue;
        }

        if (status == 404 || status == 409 || status == 422) {
            pthread_mutex_unlock(&client->lock);
            free(response.data);
            return NULL;
        }

        if (status == 429) {
            free(response.data);
            sleep(60);
            continue;
        }

        if (status == 500 || status == 503) {
            free(response.data);
            sleep(10);
            continue;
        }

        if (status < 200 || status >= 300) {
            free(response.data);
            if (++unexpected_errors_count == MAX_ERRORS) {
                pthread_mutex_unlock(&client->lock);
                return NULL;
            }

            http_client_reset(client);
            sleep(GITHUB_TIME_BETWEEN_REQUESTS);
            continue;
        }

        pthread_mutex_unlock(&client->lock);

        *response_size = response.size;
        return response.data;
    }
}
