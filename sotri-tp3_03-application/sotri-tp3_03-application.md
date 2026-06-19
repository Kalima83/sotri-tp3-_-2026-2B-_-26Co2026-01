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

# Informe - Problema de Cruce Vehicular de Capacidad Limitada

## 1. Mecanismos de Sincronización Implementados
Para resolver el problema del control de acceso al cruce vial bajo un paradigma de **Sistema Dirigido por Eventos (Event-Triggered System)**, se implementó una arquitectura basada en dos tipos de primitivos de FreeRTOS:

* **Semáforos Binarios (Señalización):** Se instanciaron cuatro semáforos binarios (`xSemEntryA`, `xSemExitA`, `xSemEntryB`, `xSemExitB`) para actuar como mecanismo de señalización asincrónica. Permiten que la tarea generadora de estímulos (`task_test`) notifique a las tareas de control correspondientes. Gracias a esto, las tareas de ingreso y egreso permanecen en estado `Blocked` (sin consumir ciclos de reloj de la CPU) hasta que realmente ocurre un evento en los sensores.
* **Mutex (Exclusión Mutua y Control de Recursos):** Se utilizó un único Mutex (`xMutexCruce`) para proteger la variable global `g_vehicles_in_crossing`. Dado que cuatro tareas distintas (2 de entrada, 2 de salida) necesitan leer y modificar este contador, el Mutex garantiza operaciones atómicas, previniendo **condiciones de carrera (Race Conditions)**. Además, el Mutex envuelve la lógica condicional que verifica si se superó el límite `G_TASKS_CNT_MAX`, asegurando que la evaluación de capacidad sea completamente segura.

## 2. Comportamiento Observado durante la Depuración (Debugging)
Al ejecutar el firmware en el microcontrolador y monitorear la salida mediante la terminal serial (Logger), se observaron las siguientes dinámicas del sistema en tiempo real:

1. **Gestión de Eventos:** El sistema no utiliza *polling* (espera activa). Las tareas `task_entry` y `task_exit` solo pasan a estado `Running` en el momento exacto en que `task_test` inyecta un estímulo y ejecuta un `xSemaphoreGive`.
2. **Ingreso y Semáforo Verde:** Ante ráfagas de eventos `Entry_A` o `Entry_B`, las tareas de ingreso toman el Mutex, verifican que hay espacio en el cruce, incrementan el contador y reportan por consola el estado del semáforo vial en **`[VERDE]`**, simulando el tiempo de cruce mediante un `vTaskDelay`.
3. **Bloqueo por Capacidad Máxima (Semáforo Rojo):** Cuando el contador de vehículos concurrentes en el cruce alcanza el límite establecido (`G_TASKS_CNT_MAX = 5`), el sistema reacciona de manera robusta. Si llega un nuevo estímulo de ingreso, la tarea adquiere el Mutex, detecta que la capacidad está colmada y rechaza el ingreso cambiando el semáforo vial a **`[ROJO]`**. Acto seguido, libera inmediatamente el Mutex sin incrementar el contador, protegiendo al cruce del desbordamiento físico.
4. **Egreso y Liberación de Cupo:** Al recibir eventos `Exit_A` o `Exit_B`, las tareas de egreso entran a su sección crítica protegida por el Mutex, decrementan el contador y liberan el recurso. Se comprobó que, tras un egreso, el sistema vuelve a aceptar vehículos en el siguiente estímulo de entrada, restaurando el color verde en el semáforo de acceso.

## 3. Conclusiones
El diseño cumple exitosamente con los requisitos de concurrencia y control de capacidad. La combinación de **Semáforos Binarios** (para la sincronización de interrupciones o eventos de hardware/software) junto con un **Mutex** (para proteger la memoria compartida) demuestra ser el estándar óptimo y eficiente en FreeRTOS para coordinar el acceso a zonas físicas de recursos limitados sin sufrir interbloqueos (*deadlocks*) ni corrupción de datos.