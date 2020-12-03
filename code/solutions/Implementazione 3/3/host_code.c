#include "./lib/ocl_boiler.h"
#include "./lib/host_assets.h"

//Variabile che contiene il numero massimo di Work-Item in un Work-Group per il device
size_t max_wg_size;

//Questo flag regola lo spezzamento del kernel
int flag = 0;

//Dimensione massima sotto-problema ad uno specifico livello di ricorsione dell'Albero
int max_sott_size = -1;

//Funzione che lancia il Kernel "parallel_quick"
cl_event split_f( 
			  cl_command_queue que, cl_kernel split, 

			  cl_mem input, 
			  
			  cl_mem smaller_pivot, cl_mem greater_pivot, 
			  
			  cl_mem delimiters, cl_mem map,

			  int n_wait_events, cl_event* wait_events, 

			  cl_int global_ws

			  ) {

	//Variabile d'errore per le chiamate OpenCL
	cl_int err;

	//Variabile che registra l'evento della chiamata del kernel 
	cl_event evt_split;

	//Inserisco gli argomenti da passare al kernel
	err = clSetKernelArg(split, 0, sizeof(input), &input);
	ocl_check(err, "set arg 0");

	err = clSetKernelArg(split, 1, sizeof(smaller_pivot), &smaller_pivot);
	ocl_check(err, "set arg 1");

	err = clSetKernelArg(split, 2, sizeof(greater_pivot), &greater_pivot);
	ocl_check(err, "set arg 2");

	err = clSetKernelArg(split, 3, sizeof(delimiters), &delimiters);
	ocl_check(err, "set arg 3");

	err = clSetKernelArg(split, 4, sizeof(map), &map);
	ocl_check(err, "set arg 4");

	//Calcolo il Global Work Size
	const size_t gws[] = { global_ws };

	//Lancio del Kernel
	err = clEnqueueNDRangeKernel(que, split, 1, NULL, gws, NULL, n_wait_events, wait_events, &evt_split);
	ocl_check(err, "enqueue split");

	//Restituisco l'evento openCL associato al lancio del kernel
	return evt_split;
}

//Funzione che lancia il Kernel "fill"
cl_event fill_f( 
			  cl_command_queue que, cl_kernel fill, 

			  cl_mem input, 
			  
			  cl_mem smaller_pivot, cl_mem greater_pivot, 
			  
			  cl_mem delimiters, cl_mem app,

			  int n_wait_events, cl_event* wait_events, 

			  cl_int global_ws

			  ) {

	//Variabile d'errore per le chiamate OpenCL
	cl_int err;

	//Variabile che registra l'evento della chiamata del kernel 
	cl_event evt_fill;

	//Inserisco gli argomenti da passare al kernel
	err = clSetKernelArg(fill, 0, sizeof(input), &input);
	ocl_check(err, "set arg 0");

	err = clSetKernelArg(fill, 1, sizeof(smaller_pivot), &smaller_pivot);
	ocl_check(err, "set arg 1");

	err = clSetKernelArg(fill, 2, sizeof(greater_pivot), &greater_pivot);
	ocl_check(err, "set arg 2");

	err = clSetKernelArg(fill, 3, sizeof(delimiters), &delimiters);
	ocl_check(err, "set arg 3");

	err = clSetKernelArg(fill, 4, sizeof(app), &app);
	ocl_check(err, "set arg 4");

	//Calcolo il Global Work Size
	const size_t gws[] = { global_ws };

	//Lancio del Kernel
	err = clEnqueueNDRangeKernel(que, fill, 1, NULL, gws, NULL, n_wait_events, wait_events, &evt_fill);
	ocl_check(err, "enqueue fill");

	//Restituisco l'evento openCL associato al lancio del kernel
	return evt_fill;
}

