	

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
    int flags = 0;

static unsigned char _token_data[8];
str the_token = { 0, _token_data };

#define FLAGS_BLOCK 0x01

static coap_list_t *optlist = NULL;
/* Request URI.
 * TODO: associate the resources with transaction id and make it expireable */
static coap_uri_t uri;

/* reading is done when this flag is set */
static int ready = 0;

static str payload = { 0, NULL }; /* optional payload to send */

unsigned char msgtype = COAP_MESSAGE_CON; /* usually, requests are sent confirmable */

typedef unsigned char method_t;
method_t method = 2;		/* the method we are using in our requests - here, POST */

coap_block_t block = { .num = 0, .m = 0, .szx = 6 };

unsigned int wait_seconds = 2;	/* default timeout in seconds, here 2 second */
coap_tick_t max_wait;		/* global timeout (changed by set_timeout()) */

unsigned int obs_seconds = 1;	/* default observe time */
coap_tick_t obs_wait = 0;	/* timeout for current subscription */

#define min(a,b) ((a) < (b) ? (a) : (b))

  coap_context_t  *ctx = NULL;	
    coap_address_t dst;
  static char addr[INET6_ADDRSTRLEN];
  void *addrptr = NULL;
  fd_set readfds;
  struct timeval tv;
  int result;
  coap_tick_t now;
  coap_queue_t *nextpdu;
  coap_pdu_t  *pdu;
  static str server;
  unsigned short port = COAP_DEFAULT_PORT;
  char port_str[NI_MAXSERV] = "0";
  char node_str[NI_MAXHOST] = "";
  int opt, res;
  char *group = NULL;
  coap_log_t log_level = LOG_WARNING;
  coap_tid_t tid = COAP_INVALID_TID;
  
  // End of COAP variables

char ipaddress[] = "coap://localhost:5683";

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
cmdline_uri( ipaddress);
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
      && (inet_ntop(dst.addr.sa.sa_family, addrptr, addr, sizeof(addr)) != 0)
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
            
            if (!cmdline_input(new_str,&payload)){
			payload.length = 0;
			}
            
             if (! (pdu = coap_new_request(ctx, method, optlist, payload.s, payload.length))){
					printf("\n pdu failed.");
					//return -1;
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
    coap_split_uri((unsigned char *)arg, strlen(arg), &uri );

    if (uri.port != COAP_DEFAULT_PORT) {
      coap_insert( &optlist, 
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

coap_context_t * get_context(const char *node, const char *port) {
  coap_context_t *ctx = NULL;  
  int s;
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Coap uses UDP */
  hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV | AI_ALL;
  
  s = getaddrinfo(node, port, &hints, &result);
  if ( s != 0 ) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    return NULL;
  } 

int
order_opts(void *a, void *b) {
  if (!a || !b)
    return a < b ? -1 : 1;

  if (COAP_OPTION_KEY(*(coap_option *)a) < COAP_OPTION_KEY(*(coap_option *)b))
    return -1;

  return COAP_OPTION_KEY(*(coap_option *)a) == COAP_OPTION_KEY(*(coap_option *)b);
}


coap_list_t *
new_option_node(unsigned short key, unsigned int length, unsigned char *data) {
  coap_option *option;
  coap_list_t *node;

  option = coap_malloc(sizeof(coap_option) + length);
  if ( !option )
    goto error;

  COAP_OPTION_KEY(*option) = key;
  COAP_OPTION_LENGTH(*option) = length;
  memcpy(COAP_OPTION_DATA(*option), data, length);

  /* we can pass NULL here as delete function since option is released automatically  */
  node = coap_new_listnode(option, NULL);

  if ( node )
    return node;

 error:
  perror("new_option_node: malloc");
  coap_free( option );
  return NULL;
}

    
}
