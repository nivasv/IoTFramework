#!/bin/sh

ps -ef | grep coap-sender | grep -v grep | awk '{print $2}' | xargs kill ;
ps -ef | grep checker | grep -v grep | awk '{print $2}' | xargs kill ;
ps -ef | grep modeller | grep -v grep | awk '{print $2}' | xargs kill ;
ps -ef | grep sensordriver | grep -v grep | awk '{print $2}' | xargs kill;
ps -ef | grep serverstatus | grep -v grep | awk '{print $2}' | xargs kill;
