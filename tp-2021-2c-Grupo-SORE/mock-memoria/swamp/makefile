	
	#Si coloco el flag -lreadline antes del -lprotocolo no encuentra la biblioteca readline
	#-I busca los .h de la biblio, -L busca la biblioteca -lib***.so-


TARGET = Swamp
LIBS = -pthread -lreadline -lcommons 
#LIBS = -lcommons 
CFLAGS = -I"../shared" -L"../shared" -g -Wall 


.PHONY: default all clean

default: $(TARGET) 
all: default 


OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS) 
	$(CC) $(CFLAGS) -c $< -o $@ 

.PRECIOUS: $(TARGET) $(OBJECTS) 

$(TARGET): $(OBJECTS) 
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@ 
	
clean:
	-rm -f *.o
	-rm -f $(TARGET)
