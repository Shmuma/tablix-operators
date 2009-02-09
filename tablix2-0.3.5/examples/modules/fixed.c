#include <stdlib.h>
#include "module.h"

#define	RESTYPE		"dummy-type"

struct fixed {
	int tupleid;
	int resid;
};

static int fixed_num;
static struct fixed *fixed_tuples;

static resourcetype *dummy;

int handler(char *restriction, char *content, tupleinfo *tuple)
{
	int resid;

	resid=res_findid(dummy, content);
	if(resid==INT_MIN) {
		error(_("Resource '%s' not found"), content);
		return -1;
	}

	fixed_tuples[fixed_num].resid=resid;
	fixed_tuples[fixed_num].tupleid=tuple->tupleid;

	fixed_num++;

	return 0;
}

int fitness(chromo **c, ext **e, slist **s)
{
	chromo *dummy_c;
	int n, sum;
	int tupleid, resid;

	dummy_c=c[0];

	sum=0;

	for(n=0;n<fixed_num;n++) {
		tupleid=fixed_tuples[n].tupleid;
		resid=fixed_tuples[n].resid;

		if(dummy_c->gen[tupleid]!=resid) sum++;
	}

	return sum;
}

int module_init(moduleoption *opt)
{
	fitnessfunc *f;

	dummy=restype_find(RESTYPE);
	if(dummy==NULL) {
		error(_("Resource type '%s' not found"), RESTYPE);
		return -1;
	}

	fixed_tuples=malloc(sizeof(*fixed_tuples)*dat_tuplenum);
	fixed_num=0;

	if(fixed_tuples==NULL) {
		error(_("Can't allocate memory"));
		return -1;
	}

	if(handler_tup_new("fixed-" RESTYPE, handler)==NULL) return -1;

	f=fitness_new("fixed-" RESTYPE,
		option_int(opt, "weight"),
		option_int(opt, "mandatory"),
		fitness);

	if(f==NULL) return -1;

	fitness_request_chromo(f, RESTYPE);

	return(0);
}
