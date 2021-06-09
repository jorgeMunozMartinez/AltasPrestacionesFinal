#include <stdlib.h>

#include <stdio.h>

#include <time.h>

#include <math.h>

#include <omp.h>

#include <unistd.h>

/* Los parámetros establecidos. Son fijos. */
#define WIDTH 1280
#define HEIGHT 720
#define BS 16
#define SA 24

int hilos;
float MSE(unsigned char * bloque_actual, unsigned char * bloque_referencia);

int main(int argc, char * argv[]) {

  unsigned char * ref; /* Datos del frame de referencia */
  unsigned char * act; /* Datos del frame actual */
  FILE * fref, * fact;
  unsigned char linea[WIDTH];
  char Vx[(HEIGHT / BS)][(WIDTH / BS)];
  char Vy[(HEIGHT / BS)][(WIDTH / BS)];
  float costes[(HEIGHT / BS)][(WIDTH / BS)];
  unsigned int i, z, x, y, j, k;
  float coste_bloque;
  if (argc != 2) {
    fprintf(stderr, "USO:  %s NºMuestras NºHilos \n", argv[0]);
    exit(EXIT_FAILURE);
  }

  hilos = atoi(argv[1]);
  printf(" NºMuestras NºHilos static %i \n", hilos);
  omp_set_num_threads(hilos);

  //struct timespec inicio,fin;
  double inicio, fin;

  /* Reservamos memoria */
  ref = (unsigned char * ) malloc((WIDTH + 2 * SA) * (HEIGHT + 2 * SA)); /* Extensión de bordes */
  if (ref == NULL) {
    printf("Error al obtener memoria para el frame de referencia\n");
    return -1;
  }
  act = (unsigned char * ) malloc(WIDTH * HEIGHT);
  if (act == NULL) {
    printf("Error al obtener memoria para el frame actual\n");
    free(ref);
    return -2;
  }

  /* Leemos la información de los ficheros */
  fref = fopen("FrameReferencia.data", "r");
  if (fref == NULL) {
    printf("Error al abrir el archivo con los datos del frame de referencia\n");
    free(ref);
    free(act);
    return -3;
  }

  fact = fopen("FrameActual.data", "r");
  if (fact == NULL) {
    printf("Error al abrir el archivo con los datos del frame actual\n");
    free(ref);
    free(act);
    fclose(fref);
    return -3;
  }

  /* Frame actual es directo */
  fread(act, WIDTH * HEIGHT, sizeof(unsigned char), fact);
  /* Leer el frame de referencia debe tener en cuenta la extensión de bordes */

  /* Usado para depuración */
  /* int cnt1 = 0, cnt2 = 0; */
  for (y = 0; y < HEIGHT + 2 * SA; y++) {
    if (y == 0) {
      fread(linea, WIDTH, sizeof(unsigned char), fref);
      /* cnt1++; */
    } else if (y > SA && y < HEIGHT + SA) {
      fread(linea, WIDTH, sizeof(unsigned char), fref);
      /* cnt1++; */
    }
    for (x = 0; x < WIDTH + 2 * SA; x++) {
      if (x < SA) {
        ref[y * (WIDTH + 2 * SA) + x] = linea[0];
      } else if (x >= SA && x < WIDTH + SA) {
        ref[y * (WIDTH + 2 * SA) + x] = linea[x - SA];
      } else {
        ref[y * (WIDTH + 2 * SA) + x] = linea[WIDTH - 1];
      }
    }
  }

  /* Inicializar los costes mínimos asociados a cada bloque */
  for (y = 0; y < HEIGHT / BS; y++) {
    for (x = 0; x < WIDTH / BS; x++) {
      costes[y][x] = __FLT_MAX__;
    }
  }

  //clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&inicio);
  inicio = omp_get_wtime();
  /* Los datos ahora sí que están preparados. Procedemos a calcular los costes */
  #pragma omp parallel for  private(x,y,j,k,z, coste_bloque) schedule(dynamic) num_threads(hilos)
  for (i = 0; i < (HEIGHT * WIDTH) / BS; i += BS) {

    /* Calcular MSE para todos los bloques en el área de búsqueda. 
           Las coordenadas en ref y act están alineadas.   */
    for (z = 0; z < 96 * SA; z++) {
      x = i % WIDTH;
      y = (i / WIDTH) * BS;

      /*Calcular MSE */
      j = z / (2 * SA);
      k = z % (2 * SA);

      coste_bloque = MSE( & act[y * WIDTH + x], & ref[(y + j) * (WIDTH + 2 * SA) + (x + k)]);
      /* Podría haber una optimización. A igualdad de coste, elegir aquel cuyo vector de movimiento tenga la menor distancia
      respecto al origen */
      if (coste_bloque < costes[y / BS][x / BS]) {
        costes[y / BS][x / BS] = coste_bloque;
        Vx[y / BS][x / BS] = j - SA;
        Vy[y / BS][x / BS] = k - SA;
      }

    }

  }

  //clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&fin);
  fin = omp_get_wtime();
  printf("\nVectores de movimiento\n");
  for (y = 0; y < HEIGHT / BS; y++) {
    for (x = 0; x < WIDTH / BS; x++) {

      printf("[%d,%d] ", Vy[y][x], Vx[y][x]);
    }
    printf("\n");
  }

  //printf("Tiempo transcurrido: %ld.%ld segundos.\n", fin.tv_sec-inicio.tv_sec,(fin.tv_nsec-inicio.tv_nsec)%(int)(1e+9));
  printf("Tiempo Total : %f\n", fin - inicio);
  fclose(fref);
  fclose(fact);
  free(ref);
  free(act);

  return 0;
}

