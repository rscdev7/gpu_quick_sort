<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< PROGRAMMA 9 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Nona Implementazione Parallela del Quick-Sort.

L'idea di base di questa implementazione è stata assegnare ogni sotto-problema del Quick-Sort ad una serie di Work-Item.

Infatti la partition avviene questa volta in parallelo, cioè ogni Work-Item scrive il suo valore nell'array "smaller_pivot" se il suo valore è più piccolo o uguale al pivot oppure nell'array "greater_pivot" se il suo valore è più grande del pivot.

Detto ciò, la riscrittura dei valori corretti sull'array di Input viene effettuata in Parallelo sfruttando un approccio di tipo Sliding-Window.

Nello specifico, gli array "smaller e greater_pivot" vengono "spazzolati" e tutti i valori "consistenti" vengono scritti nel giusto ordine nell'array di Input.

Al livello di lancio di kernel, vengono lanciati tanti Work-Group (di dimensione uguale alla Dimensione della Finestra che passerà l'utente in input) quanti sono i sottoproblemi ancora da risolvere.

Data l'impossibilità di sincronizzare tutta la griglia di lancio, Partition e Riscrittura vengono sempre attuate in 2 lanci di Kernel diversi.

Inoltre, i Work-Item vengono mappati alla struttura dati di input attraverso un ulteriore array map che viene risistemato ad ogni lancio.
