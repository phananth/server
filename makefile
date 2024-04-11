#
# this makefile will compile and and all source
# found in the ~/develop/MyApp directory.  This represents a sample
# development directory structure and project
# 
# =======================================================
#                  MyApp Example
# =======================================================
# FINAL BINARY Target
./bin/chat-server : ./obj/chat-server.o 
	cc ./obj/chat-server.o -o ./bin/chat-server -lpthread
#
# =======================================================
#                     Dependencies
# =======================================================                     
./obj/chat-server.o : ./src/chat-server.c ./inc/chat-server.h ./inc/message.h
	cc -c ./src/chat-server.c -o ./obj/chat-server.o
#
# =======================================================
# Other targets
# =======================================================                     
all : ./bin/chat-server

clean:
	rm -f ./bin/*
	rm -f ./obj/*.o
	rm -f ./inc/*.h~
	rm -f ./src/*.c~



