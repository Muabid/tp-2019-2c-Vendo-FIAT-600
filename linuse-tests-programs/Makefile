UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS := -D_XOPEN_SOURCE -Wno-deprecated-declarations
endif

CC := gcc
NAME := hilolayex
BUILD=bin
INCL_SHARED=/home/utnso/tp-2019-2c-Vendo-FIAT-600/shared
INCL_LIBMUSE=/home/utnso/tp-2019-2c-Vendo-FIAT-600/libmuse/Debug
INCL_COMMONS=/home/utnso/so-commons-library
all: clean $(NAME).so

clean:
	$(RM) *.o
	$(RM) *.so
	$(RM) -r bin/
	mkdir -p bin

$(NAME).o:
	$(CC) -c -Wall $(CFLAGS) -fpic hilolay_alumnos.c -o $(BUILD)/$(NAME).o -lhilolay

$(NAME).so: $(NAME).o
	$(CC) -shared -o $(BUILD)/lib$(NAME).so $(BUILD)/$(NAME).o -lhilolay 

entrega:
	$(CC) -I$(INCL_SHARED) -I$(INCL_LIBMUSE) -L$(INCL_COMMONS) -L./$(BUILD)/ -Wall $(CFLAGS) -o $(BUILD)/archivo_de_swap_supermasivo archivo_de_swap_supermasivo.c -l$(NAME) -lcommons
	$(CC) -I$(INCL_SHARED) -I$(INCL_LIBMUSE) -L$(INCL_COMMONS) -L./$(BUILD)/ -Wall $(CFLAGS) -o $(BUILD)/audiencia audiencia.c -l$(NAME) -lhilolay -lcommons
	$(CC) -I$(INCL_SHARED) -I$(INCL_LIBMUSE) -L$(INCL_COMMONS) -L./$(BUILD)/ -Wall $(CFLAGS) -o $(BUILD)/caballeros_de_SisOp_Afinador caballeros_de_SisOp_Afinador.c -l$(NAME) -lhilolay -lcommons
	$(CC) -I$(INCL_SHARED) -I$(INCL_LIBMUSE) -L$(INCL_COMMONS) -L./$(BUILD)/ -Wall $(CFLAGS) -o $(BUILD)/caballeros_de_SisOp_Solo caballeros_de_SisOp_Solo.c -l$(NAME) -lhilolay -lcommons
	$(CC) -I$(INCL_SHARED) -I$(INCL_LIBMUSE) -L$(INCL_COMMONS) -L./$(BUILD)/ -Wall $(CFLAGS) -o $(BUILD)/estres_compartido estres_compartido.c -l$(NAME) -lhilolay -lcommons
	$(CC) -I$(INCL_SHARED) -I$(INCL_LIBMUSE) -L$(INCL_COMMONS) -L./$(BUILD)/ -Wall $(CFLAGS) -o $(BUILD)/estres_privado estres_privado.c -l$(NAME) -lhilolay -lcommons
	$(CC) -I$(INCL_SHARED) -I$(INCL_LIBMUSE) -L$(INCL_COMMONS) -L./$(BUILD)/ -Wall $(CFLAGS) -o $(BUILD)/recursiva recursiva.c -l$(NAME) -lhilolay -lcommons
	$(CC) -I$(INCL_SHARED) -I$(INCL_LIBMUSE) -L$(INCL_COMMONS) -L./$(BUILD)/ -Wall $(CFLAGS) -o $(BUILD)/revolucion_compartida revolucion_compartida.c -l$(NAME) -lhilolay -lcommons
	$(CC) -I$(INCL_SHARED) -I$(INCL_LIBMUSE) -L$(INCL_COMMONS) -L./$(BUILD)/ -Wall $(CFLAGS) -o $(BUILD)/revolucion_privada revolucion_privada.c -l$(NAME) -lhilolay -lcommons

archivo_de_swap_supermasivo:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/archivo_de_swap_supermasivo

audiencia:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/audiencia

caballeros_de_SisOp_Afinador:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/caballeros_de_SisOp_Afinador

caballeros_de_SisOp_Solo:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/caballeros_de_SisOp_Solo

estres_compartido:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/estres_compartido

estres_privado:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/estres_privado

recursiva:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/recursiva

revolucion_compartida:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/revolucion_compartida

revolucion_privada:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):./$(BUILD) ./$(BUILD)/revolucion_privada

