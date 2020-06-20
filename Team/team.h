/*
 * team.h
 *
 *  Created on: 19 abr. 2020
 *      Author: utnso
 */

#ifndef TEAM_H_
#define TEAM_H_

#include"includes.h"
#include"utilities.h"
#include"team_structs.h"

//---GLOBALS---

//logs: pueden desactivarse para no mostrarse en consola
//

//configuracion
t_config* config;
t_log* log;
uint32_t time_delay = 1; // TIENE QUE LEVANTAR DATO DEL CONFIG
t_list* objectives_list;
t_algorithm algorithm = FIFO;
uint32_t retry_time = 15; // TIENE QUE LEVANTAR DATO DEL CONFIG


//de aca para abajo, revisar condiciones de carrera
t_dictionary* poke_map;
//planificación
t_list* new_list;
t_list* ready_list;
t_list* block_list;
t_list* exec_list;
t_list* exit_list;


//planificacion
uint32_t context_changes = 0;
uint32_t cpu_cycles = 0;

//comunicacion
t_list* messages_list;
t_dictionary* message_response;

//semaforos
sem_t sem_exec;
sem_t sem_messages_list;
sem_t sem_messages;
sem_t sem_messages_recieve_list;

//---FIN GLOBALS---



//entrenadores y objertivos
t_trainer* initialize_trainer(uint32_t id, char* config_position, char* onfig_objectives, char* config_pokemons);//inicializa un entrenador (pthread) en new_list
void initialize_trainers();//inicializa todos los entrenadores del conig
void initialize_global_objectives();
//FIN funciones iniciales

//funciones de entrenadores
t_list* add_trainer_to_objective(t_list* list_global_objectives, t_trainer* trainer);
void* trainer_thread(t_callback* callback_thread);
void trainer_assign_job(char* pokemon, t_list* positions);
void trainer_assign_move(char* type,char* pokemon, uint32_t index, t_position* position, bool catching);
t_list* trainer_actual_list(t_trainer* trainer);
t_list* trainer_actual_list_by_id(uint32_t id);
//FIN funciones de entrenadores

//objetivos
void add_caught(t_list* list, char* pokemon);
bool success_global_objective(t_list* global_objectives);
bool pokemon_is_needed(char* pokemon);
//Pasar a diccionario ?
void add_catching(t_list* list, char* pokemon);
void sub_catching(t_list* list, char* pokemon);
//FIN objetivos

//transiciones
void state_change(uint32_t index, t_list* from,t_list* to);
void transition_by_id(uint32_t id, t_list* from,t_list* to);
void transition_new_to_ready(uint32_t index);
void transition_ready_to_exec(uint32_t index);
void transition_exec_to_ready();
void transition_exec_to_block();
void transition_exec_to_exit();
void transition_block_to_ready(uint32_t index);
void transition_block_to_exit(uint32_t index);
//FIN transiciones

//planificacion
void long_term_scheduler();
void* exec_thread();
void fifo_algorithm();
void rr_algorithm();
void sjfs_algorithm();
void sjfc_algorithm();
void callback_fifo(t_trainer* trainer);
//FIN planificacion

//movimientos
void move_up(t_trainer* trainer);
void move_down(t_trainer* trainer);
void move_right(t_trainer* trainer);
void move_left(t_trainer* trainer);
void move(t_trainer* trainer);
//FIN movimientos

//catch
void catch(t_trainer* trainer);
//FIN catch


//comunicación
void* sender_thread();
void process_message(operation_code op_code, void* message);
void subscribe(queue_code queue_code);
//FIN comunicación



void callback_fifo(t_trainer* trainer){
	if(trainer->action == CATCHING){
			//llama funcion para enviar mensaje, recibe entrenador por parametro y hace post a hilo de sender
			//sem_post(&sem_sender)
			sem_post(&sem_exec);
			//signal a semaforo de exec
		}
	else if(trainer->action == FREE){
		sem_post(&sem_exec);
		//signal a semaforo de exec
	}
	else
		sem_post(&trainer->sem_thread);

}

