.ORIG x3000 ;Esta es la dirreccion de memoria que sera cargada
LEA  R0, HELLO_STR ; Aca pongo en memoria la cadena  HELLO_STR, es decir, meto esto "Hello world", en memoria
PUTs ;Ahora una vez puesto,lo escupo, es decir muestro la cadena montada en R0 en la consola
HATL ; Detengo el programa
HELLO_STR .STRINGZ "Hello world" ; Almacena el "Hello world" en el programa
.END