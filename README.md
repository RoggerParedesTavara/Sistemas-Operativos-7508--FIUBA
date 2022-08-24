TP2: Procesos de usuario
========================

env_alloc
---------

### 1.  ¿Qué identificadores se asignan a los primeros  **5 procesos**  creados? (Usar base hexadecimal)
Para los primeros 5 procesos, el `env_id` es 0 por lo tanto el generation empieza desde 4096. A su vez, `e - envs` nos devuelve la posición del proceso en el arreglo,  la cual se añade al `generation` mediante el `| (or)`, de manera tal que los ids se van incrementando en 1 a medida que recorremos el arreglo de procesos.

Entonces, los identificadores asignados a los primeros 5 procesos son:
1. 0x00001000
2. 0x00001001 
3. 0x00001002
4. 0x00001003
5. 0x00001004

#### 2.  Supongamos que al arrancar el kernel se lanzan  `NENV`  procesos a ejecución. A continuación, se destruye el proceso asociado a  `envs[630]`  y se lanza un proceso que cada segundo, muere y se vuelve a lanzar (se destruye, y se vuelve a crear). ¿Qué identificadores tendrán esos procesos en las primeras cinco ejecuciones?

Al lanzar `NENV` procesos y eliminar uno, se garantiza que el próximo proceso lanzado va a ocupar la posición del que se eliminó. Además, el `env_id` del proceso destruido que anteriormente ocupaba esa posición se conserva (no se resetea a 0), con lo cual va a influir en el `env_id` que se le asigne al nuevo proceso. 
En este caso en particular, cuando destruimos el proceso de la posición 630 y volvemos a lanzar otro proceso, éste también ocupará dicha posición (las demás están ocupadas) y se calculará su `env_id` sumándole al id del proceso anterior 4096. El `generation` devuelve un múltiplo de `4096` y después mediante el `| (or)` se añade la posición en el array (630).

En un comienzo, al llenar el array con `NENV` procesos, al proceso de la posición 630 le corresponde el id: `4096+630 = 4726` (decimal), o  `0x00001276` (hexadecimal) .
Al destruirlo, y volver a lanzar un proceso (repitiendo esto 5 veces), cada uno de estos procesos tendrá los siguientes ids:

* Para el primer proceso, `generation` devuelve: `8192`, con el `or` queda `8192+630=8822`, entonces en hexadecimal queda que `env_id=0x00002276`.

* Para el segundo proceso, `generation` devuelve: `12288`, con el `or` queda `12288+630=12918`, entonces en hexadecimal queda que `env_id=0x00003276`.

* Para el tercer proceso, `generation` devuelve: `16384`, con el `or` queda `16384+630=17014`, entonces en hexadecimal queda que `env_id=0x00004276`.

* Para el cuarto proceso, `generation` devuelve: `20480`, con el `or` queda `20480+630=21110`, entonces en hexadecimal queda que `env_id=0x00005276`.

* Para el quinto proceso, `generation` devuelve: `24576`, con el `or` queda `24576+630=25206`, entonces en hexadecimal queda que `env_id=0x00006276`.

env_pop_tf
----------

### 1.  Dada la secuencia de instrucciones  _assembly_  en la función, describir qué contiene durante su ejecución:
*   el tope de la pila justo antes  `popal`
*   el tope de la pila justo antes  `iret`
*	el tercer elemento de la pila justo antes de  `iret`
  
* El tope de la pila justo antes de `popal` contiene el inicio del Trapframe, donde están los `PushRegs` (siendo el primero `tf_reg_edi`)
* el tope de la pila justo antes  `iret` contiene el Instruction Pointer -> `tf_eip`
* el tercer elemento de la pila justo antes de  `iret` es el registro que contiene los flags -> `tf_e_flags`

### 2. ¿Cómo determina la CPU (en x86) si hay un cambio de ring (nivel de privilegio)?  _Ayuda:_  Responder antes en qué lugar exacto guarda x86 el nivel de privilegio actual. ¿Cuántos bits almacenan ese privilegio?
   
