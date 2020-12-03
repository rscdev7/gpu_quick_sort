<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< PROGRAMMA 8 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Ottava Implementazione Parallela del Quick-Sort.

In questa implementazione si è sfruttato un aprroccio Sliding Window.

In sostanza vengono lanciati tanti Work-Group quanti sono i sottoproblemi da risolvere.

Ogni Work-Group computa progressivamente il sottoproblema, la logica di computazione è simile al PROGRAMMA 1; l'unica differenza è che il calcolo degli scambi viene fatto in parallelo.
