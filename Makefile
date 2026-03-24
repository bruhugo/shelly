CC =g++
GFLAGS = Wall 
TARGET = main

$(TARGET): main.cpp
	$(CC) $(CFLAGS) -o $(TARGET) main.cpp

run: $(TARGET)
	./$(TARGET)