t_trainer* initialize_trainer(uint32_t id, char* config_position, char* onfig_objectives, char* config_pokemons)
{
	t_trainer* trainer = create_trainer_from_config(id, config_position, onfig_objectives, config_pokemons);

	t_callback* callback_thread = malloc(sizeof(t_callback));
	callback_thread->trainer = trainer;

	callback_thread->callback = &callback_fifo;
	pthread_create(&(trainer->tid), NULL, trainer_thread, callback_thread);

	return trainer;
}


void initialize_trainers()
{
	char** positions_config = config_get_array_value(config,"POSICIONES_ENTRENADORES");
	char** objectives_config = config_get_array_value(config,"OBJETIVOS_ENTRENADORES");
	char** pokemons_config = config_get_array_value(config,"POKEMON_ENTRENADORES");
	int i = 0;
	while(positions_config[i] != NULL){
		t_trainer* test_entrenador = initialize_trainer(i+1, positions_config[i], objectives_config[i], pokemons_config[i]);
		if(trainer_success_objective(test_entrenador))
			list_add(exit_list, test_entrenador);
		else
			list_add(new_list, test_entrenador);
		i++;
	}
	//liberar memoria (todos los char**)
	//free_string_list(positions_config);
	//free_string_list(objectives_config);
	//free_string_list(pokemons_config);


}




void add_objective(t_list* list, char* pokemon)
{
	t_objective* objective = find_key(list, pokemon);
	if(objective != NULL)
		objective->count++;
	else{
		objective = malloc(sizeof(t_objective));
		objective->pokemon = pokemon;
		objective->count = 1;
		objective->caught = 0;
		objective->catching = 0;
		list_add(list,(void*)objective);
	}
}

void add_caught(t_list* list, char* pokemon)
{
	t_objective* objective = find_key(list, pokemon);
	if(objective != NULL){
		objective->caught++;
		objective->catching--;
	}
	else
		printf("Lo rompiste todo, maldito idiota\n");
}

void add_catching(t_list* list, char* pokemon)
{
	t_objective* objective = find_key(list, pokemon);
	if(objective != NULL){
		objective->catching++;
	}
	else
		printf("Lo rompiste todo, maldito idiota\n");
}

void sub_catching(t_list* list, char* pokemon)
{
	t_objective* objective = find_key(list, pokemon);
	if(objective != NULL){
		objective->catching--;
	}
	else
		printf("Lo rompiste todo, maldito idiota\n");
}


bool pokemon_is_needed(char* pokemon)
{
	printf("Consultando si %s esta en la lista de objetivos\n", pokemon);
	//t_objective* test = (t_objective*) list_get(objectives_list,0);
	//printf("LN 301 the pokemon OBJECTIVE is %s\n", test->pokemon);
	t_objective* objective = find_key(objectives_list, pokemon);

	if(objective == NULL)
		printf("no necesitamos un %s\n", pokemon);
	//return (objective->count > (objective->caught + objective->catching));
	return objective->count > (objective->caught + objective->catching);
}

t_list* add_trainer_to_objective(t_list* list_global_objectives,t_trainer* trainer)
{
	int i = 0;
	while(trainer->objectives[i]!= NULL){
		add_objective(list_global_objectives, trainer->objectives[i]);
		i++;
	}
	i = 0;
	while(trainer->pokemons[i]!= NULL){
		add_caught(list_global_objectives, trainer->pokemons[i]);
		i++;
	}
	return list_global_objectives;
}


void initialize_global_objectives()
{
	t_list* list_global_objectives = list_create();
	objectives_list = (t_list*) list_fold( new_list,(void*)list_global_objectives,(void*)&add_trainer_to_objective);
}


