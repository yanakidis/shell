#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "tree.h"
#include "list.h"
#include <signal.h>

long * pids_nobackgrn = NULL; /* список процессов без & */
int size_nobackgrn = 0;
int cur_nobackgrn = -1;
long * pids_backgrn = NULL; /* с & */
int size_backgrn = 0;
int cur_backgrn = -1;

void execute(tree);
int depth_conv(tree);
void exec_conv(tree,int);
void add_pid(int);
void write_pid(int,int);
void clear_nobackgrn(void);
void check_backgrn(void);
void error_fork(void);
int not_empty_backgrn(void);

int a = 1; /* для EOF */
extern int err; /* признак ошибки */

//int main(int argc, char** argv) {
//    char ** list;
//    tree t;
//    while (1 && (a!=0)) {
//        printf("=> ");
//        fflush(stdout);
//        err = 0;
//        list = createlist(&a);
//        //printlist(list);
//        t = build_tree(list);
//        //print_tree(t,5);
//        if (t != NULL) execute(t);
//        clear_tree(t);
//        clearlist(list);
//    }
//    if (pids_backgrn != NULL) free(pids_backgrn);
//    fclose(stdout);
//}

void execute(tree t) {
    int depth; /* кол-во процессов в конвейере */
    do {
        depth = depth_conv(t);
        exec_conv(t,depth);
        clear_nobackgrn();
    } while ((t = t->next) != NULL);
}

int depth_conv(tree t) { /* вычисляет сколько процессов в конвейере */
    if (t->pipe != NULL)
        return 1 + depth_conv(t->pipe);
    else return 1;
}

