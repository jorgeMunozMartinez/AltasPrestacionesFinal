# Reducción del tiempo de cómputo de la etapa de estimación de movimiento en algoritmos de codificación de vídeo

Para la realización de esta práctica se ha realizado varias pruebas

## Optimización de los bucles

En una primera aproximación para mejorar el tiempo de cómputo. Se ha reescrito los bucles anidados.

Búcle sin modificar
````c
for (unsigned int y = 0; y < HEIGHT; y += BS){
  for (unsigned int x = 0; x < WIDTH; x += BS){
    /* Calcular MSE para todos los bloques en el área de búsqueda.
    Las coordenadas en ref y act están alineadas.   */
    for (unsigned char j = 0; j < 2 * SA; j++){
      for (unsigned char k = 0; k < 2 * SA; k++){
        /*Calcular MSE */
        float coste_bloque = MSE(&act[y * WIDTH + x], &ref[(y + j) * (WIDTH + 2 * SA) + (x + k)]);
        /* Podría haber una optimización. A igualdad de coste, elegir aquel cuyo vector de movimiento tenga la menor                       distancia respecto al origen */
         if (coste_bloque < costes[y / BS][x / BS]){
            costes[y / BS][x / BS] = coste_bloque;
            Vx[y / BS][x / BS] = j - SA;
            Vy[y / BS][x / BS] = k - SA;
        }
      }
    }
  }
}
````

Bucle modificado
````c
for (i = 0; i < (HEIGHT * WIDTH)/BS; i += BS){
  x = i % WIDTH;
  y = (i / WIDTH) *BS;
  /* Calcular MSE para todos los bloques en el área de búsqueda.
  Las coordenadas en ref y act están alineadas.   */
  for (z = 0; z < 96 * SA; z++){  
    /*Calcular MSE */
    j= z/ (2 * SA);
 		k= z % (2 * SA);
    coste_bloque = MSE(&act[y * WIDTH + x], &ref[(y + j) * (WIDTH + 2 * SA) + (x + k)]);
    /* Podría haber una optimización. A igualdad de coste, elegir aquel cuyo vector de movimiento tenga la menor   distancia respecto al origen */
    if (coste_bloque < costes[y / BS][x / BS]){
      costes[y / BS][x / BS] = coste_bloque;
      Vx[y / BS][x / BS] = j - SA;
      Vy[y / BS][x / BS] = k - SA;
    }
  }
}
````

Bucle sin modificar
````c
float MSE(unsigned char *bloque_actual, unsigned char *bloque_referencia){
  float error = 0;
  for (unsigned char y = 0; y < BS; y++){
    for (unsigned char x = 0; x < BS; x++){
      error += pow((bloque_actual[y * WIDTH + x] - bloque_referencia[y * (WIDTH + 2 * SA) + x]), 2);
     }
   }
 return error / (BS*BS);
}
````
Bucle modificado
````c
float MSE(unsigned char *bloque_actual, unsigned char *bloque_referencia){
  float error = 0;
  int i=0;
  unsigned char x,y, z;   
  for (i= 0; i < BS*BS; i++){
    y= i/ BS;
    x= i % BS;
      error += pow((bloque_actual[y * WIDTH + x] - bloque_referencia[y * (WIDTH + 2 * SA) + x]), 2);  
    }
  return error / (BS*BS);
}
````
## OpenMP
