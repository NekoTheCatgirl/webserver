#ifndef WEBSERVER_DYNSTR_H
#define WEBSERVER_DYNSTR_H

#include <stddef.h>

typedef struct dynstr_t dynstr_t;

dynstr_t *dynstr_create(void);
void dynstr_destroy(dynstr_t *str);
void dynstr_append(dynstr_t *str, const char *data, size_t len);
const char *dynstr_get(const dynstr_t *str);
size_t dynstr_len(const dynstr_t *str);

#endif