bool success_global_objective(t_list* global_objectives)
{
	bool success = false;
	 if(list_all_satisfy(global_objectives,&success_objective)) {
		 if(list_size(block_list) == 0 && list_size(new_list) == 0 && list_size(ready_list) == 0 && list_size(exec_list) == 0){
			 success = true;
		 }

	 }
	return success;
}

void* trainer_thread(t_callback* callback_thread)
{
	//if(/*si es 0 menor*/)
		//funcion cambiar valor global de variable interrumption
		//COMO SE DESALOJA A UN HILO DE ENTENADOR, COMO SE ENTERA EL PLANIFICADOR O EL HILO DE EJECUCION!!
		//while(1) agregar
	t_trainer* trainer = callback_thread->trainer;

	printf("hola soy el entrenador %d\n", (int)trainer->tid);
	printf("mi target actual es: %s\n", trainer->target->pokemon);
	while(1){

		sem_wait(&trainer->sem_thread);

		switch(trainer->action){
			case MOVE:
				printf("Me estoy moviendo (comando MOVE), mi objetivo: %s <----\n", trainer->target->pokemon);
				printf("Posicion actual: (%d,%d)", trainer->position->x,trainer->position->y);
				//aca va un if, no un while
				if(trainer->position->x != trainer->target->position->x || trainer->position->y != trainer->target->position->y)
					move(trainer);
				else if(trainer->target->catching)
					trainer->action = CATCH;
				else
					trainer->action = TRADE;
				printf("(%d,%d)\n", trainer->position->x,trainer->position->y);
				break;
			case CATCH:
				catch(trainer);
				break;
			case CATCHING:
				printf("Estoy atrapando pokemon, comando CATCHING\n");
				break;
			case TRADE:
				printf("Estoy tradeando pokemon, comando TRADE\n");
				break;
			default:
				printf("No hago nada\n");
				trainer->action = FREE;
				break;
		}
		callback_thread->callback(trainer);
	}

	printf("HILO debug del entrenador %s\n", trainer->objectives[0]);
	//pthread_mutex_unlock(trainer->semThread);
	//sem_post(&trainer->sem_thread);
	return NULL;
}

//encuentra al entrenador mas cerca de la posicion en ambas listas

//compara la posicion de los 2 entrenadores y la lista.

//setea al entrenador, el pokemon y la distancia objetivo

//dependiendo de la lista en la que este hace el transition.

//VAMOS POR ACA FEDE!!!!!!!!! 13/05/2020
//large_term_scheduler
//void 		  dictionary_iterator(t_dictionary *, void(*closure)(char*,void*));

void long_term_scheduler(){
	dictionary_iterator(poke_map, &trainer_assign_job);
	//TODO completarlo: que pasa cuando no tenemos posiciones en el pokemap
}

void trainer_assign_move(char* type,char* pokemon, uint32_t index, t_position* position, bool catching)
{
	printf("\nse asignara al entrenado X a atrapar al pokemon %s, en la posicion (%d, %d)\n", pokemon, position->x, position->y);
	if(strcmp(type,"NEW") == 0){
		t_trainer* trainer = (t_trainer*) list_get(new_list, index);
		printf("THIS IS TRAINER %d", trainer->id);
		printf("ACA ASIGNAMOS EL TARGET, %s reemplaza a %s \n", pokemon, trainer->target->pokemon);
		trainer->target->pokemon = malloc(strlen(pokemon)+1);
		memcpy(trainer->target->pokemon, pokemon, strlen(pokemon)+1);
		printf("ACA ASIGNAMOS EL TARGET, POKEMON ON TARGET  %s \n", trainer->target->pokemon);
		//trainer->target->pokemon = pokemon;
		trainer->action = MOVE;
		trainer->target->position = position;
		trainer->target->catching = catching;
		printf("aca el ACTION ES %d\n", trainer->action);
		transition_new_to_ready(index);
	}
	else if(strcmp(type,"BLOCK") == 0){
		t_trainer* trainer = (t_trainer*) list_get(block_list, index);
		memcpy(trainer->target->pokemon,pokemon, strlen(pokemon)+1);
		//trainer->target->pokemon = pokemon;
		trainer->action = MOVE;
		trainer->target->position = position;
		trainer->target->catching = catching;
		transition_block_to_ready(index);
	}
}

