#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "tree.h"
#include "list.h"
#include "exec.h"
#include <signal.h>

extern int a;
extern int err;
extern long * pids_backgrn;

int main(int argc, char** argv) {
    char ** list;
    tree t;
    while (1 && (a!=0)) {
        signal(SIGINT,SIG_IGN);
        printf("=> ");
        fflush(stdout);
        err = 0;
        list = createlist(&a);
        t = build_tree(list);
        check_backgrn();
        if (t != NULL) execute(t);
        clear_tree(t);
        clearlist(list);
    }
    if (pids_backgrn != NULL) free(pids_backgrn);
    fclose(stdout);
}

