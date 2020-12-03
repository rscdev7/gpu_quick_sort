#define CL_TARGET_OPENCL_VERSION 120
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define SEED 20

//Funzione d'errore per le chiamate openCL
void error(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

//Funzione che inizializza il vettore di input con dei numeri casuali
void init(cl_int* v,int n) {
	//srand(time(NULL));
	//CON IL SEED FISSO, SUCCEDE CHE LA CPU SFRUTTA LA CACHE 
	//E RISULTA AVERE UNA VELOCITA' FULMINEA NELLE ITERAZIONI SUCCESSIVE
	srand(SEED); 
	for (int i=0;i<n;i++) {
		v[i] = (cl_int) (rand()%n);
	}
}

//Funzione che stampa un vettore
void print_vec (cl_int* vec, int size) {
	for (int i=0;i<size;i++) {
			printf ("idx = %d ---> [ %d ] \n",i,vec[i]);
	}
}

//Funzione che verifica la correttezza del risultato ottenuta dal device
void verify (cl_int* vec, cl_int size) {
	for (int i=0;i<size-1;i++) {
		if (vec[i]>vec[i+1]) {
			printf ("Mismatch @ vec[i=%d]= %d > vec[i+1=%d]= %d \n",i,vec[i],i+1,vec[i+1]);
			break;
		}
	}
}