#include "./lib/ocl_boiler.h"
#include "./lib/host_assets.h"
#define MAX_EVT 150

size_t max_wg_size;
int flag = 0;
int max_sott_size = -1;

//Funzione che lancia il Kernel "parallel_quick"
cl_event split_f( 
			  cl_command_queue que, cl_kernel split, 

			  cl_mem input, 
			  
			  cl_mem smaller_pivot, cl_mem greater_pivot, 
			  
			  cl_mem delimiters, 

			  int n_wait_events, cl_event* wait_events, 

			  cl_int global_ws

			  ) {

	//Variabili
	cl_int err;
	cl_event evt_split;

	//Inserisco gli argomenti da passare al kernel
	err = clSetKernelArg(split, 0, sizeof(input), &input);
	ocl_check(err, "set vecinit arg 0");

	err = clSetKernelArg(split, 1, sizeof(smaller_pivot), &smaller_pivot);
	ocl_check(err, "set vecinit arg 1");

	err = clSetKernelArg(split, 2, sizeof(greater_pivot), &greater_pivot);
	ocl_check(err, "set vecinit arg 2");

	err = clSetKernelArg(split, 3, sizeof(delimiters), &delimiters);
	ocl_check(err, "set vecinit arg 3");


	//Calcolo il Global Work Size
	const size_t gws[] = { global_ws };

	//Lancio del Kernel
	err = clEnqueueNDRangeKernel(que, split, 1, NULL, gws, NULL, n_wait_events, wait_events, &evt_split);
	ocl_check(err, "enqueue split");

	//Restituisco l'evento openCL associato al lancio del kernel
	return evt_split;
}

//Funzione che lancia il Kernel "parallel_quick"
cl_event fill_f( 
			  cl_command_queue que, cl_kernel fill, 

			  cl_mem input, 
			  
			  cl_mem smaller_pivot, cl_mem greater_pivot, 
			  
			  cl_mem delimiters, cl_mem app,

			  int n_wait_events, cl_event* wait_events, 

			  cl_int global_ws

			  ) {

	//Variabili
	cl_int err;
	cl_event evt_fill;

	//Inserisco gli argomenti da passare al kernel
	err = clSetKernelArg(fill, 0, sizeof(input), &input);
	ocl_check(err, "set vecinit arg 0");

	err = clSetKernelArg(fill, 1, sizeof(smaller_pivot), &smaller_pivot);
	ocl_check(err, "set vecinit arg 1");

	err = clSetKernelArg(fill, 2, sizeof(greater_pivot), &greater_pivot);
	ocl_check(err, "set vecinit arg 2");

	err = clSetKernelArg(fill, 3, sizeof(delimiters), &delimiters);
	ocl_check(err, "set vecinit arg 3");

	err = clSetKernelArg(fill, 4, sizeof(app), &app);
	ocl_check(err, "set vecinit arg 4");

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

	//Variabili
	cl_int err;
	cl_event evt_sort;

	//Inserisco gli argomenti da passare al kernel
	err = clSetKernelArg(parallel_quick, 0, sizeof(input), &input);
	ocl_check(err, "set vecinit arg 0");

	err = clSetKernelArg(parallel_quick, 1, sizeof(smaller_pivot), &smaller_pivot);
	ocl_check(err, "set vecinit arg 1");

	err = clSetKernelArg(parallel_quick, 2, sizeof(greater_pivot), &greater_pivot);
	ocl_check(err, "set vecinit arg 2");

	err = clSetKernelArg(parallel_quick, 3, sizeof(delimiters), &delimiters);
	ocl_check(err, "set vecinit arg 3");

	err = clSetKernelArg(parallel_quick, 4, sizeof(app), &app);
	ocl_check(err, "set vecinit arg 4");

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
	int l_flag = 0;
	cl_int err;
	max_sott_size = -1;
	
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

			int current_sott_size = ( h_delimiters[counter-1] - h_delimiters[counter-2] ) + 1;

			if (l_flag==0 &&  current_sott_size > max_wg_size ) {
				l_flag = 1;
			}else if (max_sott_size < current_sott_size) {
				max_sott_size = current_sott_size;
			}

			
		}else{
			c+=2;
		}
	}

	if (l_flag == 0) flag=1;



	//Questo return e' indispensabile su ICD Apple GPU, altrimenti avremo un segmentation fault
	if (counter==0) {
		return 0;
	}

	err = clEnqueueWriteBuffer(que,delimiters,CL_TRUE,0,counter*sizeof(int),h_delimiters,0,NULL,NULL);
	ocl_check(err, "Scrittura vettore delimiters per la prossima iterazione");

	free (h_delimiters);
	free(h_app);

	return (counter/2); //Sarà il numero di Sottoproblemi nella prossima iterazione

}


