// cnf.h
//

#include "sat_tools.h"

#ifndef CNF_H
#define CNF_H

int  cnf_var_unsat(st_state *bs, int curr_var);
var_state cnf_var_read(st_state *bs, int var_id);
int  cnf_var_assigned(st_state *bs, int var_id);
void cnf_var_write(st_state *bs, int var_id, var_state vs);
int  cnf_var_assign(st_state *bs, int var_id, var_state vs);
void cnf_var_negate(st_state *bs, int var_id);
void cnf_var_unassign(st_state *bs, int var_id);
int  cnf_var_vs_to_polar(var_state vs);
int  cnf_var_eq(st_state *bs, int var_id, int constant);

int  cnf_clause_all_sat(st_state *bs);
int  cnf_clause_unsat(st_state *bs, mword *clause);
int  cnf_clause_sat(st_state *bs, mword *clause);
int cnf_clause_sat_lit(st_state *bs, int clause_id, char *candidate_assignment);
clause_prop cnf_clause_propagate(st_state *bs, int clause_id);


#endif // CNF_H


// Clayton Bauman 2018

