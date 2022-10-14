#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>

#include "dnsserver.h"
#include "lwip/udp.h"

#define PORT_DNS_SERVER 53
#define DUMP_DATA 0

typedef struct dns_header_t_
{
    uint16_t id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_record_count;
    uint16_t authority_record_count;
    uint16_t additional_record_count;
} dns_header_t;

#define MAX_DNS_MSG_SIZE 300

static int dns_socket_new_dgram(struct udp_pcb **udp, void *cb_data, udp_recv_fn cb_udp_recv)
{
    *udp = udp_new();
    if (*udp == NULL)
    {
        return -ENOMEM;
    }
    udp_recv(*udp, cb_udp_recv, (void *)cb_data);
    return ERR_OK;
}

static void dns_socket_free(struct udp_pcb **udp)
{
    if (*udp != NULL)
    {
        udp_remove(*udp);
        *udp = NULL;
    }
}

static int dns_socket_bind(struct udp_pcb **udp, uint32_t ip, uint16_t port)
{
    ip_addr_t addr;
    IP4_ADDR(&addr, ip >> 24 & 0xff, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
    err_t err = udp_bind(*udp, &addr, port);
    if (err != ERR_OK)
        assert(false);

    return err;
}

#if DUMP_DATA
static void dump_bytes(const uint8_t *bptr, uint32_t len)
{
    unsigned int i = 0;

    for (i = 0; i < len;)
    {
        if ((i & 0x0f) == 0)
        {
            printf("\n");
        }
        else if ((i & 0x07) == 0)
        {
            printf(" ");
        }
        printf("%02x ", bptr[i++]);
    }
    printf("\n");
}
#endif

static int dns_socket_sendto(struct udp_pcb **udp, const void *buf, size_t len, const ip_addr_t *dest, uint16_t port)
{
    if (len > 0xffff)
    {
        len = 0xffff;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL)
        return -ENOMEM;

    memcpy(p->payload, buf, len);
    err_t err = udp_sendto(*udp, p, dest, port);

    pbuf_free(p);

    if (err != ERR_OK)
        return err;

#if DUMP_DATA
    dump_bytes(buf, len);
#endif
    return len;
}

static void dns_server_process(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *src_addr, u16_t src_port)
{
    dns_server_t *d = arg;

    uint8_t dns_msg[MAX_DNS_MSG_SIZE];
    dns_header_t *dns_hdr = (dns_header_t *)dns_msg;

    size_t msg_len = pbuf_copy_partial(p, dns_msg, sizeof(dns_msg), 0);
    if (msg_len < sizeof(dns_header_t))
    {
        goto ignore_request;
    }

#if DUMP_DATA
    dump_bytes(dns_msg, msg_len);
#endif

    uint16_t flags = lwip_ntohs(dns_hdr->flags);
    uint16_t question_count = lwip_ntohs(dns_hdr->question_count);

    // flags from rfc1035
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    // Check QR indicates a query
    if (((flags >> 15) & 0x1) != 0)
        goto ignore_request;

    // Check for standard query
    if (((flags >> 11) & 0xf) != 0)
        goto ignore_request;

    // Check question count
    if (question_count < 1)
        goto ignore_request;

    const uint8_t *question_ptr_start = dns_msg + sizeof(dns_header_t);
    const uint8_t *question_ptr_end = dns_msg + msg_len;
    const uint8_t *question_ptr = question_ptr_start;
    while (question_ptr < question_ptr_end)
    {
        if (*question_ptr == 0)
        {
            question_ptr++;
            break;
        }
        else
        {
            int label_len = *question_ptr++;
            if (label_len > 63)
                goto ignore_request;

            question_ptr += label_len;
        }
    }

    // Check question length
    if (question_ptr - question_ptr_start > 255)
        goto ignore_request;

    // Skip QNAME and QTYPE
    question_ptr += 4;

    // Generate answer
    uint8_t *answer_ptr = dns_msg + (question_ptr - dns_msg);
    *answer_ptr++ = 0xc0;                         // pointer
    *answer_ptr++ = question_ptr_start - dns_msg; // pointer to question

    *answer_ptr++ = 0;
    *answer_ptr++ = 1; // host address

    *answer_ptr++ = 0;
    *answer_ptr++ = 1; // Internet class

    *answer_ptr++ = 0;
    *answer_ptr++ = 0;
    *answer_ptr++ = 0;
    *answer_ptr++ = 60; // ttl 60s

    *answer_ptr++ = 0;
    *answer_ptr++ = 4;                  // length
    memcpy(answer_ptr, &d->ip.addr, 4); // use our address
    answer_ptr += 4;

    dns_hdr->flags = lwip_htons(
        0x1 << 15 | // QR = response
        0x1 << 10 | // AA = authoritive
        0x1 << 7);  // RA = authenticated
    dns_hdr->question_count = lwip_htons(1);
    dns_hdr->answer_record_count = lwip_htons(1);
    dns_hdr->authority_record_count = 0;
    dns_hdr->additional_record_count = 0;

    dns_socket_sendto(&d->udp, &dns_msg, answer_ptr - dns_msg, src_addr, src_port);

ignore_request:
    pbuf_free(p);
}

void dns_server_init(dns_server_t *d, ip_addr_t *ip)
{
    if (dns_socket_new_dgram(&d->udp, d, dns_server_process) != ERR_OK)
        return;

    if (dns_socket_bind(&d->udp, 0, PORT_DNS_SERVER) != ERR_OK)
        return;

    ip_addr_copy(d->ip, *ip);
}

void dns_server_deinit(dns_server_t *d)
{
    dns_socket_free(&d->udp);
}
