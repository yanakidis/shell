#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "list.h"
#define SIZE 16
#define N 10

static int flag = 0; /* флаг, если используем кавычки*/
static int ekr = 0; /* если есть экранирование */
static int dol = 0; /* если был символ $ */
static int prev = 0; /* для нескольких кавычек подряд*/
static int c; /*текущий символ */
static int b; /* если два спец символа подряд */
static int n = 0; /* кол-во элементов в буфере для read */
static char buf_read[N]; /* буфер для read */
static char ** lst = NULL; /* список слов (в виде массива)*/
static char * buf; /* буфер для накопления текущего слова*/
static int sizebuf; /* размер буфера текущего слова*/
static int sizelist; /* размер списка слов*/
static int curbuf; /* индекс текущего символа в буфере*/
static int curlist; /* индекс текущего слова в списке*/
static int dol; /* если был символ $ */
static char * okr[] = {"$HOME", "$SHELL", "$USER", "$EUID"};
int err = 0;

static int getsym(void);
static void null_list(void);
static void termlist(void);
// void printlist(void);
// void clearlist(void);
static void nullbuf(void);
static int notsymset(int);
static void addsym(void);
static int symset(int);
static void addword(void);
void error_memory(void);

char ** createlist(int *a) {
    typedef enum {Start, Word, Greater, Greater2, Newline, Stop} vertex;
    int i;
    vertex V=Start;
   // n = read(0,buf_read,N);
    c=getsym();
    null_list();
    nullbuf();
    while(1 && !err) switch(V) {
        case Start:
            if (ekr) {
                V=Word;
                c=getsym();
            } else {
                if(c==' '||c=='\t') {
                    c=getsym();
                }
                else if (c==EOF) {
                    termlist();
                    *a = 0;
//                    printlist();
//                    clearlist();
                    V=Stop;
                } else if (c=='\n') {
                    termlist();
//                    printlist(lst);
                    V= Stop;
//                    c=getsym();
                    flag =0;
                } else if (c=='\\') {
                    nullbuf();
                    ekr = 1;
                } else  if (c=='#') {
                    while ((c = getsym()) != '\n') ;
                    V = Start;
                } else if (c!='$') {
                    nullbuf();
                    if (notsymset(c)) {
                        V = Greater;
                        addsym();
                    }
                    else {
                        if (c=='"') {
                            flag = 1;
                        }
                        else {
                            flag =0;
                            addsym();
                        }
                        V = Word;
                    }
                    b=c;
                    c=getsym();
                    if (c=='$') dol++;;
                } else {
                    dol =1;
                    nullbuf();
                    addsym();
                    c=getsym();
                    V=Word;
                }
            }
            break;
        case Word:
            if (((symset(c) || (flag && !symset(c) ) ) && (c!='"') && (flag || c!='#')) || ekr || c=='\\') {
                if (c=='\\' && !ekr ) {
                    c=getsym(); /* если в слове встретилось \ */
                    ekr=1;
                }
                addsym();
                prev = c;
                c=getsym();
                if (ekr == 1) {
                    ekr = 0;
                    if (c=='"' && flag) prev='\0';
                }
                if (c=='$') dol++;
//                { /* если в слове несколько раз встречается $*/
//                    if (!flag) {
//                        V=Start;
//                        addword();
//                    } else dol=1;
//                }
            } else {
                V=Start;
                addword();
                if (c=='"') {
                    if (prev!='"') {
                        c=getsym();
                        flag=0;
                    }
                } else if (c=='#') while ((c = getsym()) != '\n');
            }
            break;
        case Greater:
            if(notsymset(c))
                if (b==c && b=='>') {
                    addsym();
                    V = Greater2;
                    c = getsym();
                } else {
                    V = Start;
                    addword();
                }
            else {
                V=Start;
                addword();
            }
            break;
        case Greater2:
            V=Start;
            addword();
            break;
        case Newline:
            clearlist(lst);
            V=Start;
            break;
        case Stop:
            //fclose(stdout);
            return lst;
            break;
        }
    return NULL;
}

static int podstroka() {
    int i;
    for (i=0; i<=3; i++) {
        if ((strstr(buf,okr[i])) != NULL) return i;
    }
    return -1;
}

static char * preobr(int s) {
    char * c = malloc(10 * sizeof(char));
    if (c == NULL) {
        error_memory();
        return NULL;
    }
    int v = 0; //количество цифр в числе s
    //разбиваем на отдельные символы число s
    while (s > 9) {
        c[v++] = (s % 10) + '0';
        s = s / 10;
    }
    c[v++] = s + '0';
    c[v] = '\0';
    char t;
    int i;
    //инвертируем массив символов
    for (i = 0; i < v / 2; i++)
    {
        t = c[i];
        c[i] = c[v - 1 - i];
        c[v - 1 - i] = t;
    }
    c = realloc(c,v+1);
    return c;
}

