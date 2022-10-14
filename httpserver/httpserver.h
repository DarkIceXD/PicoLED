#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include "lwip/ip_addr.h"

typedef const char *(*http_api_handler)(const char *path, const char *content);

typedef struct http_server_t_
{
    struct tcp_pcb *pcb;
    http_api_handler post_handler;
    ip_addr_t ip;
} http_server_t;

void http_server_init(http_server_t *d, ip_addr_t *ip, http_api_handler post_handler);
void http_server_deinit(http_server_t *d);

#endif
