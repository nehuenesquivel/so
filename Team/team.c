/*
 * team.c
 *
 *  Created on: 11 abr. 2020
 *      Author: utnso
 */
#include <stdio.h>
#include "team.h"
#include <stdint.h>



//#include <libs/conexion.h>
//atoi(test_split[0])


void *myThreadFun(void *vargp)
{
    sleep(1);
    printf("Printing GeeksQuiz from Thread \n");
    return NULL;
}


int main(void)
{
	uint32_t a = 123;
	int conexion;
	char* ip;
	char* puerto;

	t_log* logger;
	t_config* config;

	logger = iniciar_logger();
	log_info(logger, "hola soy lasti123");

	config = leer_config();
	bool test = config_has_property(config, "IP");
	printf(test ? "true" : "false");
	ip = config_get_string_value(config, "IP");

	//char** test_list = config_get_array_value(config,"POSICIONES_ENTRENADORESE");
	char** test_postions = config_get_array_value(config,"POSICIONES_ENTRENADORES");
	char** test_split_position = string_split(test_postions[0], "|");

	printf("debug del test POSICION %d\n", atoi(test_split_position[0]));
	printf("debug del test POSICION 1 %d\n", atoi(test_split_position[1]));


	char** test_pokemons = config_get_array_value(config,"POKEMON_ENTRENADORES");
	char** test_split_pokemon = string_split(test_pokemons[0], "|");

	printf("debug del test POKEMON %s\n", test_split_pokemon[0]);
	printf("debug del test POKEMON 1 %s\n", test_split_pokemon[1]);

	//OBJETIVOS_ENTRENADORES

	char** test_objetivos = config_get_array_value(config,"OBJETIVOS_ENTRENADORES");
	char** test_split_objetivo = string_split(test_objetivos[0], "|");

	printf("debug del test OBJETIVO %s\n", test_split_objetivo[0]);
	printf("debug del test OBJETIVO 1 %s\n", test_split_objetivo[1]);

	int j = size_array_config(test_split_objetivo);



	printf("el valor de la J es %d\n", j);

	int count = size_array(test_objetivos[0]);
	printf("el count termina con %d\n", count);





	//printf("tamaño de la lista de entrenadores es %d\n", size_test);
	//printf("debug del test list 2 %s\n", test_split[2]);
	puerto = config_get_string_value(config, "PUERTO");


	new_list = list_create();
	ready_list = list_create();
	block_list = list_create();
	exec_list = list_create();
	exit_list = list_create();
	//ciclo para cargar una lista de entrenadores.
	//t_trainer* test_entrenador = construct_trainer(test_postions[0], test_objetivos[0], test_pokemons[0]);
	initialize_trainers(test_postions, test_objetivos, test_pokemons);

	objetives_list = list_create();

	objetives_list = initialize_global_objectives(new_list);


	pthread_t tid;
	pthread_t tid2;
	pthread_attr_t attr;


	/*
	sem_post(&trainer->sem_thread);
	sleep(1);
	sem_post(&trainer2->sem_thread);

	pthread_join(tid, NULL);
	pthread_join(tid2, NULL);

	*/
	//state_change(0,new_list, ready_list);
	transition_new_to_ready(1);
	transition_new_to_ready(0);
	transition_new_to_ready(0);
	short_term_scheduler();

	t_trainer* trainer_test = list_get(exec_list,0);
	pthread_join(trainer_test->tid, NULL);
	short_term_scheduler();
	trainer_test = list_get(exec_list,0);
	sleep(1);
	pthread_join(trainer_test->tid, NULL);



	trainer_test = list_get(ready_list,0);
	trainer_test->move_destiny = malloc(sizeof(t_position));
	trainer_test->move_destiny->x = 1;
	trainer_test->move_destiny->y = 9;
	trainer_test->action = MOVE;

	short_term_scheduler();
	trainer_test = list_get(exec_list,0);
	sleep(1);
	pthread_join(trainer_test->tid, NULL);
	list_get(exit_list,0);

	printf("cantidad de CPU %d\n", cpu_cycles);

















	//printf("el debug de los entrenador %d\n", ((t_trainer*) list_get(exec_list,0))->position->y);
	//5 colas para los estados

	/*
	conexion = crear_conexion(ip, puerto);

	printf("Number = %d \n",conexion);
	//enviar mensaje
	enviar_mensaje("test servidor up \n",conexion);
	//recibir mensaje
	char* message;
	message = recibir_mensaje(conexion);
	//loguear mensaje recibido
	log_info(logger, message);
	terminar_programa(conexion, logger, config);
	*/
  /*
	char* string = "Team";

	int peso = pesoString(string);

	printf("el peso es %d. \n", peso);
  */
	exit(0);

}
