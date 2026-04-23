#include "page_error.h"

void PageErrorBase::setMessage(const char *title, const char *body) {
    _title = title;
    _body  = body;
}
