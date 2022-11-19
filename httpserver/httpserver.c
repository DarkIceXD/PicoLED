#include "httpserver.h"
#include "data.h"
#include "lwip/tcp.h"
#include "../algorithms/algorithms.h"
#include <string.h>
#define PORT_HTTP_SERVER 80

struct content
{
    const char *content;
    const char *type;
    int size;
};

typedef struct TCP_CONNECT_STATE_T_
{
    http_server_t *server;
    struct tcp_pcb *client_pcb;
    int total_size;
    struct content content;
} TCP_CONNECT_STATE_T;

static const char *extension_to_type(const char *extension)
{
    struct extension
    {
        const char *file_extension;
        const char *type;
    };
    static const struct extension e[] = {
        {"txt", "text/plain"},
        {"htm", "text/html"},
        {"html", "text/html"},
        {"js", "text/javascript"},
        {"css", "text/css"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"svg", "image/svg+xml"},
    };
    for (int i = 0; i < sizeof(e) / sizeof(e[0]); i++)
    {
        if (strncmp(extension, e[i].file_extension, strlen(e[i].file_extension)))
            continue;

        return e[i].type;
    }
    return NULL;
}

static int find_content(struct content *content, const char *path, const int path_size)
{
    for (int i = 0; i < sizeof(entries) / sizeof(entries[0]); i++)
    {
        if (strlen(entries[i].file_name) != path_size)
            continue;

        if (strncmp(entries[i].file_name, path, path_size) != 0)
            continue;

        content->content = entries[i].data;
        content->size = entries[i].data_size;
        content->type = extension_to_type(strchr(path + path_size - 5, '.') + 1);
        return 1;
    }

    for (int i = 0; i < sizeof(entries) / sizeof(entries[0]); i++)
    {
        if (strlen(entries[i].file_name) - 11 > path_size)
            continue;

        if (!strstr(entries[i].file_name, "index.htm"))
            continue;

        if (strncmp(entries[i].file_name, path, path_size) != 0)
            continue;

        content->content = entries[i].data;
        content->size = entries[i].data_size;
        content->type = extension_to_type(strchr(path + path_size - 5, '.') + 1);
        return 1;
    }
    return 0;
}

static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T *con_state)
{
    err_t err = ERR_OK;
    if (con_state->client_pcb)
    {
        tcp_arg(con_state->client_pcb, NULL);
        tcp_poll(con_state->client_pcb, NULL, 0);
        tcp_sent(con_state->client_pcb, NULL);
        tcp_recv(con_state->client_pcb, NULL);
        tcp_err(con_state->client_pcb, NULL);
        err = tcp_close(con_state->client_pcb);
        if (err != ERR_OK)
        {
            tcp_abort(con_state->client_pcb);
            err = ERR_ABRT;
        }
    }
    free(con_state);
    return err;
}

static int tcp_send_with_options(struct tcp_pcb *tpcb, const char *content, const int size, const int sent, const int options)
{
    if (sent >= size)
        return sent - size;

    const int max_size = min(tpcb->snd_buf, size - sent);
    err_t err = tcp_write(tpcb, content + sent, max_size, options);
    if (err != ERR_OK)
        return sent;

    return 0;
}

static int tcp_send_no_options(struct tcp_pcb *tpcb, const char *content, const int size, const int sent)
{
    return tcp_send_with_options(tpcb, content, size, sent, 0);
}

static int send_200_packet(struct tcp_pcb *tpcb, const struct content *content, int sent)
{
    const int buffer_start = tpcb->snd_buf;
    static const char header[] = "HTTP/1.0 200 OK\r\nContent-Length: ";
    static const int header_len = sizeof(header) - 1;
    sent = tcp_send_no_options(tpcb, header, header_len, sent);
    char len[16];
    snprintf(len, 16, "%d", content->size);
    sent = tcp_send_with_options(tpcb, len, strlen(len), sent, TCP_WRITE_FLAG_COPY);
    if (content->type)
    {
        static const char type[] = "\r\nContent-Type: ";
        static const int type_len = sizeof(type) - 1;
        sent = tcp_send_no_options(tpcb, type, type_len, sent);
        sent = tcp_send_no_options(tpcb, content->type, strlen(content->type), sent);
    }
    static const char header2[] = "\r\nConnection: close\r\n\r\n";
    static const int header2_len = sizeof(header2) - 1;
    sent = tcp_send_no_options(tpcb, header2, header2_len, sent);
    sent = tcp_send_no_options(tpcb, content->content, content->size, sent);
    tcp_output(tpcb);
    return buffer_start - tpcb->snd_buf;
}

