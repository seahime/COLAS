//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

#include "client.h"

#define DEBUG_MODE  0

extern int s_interrupted;

static clock_t start;
static clock_t finish;

void timer_start() {
	start = clock();
}

void timer_stop() {
	finish = clock();
}

clock_t get_time_inter() {
	return finish - start;
}

void send_multicast_servers(void *sock_to_servers, int num_servers, char *names[],  int n, ...) {
    RawData *rawdata;
    va_list valist;
    int i =0, j;

    va_start(valist, n);
    void **values = (void **)malloc(n*sizeof(void *));
    zframe_t **frames = (zframe_t **)malloc(n*sizeof(zframe_t *));
    assert(values!=NULL);
    assert(frames!=NULL);
    for(i=0; i < n; i++ ) {
        if( strcmp(names[i], OPNUM)==0)   {
            values[i] = (void *)va_arg(valist, unsigned  int *);
            frames[i]= zframe_new((const void *)values[i], sizeof(unsigned int));
        }
        else if( strcmp(names[i], PAYLOAD)==0)   {
            rawdata = va_arg(valist, RawData *);
            values[i] = rawdata;
            frames[i]= zframe_new(rawdata->data, rawdata->data_size);
        } else {
            values[i] = va_arg(valist, char *);
            frames[i]= zframe_new(values[i], strlen((char *)values[i]));
        }
    }
    va_end(valist);


    // it to all servers in a round robin fashion
    int rc;
    if(DEBUG_MODE)  printf("\n");
    if(DEBUG_MODE)  printf("\t\tsending %d ..\n", num_servers);
    for(i=0; i < num_servers; i++) {
        printf("\t\t\tserver : %d\n", i);
        for(j=0; j < n-1; j++) {
            if(DEBUG_MODE) {
                if( strcmp(names[j], OPNUM)==0)
                    printf("\t\t\tFRAME%d :%s  %u\n",j, names[j], *((unsigned int *)values[j]) );
                else if( strcmp(names[j], PAYLOAD)==0)
                    printf("\t\t\tFRAME%d :%s  %d\n", j, names[j],  ((RawData *)values[j])->data_size);
                else{
                    printf("\t\t\tFRAME%d :%s  %s\n", j, names[j],   (char *)values[j]);
		        }
	        }

             rc = zframe_send(&frames[j], sock_to_servers, ZFRAME_REUSE +  ZFRAME_MORE);
             if( rc < 0) {
                 printf("ERROR: %d\n", rc);
                 exit(EXIT_FAILURE);
             }

        }

        rc = zframe_send(&frames[j], sock_to_servers, ZFRAME_REUSE + ZFRAME_DONTWAIT);
        if( rc < 0) {
            printf("ERROR: %d\n", rc);
            exit(EXIT_FAILURE);
        }

        if(DEBUG_MODE) {
            if( strcmp(names[j], OPNUM)==0) {
                printf("\t\t\tFRAME%d :%s  %u\n", j, names[j],   *((unsigned int *)values[j]) );
            } else if( strcmp(names[j], PAYLOAD)==0) {
                printf("\t\t\tFRAME%d :%s  %d\n", j, names[j],  ((RawData *)values[j])->data_size);
            } else {
                printf("\t\t\tFRAME%d :%s  %s\n", j, names[j],   (char *)values[j]);
            }
        }
    }

    printf("\n");
    if(DEBUG_MODE)  printf("\n");


    for(i=0; i < n; i++ ) {
        zframe_destroy(&frames[i]);
    }
    if( frames!=NULL) {
	free(frames);
    }

    if( values!=NULL) {
	free(values);
    }
}

void send_multisend_servers(void *sock_to_servers,
                            int num_servers,
                            uint8_t **messages,
                            int msg_size,
                            char *names[],
                            int n, ...) {
    va_list valist;
    int i, j;

    void **values = malloc(n*sizeof(void *));
    assert(values!=NULL);

    zframe_t **frames = (zframe_t **)malloc( (n+1)*sizeof(zframe_t *));
    assert(frames!=NULL);

    va_start(valist, n);
    // for n arguments
    for(i=0; i < n; i++ ) {
        if( strcmp(names[i], OPNUM)==0)   {
            values[i] = (void *)va_arg(valist, unsigned  int *);
        } else {
            values[i] = va_arg(valist, void *);
        }

        if( strcmp(names[i], OPNUM)==0) {
            frames[i]= zframe_new((const void *)values[i], sizeof(unsigned int));
        } else {
            frames[i]= zframe_new(values[i], strlen((char *)values[i]));
        }
    }
    va_end(valist);

    // it to all servers in a round robin fashion
    if( DEBUG_MODE) printf("\tsending ..\n");
    for(i=0; i < num_servers; i++) {  // one server at a time
        for(j=0; j < n; j++) { //send the first n arguments
            if(DEBUG_MODE) {
                if( strcmp(names[j], OPNUM)==0)  {
                    printf("\t\t\tFRAME%d :%s  %d\n", j, names[j], *((unsigned int *)values[j]) );
                } else {
                    printf("\t\t\tFRAME%d :%s  %s\n", j, names[j], (char *)values[j]);
                }
            }
            zframe_send( &frames[j], sock_to_servers, ZFRAME_REUSE + ZFRAME_MORE);
        }
        // different coded element for different server
        frames[n]= zframe_new(messages[i], msg_size);
        assert( zframe_size(frames[n])==msg_size);
        if(DEBUG_MODE) printf("\t\t\tFRAME%d :%s  %d\n", n, PAYLOAD,  msg_size );
        zframe_send( &frames[n], sock_to_servers, 0);

        if(DEBUG_MODE)  printf("\n");
        zframe_destroy(&frames[n]);
    } //for a server end

    if( values!=NULL) free(values);

    for(i=0; i < n; i++ ) {
        zframe_destroy(&frames[i]);
    }
    if( frames!=NULL) free(frames);
}

