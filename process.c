#include "headers.h"
#include "time.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    
    while ( (clock()/CLOCKS_PER_SEC) < atoi(argv[1]) )
    {
    
    }
    
    destroyClk(false);
    
    return 0;
}
