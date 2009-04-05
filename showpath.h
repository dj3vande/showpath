#ifndef H_SHOWPATH
#define H_SHOWPATH

#include <stddef.h>

struct path_list
{
	char **entries;
	size_t num,max;
};

struct path_list *alloc_entry(void);
void free_entry(struct path_list *);

/*Add a single entry, doing appropriate interpretation, but not
    separator splitting
*/
int add_entry(struct path_list *,const char *);

/*Add a path fragment as if by doing separator splitting and then
    calling add_entry for each entry in the fragment
  add_entries is currently called for EVERYTHING, whether it contains
    a path separator or not.  If we're going to allow shortnames like
    '%fink:perl', that will need to change.
*/
int add_entries(struct path_list *,const char *,char);

/*Called by add_entry if its entry starts with '%'.
  Should eventually end up calling add_entry or add_entries,
    possibly multiple times.
*/
int add_magic_entry(struct path_list *,const char *);

#endif	/*H_SHOWPATH #include guard*/
