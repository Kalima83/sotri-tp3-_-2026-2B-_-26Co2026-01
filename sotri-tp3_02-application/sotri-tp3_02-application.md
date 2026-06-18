# Análisis Técnico del Sistema de Tiempo Real (FreeRTOS)
## Proyecto: Simulación del Problema de Lectores-Escritores (Readers-Writers Problem)

Este repositorio contiene una implementación base en **FreeRTOS** diseñada para abordar el problema clásico de sincronización de **Lectores-Escritores** (basado en la literatura de *Allen B. Downey, "The Little Book of Semaphores"*). El sistema está estructurado bajo un esquema multitarea dirigido por eventos periódicos.

---

## 1. Arquitectura General del Sistema

El firmware está diseñado para ejecutarse en un microcontrolador de arquitectura **ARM Cortex-M** y se compone de cinco módulos principales que interactúan con el kernel de FreeRTOS:

* **`app.c` (Inicialización Central):** Configura las variables globales, crea las tareas con sus respectivas prioridades y verifica la integridad de la memoria.
* **`task_reader.c` (Hilo Lector):** Simula el comportamiento de un proceso que consulta o lee un recurso compartido de forma periódica.
* **`task_writer.c` (Hilo Escritor):** Simula el comportamiento de un proceso que modifica o escribe en un recurso compartido de forma periódica.
* **`freertos.c` (Callbacks de Sistema / Hooks):** Monitorea el rendimiento del sistema, gestiona el tiempo de CPU inactivo y actúa como salvaguarda ante desbordamientos de pila.
* **`app_it.c` (Manejo de Interrupciones):** Provee una plantilla base para habilitar o deshabilitar interrupciones a nivel de hardware.

---

## 2. Análisis Detallado por Archivo Fuente

### 📋 `app.c` - Módulo de Configuración y Arranque
Este archivo actúa como el punto de entrada de la lógica de usuario una vez que el hardware básico ha sido inicializado. Su función principal es `app_init()`.

* **Inicialización de Telemetría:** Configura e inicializa a cero los contadores globales del sistema mediante macros de inicialización (`g_app_cnt`, `g_app_task_cnt`, `g_app_tick_cnt`, `g_task_idle_cnt`, `g_app_stack_overflow_cnt`).
* **Creación de Tareas (`xTaskCreate`):** Instancia de manera dinámica los dos hilos de trabajo primarios utilizando la API de FreeRTOS:
    * **Task Reader:** Prioridad `tskIDLE_PRIORITY + 1ul` (Prioridad 1), tamaño de stack mínimo (`configMINIMAL_STACK_SIZE`).
    * **Task Writer:** Prioridad `tskIDLE_PRIORITY + 1ul` (Prioridad 1), tamaño de stack mínimo (`configMINIMAL_STACK_SIZE`).
* **Mecanismos de Seguridad y Diagnóstico:**
    * Utiliza `configASSERT(pdPASS == ret)` después de cada creación para colgar el sistema intencionalmente si el RTOS se queda sin memoria (Heap) para asignar los TCB (*Task Control Blocks*) o las pilas de las tareas.
    * Invoca a `xPortGetFreeHeapSize()` para auditar la memoria dinámica remanente disponible en el esquema de memoria (típicamente `heap_4.c`).
* **Inicializaciones de Bajo Nivel:** Llama a `app_it_init()` para preparar el entorno de interrupciones y a `cycle_counter_init()` para activar los registros de conteo de ciclos de CPU (DWT).

### 🔍 `task_reader.c` y ✍️ `task_writer.c` - Hilos de Ejecución (Tareas)
Ambos archivos presentan actualmente una estructura homóloga que sirve como esqueleto operativo periódico.

* **Inicialización Local:** Al arrancar, configuran su propio contador de ejecuciones (`g_task_reader_cnt` y `g_task_writer_cnt`) en `0ul`.
* **Registro por Consola (Log):** Envían un mensaje informativo inicial utilizando `LOGGER_INFO`, extrayendo dinámicamente el nombre de la tarea con `pcTaskGetName(NULL)` y el tiempo actual del sistema mediante `xTaskGetTickCount()`.
* **Bucle Infinito (`for(;;)`):**
    1.  Incrementan su respectivo contador global de vueltas.
    2.  Imprimen un mensaje indicando que la tarea está ingresando al estado de espera (`p_task_reader_wait_250mS` / `p_task_writer_wait_250mS`).
    3.  Ceden el control del procesador bloqueándose voluntariamente durante 250 milisegundos mediante la llamada:
        ```c
        vTaskDelay(pdMS_TO_TICKS(250ul));
        ```
* **Comportamiento del Planificador (Scheduler):** Al tener exactamente la **misma prioridad (1)** y el mismo tiempo de bloqueo (250 ms), el planificador de FreeRTOS las ejecuta alternadamente o de forma secuencial aplicando un esquema *Round-Robin* con tiempo compartido (Time-slicing).

