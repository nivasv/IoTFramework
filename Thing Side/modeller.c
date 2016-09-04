
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "thing.h"

int main(int argc, char **argv) {

    int msqid_modeller;
    read_message rbuf;
    key_t msg_key_modeller = 0; // (key_t) 6000

    write_message wbuf;
    int msqid_sender;
    key_t msg_key_sender = (key_t) 2000;
    int msqid_checker;
    key_t msg_key_checker = (key_t) 4000;

    char *shm;
    int shmid_input;

	int opt;
    int i;
    int key_int = 0;

    char *thinginfo = NULL ; //"SSN01"

    int queue_size = 0; //10
    long shm_key_modeller = 0;// 7000
    size_t segment_len = 0; //128
    key_t key_shm;
    
    while ((opt = getopt(argc, argv, "k:l:m:q:t:")) != -1) {
    switch (opt) {
	case 'k':
	shm_key_modeller = atol(optarg);
	break;
	
	case 'l':
	segment_len = (size_t) atoi(optarg);
	break;
	
	case 'm':
	msg_key_modeller = (key_t) atoi(optarg);
	break;
	
	case 'q' :
	queue_size = atoi(optarg);
	break;
	
	case 't':
	thinginfo = optarg;
	break;
	}
  }
    
    if (shm_key_modeller  == 0 || segment_len == 0 || msg_key_modeller == 0 || queue_size == 0 || thinginfo == NULL){
	printf ("Invalid input values \n");
	exit(2);
}

    int shmid[queue_size];
    char new_str[(int) segment_len];
    
    if ((msqid_modeller = msgget(msg_key_modeller, msgflg_create)) < 0) {
        perror("msgget");
        exit(1);
    } else
        (void) fprintf(stderr, "msgget: msgget succeeded: msqid_modeller = %d\n", msqid_modeller);

    if ((msqid_sender = msgget(msg_key_sender, msgflg_create)) < 0) {
        perror("msgget");
        exit(1);
    } else
        (void) fprintf(stderr, "msgget: msgget succeeded: msqid_sender = %d\n", msqid_sender);

    if ((msqid_checker = msgget(msg_key_checker, msgflg_create)) < 0) {
        perror("msgget");
        exit(1);
    } else
        (void) fprintf(stderr, "msgget: msgget succeeded: msqid_checker = %d\n", msqid_checker);

    for (i = 0; i < queue_size; i++) {

        key_shm = (key_t) (shm_key_modeller + i);

        shmid[i] = createsharedmemory(key_shm, segment_len);

        printf("\nshmid[%d]: %d", i, shmid[i]);

        /*
         * Now we attach the segment to our data space.
         */

        if ((shm = shmat(shmid[i], NULL, 0)) == (char *) - 1) {
            perror("shmat");
            printf("\nshmid[i]: %d", shmid[i]);
            exit(1);
        }

        strcpy(shm, "*"); // Initialize the share memory

    }

    checkserver();

    while (1) {

        //waitifemptyqueue(msqid_modeller, 3);

        /*
         * Receive an answer of message type 1.
         */
        printf("\nmsqid_modeller %d", msqid_modeller);
        if (msgrcv(msqid_modeller, &rbuf, msgbuf_length, 1, 0) < 0) {
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
                printf("\nshmid: %d", shmid);
                exit(1);
            }

            /*
             * Now put some things into the memory for the
             * other process to read.
             */

            printf("\n shm %s, %d", shm, strlen(shm));

            strcpy(new_str, shm);
            strcpy(shm, "*"); // Initialize the share memory

            strcat(new_str, ",");
            strcat(new_str, thingid_tag);
            strcat(new_str, "'");
            strcat(new_str, thinginfo);
            strcat(new_str, "'");

            printf("\nValue: %s %d", new_str, (int) strlen(new_str));

            key_int = key_int % (queue_size);

            /*
             * Now we attach the segment to our data space.
             */

            if ((shm = shmat(shmid[key_int], NULL, 0)) == (char *) - 1) {
                perror("shmat");
                printf("\nshmid[key_int]: %d", shmid[key_int]);
                exit(1);
            }

            while (*shm != '*') {
                printf("\nShared memory wait");
                sleep(1);
            }

            /*
             * Now put some things into the memory for the
             * other process to read.
             */
            strcpy(shm, new_str);
            wbuf.mtype = 1;
            wbuf.key = (key_t) (shm_key_modeller + key_int);
            wbuf.len_shmid = (int) strlen(shm);
            printf("\nshm: %s %d", shm, strlen(shm));

            /*
             * Send a message.
             */
            printf("\nserver status %s", serverstatus);
            if (strcmp(serverstatus, "0") == 0) {
                if (msgsnd(msqid_checker, &wbuf, msgbuf_length, IPC_NOWAIT) < 0) {
                    printf("\n%d, %x, %d", msqid_checker, wbuf.key, (int) msgbuf_length);
                    perror("msgsnd");
                }
            } else {
                if (msgsnd(msqid_sender, &wbuf, msgbuf_length, IPC_NOWAIT) < 0) {
                    printf("\n%d, %x, %d", msqid_sender, wbuf.key, (int) msgbuf_length);
                    perror("msgsnd");
                }
            }

            //	sleep(1);
            key_int++;
        }
    }
    sleep(5);
    for (i = 0; i < queue_size; i++) {
        key_shm = shm_key_modeller + i;
        deletesharedmemory(key_shm, segment_len);
    }
}