static void dob_okr () {
    int nomer;
    dol --;
    if (sizebuf > 4) {
        while ((nomer=podstroka())>=0) {
            char * s;
            int id;
            int len;
            int k; /* сколько букв в $слово */
            switch(nomer){
                case 0:
                    s=getenv("HOME");
                    len = strlen(s);
                    k=5;
                    break;
                case 1:
                    s=getenv("SHELL");
                    len=strlen(s);
                    k=6;
                    break;
                case 2:
                    s=getenv("USER");
                    len=strlen(s);
                    k=5;
                    break;
                case 3:
                    id=geteuid();
                    s=preobr(id); /* число в строку */
                    len=strlen(s);
                    k=5;
                    break;
            }
            if (s != NULL) {
                /* начинаем менять слово */
                int j,i;
                char * vh = strstr(buf,okr[nomer]) ; /* вхождение подстроки */
                int dl = vh - buf;
                char * dop_bufer = malloc(dl+len+sizebuf-dl-k);
                if (dop_bufer == NULL) {
                    error_memory();
                    return;
                }
                strncpy(dop_bufer,buf,dl);
                for (i=0; i< len; i++) dop_bufer[dl+i] = s[i];
                for (i=dl+k,j=1; i<sizebuf; i++,j++) dop_bufer[dl+len+j-1] = buf[i];
                sizebuf = dl+len+sizebuf-dl-k;
                char * tmp = realloc(buf,sizebuf);
                if (tmp == NULL) {
                    error_memory();
                    free(buf);
                    nullbuf();
                    return;
                } else {
                    buf = tmp;
                }
                for (i=0;i<sizebuf;i++) buf[i] = dop_bufer[i];
                free(dop_bufer);
                if (nomer == 3) free(s);
            }
        }
    }
}

//void bubble_sort(char **list) {
//    int i,j;
//    char * temp;
//    for (i=sizelist-2; i>=0; i--)
//        for (j=0; j<i; j++)
//            if (strcmp(list[j],list[j+1]) > 0) {
//                temp = list[j+1];
//                list[j+1] = list[j];
//                list[j] = temp;
//            }
//}

static int getsym() {
    static int i = 0;
    if (i>=n) {
        n = read(0,buf_read,N);
        if (n==0) return EOF;
            else {
                i = 0;
                return buf_read[i++];
            }
    } else {
        return buf_read[i++];
    }
}

void clearlist(char **list) {
    int i;
    sizelist=0;
    curlist=0;
    if (list==NULL) return;
    for (i=0; list[i]!=NULL; i++) {
        free(list[i]);
    }
    free(list);
    lst=NULL;
}

static void null_list() {
    sizelist=0;
    curlist=0;
    lst=NULL;
}

static void termlist() {
    if (lst==NULL) return;
    if (curlist>sizelist-1) {
        char ** tmp = realloc(lst,(sizelist+1)*sizeof(*lst));
        if (tmp == NULL) {
            error_memory();
            clearlist(lst);
            return;
        } else {
            lst = tmp;
    }
    lst[curlist]=NULL;
    /*выравниваем используемую под список память точно по размеру списка*/
    char **tmp =realloc(lst,(sizelist=curlist+1)*sizeof(*lst));
    if (tmp == NULL) {
        error_memory();
        clearlist(lst);
        return;
    } else lst = tmp;
}

static void nullbuf() {
    buf=NULL;
    sizebuf=0;
    curbuf=0;
}

static void addsym() {
    if (curbuf>sizebuf-1) {
        char * tmp = realloc(buf, sizebuf+=SIZE); /* увеличиваем буфер при необходимости */
        if (tmp == NULL) {
            free(buf);
            nullbuf();
            error_memory();
            return;
        } else buf = tmp;
    }
    buf[curbuf++]=c;
}

static void addword() {
    if (curbuf>sizebuf-1) {
        char * tmp = realloc(buf, sizebuf+=1); /* для записи ’\0’
                                                увеличиваем буфер при необходимости */
        if (tmp == NULL) {
            free(buf);
            nullbuf();
            error_memory();
            return;
        } else buf = tmp;
    }
    buf[curbuf++]='\0';

    /*выравниваем используемую память точно по размеру слова*/
    char * tmp = realloc(buf,sizebuf=curbuf);
    if (tmp == NULL) {
        free(buf);
        nullbuf();
        error_memory();
        return;
    } else buf = tmp;
    while (dol) dob_okr(); /* если были $*/
    
    if (curlist>sizelist-1) {
        char ** tmp = realloc(lst, (sizelist+=SIZE)*sizeof(*lst)); /* увеличиваем
                                                                   массив под список при необходимости */
        lst=realloc(lst, (sizelist+=SIZE)*sizeof(*lst)); /* увеличиваем
                                                          массив под список при необходимости */
        if (tmp == NULL) {
            error_memory();
            clearlist(lst);
            return;
        } else lst = tmp;
    }
    lst[curlist++]=buf;
}

void printlist(char ** list) {
    int i;
    if (list==NULL) {
        printf("%d\n",sizelist);
        return;
    }
    putchar('\n');
    printf("Количество - %d\n",sizelist-1);
    for (i=0; i<sizelist-1; i++)
        printf("%s\n",list[i]);
    putchar('\n');
//    bubble_sort(lst);
//    putchar('\n');
//    for (i=0; i<sizelist-1; i++)
//        printf("%s\n",lst[i]);
//    putchar('\n');
}

static int symset(int c) {
    return c!='\n' && c!=' ' && c!='\t' && c!='>' && c!= EOF &&
           c!='|' && c!='&' && c!=';' && c!='<';
}

static int notsymset(int c) {
    return c=='>' || c=='|' || c=='&' || c=='<' || c==';';
}

void error_memory() {
    fprintf(stderr,"error memory allocation\n");
    err = 1;
}

