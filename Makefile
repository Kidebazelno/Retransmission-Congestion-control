CC = g++

INCLUDE_OPENCV = `pkg-config --cflags --libs opencv4`

AGENT_SRC = agent.cpp
CRC32_SRC = crc32.cpp
SENDER_SRC = sender.cpp
RECEIVER_SRC = receiver.cpp
AGENT_EXEC = agent
CRC32_EXEC = crc32
SENDER_EXEC = sender
RECEIVER_EXEC = receiver

all: agent crc32 sender receiver

agent:
	$(CC) $(AGENT_SRC) -o $(AGENT_EXEC) -std=c++11
crc32:
	$(CC) $(CRC32_SRC) -lz -o $(CRC32_EXEC)
sender: $(SENDER_SRC)
	$(CC) $(SENDER_SRC) -lz -o $(SENDER_EXEC) $(INCLUDE_OPENCV) -std=c++11
receiver: $(RECEIVER_SRC)
	$(CC) $(RECEIVER_SRC) -lz -o $(RECEIVER_EXEC) $(INCLUDE_OPENCV) -std=c++11

.PHONY: clean

clean:
	rm $(AGENT_EXEC) $(CRC32_EXEC) $(SENDER_EXEC) $(RECEIVER_EXEC)