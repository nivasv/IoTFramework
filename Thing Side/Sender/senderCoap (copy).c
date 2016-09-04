/* coap-client -- simple CoAP client
 *
 * Copyright (C) 2010--2014 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use. 
 */

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "thing.h"

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "coap.h"

int flags = 0;

static unsigned char _token_data[8];
str the_token = {0, _token_data};

#define FLAGS_BLOCK 0x01

static coap_list_t *optlist = NULL;
/* Request URI.
 * TODO: associate the resources with transaction id and make it expireable */
static coap_uri_t uri;
static str proxy = {0, NULL};
static unsigned short proxy_port = COAP_DEFAULT_PORT;

/* reading is done when this flag is set */
static int ready = 0;

static str output_file = {0, NULL}; /* output file name */
static FILE *file = NULL; /* output file stream */

static str payload = {0, NULL}; /* optional payload to send */

unsigned char msgtype = COAP_MESSAGE_CON; /* usually, requests are sent confirmable */

typedef unsigned char method_t;
method_t method = 1; /* the method we are using in our requests */

coap_block_t block = {.num = 0, .m = 0, .szx = 6};

unsigned int wait_seconds = 90; /* default timeout in seconds */
coap_tick_t max_wait; /* global timeout (changed by set_timeout()) */

unsigned int obs_seconds = 30; /* default observe time */
coap_tick_t obs_wait = 0; /* timeout for current subscription */

#define min(a,b) ((a) < (b) ? (a) : (b))

static inline void
set_timeout(coap_tick_t *timer, const unsigned int seconds) {
    coap_ticks(timer);
    *timer += seconds * COAP_TICKS_PER_SECOND;
}

int
append_to_output(const unsigned char *data, size_t len) {
    size_t written;

    if (!file) {
        if (!output_file.s || (output_file.length && output_file.s[0] == '-'))
            file = stdout;
        else {
            if (!(file = fopen((char *) output_file.s, "w"))) {
                perror("fopen");
                return -1;
            }
        }
    }

    do {
        written = fwrite(data, 1, len, file);
        len -= written;
        data += written;
    } while (written && len);
    fflush(file);

    return 0;
}

void
close_output(void) {
    if (file) {

        /* add a newline before closing in case were writing to stdout */
        if (!output_file.s || (output_file.length && output_file.s[0] == '-'))
            fwrite("\n", 1, 1, file);

        fflush(file);
        fclose(file);
    }
}

coap_pdu_t *
new_ack(coap_context_t *ctx, coap_queue_t *node) {
    coap_pdu_t *pdu = coap_new_pdu();

    if (pdu) {
        pdu->hdr->type = COAP_MESSAGE_ACK;
        pdu->hdr->code = 0;
        pdu->hdr->id = node->pdu->hdr->id;
    }

    return pdu;
}

coap_pdu_t *
new_response(coap_context_t *ctx, coap_queue_t *node, unsigned int code) {
    coap_pdu_t *pdu = new_ack(ctx, node);

    if (pdu)
        pdu->hdr->code = code;

    return pdu;
}

coap_pdu_t *
coap_new_request(coap_context_t *ctx, method_t m, coap_list_t *options,
        unsigned char *data, size_t length) {
    coap_pdu_t *pdu;
    coap_list_t *opt;

    if (!(pdu = coap_new_pdu()))
        return NULL;

    pdu->hdr->type = msgtype;
    pdu->hdr->id = coap_new_message_id(ctx);
    pdu->hdr->code = m;

    pdu->hdr->token_length = the_token.length;
    if (!coap_add_token(pdu, the_token.length, the_token.s)) {
        debug("cannot add token to request\n");
    }

    //coap_show_pdu(pdu);

    for (opt = options; opt; opt = opt->next) {
        coap_add_option(pdu, COAP_OPTION_KEY(*(coap_option *) opt->data),
                COAP_OPTION_LENGTH(*(coap_option *) opt->data),
                COAP_OPTION_DATA(*(coap_option *) opt->data));
    }

    if (length) {
        if ((flags & FLAGS_BLOCK) == 0)
            coap_add_data(pdu, length, data);
        else
            coap_add_block(pdu, length, data, block.num, block.szx);
    }

    return pdu;
}

