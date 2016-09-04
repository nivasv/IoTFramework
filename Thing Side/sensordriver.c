
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "thing.h"
#include "abstract.h"

int main(int argc, char **argv) {

	int opt;
    int i;
    int key_int = 0;

    int queue_size = 0; //10
    long shm_key_sensor = 0; //8000
    size_t segment_len = 0; //128
    key_t key_shm;
    char *shm, *s;

    char *sensorid = NULL;

    int msqid;
    write_message sbuf;
    key_t msg_key_modeller; // (key_t) 6000;
    
    while ((opt = getopt(argc, argv, "k:l:m:q:s:")) != -1) {
    switch (opt) {
	case 'k':
	shm_key_sensor = atol(optarg);
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
	
	case 's':
	sensorid = optarg;
	break;
	}
  }

if (shm_key_sensor  == 0 || segment_len == 0 || msg_key_modeller == 0 || queue_size == 0 || sensorid == NULL){
	printf ("Invalid input values \n");
	exit(2);
}

int shmid[queue_size];

    if ((msqid = msgget(msg_key_modeller, msgflg_create)) < 0) {
        perror("msgget");
        exit(1);
    } else
        (void) fprintf(stderr, "msgget: msgget succeeded: msqid = %d\n", msqid);

    for (i = 0; i < queue_size; i++) {

        key_shm = (key_t) (shm_key_sensor + i);

        shmid[i] = createsharedmemory(key_shm, segment_len);

        printf("\nshmid[%d]: %d", i, shmid[i]);

        /*
         * Now we attach the segment to our data space.
         */

        if ((s = shmat(shmid[i], NULL, 0)) == (char *) - 1) {
            perror("shmat");
            printf("\nshmid[i]: %d", shmid[i]);
            exit(1);
        }

        strcpy(s, "*"); // Initialize the share memory

    }

    while (1) {
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

        printf("\nshm1: %s", shm);

        strcpy(shm, data_tag);
        strcat(shm, "'");
        strcat(shm, convert(sensorid));   
        strtok(shm, "\n");
        strcat(shm, "'");
        strcat(shm, ",");
        strcat(shm, time_tag);
        strcat(shm, timestamp());
        strcat(shm, ",");
        strcat(shm, sensor_tag);
        strcat(shm, "'");
        strcat(shm, sensorid);
        strcat(shm, "'");

        sbuf.mtype = 1;
        sbuf.key = (key_t) (shm_key_sensor + key_int);
        sbuf.len_shmid = (int) strlen(shm);
        printf("\nshm: %s %d", shm, strlen(shm));

        /*
         * Send a message.
         */
        if (msgsnd(msqid, &sbuf, msgbuf_length, IPC_NOWAIT) < 0) {
            printf("\n%d, %x, %d", msqid, sbuf.key, (int) msgbuf_length);
            perror("msgsnd");
        }

        sleep(5);
        key_int++;
    }

    sleep(5);
    for (i = 0; i < queue_size; i++) {
        key_shm = shm_key_sensor + i;
        deletesharedmemory(key_shm, segment_len);
    }
}
