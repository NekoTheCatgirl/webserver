#include <dynstr.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct [[maybe_unused]] dynstr_t {
    char *data;
    size_t len;
    size_t cap;
};

dynstr_t *dynstr_create(void) {
    dynstr_t *str = malloc(sizeof(dynstr_t));
    if (str == nullptr) return nullptr;

    str->data = malloc(16 * sizeof(char));

    if (str->data == nullptr) {
        free(str);
        return nullptr;
    }

    str->len = 0;
    str->cap = 16;

    return str;
}

void dynstr_destroy(dynstr_t *str) {
    if (str == nullptr) return;

    free(str->data);
    free(str);
}

void dynstr_append(dynstr_t *str, const char *data, const size_t len) {
    if (str == nullptr || data == nullptr || len == 0) return;

    if (str->len + len + 1 > str->cap - 1) {
        const size_t new_cap = str->cap + len;
        char *ns = realloc(str->data, new_cap * sizeof(char));
        if (!ns) return;
        str->data = ns;
        str->cap = new_cap;
    }

    memcpy(str->data + str->len, data, len);
    str->len += len;
}

const char *dynstr_get(const dynstr_t *str) {
    if (str == nullptr) return nullptr;
    return str->data;
}

size_t dynstr_len(const dynstr_t *str) {
    if (str == nullptr) return 0;
    return str->len;
}
