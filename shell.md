# Lab: shell

### Búsqueda en $PATH
#### ¿Cuáles son las diferencias entre la _syscall_  `execve(2)` y la familia de _wrappers_ proporcionados por la librería estándar de _C_ (_libc_) `exec(3)`?
 
Execve es una system call, es decir, es una funcionalidad que provee el Kernel la cual es llamada desde "User mode" para solicitar a dicho Kernel realizar una determinada operación que requiere llevarse a cabo en el "Kernel mode". En este caso, execve permite cambiar la imagen de un proceso por la de otro proceso perteneciente a la ejecución de un programa especificado previamente por parámetro. Reemplazar la imagen del proceso implica reemplazar las diferentes secciones de memoria (code, data, stack y heap) y los argumentos recibidos. Sin embargo, se mantienen ciertos datos asociados a ese proceso como el pid, el ppid (el padre seguirá siendo el mismo) y files descriptors. Esta es una syscall única.

En cambio, la familia de wrappers que nos brinda la biblioteca estándar de C NO son una syscall, sino que son un conjunto de funciones que hacen uso de la syscall execve. Para el caso particular de execve, tenemos una familia de wrappers las cuales nos proporcionan una gran flexibilidad ya que cada función de esta familia toma distintos tipos de parámetros, de modo que utilicemos la funcion que mejor se adapte a nuestro problema. Estos wrappers tienen en su nombre el prefijo "exec" y luego letras que determinan una caracteristica:

* l: Los argumentos del nuevo programa a ejecutarse se pasan uno por uno en forma de lista, terminando con NULL.
* v: Los argumentos se pasan todos juntos en un vector, también terminado en NULL.
* p: Permite no tener que especificar la ruta completa del binario a ejecutar, sino que la busca en la variable de entorno PATH.
* e: Se pasa explicitamente un array con las variables de entorno que tendrá nuestro proceso.

#### ¿Puede la llamada a `exec(3)` fallar? ¿Cómo se comporta la implementación de la _shell_ en ese caso?
Sí, puede fallar. En ese caso retorna `-1` y `errno` es seteado segun el error. Además, como no se reemplazó la imagen del proceso, se seguirán ejecutándose las instrucciones restantes que estén por debajo de la llamada a `exec(3)`.
En la implementación de la shell, si falla el `exec(3)`, se termina inmediatamente el proceso de ejecución de comando, alertando del error mediante `perror()` y el proceso shell se queda esperando a que el usuario ingrese el siguiente comando.

---

### Comandos built-in
#### ¿Entre `cd` y `pwd`, alguno de los dos se podría implementar sin necesidad de ser _built-in_? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como _built-in_? (para esta última pregunta pensar en los _built-in_ como `true` y `false`)

El que se puede implementar sin ser built-in es `pwd`, debido a que muestra el path del _current working directory_, el cual es heredado por el proceso del comando luego de hacer el fork del proceso de la shell. En cambio, no se podría hacerlo con `cd` ya que estariamos cambiando el directorio del proceso del comando (hijo de shell), y NO el directorio del proceso de la shell.

El motivo de hacer `pwd` built-in es que tiene mejor performance ya que nos ahorramos tener que hacer el `fork(2)` del proceso de la shell, reservar memoria para alocar el struct del comando, llamar a `exec(3)`  y finalmente hacer el `wait(2)` desde el proceso de la shell para esperar a que finalice el proceso del comando.

---

### Variables de entorno adicionales

#### ¿Por qué es necesario hacerlo luego de la llamada a `fork(2)`?
Porque queremos que estas variables estén en el entorno del proceso del comando a ejecutar, no en el entorno del proceso de la shell.

#### En algunos de los _wrappers_ de la familia de funciones de `exec(3)` (las que finalizan con la letra _e_), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar `setenv(3)` por cada una de las variables, se guardan en un array y se lo coloca en el tercer argumento de una de las funciones de `exec(3)`.

-   ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.

No, el comportamiento no es el mismo. Lo que sucede es que con las funciones de `exec(3)` que terminan con _e_ es que sólo podemos usar las variables pasadas por parámetro en el 3er argumento, ya que se reemplaza la imagen del proceso de modo que ahora el entorno tiene unicamente las variables pasadas por parámetro. En cambio, al usar `setenv(3)` no se reemplaza el entorno, sino que las variables  indicadas se añaden al mismo, pudiendo así usar tanto las que ya tenia en su entorno como también las añadidas mediante setenv.

-   Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

Una posible implementación es pasar en el tercer argumento un array que contenga todas las variables del entorno actual, para obtener dichas variables podemos usar la variable global `extern char **environ` (`environ(7)`), que apunta a un array de punteros a cada una de las variables del entorno.

---

