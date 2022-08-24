TP4: Sistema de archivos e intérprete de comandos
=================================================

caché de bloques
----------------

### Se recomienda leer la función  `diskaddr()`  en el archivo  _fs/bc.c_. Responder:
-   ¿Qué es  `super->s_nblocks`?

`super->s_nblocks` es un atributo del struct Super (superblock del file system) y representa la cantidad total de bloques que se utilizan para representar a todo el disco.

 -   ¿Dónde y cómo se configura este bloque especial?
 Se configura en `fs/fsformat.c` en la función `opendisk()`. Se aloca la memoria necesitaria para el bloque y luego se setean sus atributos.
```
super->s_magic = FS_MAGIC; -> Numero mágico
super->s_nblocks = nblocks; -> Cantidad total de bloques
super->s_root.f_type = FTYPE_DIR; -> La raiz es de tipo directorio
strcpy(super->s_root.f_name, "/"); -> Nombre del directorio raiz

