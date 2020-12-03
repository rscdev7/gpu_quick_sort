<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< PROGRAMMA 3 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Terza Implementazione Parallela del Quick-Sort.

La logica è simile al PROGRAMMA 2.

La differenza è che le fasi di Partition e Riscrittura vengono attuate in 2 lanci di Kernel diversi finchè esiste un sottoproblema che ha una cardinalità più grande del Numero Massimo di Work-Item in un Work-Group per il device in esame, quando quest'ultima frase sarà falsa verrà effettuata la fase di partition e di riscrittura nello stesso lancio di kernel e quindi avremo 1 Work-Group per ogni sotto-problema.
