#include <status.h>
#include <dispatcher.h>

response_t * handle_index([[maybe_unused]] const request_t *req) {
    response_t *res = create_response("HTTP/1.1", STATUS_OK, nullptr);
    add_or_modify_header_simple(res, "Content-Type", "text/plain");
    set_content(res, "Hello, World!");
    return res;
}