void trainer_assign_move2(t_trainer* trainer, char* pokemon, t_position* position, bool catching)
{
	//TODO algo no está andando, se rompe en el segundo entrenador. . .
	printf("\nse asignara al entrenado %d a atrapar al pokemon %s, en la posicion (%d, %d)\n", trainer->id, pokemon, position->x, position->y);

	memcpy(trainer->target->pokemon,pokemon, strlen(pokemon)+1);
	trainer->action = MOVE;
	trainer->target->position = position;
	trainer->target->catching = catching;
	transition_from_id_to_ready(trainer->id);
	debug_trainer(trainer);
}


void trainer_assign_job(char* pokemon, t_list* positions)
{
	//char* pokemon = malloc((strlen(key)+1)*sizeof(char));
	//memcpy(pokemon, key, strlen(key)+1);//sin esto rompe

	t_link_element* element = positions->head;
	t_position* position;
	int32_t i = -1;
	if(pokemon_is_needed(pokemon)){
		while(element != NULL) {
			position = (t_position*) element->data;
			// se remplaza la position por lo que devuelva del diccionario
			t_trainer* trainer_new = NULL;
			t_trainer* trainer_block = NULL;
			printf("\n---Buscar entrenador más cercano a %s (%d, %d) en la cola NEW---\n", pokemon, position->x, position->y);
			int32_t closest_from_new = closest_free_trainer(new_list, position);

			printf("\n---Buscar entrenador más cercano a (%d. %d) en la cola BLOCKED---\n", position->x, position->y);
			int32_t closest_from_block = closest_free_trainer(block_list, position);

			if(closest_from_new >= 0){
				trainer_new = list_get(new_list,closest_from_new);
			}
			if(closest_from_block >= 0){
				trainer_block = list_get(block_list,closest_from_block);
			}

			//bool first_closer(t_trainer* trainer, t_trainer* trainer2,t_position* position)



			if(trainer_new != NULL && (trainer_block == NULL || first_closer(trainer_new, trainer_block, position))){
				add_catching(objectives_list, pokemon);
				trainer_assign_move("NEW",pokemon, closest_from_new,position,1);
				//trainer_assign_move2(trainer_new, pokemon, position, 1);
				list_remove(positions, (i+1));
				//aca deberia sacar la posicion de la lista de posiciones del pokemon, solo sacarla NO! borrarla
			}
			else if(trainer_block != NULL && (trainer_new == NULL || first_closer(trainer_block, trainer_new, position))){
				add_catching(objectives_list, pokemon);
				trainer_assign_move("BLOCK",pokemon, closest_from_block,position,1);
				//trainer_assign_move2(trainer_block, pokemon, position, 1);
				list_remove(positions, (i+1));
				//aca deberia sacar la posicion de la lista de posiciones del pokemon, solo sacarla NO! borrarla
			}
			else{
				printf("no hay entrenadores en lasstas de new ni block \n");
			}

			if(list_size(positions) == 0){
				dictionary_remove_and_destroy(poke_map, pokemon,&list_destroy);

			}

			//si el size de positions es , deberia eliminar el pokemon del mapa
			element = element->next;

			// NO OLVIDAR BORRAR LA POSICION QUE YA SE USO
			// Si tiene una sola posicion, entonces se borra el pokemon del pokemap ??
			i++;
		}
	}
	else
		printf("The pokemon is already full\n");


}

uint32_t trainer_get_index(t_trainer* trainer, t_list* list) {
	return 0;
}

t_list* trainer_actual_list(t_trainer* trainer) {
	return trainer_actual_list_by_id(trainer->id);
}//TODO revisar si se usa o no

