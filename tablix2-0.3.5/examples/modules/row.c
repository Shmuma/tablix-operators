#include <stdio.h>
#include <stdlib.h>
#include "module.h"

#define	RESTYPE		"dummy-type"

static resourcetype *dummy;
static int width, height;

int handler(char *restriction, char *content, tupleinfo *tuple)
{
	int row;
	int result;
	int typeid;

	int *resid_list;
	int resid_num;

	int n;

	domain *dom;

	result=sscanf(content, "%d", &row);
	if(result!=1) {
		error(_("Row index must be an integer"));
		return -1;
	}

	if(row<0||row>height-1) {
		error(_("Row index must be between 0 and %d"), height-1);
		return -1;
	}

	typeid=dummy->typeid;

	resid_list=malloc(sizeof(*resid_list)*width);
	if(resid_list==NULL) {
		error(_("Can't allocate memory"));
		return -1;
	}

	for(n=0;n<width;n++) resid_list[n]=row+n*height;
	resid_num=width;

	dom=tuple->dom[typeid];

	domain_and(dom, resid_list, resid_num);

	free(resid_list);
	
	return 0;
}

int module_init(moduleoption *opt)
{
	int result;

	dummy=restype_find(RESTYPE);
	if(dummy==NULL) {
		error(_("Resource type '%s' not found"), RESTYPE);
		return -1;
	}

	result=res_get_matrix(dummy, &width, &height);
	if(result) {
		error(_("Resource type " RESTYPE " is not a matrix"));
		return -1;
	}

	if(handler_tup_new("row-" RESTYPE, handler)==NULL) return -1;

	return(0);
}
