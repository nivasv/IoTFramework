
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unqlite.h>

#include "thing.h"

int main(void) {
				int shmid;
	//		shmid = createsharedmemory(shm_key_sender, 1);
      //  printf("\nshmid: %d",  shmid);
        
        
			if ((shmid = shmget(shm_key_sender, 1, IPC_CREAT | 0666)) < 0) {
				perror("shmget");
				//printf("\n key: %x", key);
				printf("\n error1");
			}
	
			/*
			* Now we attach the segment to our data space.
			*/
			
			if ((serverstatus = shmat(shmid, NULL, 0)) == (char *) -1) {
				perror("shmat");
				printf("\nshmid: %d", shmid);
				exit(1);
			}
			
			printf ("\nserver status %s", serverstatus);
			if(strcmp(serverstatus,"0") == 0){
					strcpy(serverstatus, "1");
				} else{
					strcpy(serverstatus, "0");	 
				}
				
			printf ("\nserver status updated to %s", serverstatus);
}

