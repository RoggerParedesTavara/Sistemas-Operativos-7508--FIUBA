# TP1: Memoria virtual en JOS

===========================

  

## boot_alloc_pos

### a. Un cálculo manual de la primera dirección de memoria que devolverá  `boot_alloc()`  tras el arranque. Se puede calcular a partir del binario compilado (`obj/kern/kernel`), usando los comandos  `readelf`  y/o  `nm`  y operaciones matemáticas.

La primera dirección de memoria que devuelve `boot_alloc()` es la correspondiente a la última dirección donde se encuentra JOS (`end`) pero redondeada a un múltiplo de 4Kb (hacia arriba). 

Para calcularla, primero obtenemos dicha dirección, para ello nos fijamos a que dirección corresponde el end a través del siguiente comando en la shell:
```
nacho@nacho-HP-ProBook-440-G6:~/Desktop/sisop_tps/obj/kern$ nm kernel
f0117530 b addr_6845
f0100b29 t boot_alloc
f010162a t boot_map_region
...
f0118950 B end
...
0010000c T _start
...
f0102c45 T vprintfmt
f0102f99 T vsnprintf
f01000f5 T _warn
```
Vemos que corresponde a `0xf0118950` (`4027681104` en decimal). Una vez que se llama a `boot_alloc()`, se redondea esta dirección mediante `ROUNDUP((char  *)  end, PGSIZE)`, de modo que se busca la dirección de la próxima página. Hacemos la cuenta:
```
end = 4027681104
pgsize = 4096
result = end + (pgsize-(end%pgsize))

=> result = 4027682816
```
La dirección que retorna `boot_alloc()` es `4027682816` = `0xf0119000`

---

### b. 1.  Una sesión de GDB en la que, poniendo un breakpoint en la función  `boot_alloc()`, se muestre el valor devuelto en esa primera llamada, usando el comando GDB  finish
```
nacho@nacho-HP-ProBook-440-G6:~/Desktop/sisop_tps$ make gdb
gdb -q -s obj/kern/kernel -ex 'target remote 127.0.0.1:26000' -n -x .gdbinit
Reading symbols from obj/kern/kernel...
Remote debugging using 127.0.0.1:26000
warning: No executable has been specified and target does not support
determining executable automatically.  Try using the "file" command.
0x0000fff0 in ?? ()
(gdb) b boot_alloc
Breakpoint 1 at 0xf0100b29: file kern/pmap.c, line 89.
(gdb) continue
Continuing.
The target architecture is assumed to be i386
=> 0xf0100b29 <boot_alloc>:	push   %ebp

Breakpoint 1, boot_alloc (n=4096) at kern/pmap.c:89
89	{
(gdb) finish
Run till exit from #0  boot_alloc (n=4096) at kern/pmap.c:89
=> 0xf01025e6 <mem_init+26>:	mov    %eax,0xf0118948
mem_init () at kern/pmap.c:140
140		kern_pgdir = (pde_t *) boot_alloc(PGSIZE);
Value returned is $1 = (void *) 0xf0119000
(gdb) 
```

Como se puede observar en la última linea:
`Value returned is $1 = (void *) 0xf0119000`
El valor que retorna coincide con el que calculamos anteriormente.


--------------

##  map_region_large

### ¿Cuánta memoria se ahorró de este modo? (en KiB)

 Haciendo uso de large pages, no se utiliza una segunda tabla para pasar de la dirección virtual a la física. Por lo tanto, cada large page que se usa permite ahorrar la memoria que ocupa una page table, la cual en este caso es 1024 (entradas en una page table) * 32 bits (memoria por entrada) = 4Kbytes. 

---
### ¿Es una cantidad fija, o depende de la memoria física de la computadora?
La memoria ahorrada por cada tabla no depende de la memoria física de la computadora, sino que depende de la arquitectura ya que ésta determina cuanto ocupa cada page table. En el caso de JOS, esta cantidad es fija ya que está determinada por la arquitectura que utiliza -> i386 (de 32 bits).