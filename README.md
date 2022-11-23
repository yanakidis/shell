# shell_interpreter
## Командный интерпретатор shell 
Работает как Bash, только в более узком синтаксисе (он в syntax.txt) 

В директории solution находится файл Makefile - он упростит компиляцию и 
последующий запуск программы

## Для запуска нужно: 

1) Сначала скомпилировать все объектные файлы: \
make list.o \
make tree.o \
make exec.o \
P.S.: если были внесены изменения в один из файлов list.c , tree.c или exec.c, то достаточно 
ввести только одну соответствующую команду make list.o , make tree.o , make exec.o 
2) написать в командной строке: make shell - происходит компиляция 
3) написать в командной строке: make run - запуск самого shell 
4) если хотите очистить директорию от ненужных объектных файлов - make clear 
5) для завершений программы нужно нажать Ctrl-D 

## Примеры команд есть в файле tests.txt
