TP3: Multitarea con desalojo
============================

sys_yield
---------
### Leer y estudiar el código del programa  _user/yield.c_. Cambiar la función  `i386_init()`  para lanzar tres instancias de dicho programa, y mostrar y explicar la salida de  `make qemu-nox`.

En un comienzo, lo que ocurre es que se imprime por pantalla el mensaje con la id del proceso actual, pero luego al ingresar al _for_ se llama a syscall `sys_yield`, la cual desaloja el proceso actual en ejecución y pone a correr al siguiente proceso. Como el nuevo proceso también corre el mismo programa, tiene el mismo comportamiento, por lo cual al llamar a la syscall `sys_yield` se pone a correr al tercer proceso para luego ser desalojado y volver al primero. Esto se repite hasta que se termina la ejecución de los tres procesos.

```
Booting from Hard Disk..6828 decimal is 15254 octal!
Physical memory: 131072K available, base = 640K, extended = 130432K
check_page_free_list() succeeded!
check_page_alloc() succeeded!
check_page() succeeded!
check_kern_pgdir() succeeded!
check_page_free_list() succeeded!
check_page_installed_pgdir() succeeded!
SMP: CPU 0 found 1 CPU(s)
enabled interrupts: 1 2
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
Hello, I am environment 00001000.
Hello, I am environment 00001001.
Hello, I am environment 00001002.
Back in environment 00001000, iteration 0.
Back in environment 00001001, iteration 0.
Back in environment 00001002, iteration 0.
Back in environment 00001000, iteration 1.
Back in environment 00001001, iteration 1.
Back in environment 00001002, iteration 1.
Back in environment 00001000, iteration 2.
Back in environment 00001001, iteration 2.
Back in environment 00001002, iteration 2.
Back in environment 00001000, iteration 3.
Back in environment 00001001, iteration 3.
Back in environment 00001002, iteration 3.
Back in environment 00001000, iteration 4.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
Back in environment 00001001, iteration 4.
All done in environment 00001001.
[00001001] exiting gracefully
[00001001] free env 00001001
Back in environment 00001002, iteration 4.
All done in environment 00001002.
[00001002] exiting gracefully
[00001002] free env 00001002
No runnable environments in the system!
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
```


dumbfork
--------
### 1.  Si una página  **no**  es modificable en el padre ¿lo es en el hijo? En otras palabras: ¿se preserva, en el hijo, el  _flag_  de solo-lectura en las páginas copiadas?

No, no se preserva el flag de sólo lectura debido a que en `duppage()` se obtiene la nueva página siempre con los permisos: `PTE_P|PTE_U|PTE_W`, sin importar si en el padre es sólo lectura o no.

### 2. Mostrar, **con código en espacio de usuario**, cómo podría `dumbfork()` verificar si una dirección en el padre es de solo lectura, de tal manera que pudiera pasar como tercer parámetro a `duppage()` un booleano llamado _readonly_ que indicase si la página es modificable o no.

```
envid_t dumbfork(void) {
    // ...
    for (addr = UTEXT; addr < end; addr += PGSIZE) {
        bool readonly;
        if( (uvpd[PDX(addr)] & PTE_W) && (uvpt[PGNUM(addr)] & PTE_W) )
		        readonly = true;
        duppage(envid, addr, readonly);
    }
    // ...

```

### 3. Supongamos que se desea actualizar el código de `duppage()` para tener en cuenta el argumento _readonly:_ si este es verdadero, la página copiada no debe ser modificable en el hijo. Es fácil hacerlo realizando una última llamada a `sys_page_map()` para eliminar el flag `PTE_W` en el hijo, cuando corresponda. Se pide mostrar una versión en el que se implemente la misma funcionalidad _readonly_, pero sin usar en ningún caso más de tres llamadas al sistema.

```
void duppage(envid_t dstenv, void *addr, bool readonly) {
    int flags = PTE_P | PTE_U;
    if(!readonly){
        flags |= PTE_W;
    }
    sys_page_alloc(dstenv, addr, flags);
    sys_page_map(dstenv, addr, 0, UTEMP, PTE_P | PTE_U | PTE_W);

    memmove(UTEMP, addr, PGSIZE);
    sys_page_unmap(0, UTEMP);
}
```


