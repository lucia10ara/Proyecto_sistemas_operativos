#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <mqueue.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/mman.h>

pid_t pid_facturacion, pid_clasificacion;

int tiempo_aleatorio(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void* escaneo(void* arg) {
    printf("[Hilo Escaneo] Comienzo mi ejecución...\n");

    printf("[Hilo Escaneo] Escaneando maleta %s...\n", (char *)arg);
    sleep(tiempo_aleatorio(1, 3));
    printf("[Hilo Escaneo] Maleta %s escaneada. Enviando a clasificación...\n", (char *)arg);

    
}

void* clasificacion(void* arg) {
    printf("[Hilo Clasificación] Comienzo mi ejecución...\n");

    printf("[Hilo Clasificación] Clasificando maleta %s...\n", (char *)arg);
    sleep(tiempo_aleatorio(2, 4));
    printf("[Hilo Clasificación] Maleta %s clasificada. Enviando a carga...\n", (char *)arg);


}

void* carga(void* arg) {
    printf("[Hilo Carga] Comienzo mi ejecución...\n");

    printf("[Hilo Carga] Cargando maleta %s en el avión...\n", (char *)arg);
    sleep(tiempo_aleatorio(1, 3));
    printf("[Hilo Carga] Maleta %s embarcada. Notificando a facturación...\n", (char *)arg);

}

int main(int argc, char* argv[]) {

    pid_facturacion = fork();
   
    if (pid_facturacion != 0) {
        pid_clasificacion = fork();
        if (pid_clasificacion != 0) {
            /* Proceso padre */
           
            printf("[Proceso Padre] Esperando a que terminen mis hijos...\n");
            

            printf("[Proceso Padre] Ejecución finalizada.\n");
        } else {
            /* Proceso Clasificación */
            printf("[Proceso Clasificación] Comienzo mi ejecución...\n");
           
            while(1) {                
                
            }
        }
    } else {
        /* Proceso Facturación */
        printf("[Proceso Facturación] Comienzo mi ejecución...\n");
        int num_maleta = 0;

        while (1) {

            sleep(tiempo_aleatorio(3, 7));
            printf("[Proceso Facturación] Facturada maleta nº %d. Enviando a clasificación...\n", num_maleta);
            

            num_maleta++;

        }
    }

    exit(0);
}
