#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#include "list.h"

static char ** plex; /* указатель текущей лексемы */
static tree make_cmd(void); /* создаёт дерево из одного элемента, обнуляет все поля */
static void make_bgrnd (tree); /* устанавливает после bckgrnd=1 во всех командах конвейера t */
int add_argv(tree); /* добавляет очередной элемент в массив argv текущей команды */
static int cmp_notsymset(char *); /* != 0, если параметр "&","|",">",">>","<",";" */
static void term_argv(tree); /* дописывает в конце argv NULL */
extern int err; /* признак ошибки */
void error(void);
tree build_tree(char **);

//int main(int argc, char** argv) {
//    char ** list;
//    int a = 1; /* для EOF */
//    tree t;
//    while (1 && (a!=0)) {
//        err = 0;
//        list = createlist(&a);
//        printlist(list);
//        t = build_tree(list);
//        print_tree(t,5);
//        clear_tree(t);
//        clearlist(list);
//    }
//    fclose(stdout);
//}

tree build_tree(char ** lst) {
    tree beg_cmd; /* начальная команда */
    tree cur_cmd; /* текущая команда */
    tree prev_cmd; /* предыдущая команда */
    tree beg_pipe; /* начальная команда конвейера */
    typedef enum {begin, conv, conv1, _in, _in1, _out, _out1, backgrn, next, end} vertex; /* next - ; */
    plex = lst;
    int nom;
    vertex V = begin;
    vertex V_prev = next;
    while (1) switch(V) {
        case begin:
            beg_pipe = beg_cmd = cur_cmd = make_cmd();
            if (cur_cmd == NULL) {
                V = end;
                break;
            }
            if ((plex != NULL) && (*plex != NULL))
                if (!cmp_notsymset(*plex)) {
                    if (add_argv(cur_cmd) == 0) {
                        V = end;
                        break;
                    }
                    prev_cmd = cur_cmd;
                    V = conv;
                } else {
                    error();
                    V = end;
                }
            else {
                if (plex != NULL) {
                    term_argv(cur_cmd);
                    V = end;
                }
                clear_tree(beg_cmd);
                return NULL;
            }
            break;
        case conv:
            plex++;
            if ((*plex != NULL) && !(nom = cmp_notsymset(*plex))) {
                if (add_argv(cur_cmd) == 0) {
                    V = end;
                    break;
                }
            }
            else if (*plex == NULL) {
                term_argv(cur_cmd);
                V=end;
            } else {
                switch(nom) {
                    case 1:
                        V = backgrn;
                        if (V_prev == next || V_prev == backgrn) beg_pipe = cur_cmd;
                        break;
                    case 2:
                        V = conv1;
//                        term_argv(cur_cmd);
                        if (V_prev == next) beg_pipe = cur_cmd;
                        break;
                    case 3:  case 4:
                        V = _out;
                        break;
                    case 5:
                        V = _in;
                        break;
                    case 6:
                        term_argv(cur_cmd);
                        V = next;
                        break;
                }
            }
            break;
        case conv1:
            plex++;
            if ((*plex != NULL) && !cmp_notsymset(*plex)) {
                term_argv(cur_cmd);
                cur_cmd = make_cmd();
                if (cur_cmd == NULL) {
                    V = end;
                    break;
                }
                if (add_argv(cur_cmd) == 0) {
                    V = end;
                    break;
                }
                prev_cmd->pipe = cur_cmd;
                prev_cmd = cur_cmd;
                V_prev = V;
                V = conv;
            } else {
                error();
                V = end;
            }
            break;
        case _in:
            plex++;
            if ((*plex != NULL) && !cmp_notsymset(*plex)) {
                cur_cmd -> infile = *plex;
                V_prev = V;
                V = _in1;
            }
            else {
                error();
                V = end;
            }
            break;
        case _in1:
            if (!strcmp(*plex,">")) {
                nom = 3;
                V_prev = V;
                V = _out;
            } else if (!strcmp(*plex,">>")) {
                nom = 4;
                V_prev = V;
                V = _out;
            } else {
                V_prev = V;
                V = conv;
            }
            break;
        case _out:
            plex++;
            if ((*plex != NULL) && !cmp_notsymset(*plex)) {
                cur_cmd -> outfile = *plex;
                if (nom == 4) cur_cmd->append = 1;
                if (nom == 3) cur_cmd->append = 0;
                V_prev = V;
                V = _out1;
            }
            else {
                error();
                V = end;
            }
            break;
        case _out1:
            if (!strcmp(*plex,"<")) {
                V_prev = V;
                V = _in;
            } else {
                V = conv;
                V_prev = V;
            }
            break;
        case next:
            plex++;
            if (*plex != NULL) {
                if (!cmp_notsymset(*plex)) {
                    term_argv(cur_cmd);
                    cur_cmd = make_cmd();
                    if (cur_cmd == NULL) {
                        V = end;
                        break;
                    }
                    if (add_argv(cur_cmd) == 0) {
                        V = end;
                        break;
                    }
//                    prev_cmd->next = cur_cmd;
                    beg_pipe->next = cur_cmd;
                    beg_pipe = prev_cmd = cur_cmd;
                    V_prev = V;
                    V = conv;
                } else {
                    error();
                    V = end;
                }
            } else {
                V = end;
            }
            break;
        case backgrn:
            plex++;
            if (*plex != NULL) {
                if (!cmp_notsymset(*plex)) {
                    term_argv(cur_cmd);
                    make_bgrnd(beg_pipe);
                    cur_cmd = make_cmd();
                    if (cur_cmd == NULL) {
                        V = end;
                        break;
                    }
                    if (add_argv(cur_cmd) == 0) {
                        V = end;
                        break;
                    }
                    prev_cmd->next = cur_cmd;
                    prev_cmd = cur_cmd;
                    V_prev = V;
                    V = conv;
                } else {
                    error();
                    V = end;
                }
            } else {
                make_bgrnd(beg_pipe);
                term_argv(cur_cmd);
                V_prev = V;
                V = end;
            }
            break;
        case end:
            if (err) {
                clear_tree(beg_cmd);
                return NULL;
            }
            else return beg_cmd;
    }
}

