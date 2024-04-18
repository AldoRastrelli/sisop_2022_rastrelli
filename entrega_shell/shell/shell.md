# Lab: shell

## Búsqueda en $PATH
### RESPONDER: ¿cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?

La syscall `execve(2)` y la familia `exec(3)` se diferencian en cómo se encuentra el programa, cómo se especifican los argumentos y de dónde proviene el entorno.

Por un lado, la syscall `execve` va a ejecutar el comando que recibe por parámetro, y lo hace pisando en la memoria el espacio del proceso que la llamó. Esto es, creando un nuevo heap, stack y data segments que van a reemplazar al proceso actual. Esta syscall cambia la imagen de un proceso (memoria virtual, entorno, argumento), pero mantiene todo lo demás (incluído el pid).

Existen un montón de familias o `*wrappers*` que difieren en los parámetros y la cantidad de parámetros: `execl, execlp, execle, execv, execvp, execvpe`. Esta es la familia de wrappers de `exec(3)`.

Para hacer una llamada a `exec`, necesitamos 1 parámetro (de función) y 1 lista (de cosas a aplicarle esa función). Todas las funciones de la familia `exec` llaman a la syscall `execve`, pero extienden su funcionalidad agrupando por sufijos.
Por ejemplo:

- Las llamadas con `v` en el nombre toman un VECTOR para especificar los argumentos del nuevo programa. El final de los argumentos se indica mediante un elemento de matriz que contiene NULL.
- Las llamadas con `l` en el nombre toman a los argumentos del nuevo programa como una lista de argumentos de longitud variable. El final de los argumentos se indica mediante un argumento (char *)NULL.
- Las llamadas con `e` en el nombre toman un argumento adicional (o argumentos en el caso de `l`) para proporcionar el entorno del nuevo programa; de lo contrario, el programa hereda el entorno del proceso actual.
- Las llamadas con `p` en el nombre buscan la variable de entorno PATH para encontrar el programa si no tiene un directorio (es decir, no contiene un carácter `/`). De lo contrario, el nombre del programa siempre se trata como una ruta al ejecutable.

### RESPONDER: ¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?
La llamada `exec(3)` puede fallar, y, en esos casos, se devuelve un número negativo (-1) y un `errno` para indicar el error.

Según el manual de linux, se indica que:

- Si se deniega el permiso para un archivo (el intento execve(2) falló con el error EACCES), estas funciones continuarán buscando el resto de la ruta de búsqueda. Si no hay otro archivo encontrados, sin embargo, regresarán con errno establecido en EACCES.
- Si no se reconoce el encabezado de un archivo (el intento execve(2) falló con el error ENOEXEC), estas funciones ejecutarán el shell (/bin/sh) con la ruta del archivo como primer argumento (Si este intento falla, no se realiza más búsqueda).

Todas las demás funciones exec() (que no incluyen 'p' en el sufijo) toman como primer argumento (relativo o absoluto) una ruta que identifica el programa a ejecutar.

---

## Comandos built-in
### RESPONDER: ¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como truey false)

Los built-ins nos permiten ejecutar o realizar acciones sin necesidad de crear un proceso por separado. Si `cd` no se realizara en el mismo proceso donde la shell se está ejecutando, el directorio actual se cambiaría en el hijo y no en el padre, por lo que al volver del proceso hijo, seguiríamos en el mismo directorio. Como debe modificar la shell para crear el cambio de directorio, no tendría sentido implementarlo sin ser built-in.

En cambio, `pwd` en todo caso sí podría llegar a implementarse sin necesidad de ser built-in, porque podría imprimir el directorio actual desde un proceso hijo y luego terminar dicho proceso y volver al proceso padre, con el que comparte directorio; por lo cual, el comportamiento sería el esperado.

Implementar estas llamadas como built-in nos permite tener una ejecución mucho más rápida, pudiendo hacerlo sobre nuestro proceso actual y no necesitando crear un proceso hijo (que podría, además, surgir un error y no poder crearse) para su ejecución.

---

## Variables de entorno adicionales
### RESPONDER: ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?
Las variables de entorno deben estar siempre en el entorno de la función que está siendo ejecutada. Si la variable se seteara desde la shell, se estaría cambiando el entorno global de la misma y; por lo tanto, no serían adicionales sino permanentes.

### `**RESPONDER:**` En algunos de los *wrappers* de la familia de funciones de `exec(3)`(las que finalizan con la letra *`e`*), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar `setenv(3)` por cada una de las variables, se guardan en un array y se lo coloca en el tercer argumento de una de las funciones de `exec(3)`. - ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué. - Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

Como mencionamos más arriba, las llamadas con `e` en el nombre toman un argumento adicional (o argumentos en el caso de `l`) para proporcionar el entorno del nuevo programa; de lo contrario, el programa hereda el entorno del proceso actual. En estos casos, el entorno de la nueva imagen del proceso se especifica mediante el argumento `envp`. El argumento envp es un array de `char *` terminados en null y que debe terminar con un puntero nulo.

