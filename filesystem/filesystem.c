/*
 * filesystem.c
 *
 *  Created on: 31/8/2017
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include "filesystem.h"
#include <commons/collections/list.h>
#include <commons/config.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <commons/string.h>
#include <readline/readline.h>

#include <netinet/in.h>

#include "../compartidas/definiciones.h"
#include "../compartidas/compartidas.c"


#define MAX_LINEA 150
#define BACKLOG 20
tFS *fileSystem;
t_list *listaNodos;
int sock_yama;
int fd_max;
fd_set read_fd, master_fd;

int contadorHardCode;

int main(int argc, char* argv[]){


 	if(argc!=2){

	//arc: numero de argumentos que se han introducido (el nombre del programa también se toma como argumento)
	//argv: valor de cada argumento
	//error en la cantidad de parametros (el parametro es la ruta al archivo config)
	if(argc!=2){

		printf("Error en la cantidad de parametros\n");
		return EXIT_FAILURE;
	}
	puts("AAAAAAAAAAAAA");
	contadorHardCode=0;
	//crea una lista de Nodos, más adelante se especifica mas este punto
	list_create(listaNodos);

	//traigo los datos del archivo de configuracion y lo guardo en la estructura
	fileSystem=getConfigFilesystem(argv[1]);
	//hay que eliminar esto, muestra por consola los datos
	mostrarConfiguracion(fileSystem);

	//crea una estructura de hilo
	pthread_t consola_thread;

	//si no se pude crear el hilo, imprimi el error.
	//ptherad_create recibe:
	// un puntero a una estructura de hilo, y luego la devuelve con todos sus datos completos
	// el segundo parametro pueden ser atributos de creacion de hilos como prioridades. Si se pasa Null, queda la default
	// la funcion que deseo ejecutar
	//  los parametros de esa funcion
	if(pthread_create(&consola_thread, NULL, (void*) consolaFS, NULL) < 0){
			perror("no pudo crear hilo. error");
			return FALLO_GRAL;
		}

	//variables a utilizar
	int stat, ready_fds;
	int fd, new_fd;
	fd_max = -1;

	int sock_lis_datanode;


	// Creamos e inicializamos los conjuntos que retendran sockets para el select()
	// creo que pone el cero el file descriptor, no entiendo muy bien
	FD_ZERO(&read_fd);
	FD_ZERO(&master_fd);



	// Creamos sockets para hacer listen() de datanodes
	// lo que hace makeListenSok¿ck, es que dado un puerto:
	// obtiene la address info, crea el socket servidor, y lo pone a escuchar (listen) en el puerto dado
	if ((sock_lis_datanode = makeListenSock(fileSystem->puerto_datanode)) < 0){
		fprintf(stderr, "No se pudo crear socket para escuchar! sock_lis_cpu: %d\n", sock_lis_datanode);
		return FALLO_CONEXION;
	}

	//no se que hace aca
	fd_max = MAX(sock_lis_datanode, fd_max);


	// Se agrega list_datanode al master

	FD_SET(sock_lis_datanode, &master_fd);

	//intenta escuchar al datanode, si no puede, vuelve a intentarlo
	while ((stat = listen(sock_lis_datanode, BACKLOG)) == -1){
		perror("Fallo listen a socket datanodes. error");
		puts("Reintentamos...\n");
	}

	//seria el buffet donde guardo la info que me llega
	tHeader *header_tmp = malloc(HEAD_SIZE); // para almacenar cada recv
	while (1){

		read_fd = master_fd;

		ready_fds = select(fd_max + 1, &read_fd, NULL, NULL, NULL);
		if(ready_fds == -1){
			perror("Fallo el select(). error");
			return FALLO_SELECT;
		}

		for (fd = 0; fd <= fd_max; ++fd){
			if (FD_ISSET(fd, &read_fd)){

				printf("Hay un socket listo! El fd es: %d\n", fd);

				// Controlamos el listen de datanode
				if (fd == sock_lis_datanode){
					new_fd = handleNewListened(fd, &master_fd);
					if (new_fd < 0){
						perror("Fallo en manejar un listen. error");
						return FALLO_CONEXION;
					}

					fd_max = MAX(new_fd, fd_max);
					break;

				}

				// Como no es un listen, recibimos el header de lo que llego
				if ((stat = recv(fd, header_tmp, HEAD_SIZE, 0)) == -1){
					perror("Error en recv() de algun socket. error");
					break;

				} else if (stat == 0){
					printf("Se desconecto el socket %d\nLo sacamos del set listen...\n", fd);
					clearAndClose(&fd, &master_fd);
					break;
				}

				// Se recibio un header sin conflictos, procedemos con el flujo..

				if (header_tmp->tipo_de_proceso == DATANODE){
					printf("Llego algo desde DATANODE!\n");

					if((stat=datanodeHandler(header_tmp->tipo_de_mensaje,fd))<0){
						printf("Llego un mensaje no manejado desde datanode\n");
					}

					break;
				}
				if (fd == sock_yama){
					printf("Llego algo desde YAMA!\n");

					if((stat=yamaHandler(header_tmp->tipo_de_mensaje))<0){
						printf("Llego un mensaje no manejado desde yama\n");
					}

					break;


				}
				puts("Si esta linea se imprime, es porque el header_tmp tiene algun valor rarito...");
				printf("El valor de header_tmp es: proceso %d \t mensaje: %d", header_tmp->tipo_de_proceso, header_tmp->tipo_de_mensaje);

			}
		}


	}
	return 0;
}

void liberarPunteroDePunteros(char ** punteros) {
	int tamanio = 1;
	int i = 0;
	while (**punteros == NULL){
		free(punteros[i]);
	}
	free(punteros);
}

void procesarInput(char** palabras) {
	if (string_equals_ignore_case(*palabras, "format")) {
		printf("ya pude formatear el fs\n");
	} else if (string_equals_ignore_case(*palabras, "rm")) {
		printf("ya pude remover el archivo\n");
	} else if (string_equals_ignore_case(*palabras, "rename")) {
		printf("ya pude renombrar el archivo\n");
	} else if (string_equals_ignore_case(*palabras, "mv")) {
		printf("ya pude mover el archivo\n");
	} else if (string_equals_ignore_case(*palabras, "cat")) {
		printf("ya pude leer el archivo\n");
	} else if (string_equals_ignore_case(*palabras, "mkdir")) {
		printf("ya pude crear el directorio\n");
	} else if (string_equals_ignore_case(*palabras, "cpfrom")) {
		printf(
				"ya pude copiar el archivo local al file system siguiendo lineamientos\n");
	} else if (string_equals_ignore_case(*palabras, "cpto")) {
		printf("ya pude copiar un archivo local al file system\n");
	} else if (string_equals_ignore_case(*palabras, "cpblock")) {
		printf("ya pude crear una copia de un bloque del archivo en un nodo\n");
	} else if (string_equals_ignore_case(*palabras, "md5")) {
		printf("ya pude solicitar el md5 de un archivo del file system\n");
	} else if (string_equals_ignore_case(*palabras, "ls")) {
		printf("ya pude listar los archivos del directorio\n");
	} else if (string_equals_ignore_case(*palabras, "info")) {
		printf("ya pude mostrar la informacion del archivo\n");
	} else {
		printf("No existe el comando\n");
	}
}

void consolaFS(void){
	puts("Bienvenido a la consola. Ingrese un comando.");
			char * linea;
			char ** palabras;
			while(1){
				char *linea = readline(">");
				palabras = string_split(linea, " ");
				procesarInput(palabras);
				free(linea);
			}
}
int datanodeHandler(tMensaje msjRecibido,int fd_dn){


	printf("Mensaje Recibido desde Datanode: %d \n",msjRecibido);
	char* buffer;
	tPack2Bytes *infoWorker;

	switch(msjRecibido){
	(tPack2Bytes *) 
	case(NEW_DN):
		contadorHardCode++;

		puts("Nuevo Datanode ingresa al sistema");

		if(sistemaEstable()){
			recibirConexionYAMA();
			contadorHardCode=-200;
		}

		break;
	case(INFO_WORKER):

		puts("Nos llega la info del Worker Asociado");
		if ((buffer = recvGeneric(fd_dn)) == NULL){
			puts("Fallo recepcion de PATH_FILE_TOREDUCE");

			return FALLO_RECV;
		}

		if ((infoWorker = deserializeDosChar(buffer)) == NULL){

			puts("Fallo deserializacion de Bytes de la info del worker");
			return FALLO_RECV;
		}

		printf("Puerto Worker: %s , ip Worker: %s \n",infoWorker->bytes2,infoWorker->bytes1);
		freeAndNULL((void **) &buffer);
		puts("fin case INFO_WORKER");
		break;
	default:
		break;

	}


	return 0;
}
int yamaHandler(tMensaje msjRecibido){


	printf("Mensaje Recibido desde YAMA: %d \n",msjRecibido);

	contadorHardCode=-100;

	switch(msjRecibido){

	case(INICIOYAMA):

		puts("Conexion con YAMA exitosa");

		break;

	default:
		break;

	}

	return 0;
}






//Aca se va a hacer la verificacion entre la lista de nodos guardados q tiene el FS y los que se van conectando.
//Cuando se conecte el ultimo y haga estable el sistema, devolvera true.
bool sistemaEstable(){

	/*if...
	 *
	 */
	//hardcodeo por ahora
	return contadorHardCode>1;
}



