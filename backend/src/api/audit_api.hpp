#ifndef AUDIT_API_HPP
#define AUDIT_API_HPP
#include <httplib.h>
void register_audit_routes(httplib::Server& svr);
#endif
