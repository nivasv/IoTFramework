
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "thing.h"

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

    /*
     * Create Message queue for receiving shared memory data
     */
    /*
     if ((msqid_checker = msgget(msg_key_checker, msgflg )) < 0) {
         perror("msgget");
         exit(1);
     }
     else 
      (void) fprintf(stderr,"msgget: msgget succeeded: msqid_checker = %d\n", msqid_checker);
    
         shmid_checker = createsharedmemory(shm_key_checker, 1);
	
                 printf("\nshmid_checker: %d", shmid_checker);
	
         /*
     * Now we attach the segment to our data space.
     */
    /*	 
            if ((shm_checker = shmat(shmid_checker, NULL, 0)) == (char *) -1) {
                    perror("shmat");
                    printf("\nshmid_checker: %d", shmid_checker);
                    exit(1);
            } 

            strcpy(shm_checker, "0"); // set the checker share memory to 0 as coap server is not checked

    /*
     * Create/Open Message queue for sending data if COAP server is not active
     */

    if ((msqid_sender = msgget(msg_key_sender, msgflg_create)) < 0) {
        perror("msgget");
        exit(1);
    } else
        (void) fprintf(stderr, "msgget: msgget succeeded: msqid_sender = %d\n", msqid_sender);

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
            strcpy(shm, "*"); // Initialize the share memory
        }
    }
}