En x86, el nivel de privilegio actual se guarda en los dos ultimos bits (bits 0 y 1) del `cs` y se lo llama CPL (Current Privilege Level). Por lo tanto, para saber si hay un cambio de ring, sólo tiene que comparar el CPL del cs que se encuentra en el procesador con el CPL del cs que se encuentra en el Trapframe del proceso a ejecutar, si difieren es porque hay cambio de ring.


gdb_hello
---------

1. Poner un breakpoint en env_pop_tf() y continuar la ejecución hasta allí.
```
(gdb) b env_pop_tf
Breakpoint 1 at 0xf0102e99: file kern/env.c, line 462.
(gdb) c
Continuing.
The target architecture is assumed to be i386
=> 0xf0102e99 <env_pop_tf>:	endbr32 

Breakpoint 1, env_pop_tf (tf=0xf01c8000) at kern/env.c:462
462	{
```

2. En QEMU, entrar en modo monitor (Ctrl-a c), y mostrar las cinco primeras líneas del comando info registers.
```
QEMU 4.2.1 monitor - type 'help' for more information
(qemu) info registers
EAX=003bc000 EBX=00010094 ECX=f03bc000 EDX=00000209
ESI=00010094 EDI=00000000 EBP=f0119fd8 ESP=f0119fbc
EIP=f0102e99 EFL=00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
```

3. De vuelta a GDB, imprimir el valor del argumento tf:
```
(gdb) p tf
$1 = (struct Trapframe *) 0xf01c8000
```

4. Imprimir, con `x/Nx tf` tantos enteros como haya en el struct Trapframe donde `N = sizeof(Trapframe) / sizeof(int)`.
```
(gdb) print sizeof(struct Trapframe) / sizeof(int)
$2 = 17
(gdb) x/17x tf
0xf01c8000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c8010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c8020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c8030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c8040:	0x00000023
```

5. Avanzar hasta justo después del `movl ...,%esp`, usando `si M` para ejecutar tantas instrucciones como sea necesario en un solo paso:
```
(gdb) disas
Dump of assembler code for function env_pop_tf:
=> 0xf0102e99 <+0>:	    endbr32 
   0xf0102e9d <+4>:	    push   %ebp
   0xf0102e9e <+5>:	    mov    %esp,%ebp
   0xf0102ea0 <+7>:	    sub    $0xc,%esp
   0xf0102ea3 <+10>:	mov    0x8(%ebp),%esp
   0xf0102ea6 <+13>:	popa   
   0xf0102ea7 <+14>:	pop    %es
   0xf0102ea8 <+15>:	pop    %ds
   0xf0102ea9 <+16>:	add    $0x8,%esp
   0xf0102eac <+19>:	iret   
   0xf0102ead <+20>:	push   $0xf01058ec
   0xf0102eb2 <+25>:	push   $0x1d8
   0xf0102eb7 <+30>:	push   $0xf01058a6
   0xf0102ebc <+35>:	call   0xf01000ad <_panic>
End of assembler dump.
(gdb) si 5
=> 0xf0102ea6 <env_pop_tf+13>:	popa   
0xf0102ea6 in env_pop_tf (tf=0x0) at kern/env.c:463
463		asm volatile("\tmovl %0,%%esp\n"
```

6. Comprobar, con `x/Nx $sp` que los contenidos son los mismos que tf (donde N es el tamaño de tf).
```
(gdb) x/17x $sp
0xf01c8000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c8010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c8020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c8030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c8040:	0x00000023
```
7. Describir cada uno de los valores. Para los valores no nulos, se debe indicar dónde se configuró inicialmente el valor, y qué representa.

0x00000000 -> `uint32_t reg_edi`
0x00000000 -> `uint32_t reg_esi`
0x00000000 -> `uint32_t reg_ebp`
0x00000000 -> `uint32_t reg_oesp`
0x00000000 -> `uint32_t reg_ebx`
0x00000000 -> `uint32_t reg_edx`
0x00000000 -> `uint32_t reg_ecx`
0x00000000 -> `uint32_t reg_eax`

