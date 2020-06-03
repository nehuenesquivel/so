/*
 * listas.h
 *
 *  Created on: 30 may. 2020
 *      Author: utnso
 */

#ifndef LISTAS_H_
#define LISTAS_H_

#include "broker.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <commons/collections/list.h>

void encontrar_numero(t_list *lista, void* numero_buscado, t_list* elementos_coincidentes);
void no_contenido(t_list* lista_enviados, void* elemento_lista_global, t_list* resultado);
t_list* interseccion_lista(t_list* unaLista, t_list* otraLista, t_list* resultado);
//asigna en la lista resultado, los elementos que estan en la primera, pero no en la segunda
t_list* no_enviados_lista(t_list* lista_global, t_list* lista_enviados, t_list* resultado);
void imprimir_lista(t_list* lista, char* nombre);

//TODO falta probar
t_pending* find_element_given_ID(void* ID_encontrar, t_list* cola, pthread_mutex_t mutex_cola, uint32_t* bytes, void** datos_mensaje);


#endif /* LISTAS_H_ */