static err_t send_404_packet(struct tcp_pcb *tpcb)
{
    static const char str404[] = "HTTP/1.0 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 12\r\nConnection: close\r\n\r\nnot found :/";
    static const int len = sizeof(str404) - 1;
    tcp_write(tpcb, str404, len, 0);
    return tcp_output(tpcb);
}

static err_t send_redirect_packet(struct tcp_pcb *tpcb, const char *location)
{
    static const char header[] = "HTTP/1.0 302 Redirect\r\nLocation: http://";
    static const int header_len = sizeof(header) - 1;
    tcp_write(tpcb, header, header_len, 0);
    tcp_write(tpcb, location, strlen(location), TCP_WRITE_FLAG_COPY);
    static const char header2[] = "\r\n\r\n";
    static const int header2_len = sizeof(header) - 1;
    tcp_write(tpcb, header2, header2_len, 0);
    return tcp_output(tpcb);
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T *)arg;
    con_state->total_size += send_200_packet(tpcb, &con_state->content, con_state->total_size);
    return ERR_OK;
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T *)arg;
    if (!p)
        return tcp_close_client_connection(con_state);

    if (p->tot_len > 0)
    {
        ((char *)p->payload)[p->len] = '\0';
        char *path_start = strchr(p->payload, '/');
        if (path_start)
        {
            if (strncmp(path_start, "/api", 4) == 0)
            {
                char *content_start = strstr(path_start, "\r\n\r\n");
                if (content_start)
                    con_state->content.content = con_state->server->post_handler(path_start, content_start + 4);
                if (con_state->content.content)
                {
                    con_state->content.size = strlen(con_state->content.content);
                    con_state->total_size = send_200_packet(tpcb, &con_state->content, 0);
                }
                else
                    send_404_packet(tpcb);
            }
            else
            {
                char *path_end = strchr(path_start, ' ');
                if (path_end)
                {
                    const int path_size = path_end - path_start;
                    if (find_content(&con_state->content, path_start, path_size))
                        con_state->total_size = send_200_packet(tpcb, &con_state->content, 0);
                    else
                        send_redirect_packet(tpcb, ipaddr_ntoa(&con_state->server->ip));
                }
            }
        }
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

static void tcp_server_err(void *arg, err_t err)
{
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T *)arg;
    if (err != ERR_ABRT)
        tcp_close_client_connection(con_state);
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err)
{
    http_server_t *server = (http_server_t *)arg;
    if (err != ERR_OK || client_pcb == NULL)
        return err;

    TCP_CONNECT_STATE_T *con_state = malloc(sizeof(TCP_CONNECT_STATE_T));
    if (!con_state)
        return ERR_MEM;

    con_state->server = server;
    con_state->client_pcb = client_pcb;
    con_state->total_size = 0;
    con_state->content.content = NULL;
    con_state->content.type = NULL;
    con_state->content.size = 0;

    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_err(client_pcb, tcp_server_err);
    return ERR_OK;
}

void http_server_init(http_server_t *d, ip_addr_t *ip, http_api_handler post_handler)
{
    d->pcb = tcp_new();
    if (!d->pcb)
        return;

    err_t err = tcp_bind(d->pcb, NULL, PORT_HTTP_SERVER);
    if (err)
        return;

    struct tcp_pcb *temp = tcp_listen_with_backlog(d->pcb, 1);
    if (!temp)
    {
        if (d->pcb)
            tcp_close(d->pcb);
        return;
    }
    d->pcb = temp;
    d->post_handler = post_handler;
    ip_addr_copy(d->ip, *ip);
    tcp_arg(d->pcb, d);
    tcp_accept(d->pcb, tcp_server_accept);
}

void http_server_deinit(http_server_t *d)
{
    tcp_arg(d->pcb, NULL);
    tcp_close(d->pcb);
}