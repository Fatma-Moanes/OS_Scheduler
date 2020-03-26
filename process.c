#include "Headers/headers.h"
#include "time.h"

int main(int agrc, char *argv[]) {

    initClk();

    int runtime = atoi(argv[1]);
    while ((clock() / CLOCKS_PER_SEC) <= runtime);

    destroyClk(false);

    return 0;
}