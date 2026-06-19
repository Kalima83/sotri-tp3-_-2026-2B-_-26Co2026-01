# Análisis Técnico del Sistema: Control de Compuertas/Puertas (Event-Triggered System)

Este proyecto implementa un sistema dirigido por eventos (*Event-Triggered System*) utilizando el sistema operativo en tiempo real **FreeRTOS**. El contexto de la aplicación simula un sistema de control de múltiples accesos o compuertas (Gates/Doors A, B, C y D), donde las tareas responden a estímulos de apertura y cierre.

A continuación, se detalla la función de cada módulo principal:

## 1. `app.c` - Inicialización y Orquestación del Sistema
Este es el archivo principal de configuración de la aplicación a nivel de usuario.
* **Propósito principal:** Inicializar los contadores globales (como `g_app_cnt` y `g_tasks_cnt`) a cero antes de arrancar el planificador.
* **Creación de Tareas:** Se encarga de instanciar 5 hilos de ejecución (*threads*) mediante la API de FreeRTOS (`xTaskCreate`). Las tareas creadas son: `task_gate_a`, `task_gate_b`, `task_gate_c`, `task_gate_d` y `task_test`.
* **Prioridades:** Inicialmente, todas las tareas se crean con el mismo nivel de prioridad base (`tskIDLE_PRIORITY + 1ul`).

## 2. `task_test.c` - Generador de Estímulos (Testbench)
Esta tarea actúa como un emulador del entorno físico o un inyector de eventos.
* **Elevación de prioridad:** Al iniciar, esta tarea eleva temporalmente su propia prioridad para asegurarse de ejecutarse primero y preparar el entorno de simulación.
* **Generación de eventos:** Posee un bucle infinito que recorre un arreglo de estímulos predefinidos (`e_task_test_array`).
* **Tipos de estímulos:** Contiene casos para eventos como `OPEN_REQUEST_A` (solicitud de apertura de la compuerta A) y `DOOR_CLOSED_A` (señal de puerta cerrada). Su objetivo es enviar estas señales al resto del sistema para verificar su comportamiento.

## 3. Archivos `task_gate_a.c`, `task_gate_b.c`, `task_gate_c.c` y `task_gate_d.c` - Controladores de Actuadores
Estos cuatro archivos contienen la lógica individual para cada una de las compuertas. Su estructura es idéntica y sirven como hilos de procesamiento independientes.
* **Inicialización:** Cada uno define su propio contador de ejecuciones (ej. `g_task_gate_a_cnt`) y envía un mensaje por el puerto serie (Logger) indicando que ha iniciado.
* **Bucle de control:** Se ejecutan en un bucle infinito `for(;;)`. Actualmente presentan una estructura base o plantilla que imprime un mensaje de espera (ej. `Wait: 2500mS`) simulando el tiempo físico que tardaría la puerta en operar. 

## 4. `app_it.c` - Manejo de Interrupciones (ISRs)
Este módulo gestiona la conexión entre los eventos físicos del hardware (pines del microcontrolador) y el software.
* **Zonas críticas:** Contiene la función `app_it_init()` que utiliza ensamblador (`CPSID i` y `CPSIE i`) para deshabilitar y habilitar interrupciones globales durante la configuración.
* **Callbacks de Hardware:** Implementa la función `HAL_GPIO_EXTI_Callback`, que es la rutina de interrupción (ISR) que se dispara cuando un botón físico o sensor cambia de estado. Este es el punto de entrada real para los eventos del hardware.

## 5. `freertos.c` - Hooks (Ganchos) del Sistema Operativo
Este archivo contiene funciones de retorno (*callbacks*) que FreeRTOS ejecuta automáticamente para auditar el sistema.
* **`vApplicationIdleHook`:** Se ejecuta cuando ninguna otra tarea tiene trabajo que hacer. Incrementa `g_task_idle_cnt`. Es ideal para poner el microcontrolador en modo de bajo consumo (Sleep).
* **`vApplicationTickHook`:** Se llama en cada interrupción del temporizador base de FreeRTOS (Tick). Incrementa de forma rápida el contador global del sistema.
* **`vApplicationStackOverflowHook`:** Es un mecanismo vital de seguridad. Si alguna tarea (como una de las *gates*) consume más memoria RAM de su pila asignada, esta función atrapa el error (usando un `configASSERT`) y congela el sistema para evitar daños mayores y permitir su