// sat_tools.c
// 

#include "sat_tools.h"
#include "sls.h"
#include "cnf.h"
#include "cnf_parse.h"
#include "babel.h"
#include "introspect.h"
#include "introspect_sat.h"
#include "bstring.h"
#include "io.h"
#include "mem.h"
#include <stdlib.h>
#include <time.h>

void dev_prompt(void);
void dev_get_line(char *buffer, FILE *stream);
void dev_menu(void);
char *slurp_file(char *filename);
FILE *open_file(char *filename, const char *attr);
int file_size(FILE *file);

int dev_i;

int main(void){

    srand((unsigned)time(NULL));
//    sls_mt_srand(rand());

    dev_prompt();

    _msg("Done");

}


//
//
void dev_prompt(void){

    char *cmd_code_str;
    int   cmd_code=0;

    char buffer[256];

    int i,j;
    char c;
    int dev_i;

    char *cnf_file;
    clause_list *cl;
    st_state *st;

    babel_env *be = babel_env_new(1);

    mword *ACC=be->nil;
    mword *temp;
    mword  tempv;
    mword  tempi=0;
    mword *test;
    mword *clause;

    clause_prop cp;

    jmp_buf dev_jmp;
    int val;
    val = setjmp(dev_jmp);

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
//                temp = mem_new_str(be, 1000, 0);
//                bsprintf(be, temp, &tempi, "digraph babel {\nnode [shape=record];\n");

                ACC = introspect_sat_gv(be,st,cl);
//                fprintf(stderr, "%s", ACC);
                io_spit(be, "work/test.dot", ACC, U8_ASIZE, OVERWRITE);
                _say("introspect_sat_gv(cl) ==> work/test.dot");

//                ACC = st->clause_array;
//                _say("ACC = st->clause_array;");

//// for i in ACC
////      clause = rdp(ACC,i)
////      for j in clause
////          int v = rdv(clause,j)
////          if(v<0)
////             use port f1
////          else
////             use port f0
//
//                for(i=0;i<size(ACC);i++){
//                    clause = rdp(ACC,i);
//                    for(j=0;j<size(clause);j++){
//                        tempi = rdv(clause,j);
//                        if(tempi<0){
//    foo:f0 -- c1
//                        }
//                        else{
//                        }
//                    }
//                }

                break;

            case 2:
                _say("exiting");
                return;

            case 3:
                cmd_code_str = strtok(NULL, " ");
                if(cmd_code_str == NULL){ _say("not enough arguments"); continue; }
                cnf_file = slurp_file(cmd_code_str);
                cl = parse_DIMACS(cnf_file);
                st = malloc(sizeof(st_state));
                st->cl = cl;
                st_init(be, st);
                break;

            case 5:
                _d(sfield(ACC));
                break;

            case 6:
                _mem(ACC);
                break;

            case 13:
                temp = introspect_gv(be, ACC);
                //_say((char*)temp);
                io_spit(be, "work/test.dot", temp, U8_ASIZE, OVERWRITE);
                _say("introspect_gv(ACC) ==> work/test.dot");
                break;

            case 14:
                temp = introspect_svg(be, ACC, MWORD_SIZE, 0, MWORD_ASIZE);
                //_say((char*)temp);
                io_spit(be, "work/intro.svg", temp, U8_ASIZE, OVERWRITE);
                _say("introspect_svg(ACC) ==> work/intro.svg");
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
            "3     .....    load & parse CNF file\n"
            "4     .....    _dq((mword)ACC);\n"
            "5     .....    _d(sfield(ACC));\n"
            "6     .....    _mem(ACC);\n"
            "7     .....    ACC <== rdp(ACC,n)\n"
            "8     .....    ACC <== rdv(ACC,n)\n"
            "9     .....    ACC <== st_state\n"
            "10    .....    st->dev_break <== n\n"
            "11    .....    cnf_var_write(st, var_id, DEC_ASSIGN[0|1]_VS)\n"
            "12    .....    cnf_var_write(st, var_id, UNASSIGNED_VS);\n"
            "13    .....    introspect_gv(ACC)\n"
            "14    .....    introspect_svg(ACC)\n"
            "15    .....    print variables using cnf_var_read()\n"
            "17    .....    print clauses using ACC\n");

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