0x00000023 -> `uint16_t tf_es | uint16_t tf_padding1`
* Representa el `Extra Segment` , el cual contiene el numero del Global Descriptor del Extra Segment en la GDT.
* Se inicializa en `env_alloc()`
	
0x00000023 -> `uint16_t tf_ds | uint16_t tf_padding2`
* Representa el `Data Segment`, el cual contiene el numero del Global Descriptor del Data Segment en la GDT.
* Se inicializa en `env_alloc()`

0x00000000 -> `uint32_t tf_trapno`
0x00000000 -> `uint32_t tf_err`

0x00800020 -> `uintptr_t tf_eip`
* Representa el `Instruction Pointer`, contiene la dirección de la próxima instrucción a ejecutar.
* Se inicializa en `load_icode()`

0x0000001b -> `uint16_t tf_cs |	uint16_t tf_padding3`
* Representa el `Code Segment`, el cual contiene el numero del Global Descriptor del Code Segment en la GDT.
* Se inicializa en `env_alloc()`

0x00000000 -> `uint32_t tf_eflags`

0xeebfe000 -> `uintptr_t tf_esp`
* Representa el `Stack Pointer`, el cual tiene la dirección del tope del stack.
* Se inicializa en `env_alloc()`

0x00000023 -> `uint16_t tf_ss | uint16_t tf_padding4`
* Representa el `Stack Segment`, el cual contiene el numero del Global Descriptor del Stack Segment en la GDT.
* Se inicializa en `env_alloc()`

8. Continuar hasta la instrucción `iret`, sin llegar a ejecutarla. Mostrar en este punto, de nuevo, las cinco primeras líneas de `info registers` en el monitor de QEMU. Explicar los cambios producidos.
```
(gdb) disas
Dump of assembler code for function env_pop_tf:
   0xf0102e99 <+0>:	endbr32 
   0xf0102e9d <+4>:	push   %ebp
   0xf0102e9e <+5>:	mov    %esp,%ebp
   0xf0102ea0 <+7>:	sub    $0xc,%esp
   0xf0102ea3 <+10>:	mov    0x8(%ebp),%esp
=> 0xf0102ea6 <+13>:	popa   
   0xf0102ea7 <+14>:	pop    %es
   0xf0102ea8 <+15>:	pop    %ds
   0xf0102ea9 <+16>:	add    $0x8,%esp
   0xf0102eac <+19>:	iret   
   0xf0102ead <+20>:	push   $0xf01058ec
   0xf0102eb2 <+25>:	push   $0x1d8
   0xf0102eb7 <+30>:	push   $0xf01058a6
   0xf0102ebc <+35>:	call   0xf01000ad <_panic>
End of assembler dump.
(gdb) si 4
```

```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=f01c8030
EIP=f0102eac EFL=00000096 [--S-AP-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
```
Los cambios son que se restauran los valores de los registros de proposito general, en este caso se setean a 0 ya que es la primera ejecución, y luego se actualiza el valor de los registros ES y DS con los valores del proceso actual. 

9. Ejecutar la instrucción `iret`. En ese momento se ha realizado el cambio de contexto y los símbolos del kernel ya no son válidos.

* imprimir el valor del contador de programa con `p $pc` o `p $eip`
```
(gdb) p $pc
$3 = (void (*)()) 0x800020
```

* cargar los símbolos de hello con el comando `add-symbol-file`
```
(gdb) add-symbol-file obj/user/hello 0x800020
add symbol table from file "obj/user/hello" at
	.text_addr = 0x800020
(y or n) y
Reading symbols from obj/user/hello...
```

* volver a imprimir el valor del contador de programa
```
(gdb) p $pc
$4 = (void (*)()) 0x800020 <_start>
```