coap_tid_t
clear_obs(coap_context_t *ctx,
        const coap_endpoint_t *local_interface,
        const coap_address_t *remote) {
    coap_pdu_t *pdu;
    coap_list_t *option;
    coap_tid_t tid = COAP_INVALID_TID;
    unsigned char buf[2];

    /* create bare PDU w/o any option  */
    pdu = coap_pdu_init(msgtype, COAP_REQUEST_GET,
            coap_new_message_id(ctx),
            COAP_MAX_PDU_SIZE);

    if (!pdu) {
        return tid;
    }

    if (!coap_add_token(pdu, the_token.length, the_token.s)) {
        coap_log(LOG_CRIT, "cannot add token");
        goto error;
    }

    for (option = optlist; option; option = option->next) {
        if (COAP_OPTION_KEY(*(coap_option *) option->data)
                == COAP_OPTION_URI_HOST) {
            if (!coap_add_option(pdu, COAP_OPTION_KEY(*(coap_option *) option->data),
                    COAP_OPTION_LENGTH(*(coap_option *) option->data),
                    COAP_OPTION_DATA(*(coap_option *) option->data))) {
                goto error;
            }
            break;
        }
    }

    if (!coap_add_option(pdu, COAP_OPTION_OBSERVE,
            coap_encode_var_bytes(buf, COAP_OBSERVE_CANCEL),
            buf)) {
        coap_log(LOG_CRIT, "cannot add option Observe: %u", COAP_OBSERVE_CANCEL);
        goto error;
    }

    for (option = optlist; option; option = option->next) {
        switch (COAP_OPTION_KEY(*(coap_option *) option->data)) {
            case COAP_OPTION_URI_PORT:
            case COAP_OPTION_URI_PATH:
            case COAP_OPTION_URI_QUERY:
                if (!coap_add_option(pdu, COAP_OPTION_KEY(*(coap_option *) option->data),
                        COAP_OPTION_LENGTH(*(coap_option *) option->data),
                        COAP_OPTION_DATA(*(coap_option *) option->data))) {
                    goto error;
                }
                break;
            default:
                ;
        }
    }

    coap_show_pdu(pdu);

    if (pdu->hdr->type == COAP_MESSAGE_CON)
        tid = coap_send_confirmed(ctx, local_interface, remote, pdu);
    else
        tid = coap_send(ctx, local_interface, remote, pdu);

    if (tid == COAP_INVALID_TID) {
        debug("clear_obs: error sending new request");
        coap_delete_pdu(pdu);
    } else if (pdu->hdr->type != COAP_MESSAGE_CON)
        coap_delete_pdu(pdu);

    return tid;
error:

    coap_delete_pdu(pdu);
    return tid;
}

int
resolve_address(const str *server, struct sockaddr *dst) {

    struct addrinfo *res, *ainfo;
    struct addrinfo hints;
    static char addrstr[256];
    int error, len = -1;

    memset(addrstr, 0, sizeof (addrstr));
    if (server->length)
        memcpy(addrstr, server->s, server->length);
    else
        memcpy(addrstr, "localhost", 9);

    memset((char *) &hints, 0, sizeof (hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_UNSPEC;

    error = getaddrinfo(addrstr, "", &hints, &res);

    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return error;
    }

    for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
        switch (ainfo->ai_family) {
            case AF_INET6:
            case AF_INET:
                len = ainfo->ai_addrlen;
                memcpy(dst, ainfo->ai_addr, len);
                goto finish;
            default:
                ;
        }
    }

finish:
    freeaddrinfo(res);
    return len;
}

#define HANDLE_BLOCK1(Pdu)						\
  ((method == COAP_REQUEST_PUT || method == COAP_REQUEST_POST) &&	\
   ((flags & FLAGS_BLOCK) == 0) &&					\
   ((Pdu)->hdr->code == COAP_RESPONSE_CODE(201) ||			\
    (Pdu)->hdr->code == COAP_RESPONSE_CODE(204)))

inline int
check_token(coap_pdu_t *received) {
    return received->hdr->token_length == the_token.length &&
            memcmp(received->hdr->token, the_token.s, the_token.length) == 0;
}

