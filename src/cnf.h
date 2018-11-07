// cnf.h
//

#ifndef CNF_H
#define CNF_H

#include "backtrack.h"

int cnf_var_assignz(backtrack_state *bs, int var_id, var_state vs);

int  cnf_var_unsat(backtrack_state *bs, int curr_var);
var_state cnf_var_read(backtrack_state *bs, int var_id);
int  cnf_var_assigned(backtrack_state *bs, int var_id);
void cnf_var_write(backtrack_state *bs, int var_id, var_state vs);
int  cnf_var_assign(backtrack_state *bs, int var_id, var_state vs);
void cnf_var_negate(backtrack_state *bs, int var_id);
void cnf_var_unassign(backtrack_state *bs, int var_id);
int  cnf_var_vs_to_polar(var_state vs);
int  cnf_var_eq(backtrack_state *bs, int var_id, int constant);

int  cnf_clause_all_sat(backtrack_state *bs);
int  cnf_clause_unsat(backtrack_state *bs, mword *clause);
int  cnf_clause_sat(backtrack_state *bs, mword *clause);
clause_prop cnf_clause_propagate(backtrack_state *bs, int clause_id);


#endif // CNF_H


// Clayton Bauman 2018

