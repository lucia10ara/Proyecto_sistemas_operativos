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
    #include <err.h>

    #include <fcntl.h>

    #define COLA "/mensajes"
    #define SHM_MEMORIA "/memoria"
    #define MAX_MALETAS 10
    #define TAM_MENSAJE 128

    //Señal que carga envia a facturacion cuando una maleta esta embarcada
    //(usamos SIGUSR1 porque es una señal libre sin funcion fija)
    #define SENAL_MALETA SIGUSR1 

    /*Estructura compartida en los objetos compartidos de memoria*/

    struct shmbuf {
        pid_t pid_facturacion; //id del proceso facturacion
        int maleta_embarcada; // nº de la ultima maleta embarcada
        char vuelo[16]; // texto del vuelo
    };

    /*VARIABLES GLOBALES*/
    pid_t pid_facturacion, pid_clasificacion;   

    mqd_t cola;
    struct mq_attr attr;
    struct shmbuf *shmp;
    int fd;

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

    //Creamos una funcion para poder pasarle la direccion de codigo a la que saltar
    void avisar_pasajero(int senal) {
        printf("[Proceso Facturación] Aviso al pasajero: maleta %d cargada en el vuelo %s\n", shmp->maleta_embarcada, shmp->vuelo);
    }

    int main(int argc, char* argv[]) {

        attr.mq_maxmsg = MAX_MALETAS;
        attr.mq_msgsize = TAM_MENSAJE;
        cola = mq_open(COLA, O_RDWR | O_CREAT, 0644, &attr);

        /*MEMORIA COMPARTIDA*/
        fd = shm_open(SHM_MEMORIA, O_CREAT | O_RDWR, 0600);
        if (fd == -1)
            err(EXIT_FAILURE, "shm_open");

        if (ftruncate(fd, sizeof(struct shmbuf)) == -1)
            err(EXIT_FAILURE, "ftruncate");


        /* Mapear el objeto al espacio de direcciones de quien lo llama */

        shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, 0);
        if (shmp == MAP_FAILED)
            err(EXIT_FAILURE, "mmap");



        pid_facturacion = fork();
        
        if (pid_facturacion != 0) {
            pid_clasificacion = fork();
            if (pid_clasificacion != 0) {
                /* Proceso padre */
                shmp->pid_facturacion = pid_facturacion;

                
                printf("[Proceso Padre] Esperando a que terminen mis hijos...\n");
                wait(NULL);
                wait(NULL);

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

            char mensaje[TAM_MENSAJE];

            signal(SENAL_MALETA, avisar_pasajero); //cuando "Carga" avise de que la maleta esta embarcada, avisamos al pasajero

            while (1) {

                sleep(tiempo_aleatorio(3, 7));
                printf("[Proceso Facturación] Facturada maleta nº %d. Enviando a clasificación...\n", num_maleta);
                
                sprintf(mensaje, "Maleta %d", num_maleta); //defino lo que va dentro del mensaje --> Maleta (n)
	            int resultado = mq_send(cola, mensaje, strlen(mensaje) + 1, 0);


                num_maleta++;

            }
        }

        exit(0);
    }