void
message_handler(struct coap_context_t *ctx,
        const coap_endpoint_t *local_interface,
        const coap_address_t *remote,
        coap_pdu_t *sent,
        coap_pdu_t *received,
        const coap_tid_t id) {

    coap_pdu_t *pdu = NULL;
    coap_opt_t *block_opt;
    coap_opt_iterator_t opt_iter;
    unsigned char buf[4];
    coap_list_t *option;
    size_t len;
    unsigned char *databuf;
    coap_tid_t tid;

#ifndef NDEBUG
    if (LOG_DEBUG <= coap_get_log_level()) {
        debug("** process incoming %d.%02d response:\n",
                (received->hdr->code >> 5), received->hdr->code & 0x1F);
        coap_show_pdu(received);
    }
#endif

    printf ("\ni am in message handler!!");
                sleep(2);
    /* check if this is a response to our original request */
    if (!check_token(received)) {
        /* drop if this was just some message, or send RST in case of notification */
        if (!sent && (received->hdr->type == COAP_MESSAGE_CON ||
                received->hdr->type == COAP_MESSAGE_NON))
            coap_send_rst(ctx, local_interface, remote, received);
        return;
    }

    if (received->hdr->type == COAP_MESSAGE_RST) {
        info("got RST\n");
        return;
    }

    /* output the received data, if any */
    if (COAP_RESPONSE_CLASS(received->hdr->code) == 2) {

        /* set obs timer if we have successfully subscribed a resource */
        if (sent && coap_check_option(received, COAP_OPTION_SUBSCRIPTION, &opt_iter)) {
            debug("observation relationship established, set timeout to %d\n", obs_seconds);
            set_timeout(&obs_wait, obs_seconds);
        }

        /* Got some data, check if block option is set. Behavior is undefined if
         * both, Block1 and Block2 are present. */
        block_opt = coap_check_option(received, COAP_OPTION_BLOCK2, &opt_iter);
        if (block_opt) { /* handle Block2 */
            unsigned short blktype = opt_iter.type;

            /* TODO: check if we are looking at the correct block number */
            if (coap_get_data(received, &len, &databuf))
                append_to_output(databuf, len);

            if (COAP_OPT_BLOCK_MORE(block_opt)) {
                /* more bit is set */
                debug("found the M bit, block size is %u, block nr. %u\n",
                        COAP_OPT_BLOCK_SZX(block_opt), coap_opt_block_num(block_opt));

                /* create pdu with request for next block */
                pdu = coap_new_request(ctx, method, NULL, NULL, 0); /* first, create bare PDU w/o any option  */
                if (pdu) {
                    /* add URI components from optlist */
                    for (option = optlist; option; option = option->next) {
                        switch (COAP_OPTION_KEY(*(coap_option *) option->data)) {
                            case COAP_OPTION_URI_HOST:
                            case COAP_OPTION_URI_PORT:
                            case COAP_OPTION_URI_PATH:
                            case COAP_OPTION_URI_QUERY:
                                coap_add_option(pdu, COAP_OPTION_KEY(*(coap_option *) option->data),
                                        COAP_OPTION_LENGTH(*(coap_option *) option->data),
                                        COAP_OPTION_DATA(*(coap_option *) option->data));
                                break;
                            default:
                                ; /* skip other options */
                        }
                    }

                    /* finally add updated block option from response, clear M bit */
                    /* blocknr = (blocknr & 0xfffffff7) + 0x10; */
                    debug("query block %d\n", (coap_opt_block_num(block_opt) + 1));
                    coap_add_option(pdu, blktype, coap_encode_var_bytes(buf,
                            ((coap_opt_block_num(block_opt) + 1) << 4) |
                            COAP_OPT_BLOCK_SZX(block_opt)), buf);

                    if (pdu->hdr->type == COAP_MESSAGE_CON)
                        tid = coap_send_confirmed(ctx, local_interface, remote, pdu);
                    else
                        tid = coap_send(ctx, local_interface, remote, pdu);

                    if (tid == COAP_INVALID_TID) {
                        debug("message_handler: error sending new request");
                        coap_delete_pdu(pdu);
                    } else {
                        set_timeout(&max_wait, wait_seconds);
                        if (pdu->hdr->type != COAP_MESSAGE_CON)
                            coap_delete_pdu(pdu);
                    }

                    return;
                }
            }
        } else { /* no Block2 option */
            block_opt = coap_check_option(received, COAP_OPTION_BLOCK1, &opt_iter);

            if (block_opt) { /* handle Block1 */
                block.szx = COAP_OPT_BLOCK_SZX(block_opt);
                block.num = coap_opt_block_num(block_opt);

                debug("found Block1, block size is %u, block nr. %u\n",
                        block.szx, block.num);

                if (payload.length <= (block.num + 1) * (1 << (block.szx + 4))) {
                    debug("upload ready\n");
                    ready = 1;
                    return;
                }

                /* create pdu with request for next block */
                pdu = coap_new_request(ctx, method, NULL, NULL, 0); /* first, create bare PDU w/o any option  */
                if (pdu) {

                    /* add URI components from optlist */
                    for (option = optlist; option; option = option->next) {
                        switch (COAP_OPTION_KEY(*(coap_option *) option->data)) {
                            case COAP_OPTION_URI_HOST:
                            case COAP_OPTION_URI_PORT:
                            case COAP_OPTION_URI_PATH:
                            case COAP_OPTION_CONTENT_FORMAT:
                            case COAP_OPTION_URI_QUERY:
                                coap_add_option(pdu, COAP_OPTION_KEY(*(coap_option *) option->data),
                                        COAP_OPTION_LENGTH(*(coap_option *) option->data),
                                        COAP_OPTION_DATA(*(coap_option *) option->data));
                                break;
                            default:
                                ; /* skip other options */
                        }
                    }

                    /* finally add updated block option from response, clear M bit */
                    /* blocknr = (blocknr & 0xfffffff7) + 0x10; */
                    block.num++;
                    block.m = ((block.num + 1) * (1 << (block.szx + 4)) < payload.length);

                    debug("send block %d\n", block.num);
                    coap_add_option(pdu, COAP_OPTION_BLOCK1, coap_encode_var_bytes(buf,
                            (block.num << 4) | (block.m << 3) | block.szx), buf);

                    coap_add_block(pdu, payload.length, payload.s, block.num, block.szx);
                    coap_show_pdu(pdu);
                    if (pdu->hdr->type == COAP_MESSAGE_CON)
                        tid = coap_send_confirmed(ctx, local_interface, remote, pdu);
                    else
                        tid = coap_send(ctx, local_interface, remote, pdu);

                    if (tid == COAP_INVALID_TID) {
                        debug("message_handler: error sending new request");
                        coap_delete_pdu(pdu);
                    } else {
                        set_timeout(&max_wait, wait_seconds);
                        if (pdu->hdr->type != COAP_MESSAGE_CON)
                            coap_delete_pdu(pdu);
                    }

                    return;
                }
            } else {
                /* There is no block option set, just read the data and we are done. */
                if (coap_get_data(received, &len, &databuf))
                    append_to_output(databuf, len);
            }
        }
    } else { /* no 2.05 */

        /* check if an error was signaled and output payload if so */
        if (COAP_RESPONSE_CLASS(received->hdr->code) >= 4) {
            fprintf(stderr, "%d.%02d",
                    (received->hdr->code >> 5), received->hdr->code & 0x1F);
            if (coap_get_data(received, &len, &databuf)) {
                fprintf(stderr, " ");
                fprintf(stderr, "Think data is here");
                while (len--)
                    fprintf(stderr, "%c", *databuf++);
            }
            fprintf(stderr, "\n");
        }

    }

    /* finally send new request, if needed */
    if (pdu && coap_send(ctx, local_interface, remote, pdu) == COAP_INVALID_TID) {
        debug("message_handler: error sending response");
    }
    coap_delete_pdu(pdu);

    /* our job is done, we can exit at any time */
    ready = coap_check_option(received, COAP_OPTION_SUBSCRIPTION, &opt_iter) == NULL;
}