//Funzione che lancia il Kernel "parallel_quick"
cl_event sort( 
			  cl_command_queue que, cl_kernel parallel_quick, 

			  cl_mem input, cl_mem smaller_pivot, cl_mem greater_pivot, 
			  
			  cl_mem delimiters, cl_mem app, 

			  int n_wait_events, cl_event* wait_events, 

			  cl_int global_ws, cl_int local_ws

			  ) {

	//Variabile d'errore per le chiamate OpenCL
	cl_int err;

	//Variabile che registra l'evento della chiamata del kernel 
	cl_event evt_sort;

	//Inserisco gli argomenti da passare al kernel
	err = clSetKernelArg(parallel_quick, 0, sizeof(input), &input);
	ocl_check(err, "set arg 0");

	err = clSetKernelArg(parallel_quick, 1, sizeof(smaller_pivot), &smaller_pivot);
	ocl_check(err, "set arg 1");

	err = clSetKernelArg(parallel_quick, 2, sizeof(greater_pivot), &greater_pivot);
	ocl_check(err, "set arg 2");

	err = clSetKernelArg(parallel_quick, 3, sizeof(delimiters), &delimiters);
	ocl_check(err, "set arg 3");

	err = clSetKernelArg(parallel_quick, 4, sizeof(app), &app);
	ocl_check(err, "set arg 4");

	//Calcolo il Global Work Size
	const size_t gws[] = { global_ws };

	const size_t lws[] = { local_ws };

	//Lancio del Kernel
	err = clEnqueueNDRangeKernel(que, parallel_quick, 1, NULL, gws, lws, n_wait_events, wait_events, &evt_sort);
	ocl_check(err, "enqueue parallel_quick");

	//Restituisco l'evento openCL associato al lancio del kernel
	return evt_sort;
}

//Funzione che calcola quanti sottoproblemi abbiamo 
//al prossimo livello dell'Albero di Ricorsione
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

	return (counter/2); //Sarà il numero di Sottoproblemi nella prossima iterazione

}

