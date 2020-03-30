//
//  main.c
//  assignment
//
//

#include "pt.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

#define MIN_REQUIRED 2


/* structure for global shared between threads */
typedef struct _thread_info_t
{
    unsigned char **maindata; // input data from the master thread
    unsigned char **buffer;   // data for the slaves to read from
    unsigned char **out_buffer;// location for slave output
    unsigned char ** key_chain;// ptr to the pre-generated key array
    int itter_count_set_by_master;
    int itter_count_read_by_slaves;
    int count;                // thread count
    int block_size;            // block size for the key
} thread_info_t;

static int help() {
    fprintf(stderr,"Usage: encryptUtil [-n #] [-k keyfile]\n");
    fprintf(stderr,"\t-n: number of worker threads: default value 1\n");
    fprintf(stderr,"\t-k: name of the file with the key: MANDATORY FIELD\n");
    return 1;
}

/* accept an unsigned char array, with size and rotate it left by one 1 position*/
static void rotate_left( unsigned char * input, int size)
{
    int i, carry_back= 0;
    
    if(input[size-1] & (1<<(CHAR_BIT - 1)))
    {
        carry_back = 1;
    }
    
    for(i= size-1; i>=0; i--)
    {
        unsigned char carry = 0;
        if(i!= 0 && (input[i-1] & (1<<(CHAR_BIT - 1))))
        {
            carry = 1;
        }
        input[i] <<= 1;
        input[i] |= carry;
    }
    
    if(carry_back == 1)
    {
        input[0] |= 0x01;
    }
    
}

/* returns the size of the file*/
long get_file_size(FILE * fd)
{
    long size = 0;
    //read the blockSize
    fseek(fd, 0L, SEEK_END);
    size = ftell(fd);
    //move back to the start of the file
    fseek(fd, 0L, SEEK_SET);
    return size;
    
}

/** Code executed by indevedual worker threads
 * Lookup the key based on the current intteration count and the thread id
 */
pt_addr_t thread_code(pt_arg_t *arg)
{
    int id=pt_myid(arg), i;
    thread_info_t* data = (thread_info_t*)pt_data(arg);
    int block_size = data->block_size;
    int key_chain_index = ((data->itter_count_read_by_slaves * data->count)+id) % (block_size * CHAR_BIT);

    for(i=0; i<block_size; i++)
    {
        data->out_buffer[id][i] = data->buffer[id][i] ^ data->key_chain[key_chain_index][i];
    }
    
    return(NULL);
}

/**
 *  master does this before dispatcher executes
 *  Copy the data into the buffer that the threads will use for processing
 *  This is done so that the master can keep copying new data into "maindata" buffer
 *  while the dispatcher sends data from "buffer" to the workers for processing
 * 
 *  This is the main part of the pipeline
 */
void *setup(thread_info_t *info)
{
    unsigned char **src=info->maindata,**dst=info->buffer;
    int thread_count=info->count, block_size = info->block_size, i, j;
    info->itter_count_read_by_slaves = info->itter_count_set_by_master;
    /* copy main program's data info I/O buffer */
    for (i=0; i<thread_count; i++)
        for( j=0; j<block_size; j++)
            dst[i][j]=src[i][j];
    
    
    return(NULL);
}

/** dispatcher runs this:
 * Spawn thread count number of threads and let them do the XOR operation
 * Wait for them to complete the interation and print to stdout
 */
void *dispatcher_duty(thread_info_t *info)
{
    unsigned char **buf=info->out_buffer;
    
    //unsigned char **buf=info->buffer;
    int thread_count=info->count, block_size = info->block_size, i, j;
    pt_fork(thread_count,thread_code,info,NULL); /* run code in parallel */

    for (i=0; i<thread_count; i++)
        for( j=0; j<block_size; j++)
            printf("%c",buf[i][j]);
    
   // printf("\ncurrent itter -> %d\n", info->itter_count_read_by_slaves);
    return(NULL);
}

