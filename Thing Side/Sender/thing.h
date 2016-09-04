/*
 * thing.h - The header file contains the methods required by processes running on the thing.
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>

/*
 * Tags
 */

char *data_tag = "1=";
char *time_tag = "2=";
char *sensor_tag = "3=";
//char *sensor_type_tag = "4=";
char *thingid_tag = "4=";
//char *location_tag = "6=";

// End of Tags

/*
 * Constants
 */

int msgflg_create = IPC_CREAT | 0666; //Message flag to create/access message queue
size_t msgbuf_length = 9; // Length of message buffer
key_t shm_key_server = (key_t) 1000; // Key of sender shared memory used to verify server availability

// End of Constants

/*
 * Variables
 */

char *serverstatus;

// End of Variables

/*
 * Declare the message structure.
 */

typedef struct msgbuf {
    long mtype;
    key_t key;
    int len_shmid;
    char mserverstatus;
} message_buf, read_message, write_message;

/*
 *  Creates shared memory segments
 */

int createsharedmemory(key_t key, size_t segment_len) {
    int shmid;
    char *shm;

    /*
     * Create the segment.
     */
    if ((shmid = shmget(key, segment_len, IPC_CREAT | IPC_EXCL | 0666)) < 0) {

        //	printf("\n duplicate segment - key: %x", key);

        if ((shmid = shmget(key, 0, IPC_CREAT | 0666)) < 0) {
            perror("shmget");
            //printf("\n key: %x", key);
        }

        /*
         * Delete the existing segment.
         */

        shm = shmat(shmid, NULL, 0);

        if (shmdt(shm) == -1) {
            fprintf(stderr, "shmdt failed\n");
            perror("shmdt");
        }

        if (shmctl(shmid, IPC_RMID, 0) == -1) {
            fprintf(stderr, "shmctl(IPC_RMID) failed\n");
            perror("shmctl");
        }

        if ((shmid = shmget(key, segment_len, IPC_CREAT | 0666)) < 0) {
            perror("shmget");
            //printf("\n key: %x", key);
        }
    }

    return shmid;
}//End of createsharedmemory

/*
 * Delete Shared memory segments
 */

void deletesharedmemory(key_t key, size_t segment_len) {
    int shmid;
    char *shm;

    /*
     * Create the segment.
     */
    if ((shmid = shmget(key, segment_len, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        printf("\nkey: %x", key);
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = shmat(shmid, NULL, 0)) == (char *) - 1) {
        perror("shmat");
        printf("\nshmid: %x", shmid);
        exit(1);
    }

    /*
     * Release the segments
     */
    if (shmdt(shm) == -1) {
        perror("shmop: shmdt failed");
    }

    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        fprintf(stderr, "shmctl(IPC_RMID) failed\n");
        perror("shmctl");
    }
} // End of deletesharedmemory

/*
 * Create timestamp
 */

char *timestamp() {
    char *timestamp = (char *) malloc(sizeof (char) * 16);
    time_t ltime;
    ltime = time(NULL);
    struct tm *tm;
    tm = localtime(&ltime);

    sprintf(timestamp, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    return timestamp;
} // end of timestamp

/*
 * Read the shared memory segment and release the segment by moving "*"
 */

char *readsharedmemory(key_t key, int len) {
    char *shm, *str = NULL;
    int shmid;

    /* 
     * Get the shared memory id
     */

    if ((shmid = shmget(key, len, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        //printf("\n key: %x", key);
    }

    /*
     * Now we attach the segment to our data space.
     */

    if ((shm = shmat(shmid, NULL, 0)) == (char *) - 1) {
        perror("shmat");
        printf("\nshmid: %d", shmid);
        exit(1);
    }

    strcpy(str, shm); // Store the shared memory

    strcpy(shm, "*"); // Initialize the share memory

    return str;
}

/*
 * Check if message queue is empty
 */

void waitifemptyqueue(int msqid, int waitseconds) {
    struct msqid_ds buf;
    /*
     * Check if queue is empty
     */
    msgctl(msqid, IPC_STAT, &buf);


    while ((uint) buf.msg_qnum < 1) {
        printf("\n Waiting for inputs");
        sleep(waitseconds);

        /*
         * Check if queue is empty
         */
        msgctl(msqid, IPC_STAT, &buf);
    }

    return;

}

/*
 * Check if enterprise server is available
 */

void checkserver() {
    int shmid;

    if ((shmid = shmget(shm_key_server, 1, IPC_CREAT | 0444)) < 0) {
        perror("shmget");
        //printf("\n key: %x", key);
        printf("\n error1");
    }

    /*
     * Now we attach the segment to our data space.
     */

    if ((serverstatus = shmat(shmid, NULL, 0)) == (char *) - 1) {
        perror("shmat");
        printf("\nshmid: %d", shmid);
        exit(1);
    }
}
