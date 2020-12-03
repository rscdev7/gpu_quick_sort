<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< PROGRAMMA 1 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Prima Implementazione Parallela del Quick-Sort.

L'idea di base di questa implementazione è stata assegnare ogni sotto-problema del Quick-Sort ad un Work-Item diverso.

In ogni caso, l'esecuzione del Quick-Sort all'interno del sottoproblema resta seriale come il Quick-Sort Classico.

In sostanza si sta guadagnando solamente un parallelismo al livello di sotto-problema.
