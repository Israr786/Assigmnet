#include<stdio.h>
#define MIN_REQUIRED 2


static int help() {

    fprintf(stderr,"Usage: loadGen [-s #] \n");
    fprintf(stderr,"\t-s: number of kilo bytes of data to generate : default value 1\n");
    return 1;
}

/* Our structure */
struct rec
{
    unsigned char array;
};

int main(int argc, const char * argv[])
{


    int num_kb = 1;
    int counter;
    //struct rec my_record;
    unsigned char array[8] = {0x00, 0x01,0x00, 0x01, 0x00, 0x01,0x00, 0x01};

    //temp variables
    int i = 0, j=0;

    //validate input parameters
    if (argc < MIN_REQUIRED)
    {
        return help();
    }

    for (i = 1; i < (argc - 1); i++)
    {
        if (strcmp("-s", argv[i]) == 0)
        {
            /* read the number of threads to use*/
            num_kb = atoi(argv[++i]);
            continue;
        }
        // display help message if the parameters do not match
        // the defined format.
        return help();
    }


    for (i = 0; i< num_kb; i++)
    {
        for ( counter=1; counter <= 125; counter++)
        {
            fwrite(array, sizeof(array), 1, stdout);
        }
    }

    return 0;
}

