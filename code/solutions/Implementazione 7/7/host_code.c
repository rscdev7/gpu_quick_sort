#include "./lib/ocl_boiler.h"
#include "./lib/host_assets.h"
#define SEED 20
#define MAX_EVT 150

size_t max_wg_size;
int flag = 0;
int max_sott_size = -1;

//Funzione che lancia il Kernel "parallel_quick"
cl_event split_f( 
			  cl_command_queue que, cl_kernel split, 

			  cl_mem input, cl_mem input_app,  
			  
			  cl_mem delimiters, 
			  
			  cl_mem pivots, 

			  int n_wait_events, cl_event* wait_events, 

			  cl_int global_ws

			  ) {

	//Variabili
	cl_int err;
	cl_event evt_split;

	//Inserisco gli argomenti da passare al kernel
	err = clSetKernelArg(split, 0, sizeof(input), &input);
	ocl_check(err, "set vecinit arg 0");

	err = clSetKernelArg(split, 1, sizeof(input_app), &input_app);
	ocl_check(err, "set vecinit arg 1");

	err = clSetKernelArg(split, 2, sizeof(delimiters), &delimiters);
	ocl_check(err, "set vecinit arg 2");

	err = clSetKernelArg(split, 3, sizeof(pivots), &pivots);
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
cl_event sort( 
			  cl_command_queue que, cl_kernel parallel_quick, 

			  cl_mem input, cl_mem input_app, 
			  
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

	err = clSetKernelArg(parallel_quick, 1, sizeof(input_app), &input_app);
	ocl_check(err, "set vecinit arg 1");

	err = clSetKernelArg(parallel_quick, 2, sizeof(delimiters), &delimiters);
	ocl_check(err, "set vecinit arg 2");

	err = clSetKernelArg(parallel_quick, 3, sizeof(app), &app);
	ocl_check(err, "set vecinit arg 3");

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

int adjust_delimiters (cl_command_queue que, cl_mem pivots, cl_mem delimiters, int del_dim) {

	int memsize_del = del_dim*sizeof(int);
	int memsize_new_del = 2*del_dim*sizeof(int);
	int memsize_pivots = (del_dim/2)*sizeof(int);
	cl_int err;
	cl_int* h_del = malloc(memsize_del);
	cl_int* h_pivots = malloc(memsize_pivots);
	cl_int* new_delimiters = malloc (memsize_new_del);

	int start=-1;
	int end=-1;
	int pivot_idx=-1;
	int is_left_none=-1;
	int is_right_none=-1;
	int c=0;
	int n_del_c = 0;
	int l_flag=0;
	max_sott_size=-1;

	//Leggo il buffer delimiters - Sarà il buffer da modificare
	err=clEnqueueReadBuffer(que,delimiters,CL_TRUE,0,memsize_del,h_del,0,NULL,NULL);
	ocl_check(err, "Lettura buffer delimiters");

	//Leggo il buffer app - Sarà il buffer da leggere
	err=clEnqueueReadBuffer(que,pivots,CL_TRUE,0,memsize_pivots,h_pivots,0,NULL,NULL);
	ocl_check(err, "Lettura buffer pivots");

	for (int i=0;i< ( (del_dim)-1 ) ;i+=2) {

		start= h_del[i];
		end = h_del[i+1];
		pivot_idx =h_pivots[c];

		is_left_none=0;
		is_right_none=0;

		if (pivot_idx-1 <= start) is_left_none=1;

		if (pivot_idx+1 >= end) is_right_none=1; 

		if (is_left_none==0) {

			new_delimiters[n_del_c] = start;
			n_del_c++;
			new_delimiters[n_del_c] = pivot_idx-1;
			n_del_c++;

			int current_sott_size = ( new_delimiters[n_del_c-1] - new_delimiters[n_del_c-2]) +1;

			if (l_flag==0 &&  current_sott_size > max_wg_size ) {
				l_flag = 1;
			}else if (max_sott_size < current_sott_size) {
				max_sott_size = current_sott_size;
			}

		}

		if (is_right_none==0) {

			new_delimiters[n_del_c] = pivot_idx+1;
			n_del_c++;
			new_delimiters[n_del_c] = end;
			n_del_c++;

			int current_sott_size = ( new_delimiters[n_del_c-1] - new_delimiters[n_del_c-2]) +1;

			if (l_flag==0 &&  current_sott_size > max_wg_size ) {
				l_flag = 1;
			}else if (max_sott_size < current_sott_size) {
				max_sott_size = current_sott_size;
			}

		}
		

		c++;
	}

	if (l_flag == 0) flag=1;

	err = clEnqueueWriteBuffer(que,delimiters,CL_TRUE,0,n_del_c*sizeof(int),new_delimiters,0,NULL,NULL);
	ocl_check(err, "Scrittura vettore delimiters per la prossima iterazione");

	free(h_del);
	free(h_pivots);
	free(new_delimiters);

	return (n_del_c/2);


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




	//Vettore di eventi
	cl_event* ev = malloc (MAX_EVT*sizeof(cl_event));
	
	//Inizializzo vettore di input
	cl_int* input = malloc (memsize);
	init(input,numels);

	//Oggetto che rappresenta lo stato iniziale dell'ogetto delimiters
	cl_int* empty_borders = malloc (delimiters_memsize);

	//Riempimento oggetto empty_borders
	empty_borders[0] = 0;
	empty_borders[1] = numels-1;
	for (int i=2;i<delimiters_len;i++) {
		empty_borders[i]=-1;
	}




	//Creazione Buffer atto a contenere il Dataset
	cl_mem d_v1 = clCreateBuffer (ctx,CL_MEM_READ_WRITE,memsize,NULL,&err);
	ocl_check(err, "Creazione buffer di input");

	//Scrittura del Dataset sul Buffer
	err = clEnqueueWriteBuffer(que,d_v1,CL_TRUE,0,memsize,input,0,NULL,NULL);
	ocl_check(err, "Inizializazione Dataset");

	//Creazione Buffer atto a contenere gli elementi piu piccoli del pivot
	cl_mem input_app = clCreateBuffer (ctx,CL_MEM_READ_WRITE,memsize,NULL,&err);
	ocl_check(err, "Creazione buffer input_app");

	//Scrittura stato iniziale input_app
	err = clEnqueueWriteBuffer(que,input_app,CL_TRUE,0,memsize,input,0,NULL,NULL);
	ocl_check(err, "Inizializazione input_app");

	//Creazione Buffer per i delimiters
	cl_mem delimiters = clCreateBuffer (ctx,CL_MEM_READ_WRITE,delimiters_memsize,NULL,&err);
	ocl_check(err, "creazione buffer start");

	//Scrittura dei primi delimiters
	err = clEnqueueWriteBuffer(que,delimiters,CL_TRUE,0,delimiters_memsize,empty_borders,0,NULL,NULL);
	ocl_check(err, "Inizializazione buffer delimiters");

	//Creazione Buffer di appogio per delimiters
	cl_mem app = clCreateBuffer (ctx,CL_MEM_READ_WRITE,delimiters_memsize,NULL,&err);
	ocl_check(err, "creazione buffer app");

	//Scrittura sul Buffer di appoggio per delimiters
	err = clEnqueueWriteBuffer(que,app,CL_TRUE,0,delimiters_memsize,empty_borders,0,NULL,NULL);
	ocl_check(err, "Inizializazione buffer app");

	//Creazione Buffer atto a contenere i pivots
	cl_mem pivots = clCreateBuffer (ctx,CL_MEM_READ_WRITE,delimiters_memsize,NULL,&err);
	ocl_check(err, "Creazione buffer pivots");

	//Scrittura stato smaller pivot
	err = clEnqueueWriteBuffer(que,pivots,CL_TRUE,0,delimiters_memsize,empty_borders,0,NULL,NULL);
	ocl_check(err, "Inizializazione pivots");




	while (n_sott>0) {


		if (flag == 1) {
			
			
			ev[n_iterazioni]= sort (que, parallel_q, d_v1, input_app, delimiters, app, 0, NULL,(n_sott*max_sott_size), max_sott_size);

			//Attendo che venga eseguita tutta la coda di comandi
			err = clFinish(que);
			ocl_check(err, "clFinish");
			
			err = clEnqueueCopyBuffer (que, input_app, d_v1, 0, 0, memsize,0,NULL,NULL);
			ocl_check(err, "Copy Buffer");

			//Attendo che venga eseguita tutta la coda di comandi
			err = clFinish(que);
			ocl_check(err, "clFinish");

		}else{
			
			ev[n_iterazioni]= split_f(que, split, d_v1, input_app, delimiters, pivots,  0, NULL, numels);


			//Attendo che venga eseguita tutta la coda di comandi
			err = clFinish(que);
			ocl_check(err, "clFinish - attesa");


			err = clEnqueueCopyBuffer (que, input_app, d_v1, 0, 0, memsize, 0, NULL, NULL);
			ocl_check(err, "Copy Buffer");
			

		}

		n_iterazioni++;

		printf ("Sort %d \t\t\t\tRuntime: %gms \n",n_iterazioni, runtime_ms(ev[n_iterazioni-1]));

		//Calcolo quanti sotto problemi avrò alla prossima iterazione
		if (flag==0) {
			n_sott = adjust_delimiters (que,pivots,delimiters,2*(n_sott));
		}else{
			n_sott = clean_vector (delimiters,2*(n_sott),app,que);
		}
		
	}




	rt = total_runtime_ms(ev[0],ev[n_iterazioni-1]);
	ne = numels/rt;
	printf ("\nTotal Runtime: %gms \tComputed Elements %g GE/s \n",rt,ne);

	//Dataset Ordinato
	cl_int* vet = malloc (memsize);

	
	

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
