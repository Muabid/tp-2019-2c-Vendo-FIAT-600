# sac-tools
Como las de la AFIP, pero para un FileSystem :)

## Uso

Crear un archivo para disco binario.

dd if=/dev/urandom of=disco.bin bs=1024 count=10240

** Modificar "count" para la cantidad de kilobytes necesarios.

Formatear el disco binario

./sac-format disco.bin

Hacer un dump de sus estructuras internas.

./sac-dump disco.bin


sac_mkdir(0,"/hola");
	sac_mknod(0,"/chau");
	sac_mknod(1,"/hola/holis");
	sac_mknod(1,"/xd");
	sac_readdir(0,"/dir",0);
	sac_getattr(0,"xd");