int main (int argc, char *argv[]) {
	



	//Controllo Parametri in input
	if (argc != 2) {
		error("Errore sintassi:\n\n Bisogna inserire: \n\n 1) La dimensione dell'Array da ordinare \n");
	}




	//Scelta Device e Compilazione Programma Device
	cl_platform_id p = select_platform();
	cl_device_id d = select_device(p);
	cl_context ctx = create_context(p, d);
	cl_command_queue que = create_queue(ctx, d);
	cl_program prog = create_program("device_code.ocl", ctx, d);

	cl_int err;
	
	//Creazione kernel split
	cl_kernel split = clCreateKernel(prog, "split", &err);
	ocl_check(err, "split");

	//Creazione kernel split
	cl_kernel fill = clCreateKernel(prog, "fill", &err);
	ocl_check(err, "fill");

	//Creazione kernel parallel_quick
	cl_kernel parallel_q = clCreateKernel(prog, "parallel_quick", &err);
	ocl_check(err, "parallel_q");

	/* Prelievo del Numero Massimo di Work-Item in un Work-Group */
	err = clGetKernelWorkGroupInfo(split, d, CL_KERNEL_WORK_GROUP_SIZE, sizeof(max_wg_size), &max_wg_size, NULL);
	ocl_check(err, "get max wg size info");




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

	//Variabili che regolano il ciclo
	int n_sott = 1;
	int n_iterazioni=0;

	double rt;
	double ne;
	cl_event sp;

	

	//Vettore di eventi
	cl_event* ev = malloc (MAX_EVT*sizeof(cl_event));

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
	
	//Inizializzo vettore di input
	cl_int* input = malloc (memsize);
	init(input,numels);

	//Dataset Ordinato
	cl_int* vet = malloc (memsize);




	//Creazione Buffer atto a contenere il Dataset
	cl_mem d_v1 = clCreateBuffer (ctx,CL_MEM_READ_WRITE,memsize,NULL,&err);
	ocl_check(err, "Creazione buffer di input");

	//Scrittura del Dataset sul Buffer
	err = clEnqueueWriteBuffer(que,d_v1,CL_TRUE,0,memsize,input,0,NULL,NULL);
	ocl_check(err, "Inizializazione Dataset");

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




	while (n_sott>0) {


		if (flag == 1) {
			ev[n_iterazioni]= sort (que, parallel_q, d_v1, smaller_pivot, greater_pivot, delimiters, app, 0, NULL,(n_sott*max_sott_size), max_sott_size);

			//Attendo che venga eseguita tutta la coda di comandi
			err = clFinish(que);
			ocl_check(err, "clFinish");

		}else{

			//Lancio kernel split
			sp= split_f(que, split, d_v1, smaller_pivot, greater_pivot, delimiters,0, NULL, numels);

			//Attendo che venga eseguita tutta la coda di comandi
			err = clFinish(que);
			ocl_check(err, "clFinish");

			ev[n_iterazioni]= fill_f (que, fill, d_v1, smaller_pivot, greater_pivot, delimiters, app, 1, &sp, n_sott);

			//Attendo che venga eseguita tutta la coda di comandi
			err = clFinish(que);
			ocl_check(err, "clFinish");

		}

		n_iterazioni++;

		printf ("Sort %d \t\t\t\tRuntime: %gms \n",
		n_iterazioni, runtime_ms(ev[n_iterazioni-1]));

		//Calcolo quanti sotto problemi avrò alla prossima iterazione
		n_sott = clean_vector (delimiters,2*(n_sott),app,que);


	}




	rt = total_runtime_ms(ev[0],ev[n_iterazioni-1]);
	ne = numels/rt;
	printf ("\nTotal Runtime: %gms \tComputed Elements: %g GE/s \n",rt,ne);

	


	//Leggo il vettore ordinato
	err=clEnqueueReadBuffer(que,d_v1,CL_TRUE,0,memsize,vet,0,NULL,NULL);
	ocl_check(err, "Lettura vettore ordinato");

	//Verifico la correttezza del risultato
	verify(vet,numels);




	//Libero le risorse
	free(vet);
	free(ev);




	//Uscita dal programma
	return 0;




}
