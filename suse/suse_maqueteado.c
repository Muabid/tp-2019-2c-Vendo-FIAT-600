//¿Cómo es la implementación de los semáforos?
//¿Qué significa que "SUSE deberá saber en todo momento los procesos que fueron planificados en él"
//¿Cómo se sabe que un programa deja de estar en ejecución?

static int cantidadDeHilos = 0;

struct EstructuraHilo {
  int id;
  int estado; //Los estados expresados como numeros con #define
  int tiempoDeEjecucion;
  int tiempoDeEspera;
  int tiempoDeCpu;
};

struct EstructuraPrograma {
  EstructuraHilo* listaDeHilos = NULL;
  EstructuraHilo* colaReady = NULL;
  EstructuraHilo* enEjecucion = NULL;

};

int aceptarConexion() {
  //Si es exitoso retorna puerto y LOG
  //Si no es exitoso retorna -1 y lOG
  return puertoCliente;
}

EstructuraPrograma* crearEstructuras(int id) {
  //LOG al crear estructuras exitosamente
  return estructura;
}

char** parciarInstruccion(char* intruccion) {
  return instruccionParseada;
}

void gestionarInstruccion(char* instruccion) { //Nombre de las estructuras como  #define
  instruccionParseada = parciarInstruccion(instruccion);

  switch(instruccionParseada[0], programa) {
    //case SUSE_INIT: //Creo que esto no iria acá, se ejecutaria al aceptar la conexión
    case SUSE_CREATE: //Como parámetro de Hilolay se pasa el programa a ejecutar, se considera que queda parseado en instruccionParseada[1]
      crearHilo(instruccionParseada[1]);
    case SUSE_SCHEDULE_NEXT:
      //Hay que crear una función que en el caso que no se haya ingresado ningún hilo nuevo devuelva el siguente
      //en la cola, caso contrario hay que planificar SJF y luego retornar el elemento en la cola. ¿O hay que
      //planificar de nuevo, con los nuevos valore estadísticos para SJF?
    case SUSE_WAIT:
    case SUSE_SIGNAL:
    case SUSE_JOIN:
    otherwise:
      printf("La instruccion ingresada no es correcta\n", ); //Esto debería ser un LOG
  }
}

int main() {
  EstructuraHilo* colaNew = NULL;
  EstructuraHilo* colaBlocked = NULL; //Ver si es realmente una cola o una lista, que significa "esperar a que termine otro thread".
  EstructuraHilo* colaExit = NULL; //Es necesario ???
  EstructuraPrograma* listaDeProgramas = NULL;

}