int recibirConexionYAMA(void){

	int sock_lis_yama,new_fd;
	tHeader head, h_esp;
	if ((sock_lis_yama = makeListenSock(fileSystem->puerto_entrada)) < 0){
		printf("No se pudo crear socket listen en puerto: %s\n", fileSystem->puerto_entrada);

		return FALLO_GRAL;
	}

	if(listen(sock_lis_yama, BACKLOG) == -1){

		perror("Fallo de listen sobre socket filesystem. error");
		return FALLO_GRAL;
	}

	while (1){
		if((sock_yama = makeCommSock(sock_lis_yama)) < 0){

			puts("No se pudo acceptar conexion entrante de YAMA");
			return FALLO_GRAL;
		}
		printf("Se establecio conexion con YAMA. Socket %d\n", sock_yama);
		break;
	}

	fd_max = MAX(sock_yama, fd_max);
	FD_SET(sock_yama, &master_fd);

	return 0;
}

tFS *getConfigFilesystem(char* ruta){
	printf("Ruta del archivo de configuracion: %s\n", ruta);
	tFS *fileSystem = malloc(sizeof(tFS));

	fileSystem->puerto_entrada = malloc(MAX_PORT_LEN);
	fileSystem->puerto_datanode = malloc(MAX_PORT_LEN);
	fileSystem->puerto_yama = malloc(MAX_PORT_LEN);
	fileSystem->ip_yama = malloc(MAX_IP_LEN);

	//funcion de gaston, recibe la ruta del archivo de configuracion y te devuelve
	//un puntero a una estructura con todos los datos que va leyendo del archivo de conf
	t_config *fsConfig = config_create(ruta);

	//config_get_string_value recibe el nombre del parametro, y te devuelve el valor
	//con esto voy cargando en una estructura, todos los datos del archivo de conf
	strcpy(fileSystem->puerto_entrada, config_get_string_value(fsConfig, "PUERTO_FILESYSTEM"));
	strcpy(fileSystem->puerto_datanode, config_get_string_value(fsConfig, "PUERTO_DATANODE"));
	strcpy(fileSystem->puerto_yama, config_get_string_value(fsConfig, "PUERTO_YAMA"));
	strcpy(fileSystem->ip_yama, config_get_string_value(fsConfig, "IP_YAMA"));

	//cargo el tipo de proceso (harcodeado)
	fileSystem->tipo_de_proceso = FILESYSTEM;

	//destruye la estructura de configuracion, supongo que para liberar memoria o por seguridad
	config_destroy(fsConfig);
	//retorno la configuracion
	return fileSystem;
}s
void mostrarConfiguracion(tFS *fileSystem){

	printf("Puerto Entrada: %s\n",  fileSystem->puerto_entrada);
	printf("Puerto Datanode: %s\n",       fileSystem->puerto_datanode);
	printf("Puerto Yama: %s\n", fileSystem->puerto_yama);
	printf("IP Yama: %s\n", fileSystem->ip_yama);
	printf("Tipo de proceso: %d\n", fileSystem->tipo_de_proceso);
}