t_list* trainer_actual_list_by_id(uint32_t id) {

	t_list* actual_list = NULL;
	bool condition(void* trainer) {
		return (((t_trainer*)trainer)->id != id);
	}
	if(list_any_satisfy(new_list, &condition))
		actual_list = new_list;
	else if(list_any_satisfy(ready_list, &condition))
		actual_list = ready_list;
	else if(list_any_satisfy(block_list, &condition))
		actual_list = block_list;
	else if(list_any_satisfy(exec_list, &condition))
		actual_list = exec_list;
	else if(list_any_satisfy(exit_list, &condition))
		actual_list = exit_list;

	return actual_list;
}//TODO revisar si se usa o no

void add_to_poke_map(char* pokemon, t_position* position)
{
	//CASO DE QUE NO ESTE EL POKEMON EN EL MAP
	if(!dictionary_has_key(poke_map, pokemon)){
		t_list* positions = list_create();
		list_add(positions, position);
		dictionary_put(poke_map, pokemon, positions);
	}
	else
	{
		t_list* positions = (t_list*) dictionary_get(poke_map,pokemon);
		list_add(positions, position);
	}

	//EL CASO DE QUE ESTE EL POKEMON EN EL MAPA
	//buscar el pokemon en la lista y retornar el indice


}


//transiciones
void state_change(uint32_t index, t_list* from,t_list* to)
{
	void* element = list_remove(from, index);
	list_add(to, element);
}

void transition_by_id(uint32_t id, t_list* from,t_list* to) {
	bool condition(void* trainer) {
		return (((t_trainer*)trainer)->id != id);
	}
	printf("the size of all list are %d %d %d %d %d\n",list_size(block_list),list_size(new_list),list_size(ready_list),list_size(exec_list), list_size(exit_list));
	void* element = NULL;

	if(list_size(from)>1)
		element = list_remove_by_condition(from, &condition);
	else
		element = list_remove(from, 0);

	if(element != NULL)
		list_add(to, element);
	else
		printf("*ERROR* NO SE PUDO HACER LA TRANSICIÓN, EL ENTRENADOR NO SE ENCONTRABA EN LA LISTA INDICADA *ERROR*\n");
	
	printf("the size of all list are %d %d %d %d %d\n",list_size(block_list),list_size(new_list),list_size(ready_list),list_size(exec_list), list_size(exit_list));
}

void transition_from_id_to_ready(uint32_t id) {
	bool condition(void* trainer) {
		return (((t_trainer*)trainer)->id != id);
	}

	t_trainer* trainer = list_remove_by_condition(new_list, &condition);
	if(trainer != NULL) {
		list_add(ready_list, trainer);
	} else {
		trainer = list_remove_by_condition(block_list, &condition);
		if(trainer != NULL) {
			list_add(ready_list, trainer);
		}else {
			printf("**ERROR**SE INTENTÓ HACER UNA TRANSICIÓN A READY DE UN ENTRENADOR QUE NO SE ENCUENTRA EN NEW NI BLOCK!**ERROR**\n");

		}
	}
}

void transition_new_to_ready(uint32_t index)
{
	state_change(index,new_list,ready_list);
	context_changes++;
}

void transition_ready_to_exec(uint32_t index)
{
	state_change(index,ready_list,exec_list);
	context_changes++;
	t_trainer* trainer = list_get(exec_list,0);
	printf("ACA HAREMOS EL POST DE HILO TRAINER\n");
	sem_post(&trainer->sem_thread);
	printf("SE HIZPO EL POST DE HILO TRAINER\n");
}

void transition_exec_to_ready()
{
	state_change(0,exec_list,ready_list);
	context_changes++;
}

void transition_exec_to_block()
{
	state_change(0,exec_list,block_list);
	context_changes++;
}

void transition_exec_to_exit()
{
	state_change(0,exec_list,exit_list);
	context_changes++;
}