zhash_t *receive_message_frames_at_client(zmsg_t *msg, zlist_t *names)  {
    char algorithm_name[BUFSIZE];
    char phase_name[BUFSIZE];

    zhash_t *frames = zhash_new();

    insertIntoHashAndList(SERVERID, msg, frames, names);
    insertIntoHashAndList(OBJECT, msg, frames, names);
    insertIntoHashAndList(ALGORITHM, msg, frames, names);
    get_string_frame(algorithm_name, frames, ALGORITHM);
    insertIntoHashAndList(PHASE, msg, frames, names);
    get_string_frame(phase_name, frames, PHASE);

    if( strcmp(algorithm_name, ABD) ==0 ) {

        if( strcmp(phase_name, GET_TAG) ==0 ) {
            insertIntoHashAndList(OPNUM, msg, frames, names);
            insertIntoHashAndList(TAG, msg, frames, names);
        }

        if( strcmp(phase_name, WRITE_VALUE) ==0 ) {
            insertIntoHashAndList(OPNUM, msg, frames, names);
            insertIntoHashAndList(TAG, msg, frames, names);
        }

        if( strcmp(phase_name, GET_TAG_VALUE) ==0 ) {
            insertIntoHashAndList(OPNUM, msg, frames, names);
            insertIntoHashAndList(TAG, msg, frames, names);
            insertIntoHashAndList(PAYLOAD, msg, frames, names);
        }
    }

    if( strcmp(algorithm_name, SODAW) ==0 ) {
        if( strcmp(phase_name, WRITE_GET) ==0 ) {
            insertIntoHashAndList(OPNUM, msg, frames, names);
            insertIntoHashAndList(TAG, msg, frames, names);
        }

        if( strcmp(phase_name, READ_GET) ==0 ) {
            insertIntoHashAndList(OPNUM, msg, frames, names);
            insertIntoHashAndList(TAG, msg, frames, names);
        }

        if( strcmp(phase_name, WRITE_PUT) ==0 ) {
            insertIntoHashAndList(OPNUM, msg, frames, names);
            insertIntoHashAndList(TAG, msg, frames, names);
        }

        if( strcmp(phase_name, READ_VALUE) ==0 ) {
            insertIntoHashAndList(TAG, msg, frames, names);
	     insertIntoHashAndList(OPNUM, msg, frames, names);
            insertIntoHashAndList(PAYLOAD, msg, frames, names);
        }
    }
    return frames;
}

void *get_socket_servers(ClientArgs *client_args) {
    int j;
    static int socket_create=0;
    static void *sock_to_servers =0;

    if( socket_create== 1) return sock_to_servers;

    int num_servers = count_num_servers(client_args->servers_str);
    char **servers =  create_server_names(client_args->servers_str);

    socket_create = 1;
    zctx_t *ctx  = zctx_new();
    sock_to_servers = zsocket_new(ctx, ZMQ_DEALER);
    assert (sock_to_servers);
    zsocket_set_identity(sock_to_servers,  client_args->client_id);

    for(j=0; j < num_servers; j++) {
        char *destination = create_destination(servers[j], client_args->port);
        int rc = zsocket_connect(sock_to_servers, (const char *)destination);
        assert(rc==0);
        free(destination);
    }

    destroy_server_names(servers, num_servers);
    return sock_to_servers;
}

void destroy_server_sockets(){
    /*if(socket_create == 1){
        zsocket_destroy(ctx, sock_to_servers);
        zctx_destroy(&ctx);
        socket_create = 0;
    }
    */
}

/*
void *get_md_socket_dealer(ClientArgs *client_args) {
    int j;
    static int socket_create=0;
    static void *sock_to_servers =0;

    if( socket_create==1) return sock_to_servers;

    int num_servers = count_num_servers(client_args->servers_str);
    char **servers = create_server_names(client_args->servers_str);

    socket_create=1;
    zctx_t *ctx  = zctx_new();
    sock_to_servers = zsocket_new(ctx, ZMQ_DEALER);
    assert (sock_to_servers);
    zsocket_set_identity(sock_to_servers,  client_args->client_id);

    for(j=0; j < num_servers; j++) {
        char *destination = create_destination(servers[j], client_args->port1);
        int rc = zsocket_connect(sock_to_servers, (const char *)destination);
        assert(rc==0);
        free(destination);
    }

    destroy_server_names(servers, num_servers);
    return sock_to_servers;
}
*/


