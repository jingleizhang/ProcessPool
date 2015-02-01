CFLAGS =	-g -Wall -fmessage-length=0

OBJS =		processpool_pipe_shm.o

LIBS =

TARGET =	test

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
