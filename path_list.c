/*path_list.c (part of showpath)
  Code to handle keeping track of path lists.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "showpath.h"

struct path_list *alloc_entry(void)
{
	struct path_list *ret=malloc(sizeof *ret);

	if(!ret)
		return NULL;

	ret->entries=malloc((ret->max=1)*sizeof *ret->entries);
	if(!ret->entries)
	{
		free(ret);
		return NULL;
	}
	ret->num=0;

	return ret;
}

void free_entry(struct path_list *old)
{
	size_t i;

	if(old)
	{
		for(i=0;i<old->num;i++)
			free(old->entries[i]);
		free(old->entries);
		free(old);
	}
}

static void grow(struct path_list *pl)
{
	void *t;

	if(pl->num < pl->max)
		return;		/*nothing to do*/

	t=realloc(pl->entries,2*pl->max*sizeof *pl->entries);
	if(!t)
	{
		perror("realloc");
		exit(EXIT_FAILURE);
	}
	pl->max*=2;
	pl->entries=t;
}

int add_entry(struct path_list *pl,const char *new)
{
	size_t i;

	if(new[0]=='%')
		return add_magic_entry(pl,new);

	for(i=0;i<pl->num;i++)
		if(strcmp(pl->entries[i],new)==0)
			return 0;

	grow(pl);
	pl->entries[pl->num]=strdup(new);
	if(!pl->entries[pl->num])
	{
		perror("strdup");
		exit(EXIT_FAILURE);
	}
	pl->num++;

	return 0;
}

int add_entries(struct path_list *pl,const char *list,char sep)
{
	char *p=strdup(list);
	char *q;
	char ss[2];

	ss[0]=sep;
	ss[1]='\0';

	if(!p)
	{
		perror("strdup");
		return -1;
	}
	q=strtok(p,ss);
	while(q)
	{
		int ret=add_entry(pl,q);
		if(ret)
		{
			free(p);
			return ret;
		}
		q=strtok(NULL,ss);
	}
	free(p);

	return 0;
}
