#include "./lib/ocl_boiler.h"
#include "./lib/host_assets.h"


//Funzione che lancia il Kernel "parallel_quick"
cl_event sort( 
			  cl_command_queue que, cl_kernel parallel_quick, 

			  cl_mem input, cl_mem delimiters, cl_mem app,

			  int n_wait_events, cl_event* wait_events, 

			  cl_int global_ws 

			  ) {

	//Variabili per la registrazione degli errori alle chiamate OpenCL e degli eventi
	cl_int err;
	cl_event evt_sort;

	//Inserisco gli argomenti da passare al kernel
	err = clSetKernelArg(parallel_quick, 0, sizeof(input), &input);
	ocl_check(err, "set arg 0");

	err = clSetKernelArg(parallel_quick, 1, sizeof(delimiters), &delimiters);
	ocl_check(err, "set arg 1");

	err = clSetKernelArg(parallel_quick, 2, sizeof(app), &app);
	ocl_check(err, "set arg 2");

	//Calcolo il Global Work Size
	const size_t gws[] = { global_ws };

	//Lancio del Kernel
	err = clEnqueueNDRangeKernel(que, parallel_quick, 1, NULL, gws, NULL, n_wait_events, wait_events, &evt_sort);
	ocl_check(err, "enqueue parallel_quick");

	//Restituisco l'evento openCL associato al lancio del kernel
	return evt_sort;
}


//Funzione che decide quanti Work-Item lanciare al prossimo livello dell'Albero di Ricorsione
//Questa funzione permetta anche il compattamento dell'array delimiters, questo perchè
//quest'ultimo potrebbe avere "dei buchi"
int clean_vector (cl_mem delimiters, int size, cl_mem app, cl_command_queue que) {

	//Dimensione massima che potrebbe avere l'array delimiters al livello di ricorsione
	//corrente
	int mem = (2*size)*sizeof(int);

	//Questa variabile serve a scorrere l'array delimiters durante il ciclo sottostante
	int counter = 0;

	//Questa Variabile serve a scorrere l'array app durante il ciclo sottostante
	int c =0;

	//Variabile che conterrà i codici d'errore alle chiamate OpenCL
	cl_int err;
	
	//Allocazione array su host, è in questi array che verranno copiati i buffer del device
	//Inoltre, in questi array verranno effettuate le elaborazioni con l'obbiettivo finale
	//di scrivere sui buffer device gli aggiornamenti
	cl_int* h_delimiters = malloc (mem);
	cl_int* h_app = malloc (mem); 

	//Leggo il buffer delimiters - Sarà il buffer da modificare
	err=clEnqueueReadBuffer(que,delimiters,CL_TRUE,0,mem,h_delimiters,0,NULL,NULL);
	ocl_check(err, "Lettura buffer delimiters");

	//Leggo il buffer app - Sarà il buffer da leggere
	err=clEnqueueReadBuffer(que,app,CL_TRUE,0,mem,h_app,0,NULL,NULL);
	ocl_check(err, "Lettura buffer app");
	
	//Ciclo sul numero massimo di sotto-problemi possibili al livello di ricorsione corrente
	for (int i=0;i<size;i++) {

		//Fase di compattazione
		if (h_app [c] != (-1) && h_app[c+1] != (-1)) {

			h_delimiters[counter] = h_app[c];
			counter++;
			c++;

			h_delimiters[counter] = h_app[c];
			counter++;
			c++;
			
		}else{
			c+=2;
		}
	}

	//Se ho zero sotto-problemi da risolvere, esco
	if (counter==0) {
		return 0;
	}

	//Scrivo le modifiche sul buffer delimiters
	err = clEnqueueWriteBuffer(que,delimiters,CL_TRUE,0,counter*sizeof(int),h_delimiters,0,NULL,NULL);
	ocl_check(err, "Scrittura vettore delimiters per la prossima iterazione");

	//Libero le risorse allocate
	free (h_delimiters);
	free(h_app);

	//Sarà il numero di Work-Item da lanciare nella prossima iterazione
	//Contestualmente, questo valore rappresenta il numero di sotto-problemi da risolvere al
	//prossimo livello dell'albero di ricorsione
	return (counter/2); 

}


