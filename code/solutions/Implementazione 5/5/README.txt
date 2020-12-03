<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< PROGRAMMA 5 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Quinta Implementazione Parallela del Quick-Sort.

L'idea di base di questa implementazione è stata assegnare ogni sotto-problema del Quick-Sort ad una serie di Work-Item.

La fase di partition avviene in parallelo, infatti ogni Work-Item conta quanti elementi vogliono scrivere alla sua sinistra (se è più piccolo o uguale al pivot) o alla sua destra (se è più grande del pivot) per conoscere la posizione "corretta" su cui scrivere il proprio elemento. 

Il miglioramento rispetto ai programmi precedenti è sostanziale dato che qua non c'è bisogno di ricopiare gli elementi corretti nell'array di input perchè i Work-Item "riescono a capire da soli" dove scrivere il valore senza smistare i valori in 2 array come i Programmi 4 e 5.

Come ai precedenti programmi, la fase di Partition viene attuate in 2 lanci di Kernel diversi finchè esiste un sottoproblema che ha una cardinalità più grande del Numero Massimo di Work-Item in un Work-Group per il device in esame, quando quest'ultima frase sarà falsa verrà effettuata la fase di partition e di riscrittura nello stesso lancio di kernel e quindi avremo 1 Work-Group per ogni sotto-problema.

Un ulteriore dettaglio tecnico, i Work-Item vengono mappati alla struttura dati di input attraverso un ulteriore array map che viene risistemato ad ogni lancio, ovviamente quando si passa ad utilizzare i Work-Group la mappatura smette di essere utilizzata.
