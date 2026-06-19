/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"

/********************** macros and definitions *******************************/
#define G_TASK_ENTRY_A_CNT_INI	0ul

#define TASK_ENTRY_A_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_ENTRY_A_DEL_MAX	(pdMS_TO_TICKS(2500ul))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_entry_a_wait_2500mS		= "   ==> Task Entry A - Wait:   2500mS";

/********************** external data declaration *****************************/
//uint32_t g_task_entry_a_cnt;
/* Referencias externas a los semáforos de sincronización */
uint32_t g_task_entry_a_cnt = 0ul;

extern SemaphoreHandle_t xSemEntryA;
extern SemaphoreHandle_t xMutexCruce;
extern uint32_t g_vehicles_in_crossing;


/********************** external functions definition ************************/
/* Task thread */
void task_entry_a(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_entry_a_cnt = G_TASK_ENTRY_A_CNT_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Paso 1: Bloqueo/Espera del estímulo enviado por task_test (Semáforo Binario) */
		        xSemaphoreTake(xSemEntryA, portMAX_DELAY);

		        /* Paso 3 y 6: Acceso seguro al recurso compartido y evaluación de capacidad usando Mutex */
		        xSemaphoreTake(xMutexCruce, portMAX_DELAY);

		        if (g_vehicles_in_crossing < G_TASKS_CNT_MAX)
		        {
		            /* Hay espacio disponible en el cruce vial */
		            g_vehicles_in_crossing++;
		            g_task_entry_a_cnt++;

		            /* Paso 4: Control del Semáforo Vial -> Cambia a VERDE */
		            LOGGER_INFO("Entry A [VERDE]: Ingresa. Total=%lu", g_vehicles_in_crossing);

		            xSemaphoreGive(xMutexCruce); /* Liberamos zona crítica */

		            vTaskDelay(pdMS_TO_TICKS(1000ul)); /* Simula el tiempo que tarda físicamente el auto en cruzar */
		        }
		        else
		        {
		            /* El cruce vial alcanzó la capacidad máxima (G_TASKS_CNT_MAX) */
		            /* Paso 4: Control del Semáforo Vial -> Cambia a ROJO (Impedir ingreso) */
		        	LOGGER_INFO("Entry A [ROJO]: Lleno (%lu/%lu)", g_vehicles_in_crossing, G_TASKS_CNT_MAX);

		            xSemaphoreGive(xMutexCruce); /* Liberamos zona crítica inmediatamente */
		        }
		/* Update Task Counter */
		g_task_entry_a_cnt++;

    	/* Print out: Wait 2500mS */
		LOGGER_INFO(p_task_entry_a_wait_2500mS);
		vTaskDelay(TASK_ENTRY_A_DEL_MAX);
	}
}

/********************** end of file ******************************************/
