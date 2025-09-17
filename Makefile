# Nombre del ejecutable
TARGET = imageserver

# Archivos fuente
SRCS = main.c clasificador.c histogram.c

# Archivos objeto
OBJS = $(SRCS:.c=.o)

# Compilador y flags
CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lm

# Regla principal
all: $(TARGET)

# Cómo generar el ejecutable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Cómo compilar cada .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos compilados
clean:
	rm -f $(OBJS) $(TARGET)

# Instalar en /usr/local/bin (requiere sudo)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Ejecutar servidor (modo prueba)
run: $(TARGET)
	./$(TARGET)
