#include <stdio.h>
#include "lib/networking/http_client.h"
#include "lib/evm/eoa_wallet.h"

int main(void) {
    HttpClient *c = http_client_create("https://httpbin.org");
    if (!c) {
        fprintf(stderr, "Failed to create HttpClient\n");
        return 1;
    }

    http_client_add_header(c, "Accept: application/json");

    const char *json = "{\"hello\":\"world\"}";

    HttpResponse r = http_client_post_json(c, "/post", json, 10000);

    if (r.error) {
        fprintf(stderr, "Request failed: %s\n", r.error);
        http_response_free(&r);
        http_client_destroy(c);
        return 1;
    }

    printf("Status: %ld\n", r.status_code);
    printf("Body (%zu bytes):\n%s\n", r.body_len, r.body);

    http_response_free(&r);
    http_client_destroy(c);
    return 0;
}