ipc_recv
--------
### Un proceso podría intentar enviar el valor númerico `-E_INVAL` vía `ipc_send()`. ¿Cómo es posible distinguir si es un error, o no?
```
envid_t src = -1;
int r = ipc_recv(&src, 0, NULL);

if (r < 0)
  if (/* ??? */)
    puts("Hubo error.");
  else
    puts("Valor negativo correcto.")
```

En `ipc_recv()`, si se le pasan los parámetros `from_env_store` y/o `perm_store`, los valores a los que apuntan se setean en 0 en caso de que ocurra un error en la syscall. Por lo tanto se podría hacer lo siguiente para distinguir entre un error y el recibir un valor negativo:
```
envid_t src = -1;
int r = ipc_recv(&src, 0, NULL);

if (r < 0)
  if (src == 0)
    puts("Hubo error.");
  else
    puts("Valor negativo correcto.")
```


sys_ipc_try_send
----------------
### Se pide ahora explicar cómo se podría implementar una función `sys_ipc_send()` (con los mismos parámetros que `sys_ipc_try_send()`) que sea bloqueante, es decir, que si un proceso A la usa para enviar un mensaje a B, pero B no está esperando un mensaje, el proceso A sea puesto en estado `ENV_NOT_RUNNABLE`, y despertado una vez B llame a `ipc_recv()` (cuya firma _no_ debe ser cambiada).

Una posible implementación podria ser la siguiente:
En `sys_ipc_send()` chequear si el proceso _receiver_ está esperando (env_ipc_recving == true).
En caso de que lo esté, se realiza el envío de la misma forma. Pero si el proceso _receiver_ no está bloqueado, se añade al proceso _sender_ en una cola de _senders_ que tendrá el proceso _receiver_ y posteriormente se bloquea al proceso _sender_ seteando su estado como `ENV_NOT_RUNNABLE` y desalojándolo.
Para que esto funcione, del lado del proceso _receiver_ se debe añadir en `sys_ipc_recv()` un chequeo previo de si hay algún proceso en la cola de _senders_. En caso de que sí, se selecciona al primero, se cambia el estado de éste a `ENV_RUNNABLE`, luego se sigue ejecutando la función tal como se encuentra ahora, terminando en el bloqueo del proceso _receiver_, seteando su estado como `ENV_NOT_RUNNABLE` y desalojándolo.
Cuando el scheduler ponga a correr al proceso _sender_ nuevamente es porque ya está el proceso _receiver_ bloqueado esperando el envío del mensaje, asi que se continuará con el envío de la misma manera.


#### - ¿Qué cambios se necesitan en  `struct Env`  para la implementación? (campos nuevos, y su tipo; campos cambiados, o eliminados, si los hay)
Se necesita añadir dos campos nuevos:
- `bool env_ipc_sending` -> Env is blocked sending
- `senders_queue` -> Contains all envs that want to send something to this env 


#### - ¿Qué asignaciones de campos se harían en  `sys_ipc_send()`?
En caso de que sea necesario bloquear al proceso _sender_, se deberá asignar `env_ipc_sending = true`. A su vez, también será necesario añadir el env actual (_sender_) a la cola de envs que posee el proceso _receiver_ (añadir ya sea el envid o el struct Env como tal).


#### - ¿Qué código se añadiría en `sys_ipc_recv()`?
Tal como se mencionó en la explicación, se añadiría el chequeo de si hay algún env _sender_ en la cola que posee el env _receiver_. En caso de que lo haya, se setea como _runnable_ al primero de la cola y se continúa con la función.


#### - ¿Existe posibilidad de _deadlock_?
Sí. Podría ocurrir si el proceso receiver pasa el chequeo de la cola de senders y la misma esta vacía, pero antes bloquearse, es desalojado por el scheduler. Luegom un proceso sender hace el chequeo de si el receiver está esperando, como ve que no, se duerme y finalmente se retoma el proceso receiver bloqueándose también. Entonces quedarían ambos bloqueados. Haría falta que llegue un nuevo sender para despertar al receiver.


#### - ¿Funciona que varios procesos (A₁, A₂, …) puedan enviar a B, y quedar cada uno bloqueado mientras B no consuma su mensaje? ¿En qué orden despertarían?
Sí, funcionaría lo planteado. El orden en que se despertarían es el mismo orden en el que se fueron durmiendo y añadiendo a la cola los procesos senders, es decir, sería un _First In, First Out_ (FIFO).
