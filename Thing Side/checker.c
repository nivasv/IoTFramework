
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unqlite.h>

#include "thing.h"

int main(int argc, char **argv) {

    int msqid_checker;
    read_message rbuf;
    key_t msg_key_checker = (key_t) 4000;

    write_message wbuf;
    int msqid_sender;
    key_t msg_key_sender = (key_t) 2000;

    char *shm;
    int shmid_input;
    struct msqid_ds buf;

    int i;
    int key_int = 0;
    int datastored = 1;

    int queue_size = 10;
    long shm_key_checker = 5000;
    size_t segment_len = 128;
    key_t key_shm;
    int shmid[queue_size];
    char new_str[(int) segment_len];

    unqlite *pDb; /* Database handle */
    unqlite_kv_cursor *pCur; /* Cursor handle */
    int rc;
    char zKey[12];

    unqlite_int64 lData;
    int lKey, lData1;
    char *iKey, *iData;

    if ((msqid_checker = msgget(msg_key_checker, msgflg_create)) < 0) {
        perror("msgget");
        exit(1);
    } else
        (void) fprintf(stderr, "msgget: msgget succeeded: msqid_checker = %d\n", msqid_checker);

    if ((msqid_sender = msgget(msg_key_sender, msgflg_create)) < 0) {
        perror("msgget");
        exit(1);
    } else
        (void) fprintf(stderr, "msgget: msgget succeeded: msqid_sender = %d\n", msqid_sender);

    for (i = 0; i < queue_size; i++) {

        key_shm = (key_t) (shm_key_checker + i);

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

    /* Open the local database */
    rc = unqlite_open(&pDb, "Volansure.db", UNQLITE_OPEN_CREATE);
    if (rc != UNQLITE_OK) {
        Fatal(0, "Out of memory");
    }

    checkserver();
    while (1) {

        //waitifemptyqueue(msqid_checker, 3);

        /*
         * Check if queue is empty
         */
        msgctl(msqid_checker, IPC_STAT, &buf);
        if ((uint) buf.msg_qnum > 0) {
            /*
             * Receive an answer of message type 1.
             */

            if (msgrcv(msqid_checker, &rbuf, msgbuf_length, 1, 0) < 0) {
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

				//printf("\n new_str %s", new_str);
                unqlite_util_random_string(pDb, zKey, sizeof (zKey)); // Generate key

                /* Store records */
                rc = unqlite_kv_store(pDb, zKey, sizeof (zKey), new_str, strlen(new_str));

                if (rc != UNQLITE_OK) {
                    /* Insertion fail, extract database error log and exit */
                    Fatal(pDb, 0);
                }
                unqlite_commit(pDb); // commit
                datastored = 1;
            }
        }

        if (datastored == 1 && strcmp(serverstatus, "1") == 0) {
            sleep(1);
            printf("\ntransmitting data");
            /* Allocate a new cursor instance */
            rc = unqlite_kv_cursor_init(pDb, &pCur);
            if (rc != UNQLITE_OK) {
                Fatal(0, "Out of memory"); // Fatal may be replaced by restart routine to avoid contention due to lock by original thread.
            }

            /* Point to the first record */
            unqlite_kv_cursor_first_entry(pCur);

            while (unqlite_kv_cursor_valid_entry(pCur)  && strcmp(serverstatus, "1") == 0) {
                /* Extract key length */
                unqlite_kv_cursor_key(pCur, NULL, &lKey);

                //Allocate a buffer big enough to hold the record content
                iKey = (char *) malloc(lKey);
                if (iKey == NULL) {
                    printf("\n Problem in iKey");
                    return;
                }

                /* Extract key */
                unqlite_kv_cursor_key(pCur, iKey, &lKey);
                printf("\n\nKey ==> %s", iKey);

                /* Extract data length */
                unqlite_kv_cursor_data(pCur, NULL, &lData);
                printf("\nData length ==> %lld", lData);

                lData1 = lData; // move to int from unsigned int
                printf("\nlData1 ==> %d", lData1);

                //Allocate a buffer big enough to hold the record content
                iData = (char *) malloc(lData1 + 1);
                if (iData == NULL) {
                    printf("\n Problem in iData");
                    return;
                }
				iData[lData1] = '\0'; // Initialize
				
                /* Extract data */
                unqlite_kv_cursor_data(pCur, iData, &lData);
                printf("\nData ==> %s", iData);

                //		printf("\nValue: %s %d", new_str, (int) strlen(new_str));

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
                 printf("\n iData: %s", iData);
                strcpy(shm, iData);
                wbuf.mtype = 1;
                wbuf.key = (key_t) (shm_key_checker + key_int);
                wbuf.len_shmid = lData1;
                printf("\nshm: %s %d", shm, strlen(shm));

                /*
                 * Send a message.
                 */

                if (msgsnd(msqid_sender, &wbuf, msgbuf_length, IPC_NOWAIT) < 0) {
                    printf("\n%d, %x, %d", msqid_sender, wbuf.key, (int) msgbuf_length);
                    perror("msgsnd");
                }

                /* Delete entry */
                rc = unqlite_kv_cursor_delete_entry(pCur);
                if (rc != UNQLITE_OK) {
                    printf("\n Problem in delete ");
                    char *zBuf;
                    int iLen;
                    /* Extract database error log */
                    unqlite_config(pDb, UNQLITE_CONFIG_ERR_LOG, &zBuf, &iLen);
                    if (iLen > 0) {
                        puts(zBuf);
                    }
                    if (rc != UNQLITE_BUSY && rc != UNQLITE_NOTFOUND) {
                        /* Rollback */
                        printf("\n Delete rollback ");
                        unqlite_rollback(pDb);
                    }
                }
                unqlite_commit(pDb); // commit after every delete

                free(iKey);
                free(iData);
                //	sleep(1);
                key_int++;
            }
            /* Point to the last record */
		rc = unqlite_kv_cursor_last_entry(pCur);
		if( rc != UNQLITE_OK ){
            datastored = 0;
            printf("\n Database is empty");
		}
        }
    }

    unqlite_close(pDb);
    sleep(5);
    for (i = 0; i < queue_size; i++) {
        key_shm = shm_key_checker + i;
        deletesharedmemory(key_shm, segment_len);
    }
}

/*
 * Extract the database error log and exit.
 */
void Fatal(unqlite *pDb, const char *zMsg) {
    if (pDb) {
        const char *zErr;
        int iLen = 0; /* Stupid cc warning */

        /* Extract the database error log */
        unqlite_config(pDb, UNQLITE_CONFIG_ERR_LOG, &zErr, &iLen);
        if (iLen > 0) {
            /* Output the DB error log */
            puts(zErr); /* Always null termniated */
        }
    } else {
        if (zMsg) {
            puts(zMsg);
        }
    }
    /* Manually shutdown the library */
    unqlite_lib_shutdown();
    /* Exit immediately */
    exit(0);
}