void transition_block_to_ready(uint32_t index)
{
	state_change(index,block_list,ready_list);
	context_changes++;
}

void transition_block_to_exit(uint32_t index)
{
	state_change(index,block_list,exit_list);
	context_changes++;
}
//FIN transiciones

void fifo_algorithm()
{
	printf("estoy en fifo\n");
	if(list_size(exec_list) > 0){
		printf("aca llego\n");
		transition_exec_to_block();
	}

	if(list_size(ready_list) > 0){
		printf("aca llego2\n");
		transition_ready_to_exec(0);
	}

}

void rr_algorithm()
{
	printf("Estoy en RR\n");
}

void sjfs_algorithm()
{
	printf("Estoy en SJFS\n");
}

void sjfc_algorithm()
{
	printf("Estoy en SJFC\n");
}

void short_term_scheduler()
{

	if(list_size(exec_list)>0){
		t_trainer* trainer_exec = (t_trainer*) list_get(exec_list,0);

			//ACA CONSULTAMOS SI SALE POR I/0
			printf("corriendo short term, trainer en ejecucion actual:\n");
			debug_trainer(trainer_exec);
			if(trainer_exec->target->catching){

				t_message_team* message = malloc(sizeof(t_message_team));
				message->trainer = trainer_exec;
				message->pokemon = malloc(strlen(trainer_exec->target->pokemon)+1);
				printf("the size of the fucks %d %d \n",sizeof(message->pokemon),sizeof(trainer_exec->target->pokemon));

				memcpy(message->pokemon, trainer_exec->target->pokemon, strlen(trainer_exec->target->pokemon)+1);
				message->pokemon = trainer_exec->target->pokemon;
				message->position = malloc(sizeof(t_position));
				message->position->x = trainer_exec->target->position->x;
				message->position->y = trainer_exec->target->position->y;
				sem_wait(&sem_messages_list);
				list_add(messages_list,message);
				sem_post(&sem_messages_list);

				sem_post(&sem_messages);
				transition_exec_to_block();
			}
	}

	debug_colas();

	switch(algorithm){
			case FIFO:
				printf("estoy en fifo (short term)1\n");
				fifo_algorithm();
				printf("estoy en fifo (short term)2\n");
				break;
			case RR:
				rr_algorithm();
				break;
			case SJFS:
				sjfs_algorithm();
				break;
			case SJFC:
				sjfc_algorithm();
				break;
			default:
				printf("Estoy en nada\n");
				break;
		}
	// lo unico que hace es mueve de ready to exec
}

void move_up(t_trainer* trainer)
{
	sleep(time_delay);
	trainer->position->y++;
	trainer->burst++;
	cpu_cycles++;
	printf(" -> move_up -> ");
}

void move_down(t_trainer* trainer)
{
	sleep(time_delay);
	trainer->position->y--;
	trainer->burst++;
	cpu_cycles++;
	printf(" -> move_down -> ");
}

void move_right(t_trainer* trainer)
{
	sleep(time_delay);
	trainer->position->x++;
	trainer->burst++;
	cpu_cycles++;
	printf(" -> move_right -> ");
}

void move_left(t_trainer* trainer)
{
	sleep(time_delay);
	trainer->position->x--;
	trainer->burst++;
	cpu_cycles++;
	printf(" -> move_left ->");
}

void move(t_trainer* trainer)
{
	if(trainer->target->position->x > trainer->position->x)
		move_right(trainer);
	else if(trainer->target->position->x < trainer->position->x)
		move_left(trainer);
	else if(trainer->target->position->y > trainer->position->y)
		move_up(trainer);
	else if(trainer->target->position->y < trainer->position->y)
		move_down(trainer);
}