### Procesos en segundo plano
#### Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.
Una vez que se detecta que se quiere ejecutar el proceso en segundo plano (mediante & en el comando), comienza su ejecución como cualquier otro proceso haciendo uso de `exec(3)`. Luego, para que efectivamente se trate de un proceso en segundo plano, se requiere que el proceso de la shell no espere a que la ejecución del comando finalice, es decir, que devuelva el prompt de inmediato sin colgarse. Para esto, se utiliza `waitpid()` con el flag WNOHANG, lo cual permite que si el proceso del comando terminó, efectivamente se recupera la metadata de dicho proceso y el kernel la limpia, pero en caso contrario, el proceso shell sigue ejecutándose esperando a que el usuario ingrese el siguiente comando. De forma periódica, se chequea si algún proceso en segundo plano ya terminó para que el kernel pueda limpiar su metadata y evitar así acumular procesos zombies.

---

### Flujo estándar
#### Investigar el significado de `2>&1`, explicar cómo funciona su _forma general_ y mostrar qué sucede con la salida de `cat out.txt` en el ejemplo. Luego repetirlo invertiendo el orden de las redirecciones. ¿Cambió algo?
El operador `>` se utiliza para hacer redirecciones. En este caso estamos redireccionando el file descriptor `2` correspondiente a `stderr` hacia el file descriptor 1 (`&1`) correspondiente a `stdout`. Esto se puede lograr mediante el uso de la syscall `dup(2)`. Por lo tanto, todos los errores que se escriban en el fds 2, ahora van a ser impresos por pantalla.

Salida de cat out.txt en ejemplo:
```
nacho@nachoUbuntu:~$ ls -C /home /noexiste >out.txt 2>&1
nacho@nachoUbuntu:~$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
nacho
```

Ahora invirtiendo el orden:
```
nacho@nachoUbuntu:~$ ls -C /home /noexiste 2>&1 >out.txt
ls: cannot access '/noexiste': No such file or directory
nacho@nachoUbuntu:~$ cat out.txt
/home:
nacho
```
Al cambiar el orden, el fds 1 inicialmente apunta a `stdout` por lo que al hacer `2>&1` estamos redireccionando el fds 2  a`stdout` (que estaba apuntando a `stderr`). Luego redireccionamos el fds 1 hacia `out.txt`, pero no volvemos a redireccionar el fds2, por lo tanto, todo lo que se escriba allí será impreso por pantalla, a diferencia de lo que ocurria antes de invertir el orden.

---

### Tuberías simples (pipes)

#### Investigar qué ocurre con el _exit code_ reportado por la _shell_ si se ejecuta un pipe ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando `bash`. Comparar con la implementación del este lab.

Cuando se ejecuta un pipe, el exit code que reporta la shell es el correspondiente al último comando del pipe.
Si alguno de los comandos falla, el exit code que devuelve la shell seguirá siendo el correspondiente al último comando, aunque se puede utilizar `set -o pipefail` para que la shell reporte el exit code del último comando que haya tenido un exit code distinto de cero.

**Ejemplos en bash:**
1) El retorno es distinto de cero dado que el último comando falló.
```
nacho@nachoUbuntu:~$ seq 3 | ls /noexiste
ls: cannot access '/noexiste': No such file or directory
nacho@nachoUbuntu:~$ echo $?
2
```
2) El retorno es cero debido a que no hubo fallo en el último comando (a pesar de que en el primero sí).
```
nacho@nachoUbuntu:~$ ls /noexiste | seq 3
1
2
3
ls: cannot access '/noexiste': No such file or directory
nacho@nachoUbuntu:~$ echo $?
0
```

**En la shell del lab:**

```
 (/home/nacho) 
$ seq 3 | ls /noexiste
ls: cannot access '/noexiste': No such file or directory
 (/home/nacho) 
$ echo $?
2
	Program: [echo $?] exited, status: 0 
```
	
```
 (/home/nacho) 
$ ls /noexiste | seq 3
1
2
3
ls: cannot access '/noexiste': No such file or directory
 (/home/nacho) 
$ echo $?
0
	Program: [echo $?] exited, status: 0 
```

---

### Pseudo-variables
#### Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en `bash` (u otra terminal similar).

1. `$$` -> Se expande al PID del proceso actual de la shell.
```
nacho@nachoUbuntu:~$ echo $$
68434

//Efectivamente coincide
nacho@nachoUbuntu:~$ ps
    PID TTY          TIME CMD
  68434 pts/3    00:00:00 bash
  68494 pts/3    00:00:00 ps
```

2. `$0` -> Se expande al nombre del script actual (el de la shell en este caso).
```
nacho@nachoUbuntu:~$ echo $0
bash
```

3. `$!` -> Se expande al PID del último proceso que se ejecutó en segundo plano.
```
nacho@nachoUbuntu:~$ sleep 3&
[1] 69241
nacho@nachoUbuntu:~$ sleep 2&
[2] 69260
[1]   Done                    sleep 3
nacho@nachoUbuntu:~$ sleep 1
[2]+  Done                    sleep 2
nacho@nachoUbuntu:~$ echo $!
69260

//Efectivamente coincide con el PID del proceso que ejecuta el comando 'sleep 2&'

```
---

