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

/*Add a single entry, doing duplicate pruning but no other interpretation.
  Returns 0 on success, nonzero on failure.
  (Current implementation aborts on allocation failure and never returns
    failure indication.)
*/
int insert_entry(struct path_list *,const char *);
/*Add every entry in from, as if by calling insert_entry() n times*/
int insert_entries(struct path_list *into,const struct path_list *from);

/*Remove a single entry if it exists.
  Returns 1 if something got removed, 0 otherwise
*/
int remove_entry(struct path_list *,const char *);
/*Remove every entry in to_remove, as if by calling remove_entry() n times.
  Returns number of entries actually removed.
*/
int remove_entries(struct path_list *from,const struct path_list *to_remove);

/*Expand shortnames (path_entries beginning with '%')
  Currently only handles '%current'.
  When we add config file capability, this will be where we look up
    the path entries for shortnames.
*/
struct path_list *expand_shortname(const char *name);

/*Add a path entry (from the command line or config file), handling
    shortname expansion and path splitting as appropriate.
*/
int add_entry(struct path_list *pl,const char *ent);

/*Split a path fragment and return its constituent parts wrapped up
    in a struct path_list
*/
struct path_list *split_path(const char *,char sep);

#endif	/*H_SHOWPATH #include guard*/
