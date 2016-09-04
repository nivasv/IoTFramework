#include <stdio.h>
#include <libconfig.h>
#include <stdlib.h>
#include <string.h>
 
int main()
{
    config_t cfg, cfg1;               /*Returns all parameters in this structure */
    config_setting_t *setting;
    const char *sets, *status, *tokenId, *thingId, *server;
 
    char *config_file_name = "config.txt";
    //char *status_file_name = "status.txt";
 
    /*Initialization */
    config_init(&cfg);
    //config_init(&cfg1);

    /* Read the file. If there is an error, report it and exit. */
    /*if (!config_read_file(&cfg1, status_file_name))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg1);
        return -1;
    }*/

    if (!config_read_file(&cfg, config_file_name))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }
   
    /*if (config_lookup_string(&cfg1, "Registered", &status)){
        printf("\nStatus: %s", status);
	if(strcmp(status, "Yes") == 0){
		if (config_lookup_string(&cfg1, "ThingId", &thingId))
        		printf("\nTId: %s", thingId);
		if (config_lookup_string(&cfg1, "TokenId", &tokenId))
        		printf("\nToId: %s", tokenId);
	}
      }
    else
        {
	// register
	char *r;
	r = malloc(128);
    	r[0] = '\0';
    	strcat(r, "./coap-client -m post -e \"registration\" -B 5 coap://");
	strcat(r, server);
	strcat(r, ":5683/Data");
	const char *ter = r;
	FILE *fp = popen(ter, "r");
	char *res;
	fscanf(fp, "%s", &res);
        pclose(fp);
	char *c;
   	FILE *fptr;
   	fptr=fopen("status.txt","w");
   	if(fptr==NULL){
      		printf("Error!");
      		exit(1);
   	}
		c = malloc(128);
    		c[0] = '\0';
    		strcat(c, "Registered = \"Yes\";\nThingId = \"");
		strcat(c, "7465");
		strcat(c, "\"; \nTokenId = \"");
		strcat(c, "7467");
		strcat(c, "\"; \n");
   	fprintf(fptr,"%s",c);
   	fclose(fptr);
	config_destroy(&cfg1);
 	}*/
 
    /*Read the parameter group*/
    setting = config_lookup(&cfg, "params");
	 if (config_setting_lookup_string(setting, "Server", &server))
        {
            printf("\nServerIP: %s", server);
        }
        else
            printf("\nNo 'Server' setting in configuration file.");

	if (config_setting_lookup_string(setting, "ThingId", &thingId))
        {
            printf("\n ThingId: %s", thingId);
        }
        else
            printf("\nNo thingid setting in configuration file.");

    if (setting != NULL)
    {
        
        if (config_setting_lookup_string(setting, "Sets", &sets))
        {	
	    int set = atoi(sets);	
            printf("\nNo. of sets: %d", set);
		int i,key;
	    for (i = 1; i <= set; i++){
		char count[5];
		sprintf(count, "%d", i);
		const char *sender, *modeler, *sensorNos;
		char *t1;
		t1 = malloc(64);
    		strcpy(t1,"\0");
    		strcat(t1, "Sender_");
		strcat(t1, count);
		const char *temp1 = t1;
		
		char *t2;
		t2 = malloc(64);
    		strcpy(t2,"\0");
    		strcat(t2, "Modeler_");
		strcat(t2, count);
		const char *temp2 = t2;
		
		char *t3;
		t3 = malloc(64);
    		strcpy(t3,"\0");
    		strcat(t3, "Sensor_drivers_no_");
		strcat(t3, count);
		const char *temp3 = t3;
		
		if (config_setting_lookup_string(setting, temp1, &sender)){
		char key_s[10];
		key = 1000 + i;
		sprintf(key_s, "%d", key);
		char *s;
		s = malloc(128);
    		strcpy(s,"\0");
    		strcat(s, "./coap-sender -l 128 -q 10 -s COAP://");
		strcat(s, server);
		strcat(s, ":5683/Data > sender.txt &");
		//strcat(s, key_s);
		//strcat(s, " > sender.txt &");
		const char *te = s;
			system(te);
			free(s);
			
        	 }
        	  else
            		printf("\nNo 'Sender' setting in configuration file.");

		 if(i == 1){
		 	//start checker
		 	sleep(2);
		 	system("./serverstatus > serverstatus.txt &");
			system("./checker > checker.txt &");	
		 }

		 if (config_setting_lookup_string(setting, temp2, &modeler)){
            		printf("\nNo. of modelers: %s", modeler);
			char *m;
			m = malloc(128);
    			strcpy(m,"\0");
    			strcat(m, "./modeller -k 7000 -l 128 -m 6000 -q 10 -t ");
			strcat(m, thingId);
			strcat(m, " > modeler.txt &");
			const char *te1 = m;
			system(te1);
			free(m);
        	 }
        	  else
            		printf("\nNo 'Modeler' setting in configuration file.");
		
		if (config_setting_lookup_string(setting, temp3, &sensorNos)){
			int senNo = atoi(sensorNos);
            		printf("\nNo. of sensorNos: %d", senNo);
			int j, sen = 7;
			for(j = 1;j <= senNo; j++){
			char count1[5];
			sprintf(count1, "%d", j);
			const char *driver;
			char *d1;
			d1 = malloc(128);
    			strcpy(d1,"\0");
    			strcat(d1, "Sensor_driver");
			strcat(d1, count1);
			strcat(d1, "_");
			strcat(d1, count);
			const char *tem = d1;
				
			
			if (config_setting_lookup_string(setting, tem, &driver)){
			
            		printf("\nNo. of sensorDr: %s", driver);
			char *sd;
			sd = malloc(128);
    			sd[0] = '\0';
    			strcat(sd, "./sensordriver -k ");
			char count2[6];
			int sensor_count = 0;
			sensor_count = (7 + j) * 1000;
			sprintf(count2, "%d", sensor_count);
			
			strcat(sd, count2);
			strcat(sd," -l 128 -m 6000 -q 10 -s ");
			strcat(sd, driver);
			strcat(sd, " > driver");
			strcat(sd, count1);
			strcat(sd,".txt &");
			const char *te1 = sd;
			printf("\nNo. of sensorDr: %s", te1);
			system(te1);
			free(sd);
			free(d1);
			free(t1);
			free(t2);
			free(t3);	
			
        	 }
        	  else
            		printf("\nNo 'Drivers_no' setting in configuration file.");
		}
        	 }
        	  else
            		printf("\nNo 'Sensor_drivers_no' setting in configuration file.");

	    }
	    
        }
        else
            printf("\n'Sets' not set in configuration file.");
 
 
        printf("\n");
    }
 
    config_destroy(&cfg);
}