Mostrar una última vez la salida de info registers en QEMU, y explicar los cambios producidos.
```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=eebfe000
EIP=00800020 EFL=00000002 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
```
Los cambios son que al ejecutar `iret`, se cargar en los registros del procesador los valores de los registros: `eip`, `cs`, `eflags` y `esp` del proceso actual que se encontraban en el trapframe.


10. Poner un breakpoint temporal (`tbreak`, se aplica una sola vez) en la función `syscall()` y explicar qué ocurre justo tras ejecutar la instrucción `int $0x30`. Usar, de ser necesario, el monitor de QEMU.

Al ejecutar `int $0x30` podemos notar que se pasa a ring 0 (`CPL=0`). 
```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000663
ESI=00000000 EDI=00000000 EBP=00000000 ESP=00000000
EIP=0000e05b EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0000 00000000 0000ffff 00009300
CS =f000 000f0000 0000ffff 00009b00
```
Además, si intentamos seguir con la ejecución, observamos que qemu parece reiniciarse, mostrando:
```
Booting from Hard Disk..6828 decimal is 15254 octal!
Physical memory: 131072K available, base = 640K, extended = 130432K
check_page_alloc() succeeded!
check_page() succeeded!
check_kern_pgdir() succeeded!
check_page_installed_pgdir() succeeded!
[00000000] new env 00001000
```

kern_idt
--------
### Leer  user/softint.c  y ejecutarlo con  make run-softint-nox. ¿Qué interrupción trata de generar? ¿Qué interrupción se genera? Si son diferentes a la que invoca el programa… ¿cuál es el mecanismo por el que ocurrió esto, y por qué motivos? ¿Qué modificarían en JOS para cambiar este comportamiento?

* La interrupción que se trata de generar es una Page Fault
* La interrupción que se genera es General Protection
* La razón por la que ocurre esto es debido a que no se cuentan con los permisos necesarios como para que una Page Fault sea lanzada por un programa de usuario. Cuando se definen los permisos, se establece que se debe contar con los permisos del Kernel (Ring  0).
* Lo que hay que modificar es JOS es el nivel de privilegios de la interrupción asignado en la IDT, pasando de 0 a 3.

user_evilhello
--------------

### ¿En qué se diferencia el código de la versión en  _evilhello.c_  mostrada arriba?
El código modificado de _evilhello.c_ se diferencia en que desde el lado del usuario se intenta desreferenciar una dirección a la cual no tiene acceso (punto de entrada del Kernel), lo cual provoca un error. Mientras que en _evilhello.c_ original, directamente se envía por parametro la dirección y el Kernel será quien la desreferencie.

### ¿En qué cambia el comportamiento durante la ejecución?
El comportamiento cambia en que en _evilhello.c_ original sí se logra que se imprima por pantalla algo, mientras que en _evilhello.c_ modificado no sucede esto, se produce un Page Fault.
### ¿Por qué? ¿Cuál es el mecanismo?
La razón por la que ocurre esto es que en el caso de _evilhello.c_ original, se pasa la dirección por parámetro y es el Kernel quien se encarga de desreferenciarla y acceder allí (y puede hacerlo). En cambio, en _evilhello.c_ modificado, se intenta acceder a dicha dirección desde el lado del usuario, como no tiene acceso, se lanza el Page Fault.

### Listar las direcciones de memoria que se acceden en ambos casos, y en qué  _ring_  se realizan. ¿Es esto un problema? ¿Por qué?
* En _evilhello.c_ original, se logra acceder a `0xf010000c` (punto de entrada al Kernel). Este acceso lo realiza el Kernel, por lo que se hace en ring 0. Sí, es un problema ya que si bien el usuario no puede acceder a dicha dirección, sí puede obtener la información mediante el Kernel y se debería restringir esto.
* En _evilhello.c_ modificado, se quiere acceder a `0xf010000c`, pero no se puede lograr. Esto es porque se realiza desde ring 3 y no se cuentan con los permisos. No supone un problema en cuanto a la seguridad, el único problema es que el usuario no obtendrá un resultado exitoso y se producirá un Page Fault.
