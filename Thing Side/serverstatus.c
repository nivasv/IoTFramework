
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "thing.h"

int main(int argc, char **argv) {
	
	write_message wbuf;
    int msqid_sender;
    key_t msg_key_sender = (key_t) 2000;
    
      if ((msqid_sender = msgget(msg_key_sender, msgflg_create)) < 0) {
        perror("msgget");
        exit(1);
    } else
        (void) fprintf(stderr, "msgget: msgget succeeded: msqid_sender = %d\n", msqid_sender);
    
				    checkserver();
				      wbuf.mtype = 1;
            wbuf.key = NULL;
            wbuf.len_shmid = 0;
            wbuf.mserverstatus = '1';
				    while (1){
						if (strcmp(serverstatus, "0") == 0){
							if (msgsnd(msqid_sender, &wbuf, msgbuf_length, IPC_NOWAIT) < 0) {
								printf("\n%d, %x, %d", msqid_sender, wbuf.key, (int) msgbuf_length);
								perror("msgsnd");
							}
							printf ("\n Status check request sent %c", wbuf.mserverstatus);
						}
						sleep(5);
					}
					
}