int
join(coap_context_t *ctx, char *group_name) {
    struct ipv6_mreq mreq;
    struct addrinfo *reslocal = NULL, *resmulti = NULL, hints, *ainfo;
    int result = -1;

    /* we have to resolve the link-local interface to get the interface id */
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;

    result = getaddrinfo("::", NULL, &hints, &reslocal);
    if (result < 0) {
        fprintf(stderr, "join: cannot resolve link-local interface: %s\n",
                gai_strerror(result));
        goto finish;
    }

    /* get the first suitable interface identifier */
    for (ainfo = reslocal; ainfo != NULL; ainfo = ainfo->ai_next) {
        if (ainfo->ai_family == AF_INET6) {
            mreq.ipv6mr_interface =
                    ((struct sockaddr_in6 *) ainfo->ai_addr)->sin6_scope_id;
            break;
        }
    }

    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;

    /* resolve the multicast group address */
    result = getaddrinfo(group_name, NULL, &hints, &resmulti);

    if (result < 0) {
        fprintf(stderr, "join: cannot resolve multicast address: %s\n",
                gai_strerror(result));
        goto finish;
    }

    for (ainfo = resmulti; ainfo != NULL; ainfo = ainfo->ai_next) {
        if (ainfo->ai_family == AF_INET6) {
            mreq.ipv6mr_multiaddr =
                    ((struct sockaddr_in6 *) ainfo->ai_addr)->sin6_addr;
            break;
        }
    }

    result = setsockopt(ctx->sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
            (char *) &mreq, sizeof (mreq));
    if (result < 0)
        perror("join: setsockopt");

finish:
    freeaddrinfo(resmulti);
    freeaddrinfo(reslocal);

    return result;
}

int
order_opts(void *a, void *b) {
    if (!a || !b)
        return a < b ? -1 : 1;

    if (COAP_OPTION_KEY(*(coap_option *) a) < COAP_OPTION_KEY(*(coap_option *) b))
        return -1;

    return COAP_OPTION_KEY(*(coap_option *) a) == COAP_OPTION_KEY(*(coap_option *) b);
}

coap_list_t *
new_option_node(unsigned short key, unsigned int length, unsigned char *data) {
    coap_option *option;
    coap_list_t *node;

    option = coap_malloc(sizeof (coap_option) + length);
    if (!option)
        goto error;

    COAP_OPTION_KEY(*option) = key;
    COAP_OPTION_LENGTH(*option) = length;
    memcpy(COAP_OPTION_DATA(*option), data, length);

    /* we can pass NULL here as delete function since option is released automatically  */
    node = coap_new_listnode(option, NULL);

    if (node)
        return node;

error:
    perror("new_option_node: malloc");
    coap_free(option);
    return NULL;
}