float MSE(unsigned char * bloque_actual, unsigned char * bloque_referencia) {

  float error = 0;
  int i = 0;
  unsigned char x, y, z;

  #pragma omp reducer(+:error) parallel  for  schedule(dynamic) num_threads(hilos)
  for (y = 0; y < BS ; y++) {

    error += pow((bloque_actual[y * WIDTH + 0] - bloque_referencia[y * (WIDTH + 2 * SA) + 0]), 2);	
    error += pow((bloque_actual[y * WIDTH + 1] - bloque_referencia[y * (WIDTH + 2 * SA) + 1]), 2);
    error += pow((bloque_actual[y * WIDTH + 2] - bloque_referencia[y * (WIDTH + 2 * SA) + 2]), 2);
    error += pow((bloque_actual[y * WIDTH + 3] - bloque_referencia[y * (WIDTH + 2 * SA) + 3]), 2);
    error += pow((bloque_actual[y * WIDTH + 4] - bloque_referencia[y * (WIDTH + 2 * SA) + 4]), 2);
    error += pow((bloque_actual[y * WIDTH + 5] - bloque_referencia[y * (WIDTH + 2 * SA) + 5]), 2);
    error += pow((bloque_actual[y * WIDTH + 6] - bloque_referencia[y * (WIDTH + 2 * SA) + 6]), 2);
    error += pow((bloque_actual[y * WIDTH + 7] - bloque_referencia[y * (WIDTH + 2 * SA) + 7]), 2);
    error += pow((bloque_actual[y * WIDTH + 8] - bloque_referencia[y * (WIDTH + 2 * SA) + 8]), 2);
    error += pow((bloque_actual[y * WIDTH + 9] - bloque_referencia[y * (WIDTH + 2 * SA) + 9]), 2);
    error += pow((bloque_actual[y * WIDTH + 10] - bloque_referencia[y * (WIDTH + 2 * SA) + 10]), 2);
    error += pow((bloque_actual[y * WIDTH + 11] - bloque_referencia[y * (WIDTH + 2 * SA) + 11]), 2);
    error += pow((bloque_actual[y * WIDTH + 12] - bloque_referencia[y * (WIDTH + 2 * SA) + 12]), 2);
    error += pow((bloque_actual[y * WIDTH + 13] - bloque_referencia[y * (WIDTH + 2 * SA) + 13]), 2);
    error += pow((bloque_actual[y * WIDTH + 14] - bloque_referencia[y * (WIDTH + 2 * SA) + 14]), 2);
    error += pow((bloque_actual[y * WIDTH + 15] - bloque_referencia[y * (WIDTH + 2 * SA) + 15]), 2);


  }

  return error / (BS * BS);

}
