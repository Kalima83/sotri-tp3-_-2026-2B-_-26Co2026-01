# Análisis Técnico del Sistema: Cruce Vehicular (Event-Triggered Systems)

Este proyecto implementa un sistema dirigido por eventos (*Event-Triggered System - ETS*) utilizando **FreeRTOS**. El contexto de la aplicación simula un sistema de "Cruce Vehicular" (Vehicular crossing), donde múltiples tareas responden a estímulos de sensores de entrada y salida.

A continuación, se detalla la función de cada módulo principal:

## 1. `app.c` - Inicialización y Orquestación
Este es el archivo principal de configuración de la aplicación a nivel de usuario.
* Define el propósito del software como un sistema de cruce vehicular bajo el paradigma de sistemas dirigidos por eventos.
* Se encarga de inicializar los contadores de la aplicación (como `g_app_cnt` y `g_tasks_cnt`) a cero.
* Instancia y crea los hilos de ejecución mediante FreeRTOS, preparando los controladores (`TaskHandle_t`) para las tareas: `task_entry_a`, `task_exit_a`, `task_entry_b`, `task_exit_b` y `task_test`.

## 2. `task_test.c` - Generador de Estímulos (Testbench)
Esta tarea actúa como un emulador del entorno físico.
* Su propósito principal es "excitar periódicamente a otras tareas" simulando eventos del mundo real.
* Define una enumeración `e_task_test_t` que incluye eventos como `Entry_A` (vehículo entrando por A) y `Exit_A` (vehículo saliendo por A).
* Posee lógica para alterar dinámicamente su prioridad, permitiéndole inyectar señales o errores en el sistema en momentos específicos (con esperas configurables de hasta `5000mS`) para verificar la correcta respuesta del RTOS.

## 3. `task_entry_a.c` y `task_exit_a.c` - Hilos de Procesamiento
Estos archivos definen el comportamiento de los actuadores y la lógica de negocio al detectar un vehículo.
* **`task_entry_a.c`**: Es el hilo encargado de manejar el evento de entrada de un vehículo. Ejecuta un bucle infinito que incrementa el contador `g_task_entry_a_cnt` y gestiona esperas predefinidas (ej. retardos máximos de `2500mS`) al interactuar con el entorno.
* **`task_exit_a.c`**: Funciona de forma análoga a la tarea de entrada, pero está dedicada a manejar la salida de los vehículos (`g_task_exit_a_cnt`), completando así el ciclo de tránsito en el carril A.

## 4. `app_it.c` - Manejo de Interrupciones (ISRs)
Este archivo gestiona la relación directa con los eventos asíncronos del hardware del microcontrolador.
* Contiene la función `app_it_init()`, que emplea instrucciones en ensamblador (`CPSID i` y `CPSIE i`) para deshabilitar y habilitar las interrupciones globales, creando zonas de protección de recursos compartidos durante el arranque.
* Aunque de forma esquemática en este fragmento base, este archivo está diseñado para capturar los callbacks de los pines GPIO (ej. `HAL_GPIO_EXTI_Callback`), que servirían como la conexión física con los sensores de paso.

## 5. `freertos.c` - Monitoreo y Hooks del Sistema Operativo
Este módulo implementa las funciones de retorno de llamada (*hooks*) obligatorias y opcionales de FreeRTOS para auditar la salud del sistema.
* **`vApplicationIdleHook`**: Se llama automáticamente cuando el RTOS no tiene ninguna tarea de usuario lista para ejecutarse. Aquí se incrementa `g_task_idle_cnt`, siendo un lugar ideal para poner al microcontrolador en modo de bajo consumo.
* **`vApplicationTickHook`**: Se invoca en cada pulso del reloj base del sistema (Tick), incrementando de forma rápida y segura el contador `g_app_tick_cnt`.
* **`vApplicationStackOverflowHook`**: Mecanismo de seguridad crítico; se dispara si alguna de las tareas (como `Entry` o `Exit`) consume más memoria RAM de la asignada a su pila, previniendo fallas catastróficas.