#include "get_token_balances_args.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *sequence_build_get_token_balances_args(
    const char *contract_address,
    const char *account_address,
    bool include_metadata)
{
    struct json_object *root = json_object_new_object();
    struct json_object *page = json_object_new_object();

    if (!root || !page) {
        if (root) {
            json_object_put(root);
        }
        if (page) {
            json_object_put(page);
        }
        return NULL;
    }

    json_object_object_add(root, "contractAddress", json_object_new_string(contract_address ? contract_address : ""));
    json_object_object_add(root, "accountAddress", json_object_new_string(account_address ? account_address : ""));
    json_object_object_add(root, "includeMetadata", json_object_new_boolean(include_metadata));

    json_object_object_add(root, "page", page);
    json_object_object_add(page, "page", json_object_new_int(0));
    json_object_object_add(page, "pageSize", json_object_new_int(40));
    json_object_object_add(page, "more", json_object_new_boolean(false));

    const char *json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    char *json_out = json_str ? strdup(json_str) : NULL;

    json_object_put(root);
    return json_out;
}