void* exec_thread()
{
	sem_init(&sem_exec, 0, 1);
	while(1){
			//sem_wait(&sem_exec); Este seria el unico semaforo, despues cambiar
			if(list_size(ready_list)>0 || list_size(exec_list)>0){
				sem_wait(&sem_exec);
				short_term_scheduler();
			}
			else{
				//printf("aca pasa al planificador de largo plazo\n");
				//post al planificador de largo plazo
			}


			//printf("la lista de block queda %d\n",list_size(block_list));
			//printf("la lista de new queda %d\n",list_size(new_list));
			//printf("cantidad de CPU %d\n", cpu_cycles);
			//if a funcion que consula objetivos globales
			//BREAK PARA CORTAR WHILE CUANDO TERMINAN OBJETIVOS
		}

		printf("cantidad de CPU %d\n", cpu_cycles);
}


void* sender_thread()
{

	sem_init(&sem_messages_list, 0, 1);
	sem_init(&sem_messages, 0, 0);
	sem_init(&sem_messages_recieve_list, 0, 1);
	while(1){
		sem_wait(&sem_messages);
		sem_wait(&sem_messages_list);
		t_message_team* message = list_remove(messages_list, 0);
		sem_post(&sem_messages_list);
		t_message_catch* catch = create_message_catch_long(message->pokemon, message->position->x, message->position->y);


		t_package* package = serialize_catch(catch);
		destroy_message_catch(catch);
		//TODO ESTO DEBE IR EN OTRO HILO, CREAR NUEVO HILO PARA CADA SEND_MESSAGE
		int32_t correlative_id = send_message("127.0.0.1", "6001", package);

		printf("salio para el broker\n");

		//printf("limpiando message team \n");
		//destroy_message_team(message);
		//printf("ya se limpio message team \n");
		//char str_correlative_id = malloc(6);
		char str_correlative_id[6];
		sprintf(str_correlative_id,"%d",correlative_id);
		sem_wait(&sem_messages_recieve_list);
		dictionary_put(message_response,str_correlative_id,message->trainer);
		sem_post(&sem_messages_recieve_list);

	}

	//send al broker, (pokemon, posicion, el entrenador)

	//

	//pthread_t tid;
	//pthread_create(&tid, NULL, subscribe, NULL);
	//pthread_join(tid, NULL);



}

/*void* fake_broker_thread()
{
	//SERVIDOR QUE ESCUCHA EN PUERTO TAL! 6001
	//PRINTF ALGO DE MENSAJE
	//RESPONDE ACK
}//*/



/*
subscribe --------------->>>>>> servidor que esta escuchando (BROKER)

*/