typedef struct {
    unsigned char code;
    char *media_type;
} content_type_t;

void
cmdline_content_type(char *arg, unsigned short key) {
    static content_type_t content_types[] = {
        { 0, "plain"},
        { 0, "text/plain"},
        { 40, "link"},
        { 40, "link-format"},
        { 40, "application/link-format"},
        { 41, "xml"},
        { 42, "binary"},
        { 42, "octet-stream"},
        { 42, "application/octet-stream"},
        { 47, "exi"},
        { 47, "application/exi"},
        { 50, "json"},
        { 50, "application/json"},
        { 255, NULL}
    };
    coap_list_t *node;
    unsigned char i, value[10];
    int valcnt = 0;
    unsigned char buf[2];
    char *p, *q = arg;

    while (q && *q) {
        p = strchr(q, ',');

        if (isdigit(*q)) {
            if (p)
                *p = '\0';
            value[valcnt++] = atoi(q);
        } else {
            for (i = 0; content_types[i].media_type &&
                    strncmp(q, content_types[i].media_type, p ? p - q : strlen(q)) != 0;
                    ++i)
                ;

            if (content_types[i].media_type) {
                value[valcnt] = content_types[i].code;
                valcnt++;
            } else {
                warn("W: unknown content-type '%s'\n", arg);
            }
        }

        if (!p || key == COAP_OPTION_CONTENT_TYPE)
            break;

        q = p + 1;
    }

    for (i = 0; i < valcnt; ++i) {
        node = new_option_node(key, coap_encode_var_bytes(buf, value[i]), buf);
        if (node)
            coap_insert(&optlist, node, order_opts);
    }
}

void
cmdline_uri(char *arg) {
    unsigned char portbuf[2];
#define BUFSIZE 40
    unsigned char _buf[BUFSIZE];
    unsigned char *buf = _buf;
    size_t buflen;
    int res;

    if (proxy.length) { /* create Proxy-Uri from argument */
        size_t len = strlen(arg);
        while (len > 270) {
            coap_insert(&optlist,
                    new_option_node(COAP_OPTION_PROXY_URI,
                    270, (unsigned char *) arg),
                    order_opts);
            len -= 270;
            arg += 270;
        }

        coap_insert(&optlist,
                new_option_node(COAP_OPTION_PROXY_URI,
                len, (unsigned char *) arg),
                order_opts);
    } else { /* split arg into Uri-* options */
        coap_split_uri((unsigned char *) arg, strlen(arg), &uri);

        if (uri.port != COAP_DEFAULT_PORT) {
            coap_insert(&optlist,
                    new_option_node(COAP_OPTION_URI_PORT,
                    coap_encode_var_bytes(portbuf, uri.port),
                    portbuf),
                    order_opts);
        }

        if (uri.path.length) {
            buflen = BUFSIZE;
            res = coap_split_path(uri.path.s, uri.path.length, buf, &buflen);

            while (res--) {
                coap_insert(&optlist, new_option_node(COAP_OPTION_URI_PATH,
                        COAP_OPT_LENGTH(buf),
                        COAP_OPT_VALUE(buf)),
                        order_opts);

                buf += COAP_OPT_SIZE(buf);
            }
        }

        if (uri.query.length) {
            buflen = BUFSIZE;
            buf = _buf;
            res = coap_split_query(uri.query.s, uri.query.length, buf, &buflen);

            while (res--) {
                coap_insert(&optlist, new_option_node(COAP_OPTION_URI_QUERY,
                        COAP_OPT_LENGTH(buf),
                        COAP_OPT_VALUE(buf)),
                        order_opts);

                buf += COAP_OPT_SIZE(buf);
            }
        }
    }
}

int
cmdline_blocksize(char *arg) {
    unsigned short size;

again:
    size = 0;
    while (*arg && *arg != ',')
        size = size * 10 + (*arg++ -'0');

    if (*arg == ',') {
        arg++;
        block.num = size;
        goto again;
    }

    if (size)
        block.szx = (coap_fls(size >> 4) - 1) & 0x07;

    flags |= FLAGS_BLOCK;
    return 1;
}

/* Called after processing the options from the commandline to set 
 * Block1 or Block2 depending on method. */
