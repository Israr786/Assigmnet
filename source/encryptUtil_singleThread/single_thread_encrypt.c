//
//  main.c
//  assignment
//
//

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

#define MIN_REQUIRED 2

static int help() {
    fprintf(stderr,"Usage: encryptUtil_1_thread [-k keyfile]\n");
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
int main(int argc, const char * argv[])
{
    int block_size = 0;
    FILE *key_file_fd = NULL;
    char *key_file_name = NULL;
    unsigned char *input_data = NULL;
    int input_char = 0;


    //temp variables
    int i = 0, j=0;


    //time calculation
    clock_t begin, end;
    double time_spent;
    begin = clock();
    
    //validate input parameters
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
        // display help message if the parameters do not match
        // the defined format.
        return help();
    }
    
    //Key File is mandatory
    if (key_file_name == NULL)
    {
        return help();
    }
    
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
    
    unsigned char ** key_chain = (unsigned char **) malloc(sizeof(unsigned char *)* (CHAR_BIT*block_size));
    if(key_chain == NULL)
    {
        fprintf(stderr, "\nOUT OF MEMORY\n");
        return 1;
    }
    
    for(i = 0; i <block_size * CHAR_BIT; i++)
    {
        key_chain[i] = (unsigned char*) malloc(sizeof(unsigned char) * block_size);
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
        rotate_left(key,block_size);
        i++;
        
    }while(i < (block_size * CHAR_BIT) );


    //Array to read one block_size of data 
    input_data = (unsigned char *) malloc(block_size * sizeof(unsigned char));
        

    // start reading the data from stdin and process it in blocks (block_size)
    int current_count = 0;
    int key_chain_index = 0;

    while((input_char = fgetc(stdin)) != EOF)
    {
       input_data[current_count] = (unsigned char) input_char;

       if(current_count == (block_size-1))
       {
           for(i=0; i<block_size ; i++)
           {
               printf("%c", input_data[i] ^ key_chain[key_chain_index][i]);

           }
           key_chain_index++;

           if(key_chain_index >= (block_size*CHAR_BIT))
           {
             key_chain_index = 0;
           }

           current_count = -1;

       }
       current_count++; 
        
    }

    // handle the spill over
    for(i=0; i<current_count ; i++)
    {
       printf("%c", input_data[i] ^ key_chain[key_chain_index][i]);
    }

    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    fprintf(stderr,"\nRun Time = %f\n",time_spent );

    
    return 0;





}