void exec_conv(tree t, int depth) {
    int fd[2], in, out, next_in, i;
    int pid;
    if (depth == 1) { /* если одна команда в конвейере */
        if ((strcmp( (t->argv)[0],"cd") == 0) || (strcmp( (t->argv)[0],"exit") == 0)) {
            if (strcmp( (t->argv)[0],"exit")) {
                if ((t->argv)[1] == NULL) {
                    if (chdir(getenv("HOME"))) error();
                } else {
                    if (chdir( (t->argv)[1]) ) error();
                }
            } else {
                a=0;
            }
        } else { /* если не внутренняя команда */
            int fd1 = -1;
            int fd2 = -1;
            if (t->outfile != NULL) {
                if (t->append) fd1 = open(t->outfile, O_CREAT | O_WRONLY | O_APPEND, 0666);
                else fd1 = open(t->outfile, O_CREAT | O_TRUNC | O_WRONLY, 0666);
                if (fd1 == -1) {
                    error();
                    return;
                }
            }
            if (t->infile != NULL) {
                fd2 = open(t->infile, O_RDONLY);
                if (fd2 == -1) {
                    error();
                    return;
                }
            }
            pid = fork();
            if (pid) {
                if (fd1 != -1) close(fd1);
                if (fd2 != -1) close(fd2);
                if (t->backgrn == 0) {
                    add_pid(0);
                    if (err == 1) return;
                    write_pid(0,pid);
                    while (depth) {
                        long * pids = pids_nobackgrn;
                        while (*pids != -2) {
                            if (*pids != -1) {
                                waitpid(*pids,NULL,0);
                                *pids = -1;
                                depth--;
                            }
                            pids++;
                        }
                    }
                }
                else {
                    if (!not_empty_backgrn()) {
                        add_pid(1);
                        if (err == 1) return;
                    }
                    write_pid(1,pid);
                }
            } else if (pid == 0) {
                if (t->backgrn == 1) {
                    printf("Started %d\n",getpid());
                } else signal(SIGINT,SIG_DFL);
                if (fd1 != -1) {
                    dup2(fd1,1);
                    close(fd1);
                }
                if (fd2 != -1) {
                    dup2(fd2,0);
                    close(fd2);
                }
                if (t->backgrn == 1) {
                    if (fd2 == -1) { /* закроем рот фоновому процессу */
                        int fd3;
                        fd3 = open("/dev/null",O_RDONLY);
                        dup2(fd3,0);
                        close(fd3);
                    }
                }
                execvp(t->argv[0],t->argv);
                error();
                exit(-1);
            } else {
                error_fork();
            }
        }
    } else { /* если две и более команды в конвейере */
        tree posl = t;
        if ((strcmp( (posl->argv)[0],"cd") == 0) || (strcmp( (posl->argv)[0],"exit") == 0)) {
            if (strcmp( (posl->argv)[0],"exit")) {
//                if (dep--) posl = posl->pipe; /* переходим на следующую ветку, если cd */
                posl = NULL;
            } else {
                a=0;
                posl = NULL;
            }
        } else { /* если не внутренняя команда */
            if (pipe(fd) != -1) return;
            out = fd[1];
            next_in = fd[0];
            int fd1 = -1;
            int fd2 = -1;
            if (posl->outfile != NULL) {
                if (posl->append) fd1 = open(posl->outfile, O_CREAT | O_WRONLY | O_APPEND, 0666);
                else fd1 = open(posl->outfile, O_CREAT | O_TRUNC | O_WRONLY, 0666);
                if (fd1 == -1) {
                    error();
                    return;
                }
            }
            if (posl->infile != NULL) {
                fd2 = open(posl->infile, O_RDONLY);
                if (fd2 == -1) {
                    error();
                    return;
                }
            }
            pid = fork();
            if (pid) { /* отцовский процесс */
                if (fd1 != -1) close(fd1);
                if (fd2 != -1) close(fd2);
                if (posl->backgrn == 0) {
                    add_pid(0);
                    if (err == 1) return;
                    write_pid(0,pid);
                } else {
                    if (!not_empty_backgrn()) {
                        add_pid(1);
                        if (err == 1) return;
                    }
                    write_pid(1,pid);
                }
                in = next_in;
                posl = posl->pipe;
                int i;
                for (i=1; i<depth-1; i++) {
                    close(out);
                    if ((strcmp( (posl->argv)[0],"cd") == 0) || (strcmp( (posl->argv)[0],"exit") == 0)) {
                        if (strcmp( (posl->argv)[0],"exit")) {
                            if (posl!=NULL) {
//                                posl = posl->pipe; /* переходим на следующую ветку, если cd */
//                                i++;
                                posl = NULL;
                                break;
                            }
                        } else {
                            a=0;
                            posl = NULL;
                            break;
                        }
                    } else { /* если не внутренняя команда */
                        if (pipe(fd) != -1) return;
                        out=fd[1];
                        next_in=fd[0];
                        int fd1 = -1;
                        int fd2 = -1;
                        if (posl->outfile != NULL) {
                            if (posl->append) fd1 = open(posl->outfile, O_CREAT | O_WRONLY | O_APPEND, 0666);
                            else fd1 = open(posl->outfile, O_CREAT | O_TRUNC | O_WRONLY, 0666);
                            if (fd1 == -1) {
                                error();
                                return;
                            }
                        }
                        if (posl->infile != NULL) {
                            fd2 = open(posl->infile, O_RDONLY);
                            if (fd2 == -1) {
                                error();
                                return;
                            }
                        }
                        pid = fork();
                        if (pid) {
                            if (fd1 != -1) close(fd1);
                            if (fd2 != -1) close(fd2);
                            if (t->backgrn == 0) {
                                add_pid(0);
                                if (err == 1) return;
                                write_pid(0,pid);
                            }
                            else {
                                if (!not_empty_backgrn()) {
                                    add_pid(1);
                                    if (err == 1) return;
                                }
                                write_pid(1,pid);
                            }
                            close(in);
                            in=next_in;
                            if (posl!=NULL) posl = posl->pipe;
                        } else if (pid == 0) {
                            if (posl->backgrn == 1) {
                                printf("Started %d\n",getpid());
                            } else signal(SIGINT,SIG_DFL);
                            close(next_in);
                            if (fd1 != -1) {
                                dup2(fd1,1);
                                close(fd1);
                            } else {
                                dup2(out,1);
                            }
                            close(out);
                            if (fd2 != -1) {
                                dup2(fd2,0);
                                close(fd2);
                            } else {
                                dup2(in,0);
                            }
                            close(in);
                            execvp(posl->argv[0],posl->argv);
                            error();
                            exit(-1);
                        } else {
                            error_fork();
                        }
                    }
                }
                close(out);
                if (posl == NULL) {
                    while (i) {
                        long * pids = pids_nobackgrn;
                        while (*pids != -2) {
                            if (*pids != -1) {
                                waitpid(*pids,NULL,0);
                                *pids = -1;
                                i--;
                            }
                            pids++;
                        }
                    }
                    return;
                }
                else {
                    if ((strcmp( (posl->argv)[0],"cd") == 0) || (strcmp( (posl->argv)[0],"exit") == 0)) {
                        if (strcmp( (posl->argv)[0],"exit")) {
                            if (posl!=NULL) {
                                posl = posl->pipe; /* переходим на следующую ветку, если cd */
                            }
                        } else {
                            a=0;
                            posl = NULL;
                        }
                        while (depth-1) {
                            long * pids = pids_nobackgrn;
                            while (*pids != -2) {
                                if (*pids != -1) {
                                    waitpid(*pids,NULL,0);
                                    *pids = -1;
                                    depth--;
                                }
                                pids++;
                            }
                        }
                    } else { /* если не внутренняя команда */
                        int fd1 = -1;
                        int fd2 = -1;
                        if (posl->outfile != NULL) {
                            if (posl->append) fd1 = open(posl->outfile, O_CREAT | O_WRONLY | O_APPEND, 0666);
                            else fd1 = open(posl->outfile, O_CREAT | O_TRUNC | O_WRONLY, 0666);
                            if (fd1 == -1) {
                                error();
                                return;
                            }
                        }
                        if (posl->infile != NULL) {
                            fd2 = open(posl->infile, O_RDONLY);
                            if (fd2 == -1) {
                                error();
                                return;
                            }
                        }
                        pid = fork();
                        if (pid) {
                            if (fd1 != -1) close(fd1);
                            if (fd2 != -1) close(fd2);
                            if (t->backgrn == 0) {
                                add_pid(0);
                                if (err == 1) return;
                                write_pid(0,pid);
                            }
                            else {
                                if (!not_empty_backgrn()) {
                                    add_pid(1);
                                    if (err == 1) return;
                                }
                                write_pid(1,pid);
                            }
                            close(in);
                            while (depth) {
                                long * pids = pids_nobackgrn;
                                if (pids == NULL) break;
                                else {
                                    while (*pids != -2) {
                                        if (*pids != -1) {
                                            waitpid(*pids,NULL,0);
                                            *pids = -1;
                                            depth--;
                                        }
                                        pids++;
                                    }
                                }
                            }
                        } else if (pid == 0) {
                            if (posl->backgrn == 1) {
                                printf("Started %d\n",getpid());
                            } else signal(SIGINT,SIG_DFL);
                            if (fd1 != -1) {
                                dup2(fd1,1);
                                close(fd1);
                            }
                            if (fd2 != -1) {
                                dup2(fd2,0);
                                close(fd2);
                            } else {
                                dup2(in,0);
                            }
                            close(in);
                            execvp(posl->argv[0],posl->argv);
                            error();
                            exit(-1);
                        } else {
                            error_fork();
                        }
                    }
                    
                }
            } else if (pid == 0) { /* сыновний процесс */
                if (posl->backgrn == 1) {
                    printf("Started %d\n",getpid());
                } else signal(SIGINT,SIG_DFL);
                close(next_in);
                if (fd1 != -1) {
                    dup2(fd1,1);
                    close(fd1);
                } else {
                    dup2(out,1);
                }
                close(out);
                if (fd2 != -1) {
                    dup2(fd2,0);
                    close(fd2);
                }
                if (posl->backgrn == 1) {
                    if (fd2 == -1) { /* закроем рот фоновому процессу */
                        int fd3;
                        fd3 = open("/dev/null",O_RDONLY);
                        dup2(fd3,0);
                        close(fd3);
                    }
                }
                execvp(posl->argv[0],posl->argv);
                error();
                exit(-1);
            } else {
                error_fork();
            }
        }
    }
}