void
set_blocksize(void) {
    static unsigned char buf[4]; /* hack: temporarily take encoded bytes */
    unsigned short opt;
    unsigned int opt_length;

    if (method != COAP_REQUEST_DELETE) {
        opt = method == COAP_REQUEST_GET ? COAP_OPTION_BLOCK2 : COAP_OPTION_BLOCK1;

        block.m = (opt == COAP_OPTION_BLOCK1) &&
                ((1 << (block.szx + 4)) < payload.length);

        opt_length = coap_encode_var_bytes(buf,
                (block.num << 4 | block.m << 3 | block.szx));

        coap_insert(&optlist, new_option_node(opt, opt_length, buf), order_opts);
    }
}

void
cmdline_subscribe(char *arg) {
    obs_seconds = atoi(optarg);
    coap_insert(&optlist, new_option_node(COAP_OPTION_SUBSCRIPTION, 0, NULL),
            order_opts);
}

int
cmdline_proxy(char *arg) {
    char *proxy_port_str = strrchr((const char *) arg, ':'); /* explicit port ? */
    if (proxy_port_str) {
        char *ipv6_delimiter = strrchr((const char *) arg, ']');
        if (!ipv6_delimiter) {
            if (proxy_port_str == strchr((const char *) arg, ':')) {
                /* host:port format - host not in ipv6 hexadecimal string format */
                *proxy_port_str++ = '\0'; /* split */
                proxy_port = atoi(proxy_port_str);
            }
        } else {
            arg = strchr((const char *) arg, '[');
            if (!arg) return 0;
            arg++;
            *ipv6_delimiter = '\0'; /* split */
            if (ipv6_delimiter + 1 == proxy_port_str++) {
                /* [ipv6 address]:port */
                proxy_port = atoi(proxy_port_str);
            }
        }
    }

    proxy.length = strlen(arg);
    if ((proxy.s = coap_malloc(proxy.length + 1)) == NULL) {
        proxy.length = 0;
        return 0;
    }

    memcpy(proxy.s, arg, proxy.length + 1);
    return 1;
}

inline void
cmdline_token(char *arg) {
    strncpy((char *) the_token.s, arg, min(sizeof (_token_data), strlen(arg)));
    the_token.length = strlen(arg);
}

void
cmdline_option(char *arg) {
    unsigned int num = 0;

    while (*arg && *arg != ',') {
        num = num * 10 + (*arg - '0');
        ++arg;
    }
    if (*arg == ',')
        ++arg;

    coap_insert(&optlist, new_option_node(num,
            strlen(arg),
            (unsigned char *) arg), order_opts);
}

extern int check_segment(const unsigned char *s, size_t length);
extern void decode_segment(const unsigned char *seg, size_t length, unsigned char *buf);

int
cmdline_input(char *text, str *buf) {
    int len;
    len = check_segment((unsigned char *) text, strlen(text));

    if (len < 0)
        return 0;

    buf->s = (unsigned char *) coap_malloc(len);
    if (!buf->s)
        return 0;

    buf->length = len;
    decode_segment((unsigned char *) text, strlen(text), buf->s);
    return 1;
}

int
cmdline_input_from_file(char *filename, str *buf) {
    FILE *inputfile = NULL;
    ssize_t len;
    int result = 1;
    struct stat statbuf;

    if (!filename || !buf)
        return 0;

    if (filename[0] == '-' && !filename[1]) { /* read from stdin */
        buf->length = 20000;
        buf->s = (unsigned char *) coap_malloc(buf->length);
        if (!buf->s)
            return 0;

        inputfile = stdin;
    } else {
        /* read from specified input file */
        if (stat(filename, &statbuf) < 0) {
            perror("cmdline_input_from_file: stat");
            return 0;
        }

        buf->length = statbuf.st_size;
        buf->s = (unsigned char *) coap_malloc(buf->length);
        if (!buf->s)
            return 0;

        inputfile = fopen(filename, "r");
        if (!inputfile) {
            perror("cmdline_input_from_file: fopen");
            coap_free(buf->s);
            return 0;
        }
    }

    len = fread(buf->s, 1, buf->length, inputfile);

    if (len < buf->length) {
        if (ferror(inputfile) != 0) {
            perror("cmdline_input_from_file: fread");
            coap_free(buf->s);
            buf->length = 0;
            buf->s = NULL;
            result = 0;
        } else {
            buf->length = len;
        }
    }

    if (inputfile != stdin)
        fclose(inputfile);

    return result;
}

method_t
cmdline_method(char *arg) {
    static char *methods[] = {0, "get", "post", "put", "delete", 0};
    unsigned char i;

    for (i = 1; methods[i] && strcasecmp(arg, methods[i]) != 0; ++i)
        ;

    return i; /* note that we do not prevent illegal methods */
}

