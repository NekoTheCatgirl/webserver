#ifndef WEBSERVER_RESPONSE_H
#define WEBSERVER_RESPONSE_H

#include <stdint.h>
#include <http.h>

typedef struct response_t response_t;

response_t *create_response(const char* version, uint16_t status, const char *status_text);
void set_content(response_t *response, const char *content);
void add_or_modify_header(response_t *response, http_header_t *header);
void add_or_modify_header_simple(response_t *response, const char *header_name, const char *header_value);

char *response_to_string(response_t *response);
void destroy_response(response_t *response);

#endif
