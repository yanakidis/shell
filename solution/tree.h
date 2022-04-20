struct cmd_inf {
    char ** argv; /* список из имени команды и аргументов */
    int argc; /* кол-во параметров в argv */
    char * infile; /* переназначенный файл стандартого ввода */
    char * outfile; /* переназначенный файл стандартного вывода*/
    int append; /* если дописывание в файл */
    int backgrn; /* =1, если команда подлежит выполнению в фоновом режиме */
    struct cmd_inf * pipe; /* следующая команда после | */
    struct cmd_inf * next; /* следующая команда после ";" (или после "&") */
};

typedef struct cmd_inf * tree;

extern tree build_tree(char**);
extern void clear_tree(tree);
extern void print_tree(tree, int);
extern void error();

