#include "./lib/ocl_boiler.h"
#include "./lib/host_assets.h"


//Funzione che lancia il Kernel "parallel_quick"
cl_event sort( 
			  cl_command_queue que, cl_kernel parallel_quick, 

			  cl_mem input, cl_mem delimiters, cl_mem app,

			  int n_wait_events, cl_event* wait_events, 

			  cl_int global_ws, cl_int local_ws 

			  ) {

	//Variabili
	cl_int err;
	cl_event evt_sort;

	//Inserisco gli argomenti da passare al kernel
	err = clSetKernelArg(parallel_quick, 0, sizeof(input), &input);
	ocl_check(err, "set vecinit arg 0");

	err = clSetKernelArg(parallel_quick, 1, sizeof(delimiters), &delimiters);
	ocl_check(err, "set vecinit arg 1");

	err = clSetKernelArg(parallel_quick, 2, sizeof(app), &app);
	ocl_check(err, "set vecinit arg 2");

	err = clSetKernelArg(parallel_quick, 3, local_ws*sizeof(cl_int), NULL);
	ocl_check(err, "set cache");

	err = clSetKernelArg(parallel_quick, 4, local_ws*sizeof(cl_int), NULL);
	ocl_check(err, "set scan_cache");

	err = clSetKernelArg(parallel_quick, 5, 1*sizeof(cl_int), NULL);
	ocl_check(err, "set ex_index");

	//printf ("\n N Work-Group: %d - N Elementi WG: %d \n",global_ws/local_ws,local_ws);

	//Calcolo il Global Work Size
	const size_t gws[] = { global_ws };
	const size_t lws[] = { local_ws };

	//Lancio del Kernel
	err = clEnqueueNDRangeKernel(que, parallel_quick, 1, NULL, gws, lws, n_wait_events, wait_events, &evt_sort);
	ocl_check(err, "enqueue parallel_quick");

	//Restituisco l'evento openCL associato al lancio del kernel
	return evt_sort;
}


//Funzione che decide quanti Work-Item lanciare al prossimo livello dell'Albero di Ricorsione
int clean_vector (cl_mem delimiters, int size, cl_mem app, cl_command_queue que) {

	int mem = (2*size)*sizeof(int);
	int counter = 0;
	int c =0;
	cl_int err;
	
	cl_int* h_delimiters = malloc (mem);
	cl_int* h_app = malloc (mem); 

	//Leggo il buffer delimiters - Sarà il buffer da modificare
	err=clEnqueueReadBuffer(que,delimiters,CL_TRUE,0,mem,h_delimiters,0,NULL,NULL);
	ocl_check(err, "Lettura buffer delimiters");

	//Leggo il buffer app - Sarà il buffer da leggere
	err=clEnqueueReadBuffer(que,app,CL_TRUE,0,mem,h_app,0,NULL,NULL);
	ocl_check(err, "Lettura buffer app");
	
	for (int i=0;i<size;i++) {

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

	//Questo return e' indispensabile su ICD Apple GPU, altrimenti avremo un segmentation fault
	if (counter==0) {
		return 0;
	}

	err = clEnqueueWriteBuffer(que,delimiters,CL_TRUE,0,counter*sizeof(int),h_delimiters,0,NULL,NULL);
	ocl_check(err, "Scrittura vettore delimiters per la prossima iterazione");

	free (h_delimiters);
	free(h_app);

	return (counter/2); //Sarà il numero di Work-Item da lanciare nella prossima iterazione

}


int main (int argc, char *argv[]) {




	//Controllo Parametri in input
	if (argc != 3) {
		error("Errore sintassi:\n\n Bisogna inserire: \n\n 1) La dimensione dell'Array da ordinare \n\n 2) Dimensione Sliding-Window \n");
	}




	//Scelta Device e Compilazione Programma Device
	cl_platform_id p = select_platform();
	cl_device_id d = select_device(p);
	cl_context ctx = create_context(p, d);
	cl_command_queue que = create_queue(ctx, d);
	cl_program prog = create_program("device_code.ocl", ctx, d);

	cl_int err;

	//Creazione kernel parallel_quick
	cl_kernel parallel_quick = clCreateKernel(prog, "parallel_quick", &err);
	ocl_check(err, "parallel_quick");




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

	//Imposto la dimensione della sliding-window
	int sl_size=atoi(argv[2]);
	

	

	//Oggetto che rappresenta lo stato iniziale dell'ogetto delimiters
	cl_int* empty_borders = malloc (delimiters_memsize);

	//Vettore di eventi
	cl_event* ev = malloc (MAX_EVT*sizeof(cl_event));

	//Riempimento oggetto empty_borders
	empty_borders[0] = 0;
	empty_borders[1] = numels-1;
	for (int i=2;i<delimiters_len;i++) {
		empty_borders[i]=-1;
	}

	//Dataset Ordinato
	cl_int* vet = malloc (memsize);

	//Variabili che regolano il ciclo
	int gws = 1;
	int c=0;

	double rt;
	double ne;

	//Inizializzo vettore di input
	cl_int* input = malloc (memsize);
	init(input,numels);




	//Creazione Buffer atto a contenere il Dataset
	cl_mem d_v1 = clCreateBuffer (ctx,CL_MEM_READ_WRITE,memsize,NULL,&err);
	ocl_check(err, "Creazione buffer di input");

	//Scrittura del Dataset sul Buffer
	err = clEnqueueWriteBuffer(que,d_v1,CL_TRUE,0,memsize,input,0,NULL,NULL);
	ocl_check(err, "Inizializazione Dataset");

	//Creazione Buffer per i delimiters
	cl_mem delimiters = clCreateBuffer (ctx,CL_MEM_READ_WRITE,delimiters_memsize,NULL,&err);
	ocl_check(err, "creazione buffer start");

	//Scrittura dei primi delimiters
	err = clEnqueueWriteBuffer(que,delimiters,CL_TRUE,0,delimiters_memsize,empty_borders,0,NULL,NULL);
	ocl_check(err, "Inizializazione buffer start");

	//Creazione Buffer di appogio
	cl_mem app = clCreateBuffer (ctx,CL_MEM_READ_WRITE,delimiters_memsize,NULL,&err);
	ocl_check(err, "creazione buffer app");




	while (gws>0) {

		//Lancio kernel parallel_quick
		ev[c] = sort(que, parallel_quick,d_v1,delimiters,app,0,NULL,gws*sl_size,sl_size);

		//Attendo che venga eseguita tutta la coda di comandi
		err = clFinish(que);
		ocl_check(err, "clFinish");

		c++;

		printf ("Sort %d \t\t\t\t Runtime: %gms \n",c, runtime_ms(ev[c-1]));

		//Calcolo quanti Work-Item lanciare alla prossima iterazione
		gws = clean_vector (delimiters,2*gws,app,que);

	}




	rt = total_runtime_ms(ev[0],ev[c-1]);
	ne = numels/rt;
	printf ("\nTotal Runtime: %gms \t Computed Elements: %g GE/s \n",rt,ne);




	//Leggo il vettore ordinato
	err=clEnqueueReadBuffer(que,d_v1,CL_TRUE,0,memsize,vet,0,NULL,NULL);
	ocl_check(err, "Lettura vettore ordinato");

	//Verifico la correttezza del risultato
	verify(vet,numels);




	//Libero le risorse
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
