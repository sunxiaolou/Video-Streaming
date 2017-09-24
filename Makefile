#make es una serie de reglas:
#goal:[espacio]dependencies
#[tab]comando
#dependencias dinámicas
#gcc -I inc -MM src/main.c >out/obj/main.d

CFLAGS = `pkg-config --cflags opencv` -lpthread
LIBS = `pkg-config --libs opencv`

#variables
#ruta a archivos fuente
SRC_PATH := src
#ruta a los includes
INC_PATH := inc
#ruta de salida
OUT_PATH := out
#ruta a los objetos
OBJ_PATH := $(OUT_PATH)/obj
#listado de archivos .c
SRC_FILES :=$(wildcard $(SRC_PATH)/*.c)
#listado de archivos .o
OBJ_FILES := $(subst $(SRC_PATH),$(OBJ_PATH),$(SRC_FILES:.c=.o))
#listado de archivos .o sin directorio
OBJS := $(notdir $(OBJ_FILES))
#nombre del ejecutable
APP := $(notdir $(shell pwd))

#agregamos ruta a archivos.c

vpath %.c $(SRC_PATH)

#agregamos ruta a archivos.o

vpath %.o $(OBJ_PATH)

#regla genérica de compilación
#$< es la variable con la dependencia
#$@ es la variable con el goal
%.o: %.c
	g++ -I $(INC_PATH) -c $< -o $(OBJ_PATH)/$@

#regla de linkeo, sólo los objetos
$(OUT_PATH)/$(APP): $(OBJS)
	g++ $(OBJ_FILES) $(CFLAGS) $(LIBS) -o $(OUT_PATH)/$(APP)

#regla de ejecución
run: $(OUT_PATH)/$(APP)
	@./$(OUT_PATH)/$(APP)

#regla clean para borrar archivos generados

clean:
	rm $(OUT_PATH)/$(APP)
	rm $(OBJ_PATH)/*.o

info:
	@echo SRC_FILES: $(SRC_FILES)
	@echo OBJ_FILES: $(OBJ_FILES)
	@echo OBJS: $(OBJS)
	@echo APP: $(APP)
