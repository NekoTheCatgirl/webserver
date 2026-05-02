#include <status.h>
#include <dispatcher.h>

response_t * handle_index(const request_t *req) {
    response_t *res = create_response("HTTP/1.1", 200, nullptr);
    add_or_modify_header_simple(res, "Content-Type", "text/plain");
    set_content(res, "Hello, World!");
    return res;
}
