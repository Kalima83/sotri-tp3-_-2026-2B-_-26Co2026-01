forma atómica (sin que una interrupción interfiera a mitad del proceso).

---

## 📊 Resumen del Flujo de Ejecución

Cuando el programa se inicia, sigue la siguiente secuencia temporal:

```
[ app_init() ] -> Inicializa variables y crea Tareas (Productor y Consumidor) con Prioridad 1.
      |
      v
[ Lanzamiento del Scheduler de FreeRTOS ]
      |
      +---> [ Tarea Productora ] ---> Incrementa contador -> Imprime Log -> vTaskDelay(250ms) [Bloqueada]
      |                                                                            |
      +---> [ Tarea Consumidora ] --> Incrementa contador -> Imprime Log -> vTaskDelay(250ms) [Bloqueada]
      |                                                                            |
      |     (Ambas tareas en estado Blocked durante 250ms)                         |
      v                                                                            v
[ Tarea Idle corriendo ] --------> Llama a vApplicationIdleHook() -> Incrementa g_task_idle_cnt
      |
[ Interrupción del Timer ] ------> Cada 1ms llama a vApplicationTickHook() -> Incrementa g_app_tick_cnt
```