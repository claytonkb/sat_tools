// cnf_parse.h
//

#ifndef CNF_PARSE_H
#define CNF_PARSE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

//#include <iostream>
//#include <typeinfo>
//#include <fstream>
//#include <istream>
//#include <sstream>
//#include <streambuf>
//#include <string>
//#include <vector>
//#include <map>

#define streq(x,y)  ( strcmp(x,y) == 0 )

#define CLAUSE_K 8

//struct cstring_cmp {
//    bool operator () (const char* a, const char* b) const {
//        return strcmp(a,b)<0;
//    } 
//};


typedef struct {
    uint64_t *variables;
    uint64_t *clauses;
    int       num_variables;
    int       num_clauses;
    int       num_assignments;
} clause_list;


clause_list *parse_DIMACS(const char *dimacs_str);
int read_format(char *dimacs_str, clause_list *cl);
int read_clauses(char *dimacs_str, clause_list *cl);
int skip_comments(char *dimacs_str);
int skip_ws(char *dimacs_str);

#endif  // CNF_PARSE_H


// Clayton Bauman 2018

