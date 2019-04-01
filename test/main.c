// cnf_parse.cpp
// g++ -std=c++11 -o cnf_parse cnf_parse.cpp numeric.cpp -lm

#include "cnf_parse.h"
#include "cutils.h"
#include <time.h>
#include "babel.h"

int dev_i;
void dev_prompt(void);
void dev_get_line(char *buffer, FILE *stream);
void dev_menu(void);
char *slurp_file(char *filename);
FILE *open_file(char *filename, const char *attr);
int file_size(FILE *file);

int main(void){

//    srand((unsigned)time(NULL));

    dev_prompt();

    _msg("Done");

}


//
//
void dev_prompt(void){

    char *cmd_code_str;
    int   cmd_code=0;

    char buffer[256];

    int i;

    char *cnf_file;
    clause_list *cl;

    babel_env *be = babel_env_new(1);

    mword *ACC=be->nil;
    mword *temp;
    mword *clause;
    mword  tempv;
    int    tempi=0;

    _say("type 0 for menu");

    while(1){

        _prn("% ");

        dev_get_line(buffer, stdin);

        cmd_code_str = strtok(buffer, " ");
        if(cmd_code_str == NULL) continue;
        cmd_code = atoi(cmd_code_str);

        switch(cmd_code){
            case 0:
                dev_menu();
                break;

            case 1:
                temp = mem_new_str(be, 1000, 0);
                bsprintf(be, temp, &tempi, "digraph babel {\nnode [shape=record];\n");
                introspect_sat_gv(be,cl);
                _die;

                _say("cmd_code==1");
                break;

            case 2:
                _say("exiting");
                return;

            case 3:
                cmd_code_str = strtok(NULL, " ");
                if(cmd_code_str == NULL){ _say("not enough arguments"); continue; }
                cnf_file = slurp_file(cmd_code_str);
//                printf("%s\n\n", cnf_file);
                break;

            case 4:
                cl = parse_DIMACS(cnf_file);
                break;

            default:
                _say("unrecognized cmd_code");
                dev_menu();
                break;
        }

        for(i=0;i<256;i++){ buffer[i]=0; } // zero out the buffer

    }

}


//
//
void dev_get_line(char *buffer, FILE *stream){

    int c, i=0;

    while(1){ //FIXME unsafe, wrong
        c = fgetc(stream);
        if(c == EOF || c == '\n'){
            break;
        }
        buffer[i] = c;
        i++;
    }

    buffer[i] = '\0';

}


//
//
void dev_menu(void){

    _say( "\n0     .....    list command codes\n"
            "1     .....    dev one-off\n"
            "2     .....    exit\n"
            "3     .....    load cnf_file\n"
            "4     .....    parse DIMACS\n"
            "5     .....    dump clauses\n"
            "6     .....    dump variables\n");

}


//
//
char *slurp_file(char *filename){

    FILE *f = open_file((char*)filename, "r");
    int size = file_size(f);

    char *file_buffer = (char*)malloc(size+1);
    size_t dummy = fread((char*)file_buffer, 1, size, f);

    fclose(f);

    return file_buffer;

}


//
//
FILE *open_file(char *filename, const char *attr){

    FILE* file;

    file = fopen((char*)filename, attr);

    if(file==NULL)
        _fatal((char*)filename);

    return file;

}


//
//
int file_size(FILE *file){

    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    rewind(file);

    return size;

}



// Clayton Bauman 2018