static tree make_cmd() {
    tree object;
    object=malloc(sizeof(*object));
    if (object == NULL) {
        error_memory();
        return NULL;
    }
    object->argv = NULL;
    object->argc = 0;
    object->infile = NULL;
    object->append = 0;
    object->outfile = NULL;
    object->backgrn = 0;
    object->pipe = NULL;
    object->next = NULL;
    return object;
}

int add_argv(tree object) {
    object->argc++;
    tree tmp = object;
    tmp->argv = realloc(object->argv,(object->argc+1)*sizeof(char*));
    if (tmp == NULL) {
        error_memory();
        return 0;
    } else object = tmp;
    object->argv[object->argc-1] = *plex;
    return 1;
}

static int cmp_notsymset(char * word) {
    if (!strcmp(word,"&")) return 1;
    else if (!strcmp(word,"|")) return 2;
    else if (!strcmp(word,">")) return 3;
    else if (!strcmp(word,">>")) return 4;
    else if (!strcmp(word,"<")) return 5;
    else if (!strcmp(word,";")) return 6;
    else return 0;
}

static void term_argv(tree object) {
    object->argv[object->argc] = NULL;
}

static void make_bgrnd (tree t) {
    do {
        t->backgrn = 1;
        t = t->pipe;
    } while (t != NULL);
}

void error() {
    fprintf(stderr,"error\n");
    err = 1;
}

void make_shift(int n){
    while(n--)
        putc(' ', stderr);
}

void print_argv(char **p, int shift){
    char **q=p;
    if(p!=NULL){
        while(*p!=NULL){
             make_shift(shift);
             fprintf(stderr, "argv[%d]=%s\n",(int) (p-q), *p);
             p++;
        }
    }
}

void print_tree(tree t, int shift){
    char **p;
    if(t==NULL)
        return;
    p=t->argv;
    if(p!=NULL)
        print_argv(p, shift);
    else{
        make_shift(shift);
        fprintf(stderr, "psubshell\n");
    }
    make_shift(shift);
    if(t->infile==NULL)
        fprintf(stderr, "infile=NULL\n");
    else
        fprintf(stderr, "infile=%s\n", t->infile);
    make_shift(shift);
    if(t->outfile==NULL)
        fprintf(stderr, "outfile=NULL\n");
    else
        fprintf(stderr, "outfile=%s\n", t->outfile);
    make_shift(shift);
    fprintf(stderr, "append=%d\n", t->append);
    make_shift(shift);
    fprintf(stderr, "backgroun=%d\n", t->backgrn);
    make_shift(shift);
    if(t->pipe==NULL)
        fprintf(stderr, "pipe=NULL \n");
    else{
        fprintf(stderr, "pipe---> \n");
        print_tree(t->pipe, shift+5);
    }
    make_shift(shift);
    if(t->next==NULL)
        fprintf(stderr, "next=NULL \n");
    else{
        fprintf(stderr, "next---> \n");
        print_tree(t->next, shift+5);
    }
}

void clear_tree(tree t) {
    if (t!= NULL) {
        if (t->argv != NULL) {
            free(t->argv);
            t->argv = NULL;
        }
        clear_tree(t->pipe);
        clear_tree(t->next);
        free(t);
    }
}