int main (int argc, char *argv[]) {

	//Controllo Parametri in input
	if (argc != 2) {
		error("Errore sintassi:\n\n Bisogna inserire: \n\n 1) La dimensione dell'Array da ordinare \n");
	}




	//FASE DI COMPILAZIONE DEL PROGRAMMA DEVICE, CREAZIONE KERNEL E SCELTA DEL DEVICE

	//Scelta Device e Compilazione Programma Device
	cl_platform_id p = select_platform();
	cl_device_id d = select_device(p);
	cl_context ctx = create_context(p, d);
	cl_command_queue que = create_queue(ctx, d);
	cl_program prog = create_program("device_code.ocl", ctx, d);
	
	//Variabile d'errore per le chiamate OpenCL
	cl_int err;
	
	//Creazione kernel parallel_quick
	cl_kernel parallel_quick = clCreateKernel(prog, "parallel_quick", &err);
	ocl_check(err, "parallel_quick");

	


	//FASE DI CALCOLO DELLO SPAZIO DA ALLOCARE PER LE STRUTTURE DATI

	//Prelievo dimensione vettore
	const int numels=atoi(argv[1]);

	//Spazio in memoria occupato dall'Input
	const size_t memsize = numels*sizeof(int);
	
	//Altezza Albero di Ricorsione
	const int h = log(numels)/log(2);

	//Cardinalità insieme delimiters
	const int delimiters_len = 2*pow(2,h);

	//Border memsize
	const size_t delimiters_memsize=delimiters_len*sizeof(int);




	//FASE DI ALLOCAZIONE DELLE STRUTTURE DATI SU HOST

	//Oggetto che rappresenta lo stato iniziale dell'ogetto delimiters
	cl_int* empty_borders = malloc (delimiters_memsize);

	//Riempimento oggetto empty_borders
	empty_borders[0] = 0;
	empty_borders[1] = numels-1;
	for (int i=2;i<delimiters_len;i++) {
		empty_borders[i]=-1;
	}

	//Alloco e Inizializzo vettore di input con dei numeri pseudo-casuali da 0 a numels
	cl_int* input = malloc (memsize);
	init(input,numels);

	//Vettore di eventi
	cl_event* ev = malloc (MAX_EVT*sizeof(cl_event));

	//Vettore su cui verrà effettuata la verifica del calcolo del device
	cl_int* vet = malloc (memsize);

	//Variabili che regolano il ciclo
	int gws = 1;
	int c=0;

	//Variabili di Profiling
	double rt;
	double ne;



	
	//FASE DI ALLOCAZIONE DEI BUFFER SU DEVICE

	//Creazione Buffer atto a contenere il Dataset
	cl_mem d_v1 = clCreateBuffer (ctx,CL_MEM_READ_WRITE,memsize,NULL,&err);
	ocl_check(err, "Creazione buffer di input");

	//Scrittura del Dataset sul Buffer
	err = clEnqueueWriteBuffer(que,d_v1,CL_TRUE,0,memsize,input,0,NULL,NULL);
	ocl_check(err, "Inizializazione Dataset");

	//Creazione Buffer per i delimiters
	cl_mem delimiters = clCreateBuffer (ctx,CL_MEM_READ_WRITE,delimiters_memsize,NULL,&err);
	ocl_check(err, "creazione buffer delimiters");

	//Scrittura dei primi delimiters
	err = clEnqueueWriteBuffer(que,delimiters,CL_TRUE,0,delimiters_memsize,empty_borders,0,NULL,NULL);
	ocl_check(err, "Inizializazione buffer delimiters");

	//Creazione Buffer di appogio
	cl_mem app = clCreateBuffer (ctx,CL_MEM_READ_WRITE,delimiters_memsize,NULL,&err);
	ocl_check(err, "creazione buffer app");


	

	//FASE OPERATIVA DEL PROGRAMMA, QUA VIENE LANCIATO IL KERNEL

	while (gws>0) {

		//Lancio kernel parallel_quick
		ev[c] = sort(que, parallel_quick,d_v1,delimiters,app,0,NULL,gws);

		//Attendo che venga eseguita tutta la coda di comandi
		err = clFinish(que);
		ocl_check(err, "clFinish");

		//Stampo il tempo di esecuzione
		printf ("Sort %d \t\t\t\tRuntime: %gms \n",c, runtime_ms(ev[c]));

		//Incremento la variabile che conta le iterazioni
		c++;

		//Calcolo quanti Work-Item lanciare alla prossima iterazione
		gws = clean_vector (delimiters,2*gws,app,que);

	}




	//STAMPA DEL TEMPO DI ESECUZIONE TOTALE DELL'ALGORITMO
	rt = total_runtime_ms(ev[0],ev[c-1]);
	ne = numels/rt;
	printf ("\n Total Runtime: %gms \t Computed Elements: %g GE/s \n",rt,ne);




	//FASE DI VERIFICA

	//Leggo il vettore ordinato
	err=clEnqueueReadBuffer(que,d_v1,CL_TRUE,0,memsize,vet,0,NULL,NULL);
	ocl_check(err, "Lettura vettore ordinato");

	//Verifico la correttezza del risultato
	verify(vet,numels);




	//LIBERO LE RISORSE ALLOCATE
	free(vet);
	free(ev);
	clReleaseMemObject(d_v1);
	clReleaseMemObject(app);
	clReleaseMemObject(delimiters);
	clReleaseKernel(parallel_quick);
	clReleaseProgram(prog);
	clReleaseCommandQueue(que);
	clReleaseContext(ctx);




	//Uscita dal programma
	return 0;
}
