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

static void expand_home(char *out,const char *in)
{
	char *home=getenv("HOME");
	size_t len;
	size_t space;
	if(!home)
	{
		fprintf(stderr,"Warning: Can't expand $HOME\n");
		out[0]='\0';
		space=FILENAME_MAX-1;
		/*Don't return here; we still have to check in for
		    truncation, and we can arrange things so we only
		    have to do that in one place.
		*/
	}
	else
	{
		len=strlen(home);
		if(len >= FILENAME_MAX)
		{
			fprintf(stderr,"Warning: Truncating in ~-expansion\n");
			/*This would be easier if we could count on having
			    strlcpy and strlcat available.
			*/
			out[0]=0;
			strncat(out,in,FILENAME_MAX-1);
			return;
		}
		else
		{
			strcpy(out,home);
			in++;
			space=FILENAME_MAX-len-1;
		}
	}

	len=strlen(in);
	if(len > space)
	{
		fprintf(stderr,"Warning: Truncating in ~-expansion\n");
		/*This would be easier if we could count on having strlcpy
		    and strlcat available.
		*/
		strncat(out,in,space);
	}
	else
	{
		/*All good, no truncation or buffer overflow*/
		strcat(out,in);
	}
}

int insert_entry(struct path_list *pl,const char *new)
{
	size_t i;

	/*XXX This is a HORRIBLE place to do ~-expansion, but
	    it seems less bad than any of the other options
	    (since we want to do ~-expansion on a string iff
	    it ends up here or in remove_entry at some point)
	  Expanding at input is kind of hard because of the
	    different ways input needs to be handled.
	  Expanding at output looks like it should be easier,
	    but interferes with duplicate pruning and removal
	    (since we want the ~-expanded and unexpanded values
	    to compare equal in that case).
	*/
	char expanded_name[FILENAME_MAX];
	if(new[0]=='~')
	{
		expand_home(expanded_name,new);
		new=expanded_name;
	}

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

int insert_entries(struct path_list *into,const struct path_list *from)
{
	size_t i;
	int ret;

	for(i=0;i<from->num;i++)
		if((ret=insert_entry(into,from->entries[i]))!=0)
			return ret;

	return 0;
}

int remove_entry(struct path_list *pl,const char *old)
{
	size_t i;

	/*XXX This is a HORRIBLE place to do ~-expansion, but
	    it seems less bad than any of the other options
	    (since we want to do ~-expansion on a string iff
	    it ends up here or in insert_entry at some point)
	  Expanding at input is kind of hard because of the
	    different ways input needs to be handled.
	  Expanding at output looks like it should be easier,
	    but interferes with duplicate pruning and removal
	    (since we want the ~-expanded and unexpanded values
	    to compare equal in that case).
	*/
	char expanded_name[FILENAME_MAX];
	if(old[0]=='~')
	{
		expand_home(expanded_name,old);
		old=expanded_name;
	}
	
	for(i=0;i<pl->num;i++)
	{
		if(strcmp(pl->entries[i],old)==0)
		{
			/*Remove this one*/
			size_t j;

			free(pl->entries[i]);
			for(j=i+1;j<pl->num;j++)
				pl->entries[j-1]=pl->entries[j];
			pl->num--;

			return 1;
		}
	}
	return 0;
}

/*We could be a bit lazier here by marking removed entries with a dummy
    string (have to be careful not to collide with real ones) and then
    doing a single "move up" pass after everything is removed.   --DV
*/
int remove_entries(struct path_list *from,const struct path_list *to_remove)
{
	size_t i;
	int ret=0;

	for(i=0;i<to_remove->num;i++)
		ret+=remove_entry(from,to_remove->entries[i]);

	return ret;
}

struct path_list *split_path(const char *list,char sep)
{
	struct path_list *pl;
	char *p,*q;
	char ss[2];

	ss[0]=sep;
	ss[1]='\0';

	pl=alloc_entry();
	if(!pl)
		return NULL;

	p=strdup(list);
	if(!p)
	{
		perror("strdup");
		free_entry(pl);
		return NULL;
	}

	q=strtok(p,ss);
	while(q)
	{
		if(insert_entry(pl,q))
		{
			free(p);
			free_entry(pl);
			return NULL;
		}
		q=strtok(NULL,ss);
	}

	free(p);

	return pl;
}
