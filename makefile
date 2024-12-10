headers = helper.h

executables = manager feed
manager_objects = manager.o files.o helper.o helper_messages.o helper_users.o helper_topics.o
feed_objects = feed.o helper.o

cflags = -Wall -Werror -pthread
export MSG_FICH = ./msg.txt

all: $(executables)

#link
manager: $(manager_objects)
	$(CC) -o $@ $^ $(cflags)

feed: $(feed_objects)
	$(CC) -o $@ $^ $(cflags)

#compile
%.o: %.c $(headers)
	$(CC) -c $< 

#runs
run: manager feed
	./manager

startup:
	mkdir fifos
	touch msg.txt

clean:
	$(RM) *.o
	$(RM) $(executables)

.PHONY: all clean run