Con esa información, podríamos decir que ambos casos tienen un comportamiento similar.

Sin embargo, la documentación del manual de linux define que el programa que se quiera ejecutar con estas funciones de la familia `exec` sólo será buscado en el PATH definido por el entorno llamante y no por el envp pasado pro argumento.
Entonces, a priori, el comportamiento resultante no es el mismo que haciendo un `fork()` y luego un `setenv()`.

Una forma de solucionar esta diferencia que sería, por ejemplo, enviar como argumento a todas las variables del entorno, tanto las de la shell como las adicionales -temporales-. De esta forma el comportamiento resultante sería análogo entre los casos.
---

## Procesos en segundo plano
### RESPONDER: Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.
1. Creamos un proceso, ejecutando el programa con `exec`.
2. Imprimimos el back_info.
3. waitpid va a mantener registro del proceso para cuando vuelva.
4. Al ser no bloqueante (con WNOHANG, que indica que el proceso no debe quedar bloqueado esperando a que termine), el proceso padre vuelve y termina su ejecucion. 
5. El segundo plano que termina luego queda zombie.


---

## Flujo estándar
### RESPONDER: Investigar el significado de 2>&1, explicar cómo funciona su forma general y mostrar qué sucede con la salida de cat out.txt en el ejemplo. Luego repetirlo invertiendo el orden de las redirecciones. ¿Cambió algo?
El número `1` representa a la salida estándar `stdout` y el número `2` representa a la salida de errores `stderr`.

Lo que se está haciendo es redireccionar la salida de error stderr (file descriptor 2) a la memoria del file descriptor 1, que es el file descriptor de la salida estandar stdout.

Ejemplo cat out.txt: Por un lado, se está redirigiendo la salida del file descriptor 1 al archivo out.txt. Por el otro, se está redirigiendo la salida de stderr (fd 2) a la salida estándar stdout (fd 1). 
al hacer ls de un directorio que no existe, se arroja un error: Se redirige entonces el fd2 al fd1 → El fd1 lo redirige al archivo out.txt.
Al hacer cat out.txt, se imprime el error que inicialmente se recibió en stderr.

Invirtiendo el orden (`ls -C /home /noexiste >out.txt 1>&2`), se observa:

- ls va a intentar ingresar a un directorio que no existe, pero como se está redirigiendo el stdout al stderror, el archivo nunca guarda contenido y queda por lo tanto vacío. Al hacer cat out.txt, no vamos a ver que imprima nada en pantalla.
- Sin embargo, el stderr sí recibe contenido, que se ve en pantalla al ejecutar la línea principal.



---

## Tuberías simples (pipes)
### RESPONDER: Investigar qué ocurre con el exit code reportado por la shell  si se ejecuta un pipe ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con la implementación del este lab.

La shell reporta el exit code del último comando ejecutado en el pipe y, si existiese un error o fallo en el primer comando del pipe, se devuelve el error de todas las ejecuciones fallidas.

Por ejemplo:

```c
// Ejemplo donde no funcionan ambos comandos
$ ls /noexiste | cat text
ls: cat: text: No such file or directory
cannot access '/noexiste': No such file or directory

// Ejemplo donde no funcionan ambos comandos
$ seq B > | cat noexiste
cat: seq: noexiste: No such file or directory
invalid floating point argument 'B'

// Ejemplo donde funciona un sólo comando
$ seq B > | echo hello
hello
invalid floating point argument 'B'

// Ejemplo donde funciona un sólo comando
$ echo hello > | cat noexiste
cat: noexiste: No such file or directory
// En este caso, no vemos ningún stdout, ya que la salida del primer programa se manda al cat; pero como este falla, se muestra sólamente el error.
```

---

## Pseudo-variables
### RESPONDER: Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar).

`$$` devuelve el pid actual del proceso

```c
aldo@aldo-VirtualBox:~/Documents/Aldana/Sisop/sisop_2022b_cardoso/shell$ echo $$
13103
```

`$_` devuelve el último argumento del último comando que se ejecutó

```c
aldo@aldo-VirtualBox:~/Documents/Aldana/Sisop/sisop_2022b_cardoso/shell$ echo hi how are you
hi how are you
aldo@aldo-VirtualBox:~/Documents/Aldana/Sisop/sisop_2022b_cardoso/shell$ echo $_
you
```

`$!` devuelve el id del último proceso que se ejecutó en segundo plan

```c
aldo@aldo-VirtualBox:~/Documents/Aldana/Sisop/sisop_2022b_cardoso/shell$ sleep 3 &
aldo@aldo-VirtualBox:~/Documents/Aldana/Sisop/sisop_2022b_cardoso/shell$ echo $!
37425
[1]+  Done                    sleep 3
```

---