void add_pid(int p) { /* добавляет в массив (0 - без &, 1 - с &) пидов новую ячейку */
    long ** pids;
    int * cur;
    int * size;
    if (p == 0) {
        pids = &pids_nobackgrn;
        cur = &cur_nobackgrn;
        size = &size_nobackgrn;
    } else {
        pids = &pids_backgrn;
        cur = &cur_backgrn;
        size = &size_backgrn;
    }
    (*size)++;
    (*cur)++;
    long ** tmp = pids;
    *tmp = realloc(*pids, (*size+1) * sizeof(long));
    if (*tmp == NULL) {
        error_memory();
        return;
    } else pids = tmp;
    (*pids)[*cur] = -1; /* -1, значит ячейка доступна для записи PID процесса*/
    (*pids)[*cur + 1] = -2; /* -2, значит конец списка */
}

void write_pid(int p, int pid) {
    long * pids;
    if (p==0) {
        pids = pids_nobackgrn;
    } else {
        pids = pids_backgrn;
    }
    do {
        if (*pids == -1) {
            *pids = pid;
            break;
        } else {
            pids++;
        }
    } while (1);
}

void clear_nobackgrn() { /* очищает список pids_nobackgrn */
    free(pids_nobackgrn);
    pids_nobackgrn = NULL;
    size_nobackgrn = 0;
    cur_nobackgrn = -1;
}

void check_backgrn() { /* проходится по всему списку фоновых процессов и очищает в случае завершения */
    long * pids = pids_backgrn;
    if (pids != NULL) {
        while (*pids != -2) {
            if (*pids != -1) {
                int n;
                if ((n = waitpid(*pids,NULL,WNOHANG))) {
                    printf("Done %d\n", n);
                    *pids = -1;
                }
            }
            pids++;
        }
    }
}

void error_fork() {
    fprintf(stderr,"error fork\n");
    err = 1;
}

int not_empty_backgrn() { /* проверяет наличие свободной ячейки в массиве процессов с & */
    long * pids = pids_backgrn;
    if (pids != NULL) {
        while (*pids != -2) {
            if (*pids == -1) return 1;
            pids++;
        }
        return 0;
    } else return 0;
}


