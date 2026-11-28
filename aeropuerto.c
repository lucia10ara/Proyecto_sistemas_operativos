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

    /* ============== LUCÍA DIEZ ============= */
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
        int maleta_embarcada; // num de la ultima maleta embarcada
        char vuelo[16]; // texto del vuelo
    };

    /*VARIABLES GLOBALES*/
    pid_t pid_facturacion, pid_clasificacion;   

    mqd_t cola;   // por donde pasan las maletas -> 10 huecos
    struct mq_attr attr;
    struct shmbuf *shmp;  // memoria compartida -> se apunta qué maleta se ha embarcado
    int fd;

    /* ===== ANE ===== */
    /* 3 semáforos = 3 "tickets de turno", uno por hilo. --> se ejecutan a la vez pero van en fila.
    *   sem_wait -> espero a tener mi ticket
    *   sem_post -> le paso el ticket al siguiente */
    sem_t sem_escaneo;        /* turno de escaneo       (empieza en 1) */
    sem_t sem_clasificacion;  /* turno de clasificacion (empieza en 0 -> sem dormido) */
    sem_t sem_carga;          /* turno de carga         (empieza en 0) */
    /* =============== */

    int tiempo_aleatorio(int min, int max) {
        return rand() % (max - min + 1) + min;
    }

    /* ===== ANE ===== */
    /* 3 HILOS -> Después de la facturación (los semáforos les obliga a ir en orden)*/
    void* escaneo(void* arg) { /* arg -> texto de la maleta, EJEMPLO: "Maleta 2 -- VUELO1" */
        printf("[HILO DE ESCANEO] Comienzo mi ejecución...\n");

        sem_wait(&sem_escaneo);   /* espero mi turno (Primera etapa) */

        printf("[HILO DE ESCANEO] Escaneando maleta %s...\n", (char *)arg);
        sleep(tiempo_aleatorio(1, 3)); // duerme el hilo entre 1 a 3 segundos simulando el escaneo de la maleta
        printf("[HILO DE ESCANEO] Maleta %s escaneada. Enviando a clasificación...\n", (char *)arg);

        sem_post(&sem_clasificacion);       /* turno para clasificación */
        return NULL;                        /* los hilos tienen que devolver algo */
    }


    void* clasificacion(void* arg) {
        printf("[HILO DE CLASIFICACIÓN] Comienzo mi ejecución...\n");

        sem_wait(&sem_clasificacion);       /* espero a que escaneo acabe */

        printf("[HILO DE CLASIFICACIÓN] Clasificando maleta %s...\n", (char *)arg);
        sleep(tiempo_aleatorio(2, 4));
        printf("[HILO DE CLASIFICACIÓN] Maleta %s clasificada. Enviando a carga...\n", (char *)arg);

        sem_post(&sem_carga);               /* turno para carga */
        return NULL;
    }

    void* carga(void* arg) {
        printf("[HILO DE CARGA] Comienzo mi ejecución...\n");

        sem_wait(&sem_carga);    /* espero a que clasificación acabe */

        printf("[HILO DE CARGA] Cargando maleta %s en el avión...\n", (char *)arg);
        sleep(tiempo_aleatorio(1, 3));

        /* ---- AVISO A FACTURACIÓN ---- */
        // Aviso a facturación de que la maleta ha sido embarcada!!:
        // Saco los datos de la maleta y vuelo -> ej: "Maleta 2 -- VUELO1"
        int num;
        char vuelo[16]; 
        if (sscanf((char *)arg, "Maleta %d -- %15s", &num, vuelo) == 2) { // devuelve cuantas cosas ha leido, queremos 2
            // lo apunto en la MEMORIA COMPARTIDA:
            shmp->maleta_embarcada = num;  // "maleta"
            strcpy(shmp->vuelo, vuelo);  // "vuelo"
            printf("[HILO DE CARGA] Maleta %s embarcada. Notificando a facturación...\n", (char *)arg);
            // Doy la SEÑAL (después de escribir), para avisar a pasajero de que la maleta ha sido embarcada.:
            kill(shmp->pid_facturacion, SENAL_MALETA);
        }
        /* ---------------------------------------------------------------------- */

        sem_post(&sem_escaneo); // devuelvo el turno a escaneo: semáforos otra vez 1,0,0
        return NULL;
    }

    //Creamos una funcion para poder pasarle la direccion de codigo a la que saltar
    void avisar_pasajero(int senal) {
        printf("[HILO DE FACTURACIÓN] Aviso al pasajero: maleta %d cargada en el vuelo %s\n", shmp->maleta_embarcada, shmp->vuelo);
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
            
                /* ==== ANE ====*/
                // Preparo los semáforos UNA vez, antes del bucle.
                // sem_init(&sem, 0, valor): el 0 = solo lo usan los hilos de este proceso (1 -> compartir entre procesos)
                sem_init(&sem_escaneo, 0, 1);  /* escaneo puede empezar ya */
                sem_init(&sem_clasificacion, 0, 0); /* los demás esperan */
                sem_init(&sem_carga, 0, 0);

                // Preparo los hilos:
                char mensaje[TAM_MENSAJE];  /* aquí cae cada maleta */
                pthread_t hilo_escaneo, hilo_clasificacion, hilo_carga;

            while (1) { // solo se cierra con Ctrl+C
                /* Espero una maleta. Si la cola está vacía, me quedo aquí dormida. */
                if (mq_receive(cola, mensaje, TAM_MENSAJE, NULL) == -1) {
                    perror("[Proceso Clasificación] Error en mq_receive");
                    continue;  /* lo vuelvo a intentar */
                }
                printf("[Proceso Clasificación] Recibida: %s\n", mensaje);

                /* Lanzo los 3 hilos a la vez; los semáforos los ponen en fila. */
                pthread_create(&hilo_escaneo,       NULL, escaneo,       mensaje);
                pthread_create(&hilo_clasificacion, NULL, clasificacion, mensaje);
                pthread_create(&hilo_carga,         NULL, carga,         mensaje);

                /* Espero a que acaben los 3 antes de coger otra maleta
                 * (si no, "mensaje" cambiaría mientras lo leen). */
                pthread_join(hilo_escaneo, NULL);
                pthread_join(hilo_clasificacion, NULL);
                pthread_join(hilo_carga, NULL);
            }

            }
        } else {
            /* Proceso Facturación */
            printf("[Proceso Facturación] Comienzo mi ejecución...\n");
            int num_maleta = 0;

            char mensaje[TAM_MENSAJE];

            signal(SENAL_MALETA, avisar_pasajero); //cuando "Carga" avise de que la maleta esta embarcada, avisamos al pasajero

            char *vuelos[] = {"VUELO1", "VUELO2", "VUELO3"}; //simulamos distintos vuelos

            while (1) {

                sleep(tiempo_aleatorio(3, 7));
                printf("[Proceso Facturación] Facturada maleta nº %d. Enviando a clasificación...\n", num_maleta);
                
                sprintf(mensaje, "Maleta %d -- %s", num_maleta, vuelos[num_maleta % 3]); //defino lo que va dentro del mensaje --> Maleta (n)
	            
                int resultado = mq_send(cola, mensaje, strlen(mensaje) + 1, 0);
                if (resultado == -1) {
	 	            perror("[Facturación] Error al enviar el mensaje");
	                exit(1);
    		    }

                num_maleta++;

            }
        }

        exit(0);
    }
