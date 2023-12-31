#include "main.h"
#include "watek_komunikacyjny.h"

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */

void checkCriticalSectionCondition() {
    if (getFirstSource(&requestQueue) == rank && ackCount == size) {
        debug("Mogę wejść")
        printRequestQueue(&requestQueue);
        canEnter = 1;
    }
}

void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while ( stan!=InFinish ) {
	    debug("czekam na recv");
        
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        pthread_mutex_lock(&clockMutex);
        lamport = max(lamport, pakiet.ts)+1;
        pthread_mutex_unlock(&clockMutex);

        switch ( status.MPI_TAG ) {
	    case REQUEST:
        {
            add(&requestQueue, status.MPI_SOURCE, pakiet.ts);
            debug("Ktoś coś prosi. A niech ma!")
		    sendPacket( 0, status.MPI_SOURCE, ACK );
            break;
        }
	    case ACK:
        {
	        ackCount++; /* czy potrzeba tutaj muteksa? Będzie wyścig, czy nie będzie? Zastanówcie się. */
            debug("Dostałem ACK od %d, mam już %d", status.MPI_SOURCE, ackCount);
            checkCriticalSectionCondition();
	        break;
        }
        case RELEASE:
        {
            removeItem(&requestQueue, status.MPI_SOURCE);
            debug("Dostałem RELEASE od %d", status.MPI_SOURCE);
            checkCriticalSectionCondition();
            break;
        }
	    default:
	    break;
        }
    }
}
