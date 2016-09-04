

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "thing.h"

#include "config.h"

#include <unistd.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "coap.h"

#define FLAGS_BLOCK 0x01

#define min(a,b) ((a) < (b) ? (a) : (b))

int main(void) {

    size_t segment_len_checker = 128;
    read_message rbuf;

    int msqid_sender;
    key_t msg_key_sender = (key_t) 1000;
    int shmid_input;
    char *shm;

    int msqid_checker;
    key_t msg_key_checker = (key_t) 3000;
    size_t msgbuf_length;

    key_t shm_key_sender = (key_t) 2000;
    char *shm_sender;
    int shmid_sender;

    //int queue_size = 10;
    //long shm_key_modeller = 6000;
    size_t segment_len = 128;
    //key_t key_shm;
    //int shmid[queue_size];
    char new_str[(int) segment_len];

    // COAP Variable declarations

    coap_context_t *ctx = NULL;
    coap_address_t dst;
    char addr[INET6_ADDRSTRLEN];
    void *addrptr = NULL;
    fd_set readfds;
    struct timeval tv;
    int result;
    coap_tick_t now;
    coap_queue_t *nextpdu;
    coap_pdu_t *pdu;
    str server;
    coap_uri_t uri;
    coap_list_t *optlist = NULL;
    unsigned short port = COAP_DEFAULT_PORT;
    char port_str[NI_MAXSERV] = "0";
    char node_str[NI_MAXHOST] = "";
    int opt, res;
    coap_log_t log_level = LOG_WARNING;
    coap_tid_t tid = COAP_INVALID_TID;

    /* reading is done when this flag is set */
    int ready = 0;

    // End of COAP variables

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
    cmdline_uri("COAP://localhost:5683");
    server = uri.host;
    port = uri.port;

    /* resolve destination address where server should be sent */
    res = resolve_address(&server, &dst.addr.sa);

    if (res < 0) {
        fprintf(stderr, "failed to resolve address\n");
        exit(-1);
    }

    dst.size = res;
    dst.addr.sin.sin_port = htons(port);

    while (1) {
        /*
         * Receive an answer of message type 1.
         */
        printf("\nmsqid_sender %d", msqid_sender);
        if (msgrcv(msqid_sender, &rbuf, msgbuf_length, 1, 0) < 0) {
            perror("msgrcv");
        } else {
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
                printf("\shmid_input: %d", shmid_input);
                exit(1);
            }

            /*
             * Now put some things into the memory for the
             * other process to read.
             */

            printf("\n shm %s, %d", shm, strlen(shm));
            strcpy(new_str, shm);
            strcpy(shm, "*"); // Initialize the share memory

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

            /* construct CoAP message */

            if (addrptr
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

#ifndef NDEBUG
            if (LOG_DEBUG <= coap_get_log_level()) {
                debug("sending CoAP request:\n");
                coap_show_pdu(pdu);
            }
#endif

            if (pdu->hdr->type == COAP_MESSAGE_CON)
                tid = coap_send_confirmed(ctx, ctx->endpoint, &dst, pdu);
            else
                tid = coap_send(ctx, ctx->endpoint, &dst, pdu);

            if (pdu->hdr->type != COAP_MESSAGE_CON || tid == COAP_INVALID_TID)
                coap_delete_pdu(pdu);

            set_timeout(&max_wait, wait_seconds);
            debug("timeout is set to %d seconds\n", wait_seconds);

            ready = 0;

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
                        coap_read(ctx); /* read received data */
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

            coap_free_context(ctx);


        }
    }
}

void cmdline_uri(char *arg) {
    unsigned char portbuf[2];
#define BUFSIZE 40
    unsigned char _buf[BUFSIZE];
    unsigned char *buf = _buf;
    size_t buflen;
    int res;
    /* split arg into Uri-* options */
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

inline void
set_timeout(coap_tick_t *timer, const unsigned int seconds) {
    coap_ticks(timer);
    *timer += seconds * COAP_TICKS_PER_SECOND;
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
order_opts(void *a, void *b) {
    if (!a || !b)
        return a < b ? -1 : 1;

    if (COAP_OPTION_KEY(*(coap_option *) a) < COAP_OPTION_KEY(*(coap_option *) b))
        return -1;

    return COAP_OPTION_KEY(*(coap_option *) a) == COAP_OPTION_KEY(*(coap_option *) b);
}