coap_context_t *
get_context(const char *node, const char *port) {
    coap_context_t *ctx = NULL;
    int s;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Coap uses UDP */
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV | AI_ALL;

    s = getaddrinfo(node, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return NULL;
    }

    /* iterate through results until success */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        coap_address_t addr;

        if (rp->ai_addrlen <= sizeof (addr.addr)) {
            coap_address_init(&addr);
            addr.size = rp->ai_addrlen;
            memcpy(&addr.addr, rp->ai_addr, rp->ai_addrlen);

            ctx = coap_new_context(&addr);
            if (ctx) {
                /* TODO: output address:port for successful binding */
                goto finish;
            }
        }
    }

    fprintf(stderr, "no context available for interface '%s'\n", node);

finish:
    freeaddrinfo(result);
    return ctx;
}

int
main(void) {
    coap_context_t *ctx = NULL;
    coap_address_t dst;
    static char addr[INET6_ADDRSTRLEN];
    void *addrptr = NULL;
    fd_set readfds;
    struct timeval tv;
    int result;
    coap_tick_t now;
    coap_queue_t *nextpdu;
    coap_pdu_t *pdu;
    static str server;
    unsigned short port = COAP_DEFAULT_PORT;
    char port_str[NI_MAXSERV] = "0";
    char node_str[NI_MAXHOST] = "";
    int res;
    char *group = NULL;
    coap_log_t log_level = LOG_DEBUG;
    coap_tid_t tid = COAP_INVALID_TID;

    //char serverURL[] = "COAP://10.17.27.180:5683/Nikesh";
    char serverURL[] = "COAP://localhost:5683";

    size_t segment_len_checker = 128;
    read_message rbuf;

    int msqid_sender;
    key_t msg_key_sender = (key_t) 1000;
    int shmid_input;
    char *shm;

    int msqid_checker;
    key_t msg_key_checker = (key_t) 3000;
    // size_t msgbuf_length;

    key_t shm_key_sender = (key_t) 2000;
    char *shm_sender;
    int shmid_sender;

    //int queue_size = 10;
    //long shm_key_modeller = 6000;
    size_t segment_len = 128;
    //key_t key_shm;
    //int shmid[queue_size];
    char new_str[(int) segment_len];


    /*
     * Create Message queue for receiving shared memory data
     */

    if ((msqid_sender = msgget(msg_key_sender, msgflg_create)) < 0) {
        perror("msgget");
        exit(1);
    } else
        (void) fprintf(stderr, "msgget: msgget succeeded: msqid_sender = %d\n", msqid_sender);

    /*
     * Create/Open Message queue for sending data if COAP server is not active
     */


    /*
     *  COAP routines
     */

    strcpy(new_str, "post");
    method = cmdline_method(new_str);

    strcpy(new_str, "2");
    wait_seconds = atoi(new_str);

    /*
    if (!cmdline_input("Test", &payload))
        payload.length = 0;
     */

    strcpy(new_str, "Token123");
    cmdline_token(new_str);

    /*
     * // To listen port
        strncpy(port_str, optarg, NI_MAXSERV-1);
        port_str[NI_MAXSERV - 1] = '\0';
   
     */

    coap_set_log_level(log_level);

    while (1) {
        /*
         * Receive an answer of message type 1.
         */
        printf("\nmsqid_sender %d", msqid_sender);
        if (msgrcv(msqid_sender, &rbuf, msgbuf_length, 1, 0) < 0) {
            perror("msgrcv");
        } else {
            //      waitifemptyqueue(msqid_sender, 3);

            printf("\n key: %x, %d", rbuf.key, rbuf.len_shmid);

            if ((shmid_input = shmget(rbuf.key, rbuf.len_shmid, IPC_CREAT | 0666)) < 0) {
                perror("shmget");
                //printf("\n key: %x", key);
                printf("\n error1");
            }

            /*
             * Now we attach the segment to our data space.
             */

            if ((shm = shmat(shmid_input, NULL, 0)) == (char *) - 1) {
                perror("shmat");
                printf("\nshmid_input: %d", shmid_input);
                exit(1);
            }

            /*
             * Now put some things into the memory for the
             * other process to read.
             */

            printf("\n shm %s, %d", shm, (int) strlen(shm));
            //strcpy(new_str, shm);
            if (!cmdline_input(shm, &payload))
                payload.length = 0;
            strcpy(shm, "*"); // Initialize the share memory

            /* COAP Routines */

            cmdline_uri(serverURL);

            if (proxy.length) {
                server = proxy;
                port = proxy_port;
            } else {
                server = uri.host;
                port = uri.port;
            }

            /* resolve destination address where server should be sent */
            res = resolve_address(&server, &dst.addr.sa);

            if (res < 0) {
                fprintf(stderr, "failed to resolve address\n");
                exit(-1);
            }

            dst.size = res;
            dst.addr.sin.sin_port = htons(port);


            /* add Uri-Host if server address differs from uri.host */

            switch (dst.addr.sa.sa_family) {
                case AF_INET:
                    addrptr = &dst.addr.sin.sin_addr;

                    /* create context for IPv4 */
                    ctx = get_context(node_str[0] == 0 ? "0.0.0.0" : node_str, port_str);
                    break;
                case AF_INET6:
                    addrptr = &dst.addr.sin6.sin6_addr;

                    /* create context for IPv6 */
                    ctx = get_context(node_str[0] == 0 ? "::" : node_str, port_str);
                    break;
                default:
                    ;
            }

            if (!ctx) {
                coap_log(LOG_EMERG, "cannot create context\n");
                return -1;
            }

            coap_register_option(ctx, COAP_OPTION_BLOCK2);
            coap_register_response_handler(ctx, message_handler);

            /* join multicast group if requested at command line */
            if (group)
                join(ctx, group);

            /* construct CoAP message */

            if (!proxy.length && addrptr
                    && (inet_ntop(dst.addr.sa.sa_family, addrptr, addr, sizeof (addr)) != 0)
                    && (strlen(addr) != uri.host.length
                    || memcmp(addr, uri.host.s, uri.host.length) != 0)) {
                /* add Uri-Host */

                coap_insert(&optlist, new_option_node(COAP_OPTION_URI_HOST,
                        uri.host.length, uri.host.s),
                        order_opts);
            }

            /* set block option if requested at commandline */
            if (flags & FLAGS_BLOCK)
                set_blocksize();

            if (!(pdu = coap_new_request(ctx, method, optlist, payload.s, payload.length)))
                return -1;

            /*
#ifndef NDEBUG
            if (LOG_DEBUG <= coap_get_log_level()) {
                debug("sending CoAP request:\n");
                coap_show_pdu(pdu);
            }
#endif
             */

            if (pdu->hdr->type == COAP_MESSAGE_CON)
                tid = coap_send_confirmed(ctx, ctx->endpoint, &dst, pdu);
            else
                tid = coap_send(ctx, ctx->endpoint, &dst, pdu);


            printf("\nin end of send conf: %d", tid);

            if (pdu->hdr->type != COAP_MESSAGE_CON || tid == COAP_INVALID_TID)
                coap_delete_pdu(pdu);

            set_timeout(&max_wait, wait_seconds);
            debug("timeout is set to %d seconds\n", wait_seconds);

            while (!(ready && coap_can_exit(ctx))) {
                FD_ZERO(&readfds);
                FD_SET(ctx->sockfd, &readfds);

                nextpdu = coap_peek_next(ctx);

                coap_ticks(&now);
                while (nextpdu && nextpdu->t <= now - ctx->sendqueue_basetime) {
                    coap_retransmit(ctx, coap_pop_next(ctx));
                    nextpdu = coap_peek_next(ctx);
                }

                if (nextpdu && nextpdu->t < min(obs_wait ? obs_wait : max_wait, max_wait) - now) {
                    /* set timeout if there is a pdu to send */
                    tv.tv_usec = ((nextpdu->t) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
                    tv.tv_sec = (nextpdu->t) / COAP_TICKS_PER_SECOND;
                } else {
                    /* check if obs_wait fires before max_wait */
                    if (obs_wait && obs_wait < max_wait) {
                        tv.tv_usec = ((obs_wait - now) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
                        tv.tv_sec = (obs_wait - now) / COAP_TICKS_PER_SECOND;
                    } else {
                        tv.tv_usec = ((max_wait - now) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
                        tv.tv_sec = (max_wait - now) / COAP_TICKS_PER_SECOND;
                    }
                }

                result = select(ctx->sockfd + 1, &readfds, 0, 0, &tv);

                if (result < 0) { /* error */
                    perror("select");
                } else if (result > 0) { /* read from socket */
                    if (FD_ISSET(ctx->sockfd, &readfds)) {
                        int coapreadresult;
                        coapreadresult = coap_read(ctx); /* read received data */
                        printf("\nCoap read: %d", coapreadresult);
                        /* coap_dispatch( ctx );	/\* and dispatch PDUs from receivequeue *\/ */
                    }
                } else { /* timeout */
                    coap_ticks(&now);
                    if (max_wait <= now) {
                        info("timeout\n");
                        break;
                    }
                    if (obs_wait && obs_wait <= now) {
                        debug("clear observation relationship\n");
                        clear_obs(ctx, ctx->endpoint, &dst); /* FIXME: handle error case COAP_TID_INVALID */

                        /* make sure that the obs timer does not fire again */
                        obs_wait = 0;
                        obs_seconds = 0;
                    }
                }
            }
            close_output();
            optlist = NULL;
        }
    }
    coap_free_context(ctx);
}