void process_message(operation_code op_code, void* message) {
	switch(op_code) {
	case OPERATION_NEW:
		printf("SE RECIBIO UN  NEW, PERO NO SE QUE HACER <----------------------------");
	break;
	case OPERATION_APPEARED:
		printf("SE RECIBIO UN  APPEARED, PERO NO SE QUE HACER <----------------------------");
	break;
	case OPERATION_GET:
		printf("SE RECIBIO UN  GET, PERO NO SE QUE HACER <----------------------------");
	break;
	case OPERATION_LOCALIZED:
		printf("SE RECIBIO UN  LOCALIZED, PERO NO SE QUE HACER <----------------------------");
	break;
	case OPERATION_CATCH:
		printf("SE RECIBIO UN  CATCH, PERO NO SE QUE HACER <----------------------------");
	break;
	case OPERATION_CAUGHT:
		printf("SE RECIBIO UN  CAUGHT [");
		printf("ID: %d, ",((t_message_caught*)(message))->id);
		printf("CORRELATIVE_ID %d, ",((t_message_caught*)(message))->correlative_id);
		printf("RESULT: %d]<----------\n",((t_message_caught*)(message))->result);

		char str_correlative_id[6];
		sprintf(str_correlative_id,"%d",((t_message_caught*)(message))->correlative_id);
		if(dictionary_has_key(message_response,str_correlative_id) == 1){
			t_trainer* trainer = (t_trainer*) dictionary_get(message_response, str_correlative_id);
			debug_trainer(trainer);
			if(((t_message_caught*)(message))->result){
				//ACA ROMPE NOSE POR QUE?? REVISAR ADD_POKEMON QUIZAS
				add_pokemon(trainer, trainer->target->pokemon);
				add_caught(objectives_list, trainer->target->pokemon);
				//OBJETVIO DEL ENTRANDOR ?
				//OBJETIVO GLOBAL??
				// SI SE CUMPLE ENTRENADOR PASA A EXIT
				// SINO QUEDA EN FREE
				debug_trainer(trainer);
				if(trainer_success_objective(trainer) == 1){
					printf("ESTE ENTRENADOR TERMINO RE PIOLA\n");
					trainer->action = FINISH;
//TODO nos falta poder identificarlo dentro de la lista de blocked! (agregar ID y nombre (opcional) al trainer)
//TODO DEBE pasar a EXIT
					transition_by_id(trainer->id, block_list, exit_list);
					printf("---->TAMAÑO DE EXIT: %d\n", list_size(exit_list));
//TODO como paso a exit tambient enemos verificar los objetivos globales para saber si el team termino
					if(success_global_objective(objectives_list)) {
						printf("BRAVO! EL TEAM A CUMPLIDO TODOS SUS OBJETIVOS\n");
					} else {
						printf("SEGUI PARTICIPANDO\n");
					}
				} else{
					printf("ESTE ENTRENADOR NO CUMPLIO TODOS SUS OBJETIVOS\n");
					trainer->action = FREE;
				}

			} else {
				trainer->action = FREE;
			}
			sub_catching(objectives_list, trainer->target->pokemon);
			trainer->target->catching = 0;
			//free(trainer->target->position);
			//trainer->target->position = NULL;
			trainer->target->distance = NULL;
			//free(trainer->target->pokemon);
			//trainer->target->pokemon = NULL;
		}
		else
			printf("SE IGNORA EL MENSAJE PERRO\n");

	break;
	default:
		printf("CODIGO DE OPERACION ERRONEO");
	break;
	}
}


void subscribe(queue_code queue_code) {
	printf("COD OPERATION%d\n", queue_code);
	char* ip = config_get_string_value(config, "IP_BROKER");
	char* port = config_get_string_value(config, "PUERTO_BROKER");
	int32_t socket = connect_to_server(ip, port,retry_time,log);
	uint32_t id = config_get_int_value(config, "ID");

	t_package* package = serialize_suscripcion(id, queue_code);

	send_paquete(socket, package);
	if (receive_ACK(socket, log) == -1) {
		exit(EXIT_FAILURE);
	}


	//Quedarse recibiendo mensajes permanentemente, no hace falta otro hilo

	struct thread_args* args = malloc(sizeof(struct thread_args));
	args->socket = socket;
	args->logger = log;
	args->function = process_message;
	pthread_t thread;
	pthread_create(&thread, NULL, (void*) listen_messages, args);


	//Al completar el objetivo global, enviar tres mensajes al broker con ID de proceso e ID de cola asi puede liberar memoria
}


int32_t send_message(char* ip, char* port, t_package* package) {

	int32_t socket = connect_to_server(ip, port,retry_time, log);
	send_paquete(socket, package);

	int32_t correlative_id = receive_ID(socket, log);
	send_ACK(socket, log);
	return correlative_id;
}

void destroy_message_team(t_message_team* message){
	free(message->pokemon);
	free(message->position);
	free(message);
}

void catch(t_trainer* trainer){
	sleep(time_delay);
	trainer->action = CATCHING;
	trainer->burst++;
	cpu_cycles++;
	printf(" -> Catch: %s\n", trainer->target->pokemon);
}

void debug_colas() {
	printf("the size of all list are new: %d ready: %d block: %d exec: %d exit: %d\n",list_size(new_list),list_size(ready_list),list_size(block_list),list_size(exec_list), list_size(exit_list));
}
#endif /* TEAM_H_ */