#if DEBUG_ENABLE
static void show(const char *object, size_t size)
{
    int i, j;
    for(i=size-1; i>=0; i--)
    {
        for(j =0; j<CHAR_BIT; j++)
        {
            if(1 << ((CHAR_BIT-1)-j) & object[i])
            {
                putchar('1');
                
            }
            else
            {
                putchar('0');
            }
            
        }
        putchar(' ');
    }
    putchar('\n');  
    
}
#endif



int main(int argc, const char * argv[])
{
    int thread_count = 1; // default value;
    int block_size = 0;
    FILE *key_file_fd = NULL;   // FD for the keyFile
    char *key_file_name = NULL;
    unsigned char **input_data = NULL;  // buffer to read the data from stdin
    int input_char = 0;
    
    thread_info_t info;
    pt_pipeline_t p;

    //temp variables
    int i = 0, j=0;


    //time calculation
    clock_t begin, end;
    double time_spent;
    begin = clock();
    

    /*********************validate input parameters***************************/

    if (argc < MIN_REQUIRED)
    {
        return help();
    }
    
    for (i = 1; i < (argc - 1); i++)
    {
        if (strcmp("-k", argv[i]) == 0)
        {
            /* read the name of the keyfile */
            key_file_name = (char *)argv[++i];
            continue;
        }
        if (strcmp("-n", argv[i]) == 0)
        {
            /* read the number of threads to use*/
            thread_count = atoi(argv[++i]);
            continue;
        }
        // display help message if the parameters do not match
        // the defined format.
        return help();
    }
    
    //Key File is mandatory
    if (key_file_name == NULL)
    {
        return help();
    }
    
    fprintf(stderr,"thread count = %i\n", thread_count);
    fprintf(stderr,"file name = %s\n", key_file_name);
    


    //read the key out of the file
    key_file_fd=fopen(key_file_name,"rb");
    
    if (!key_file_fd)
    {
        fprintf(stderr,"\nUnable to open key file!\n");
        return 1;
    }
    
    if((block_size = get_file_size(key_file_fd)) == 0)
    {
        fprintf(stderr,"\nkey size is 0 : in order to encrypt the data a key is mandatory\n"); 
        return 1;
    } 

    fprintf(stderr,"block size = %d\n", block_size);
    
    //allocate momory for the key
    unsigned char * key = (unsigned char*) malloc(sizeof(unsigned char)*block_size);

    //read the key from the file
    fread(key,block_size,1,key_file_fd);

    int dirty_value = 0;
    for(i=0; i<block_size; i++)
    {
       if(key[i] != 0x00 )
       {
           dirty_value = 1;
           break;
       }
    }

    if(!dirty_value)
    {
        fprintf(stderr,"\nkey value is 0 : in order to encrypt the data key should be non-zero\n"); 
        //return 1; 
    }


    /********************************* Generate All Keys Ahead OF Time **************************************/
    int out_of_memory_flag = 0;
    unsigned char ** key_chain = (unsigned char **) malloc(sizeof(unsigned char *)* (CHAR_BIT*block_size));
    if(key_chain == NULL)
    {
        fprintf(stderr, "\nOUT OF MEMORY\n");
        return 1;
    }
    
    for(i = 0; i <block_size * CHAR_BIT; i++)
    {
        key_chain[i] = (unsigned char*) malloc(block_size);
        if(key_chain[i] == NULL)
        {
            fprintf(stderr, "\nOUT OF MEMORY\n");
            return 1;
        }
    }
    
    //generate all possible keys ahead of time (key chain)
    i=0;
    do
    {
        memcpy(key_chain[i],key,block_size);
        //show(key_chain[i],block_size);
        rotate_left(key,block_size);
        i++;
        
    }while(i < (block_size * CHAR_BIT) );

    /*******************Allocate Memory For The Data Structures Shared By All Threads************************/
    
    /* initialize info structure*/
    info.buffer=(unsigned char **) malloc(thread_count*sizeof(unsigned char*));
    for(j=0; j<thread_count ; j++)
    {
        info.buffer[j] = (unsigned char *) malloc(block_size * sizeof(unsigned char));
        
    }
    info.out_buffer = (unsigned char **) malloc(thread_count*sizeof(unsigned char*));
    for(j=0; j<thread_count ; j++)
    {
        info.out_buffer[j] = (unsigned char *) malloc(block_size * sizeof(unsigned char));
        
    }
    info.maindata=input_data=(unsigned char **) malloc(thread_count*sizeof(unsigned char*));
    for(j=0; j<thread_count ; j++)
    {
        info.maindata[j] = (unsigned char *) malloc(block_size * sizeof(unsigned char));
        
    }
    info.count=thread_count;
    info.key_chain = key_chain;
    info.block_size = block_size;
    
    /**
     * SETUP PIPELINE
     * 
     * I chose to take the pipeline approach because we have two IO opetaions
     * 1> Read the data from STDIN
     * - Then perform the XOR operation using worker threads 
     * 2> Write the data to STDOUT
     * 
     * Pipelining can be benifitial if the IO operations are slow
     * 
     * Pipline function consists of a Master and a Dispatcher
     * 
     * Master reads the data from STDIN into the buffer (info.mainbuffer) when the data is ready
     * it signals the dispatcher by calling the "setup" method.
     * 
     * Setup method copies the data from (info.mainbuffer)  to (info.buffer) and releases the master 
     * to read more data into info.mainbuffer, and signals the dispatcher to process data from info.buffer
     * 
     * The dispatcher runs the "dispatcher_duty" method which spawns thread_count number of worker threads 
     * that process the data from info.buffer, and copy their partial outputs into info.out_buffer.
     * dispatcher waits for all the threads to complete and writes the output to STDOUT
     * 
     * p                :Identifies the instance of the pipeline
     * info             :Global data structure instance that is used by the pipeline
     * setup            :Method called by the master to prepare the data for the dispatcher to consume
     * dispatcher_duty  :This is the method that is called by the dispatcher thread
     * 
     */


    pt_pipeline_init(&p,     /* pt_pipeline_t struct */
                     &info,  /* global (shared) data */
                     setup,  /* setup routine called by master*/
                     dispatcher_duty);  /* Dispatchers's code */
    
    


    /********************************** Partition The Data **************************************************/

    /**
     * Start reading the data from stdin into the maindata buffer 
     * maindata buffer is a 2D of size ( thread_count x block_size ) we distribute the data evenly between all threads
     * 
     * When the buffer is full call pt_pipeline_execute function which calles the setup function to pass the data to the
     * dispatcher.
     * 
     * */
    int current_index=0;
    int current_itteration = info.itter_count_set_by_master = info.itter_count_read_by_slaves = 0;
    while((input_char = fgetc(stdin)) != EOF)
    {
        int x = current_index/block_size;
        int y = current_index%block_size;
        info.maindata[x][y] = (unsigned char) input_char;
        if((x) == (thread_count - 1) && (y) == (block_size - 1))
        {
            // if we enter this then we have filled up the input array
            // so send the data to dispatcher.

            pt_pipeline_execute(&p);
            current_index=-1;
            info.itter_count_set_by_master++;
        }
        current_index++;
        
    }
    
    /* clean up and exit */
    pt_pipeline_destroy(&p);

    /******************************** Process Spill Over Data **************************************************/
    
    int key_chain_index_base = (info.itter_count_set_by_master* info.count) % (info.block_size * CHAR_BIT);
    int key_chain_index_current = 0, m, n;

    j=0;
    for(m = 0; m< thread_count && j< current_index ; m++)
    {
        key_chain_index_current = key_chain_index_base+ m;
        
        if(key_chain_index_current >= (info.block_size * CHAR_BIT) )
        {
            key_chain_index_current %= (info.block_size * CHAR_BIT);
        }
        
        for(n = 0; n< block_size && j< current_index; n++)
        {
            
            printf("%c", info.maindata[m][n] ^ info.key_chain[key_chain_index_current][n]);
            j++;
        }
    }

    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    fprintf(stderr,"\nRun Time = %f\n",time_spent );

    
    return 0;
}