//Funzione che calcola quanti Work-Item lanciare al prossimo livello dell'Albero di   Ricorsione
//Inoltre, questa funzione ,se necessario, va a riempire la mappa che serve alla griglia di lancio
//per capire dove sono allocati i dati
int find_next_gws (cl_command_queue que, cl_mem delimiters, int dim, cl_mem map) {

	//Variabile che rappresenta il numero di Work-Item da lanciare alla prossima iterazione
	int gws = 0;

	//Variabile che permette o meno il riempimento della mappa
	int l_flag=0;

	//Spazio in memoria occupato dell'array delimiters
	int mem = sizeof (int) * dim;

	//Allocazione array su host, è in questi array che verranno copiati i buffer del device
	//Inoltre, in questi array verranno effettuate le elaborazioni
	cl_int* h_delimiters = malloc (mem);
	cl_int err;

	//Variabile che scorre i sotto-problemi nell'array delimiters
	int idx = 0;

	//Variabile che rappresenta l'indirizzo del dato appartenente al Work-Item 
	//in esame
	int inc = 0;

	//Variabile che permette di assegnare il dato al singolo Work-Item
	int c = 0;

	//Leggo il buffer delimiters 
	err=clEnqueueReadBuffer(que,delimiters,CL_TRUE,0,mem,h_delimiters,0,NULL,NULL);
	ocl_check(err, "Lettura buffer delimiters");

	//Imposto max_sott_size alla dimensionalità del primo sotto-problema
	max_sott_size = ( h_delimiters[1] - h_delimiters[0] ) + 1;

	//Ciclo sul numero di sotto-problemi che avremo alla prossima iterazione
	for (int z=0; z < dim-1; z+=2) {

		//Calcolo dimensione del sotto-problema corrente
		int current_sott_size = ( h_delimiters[z+1] - h_delimiters[z] ) + 1; 
		gws += current_sott_size;

		//Se scopro che esiste un sotto-problema che è più grande del 
		//Numero massimo di Work-Item che puù contenere il device corrente
		//Segue che setto la variabile l_flag a 1
		//Altrimenti cerco di calcolarmi la dimensionalità del 
		//sotto-problema più grande del prossimo lancio
		if ( current_sott_size > max_wg_size ) {
			l_flag = 1;
		}else if (max_sott_size < current_sott_size) {
			max_sott_size = current_sott_size;
		}
	
	}

	//Se tutti i sottoproblemi hanno una dimensionalità minore o uguale al numero massimo
	//di Work-Item che può contenere un Work-Group lo segnalo mettendo a 1 la variabile flag
	if (l_flag == 0) flag=1;

	//Se l'if di sopra NON è andato a buon fine, mi calcolo la mappa come al solito
	if (flag == 0) {

		//Spazio in memoria occupato dalla Map e allocazione di quest'ultima
		int read_dim = gws * sizeof(int);
		cl_int* h_map = malloc (read_dim);	

		//Ciclo sul numero di Work-Item da lanciare alla prossima iterazione
		for (int z=0; z<gws;z++) {

			//Indirizzo del dato per il Work-Item z-esimo
			inc = h_delimiters[idx]+c;

			//Se l'indirizzo è fuori dal sotto-problema corrente, passo al sotto-problema successivo
			if (inc > h_delimiters[idx+1]) {
				idx+=2;
				c=0;
				inc = h_delimiters[idx]+c;
			}

			//Scrittura dell'informazione e incremento di c
			h_map[z] = inc;
			c++;

		}

		//Scrittura della Mappa
		err = clEnqueueWriteBuffer(que,map,CL_TRUE,0,read_dim,h_map,0,NULL,NULL);
		ocl_check(err, "Scrittura vettore map per la prossima iterazione");
	
	}

	//Sarà il numero di Work-Item da lanciare alla prossima iterazione
	return gws;
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
	
	//Creazione kernel split
	cl_kernel split = clCreateKernel(prog, "split", &err);
	ocl_check(err, "split");

	//Creazione kernel fill
	cl_kernel fill = clCreateKernel(prog, "fill", &err);
	ocl_check(err, "fill");

	//Creazione kernel parallel_quick
	cl_kernel parallel_q = clCreateKernel(prog, "parallel_quick", &err);
	ocl_check(err, "parallel_q");

	/* Prelievo del Numero Massimo di Work-Item in un Work-Group */
	err = clGetKernelWorkGroupInfo(split, d, CL_KERNEL_WORK_GROUP_SIZE, sizeof(max_wg_size), &max_wg_size, NULL);
	ocl_check(err, "get max wg size info");




	//FASE DI CALCOLO DELLO SPAZIO DA ALLOCARE PER LE STRUTTURE DATI

	//Numero di sotto-problemi da risolvere attualmente
	int n_sott = 1;

	//Numero Iterazioni
	int n_iterazioni=0;

	//Variabili di Profiling
	double rt;
	double ne;

	//Evento kernel split
	cl_event sp;

	//Prelievo dimensione vettore
	const int numels=atoi(argv[1]);

	//Spazio in memoria occupato dal Dataset
	const size_t memsize = numels*sizeof(int);

	//Altezza Albero di Ricorsione
	const int h = log(numels)/log(2);

	//Cardinalità insieme delimiters
	const int delimiters_len = 2*pow(2,h);

	//Border memsize
	const size_t delimiters_memsize=delimiters_len*sizeof(int);




	//FASE DI ALLOCAZIONE DELLE STRUTTURE DATI SU HOST

	//Vettore di eventi
	cl_event* ev = malloc (MAX_EVT*sizeof(cl_event));

	//Creo Vettore che indicizzerà gli indici dei Work-Item
	cl_int* h_map = malloc (memsize);
	for (int z=0;z<numels;z++) {
		h_map[z] = z;
	}

	//Riempimento oggetto che rappresenta gli array smaller e greater pivot
	cl_int * p_vecs = malloc (memsize);
	for (int i=0;i<numels;i++) {
		p_vecs[i] = -1;
	}

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

	//Vettore su cui verrà effettuata la verifica del calcolo del device
	cl_int* vet = malloc (memsize);
	



	//FASE DI ALLOCAZIONE DEI BUFFER SU DEVICE

	//Creazione Buffer atto a contenere il Dataset
	cl_mem d_v1 = clCreateBuffer (ctx,CL_MEM_READ_WRITE,memsize,NULL,&err);
	ocl_check(err, "Creazione buffer di input");

	//Scrittura del Dataset sul Buffer
	err = clEnqueueWriteBuffer(que,d_v1,CL_TRUE,0,memsize,input,0,NULL,NULL);
	ocl_check(err, "Inizializazione Dataset");

	//Creazione Buffer Map
	cl_mem map = clCreateBuffer (ctx,CL_MEM_READ_WRITE,memsize,NULL,&err);
	ocl_check(err, "creazione buffer Map");

	//Scrittura dei valori di default di Map
	err = clEnqueueWriteBuffer(que,map,CL_TRUE,0,memsize,h_map,0,NULL,NULL);
	ocl_check(err, "Inizializazione buffer Map");

	//Creazione Buffer atto a contenere gli elementi piu piccoli del pivot
	cl_mem smaller_pivot = clCreateBuffer (ctx,CL_MEM_READ_WRITE,memsize,NULL,&err);
	ocl_check(err, "Creazione buffer smaller_pivot");

	//Scrittura stato smaller pivot
	err = clEnqueueWriteBuffer(que,smaller_pivot,CL_TRUE,0,memsize,p_vecs,0,NULL,NULL);
	ocl_check(err, "Inizializazione Smaller Pivot");

	//Creazione Buffer atto a contenere gli elementi piu grandi del pivot
	cl_mem greater_pivot = clCreateBuffer (ctx,CL_MEM_READ_WRITE,memsize,NULL,&err);
	ocl_check(err, "Creazione buffer smaller_pivot");

	//Scrittura stato smaller pivot
	err = clEnqueueWriteBuffer(que,greater_pivot,CL_TRUE,0,memsize,p_vecs,0,NULL,NULL);
	ocl_check(err, "Inizializazione Smaller Pivot");

	//Creazione Buffer per i delimiters
	cl_mem delimiters = clCreateBuffer (ctx,CL_MEM_READ_WRITE,delimiters_memsize,NULL,&err);
	ocl_check(err, "creazione buffer start");

	//Scrittura dei primi delimiters
	err = clEnqueueWriteBuffer(que,delimiters,CL_TRUE,0,delimiters_memsize,empty_borders,0,NULL,NULL);
	ocl_check(err, "Inizializazione buffer start");

	//Creazione Buffer di appogio
	cl_mem app = clCreateBuffer (ctx,CL_MEM_READ_WRITE,delimiters_memsize,NULL,&err);
	ocl_check(err, "creazione buffer app");

	//Scrittura sul Buffer di appoggio
	err = clEnqueueWriteBuffer(que,app,CL_TRUE,0,delimiters_memsize,empty_borders,0,NULL,NULL);
	ocl_check(err, "Inizializazione buffer app");




	//FASE OPERATIVA DEL PROGRAMMA, QUA VIENE LANCIATO IL KERNEL

	//N Work-Item da Lanciare
	int gws = find_next_gws (que,delimiters,2*(n_sott),map);

	while (n_sott>0) {

		//Se "posso", lancio un solo kernel anzichè due
		if (flag == 1) {
			ev[n_iterazioni]= sort (que, parallel_q, d_v1, smaller_pivot, greater_pivot, delimiters, app, 0, NULL,(n_sott*max_sott_size), max_sott_size);

			//Attendo che venga eseguita tutta la coda di comandi
			err = clFinish(que);
			ocl_check(err, "clFinish");

		}else{

			//Lancio kernel split
			sp= split_f(que, split, d_v1, smaller_pivot, greater_pivot, delimiters, map, 0, NULL, gws);

			//Attendo che venga eseguita tutta la coda di comandi
			err = clFinish(que);
			ocl_check(err, "clFinish");

			ev[n_iterazioni]= fill_f (que, fill, d_v1, smaller_pivot, greater_pivot, delimiters, app, 1, &sp, n_sott);

			//Attendo che venga eseguita tutta la coda di comandi
			err = clFinish(que);
			ocl_check(err, "clFinish");

		}

		//Incremento la variabile che conta le iterazioni
		n_iterazioni++;

		//Stampo il tempo di esecuzione
		printf ("Sort %d \t\t\t\tRuntime: %gms \n",n_iterazioni, runtime_ms(ev[n_iterazioni-1]));

		//Calcolo quanti sotto problemi avrò alla prossima iterazione
		n_sott = clean_vector (delimiters,2*(n_sott),app,que);
		
		//Calcolo quanti Work-Item lanciare alla prossima iterazione
		gws = find_next_gws (que,delimiters,2*(n_sott),map);


	}




	//STAMPA DEL TEMPO DI ESECUZIONE TOTALE DELL'ALGORITMO
	rt = total_runtime_ms(ev[0],ev[n_iterazioni-1]);
	ne = numels/rt;
	printf ("\nTotal Runtime: %gms \t Computed Elements: %g GE/s \n",rt,ne);

	

	//FASE DI VERIFICA

	//Leggo il vettore ordinato
	err=clEnqueueReadBuffer(que,d_v1,CL_TRUE,0,memsize,vet,0,NULL,NULL);
	ocl_check(err, "Lettura vettore ordinato");

	//Verifico la correttezza del risultato
	verify(vet,numels);





	//LIBERO LE RISORSE ALLOCATE
	clReleaseMemObject(d_v1);
	clReleaseMemObject(app);
	clReleaseMemObject(delimiters);
	clReleaseMemObject(smaller_pivot);
	clReleaseMemObject(greater_pivot);
	clReleaseMemObject(map);
	clReleaseKernel(parallel_q);
	clReleaseKernel(split);
	clReleaseKernel(fill);
	clReleaseProgram(prog);
	clReleaseCommandQueue(que);
	clReleaseContext(ctx);
	free(vet);
	free(ev);




	//Uscita dal programma
	return 0;




}