### 🛠️ `freertos.c` - Funciones Hook (Ganchos del Sistema)
Contiene las funciones de callback que el núcleo de FreeRTOS invoca automáticamente ante eventos clave del sistema operativo:

1.  **`vApplicationIdleHook`:** Se ejecuta repetidamente cuando no hay tareas de usuario listas para ejecutarse (prioridad 0). Incrementa `g_task_idle_cnt`. Es el espacio diseñado para poner al procesador en modo de bajo consumo (*Low Power Sleep Mode*). **Restricción:** No puede bloquearse bajo ninguna circunstancia.
2.  **`vApplicationTickHook`:** Se invoca dentro del contexto de la ISR (Rutina de Servicio de Interrupción) del temporizador del sistema (System Tick) en cada milisegundo. Incrementa `g_app_tick_cnt`. **Restricción:** Debe ser sumamente rápida y eficiente.
3.  **`vApplicationStackOverflowHook`:** Mecanismo de protección crítico en sistemas embebidos. Si el puntero de pila de cualquier tarea corrompe o supera los límites de su memoria asignada, esta función captura el evento, entra en una sección crítica y congela la CPU con `configASSERT( 0 )` para evitar comportamientos impredecibles y permitir la depuración mediante JTAG/SWD.

### ⚡ `app_it.c` - Gestión de Interrupciones de la Aplicación
Módulo encargado de centralizar la configuración de interrupciones de hardware. 

* En su función `app_it_init()`, cuenta con una plantilla estructural de protección mediante instrucciones en ensamblador inline nativas de ARM Cortex-M:
    ```c
    __asm("CPSID i"); /* Deshabilita de forma global las interrupciones */
    __asm("CPSIE i"); /* Habilita de forma global las interrupciones */
    ```
    *(Nota: En el código base actual, esta secuencia consecutiva sirve como marcador de posición para delimitar las zonas de configuración segura de periféricos).*

---

## 3. Estado Actual de la Sincronización (Diagnóstico)

Aunque el proyecto está titulado bajo el problema de **Lectores-Escritores**, el análisis del código revela que **el mecanismo de sincronización se encuentra en estado de maqueta (*boilerplate*) y aún no ha sido implementado**.

### Evidencias en el código:
En el archivo `app.c` se observan comentarios y bloques libres dejados explícitamente por el desarrollador original para la integración de primitivos de FreeRTOS:
* *`/* Declare a variable of type SemaphoreHandle_t (binary or counting) or mutex... */`*
* *`/* Before a queue or semaphore... is used it must be explicitly created. */`*

### Problemas potenciales en el estado actual:
1.  **Falta de Exclusión Mutua:** Si se añade una base de datos o variable compartida, múltiples lectores y escritores accederían en cualquier orden sin protección, provocando **condiciones de carrera (Race Conditions)**.
2.  **Ausencia de Recurso Compartido:** El recurso sobre el cual se debe realizar la lectura o escritura estructurada aún no está declarado.

---

## 4. Próximos Pasos para la Implementación de Sincronización

Para completar el algoritmo teórico de **Lectores-Escritores** (con prioridad a los lectores), se deben incorporar los siguientes primitivos en el código:

1.  **Declarar en `app.c` los Semáforos y Mutexes necesarios:**
    ```c
    SemaphoreHandle_t xMutex;      // Protege el contador de lectores activos
    SemaphoreHandle_t xRoomEmpty;   // Controla el acceso exclusivo al recurso compartido
    uint32_t readers_cnt = 0;      // Contador de lectores
    ```
2.  **Inicializarlos en `app_init()`:**
    ```c
    xMutex = xSemaphoreCreateMutex();
    xRoomEmpty = xSemaphoreCreateBinary();
    xSemaphoreGive(xRoomEmpty); // Inicialmente la sala está vacía
    ```
3.  **Implementar el protocolo en `task_reader.c`:**
    ```c
    xSemaphoreTake(xMutex, portMAX_DELAY);
    readers_cnt++;
    if (readers_cnt == 1) {
        xSemaphoreTake(xRoomEmpty, portMAX_DELAY); // El primer lector bloquea a los escritores
    }
    xSemaphoreGive(xMutex);

    /* --- SECCIÓN CRÍTICA: LEER RECURSO COMPARTIDO --- */

    xSemaphoreTake(xMutex, portMAX_DELAY);
    readers_cnt--;
    if (readers_cnt == 0) {
        xSemaphoreGive(xRoomEmpty); // El último lector libera la sala para los escritores
    }
    xSemaphoreGive(xMutex);
    ```
4.  **Implementar el protocolo en `task_writer.c`:**
    ```c
    xSemaphoreTake(xRoomEmpty, portMAX_DELAY); // Bloquea si hay lectores u otro escritor

    /* --- SECCIÓN CRÍTICA: ESCRIBIR RECURSO COMPARTIDO --- */

    xSemaphoreGive(xRoomEmpty);
    ```