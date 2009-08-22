#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>	/*for getopt and getcwd*/

#include "showpath.h"

/* stores argv[0] for use in output */
static char *myname;

static struct config
{
	char *envname;
	char pathsep;
} config;

static void set_defaults(void)
{
	config.envname="PATH";
	config.pathsep=':';
}


/*Expand shortnames (path_entries beginning with '%')
  Currently only handles '%current'.
  When we add config file capability, this will be where we look up
    the path entries for shortnames.
*/
struct path_list *expand_shortname(const char *name)
{

	if(strcmp(name,"%current")==0)
	{
		const char *env_string=getenv(config.envname);
		if(env_string)
			return split_path(env_string,config.pathsep);
		else
			return alloc_entry();	/*no path string is not error*/
	}
	else if(strcmp(name,"%pwd")==0)
	{
		char *pwd;
		struct path_list *ret;

		pwd=getcwd(NULL,-1);
		if(!pwd)
			return NULL;
		ret=alloc_entry();
		if(!ret)
			return NULL;

		if(insert_entry(ret,pwd))
		{
			free_entry(ret);
			ret=NULL;
		}

		free(pwd);
		return ret;
	}
	else
	{
		fprintf(stderr,"%s: Unrecognized magic path entry '%s'\n",myname,name);
		return NULL;
	}
}


/*Set the type of path we're building.
  Used for -t switch.
  Currently only affects config.envname.
*/
int set_type(const char *type)
{
	const char *types[]={"exec","man",NULL};
	char *envs[]={"PATH","MANPATH",NULL};
	size_t i = -1;
	while(types[++i])
	{
		if(strcmp(types[i],type)==0)
		{
			config.envname=envs[i];
			return 0;
		}
	}

	return 1;
}

void shortusage(void)
{
	printf("Usage: %s [-t type | -e env_name] [-s separator ] path-entry ...\n",myname);
}

void usage(void)
{
	shortusage();
	printf("  Outputs a path containing all path-entries on the command line, in\n");
	printf("    the order given, with duplicates removed.\n");
	printf("  Known types are 'exec' (for $PATH) and 'man' (for $MANPATH).\n");
	printf("  The magic path-entry '%%current' expands to the current value from\n");
	printf("    the environment.\n");
}


/*Add a path entry, handling shortname expansion and path splitting
    as appropriate
*/
int add_entry(struct path_list *pl,const char *ent)
{
	struct path_list *new;
	int ret;
	int remove=0;

	if(ent[0]=='^')
	{
		remove=1;
		ent++;
	}

	if(ent[0]=='%')
		new=expand_shortname(ent);
	else if(strchr(ent,config.pathsep))
		new=split_path(ent,config.pathsep);
	else	/*simple entry, just add*/
	{
		if(remove)
		{
			remove_entry(pl,ent);
			return 0;
		}
		else
		{
			return insert_entry(pl,ent);
		}
	}

	/*If we get here, we have a list of entries to add in new
	    (or NULL if we've failed somewhere along the line)
	*/
	if(!new)
		return -1;

	if(remove)
	{
		remove_entries(pl,new);
		ret=0;
	}
	else
	{
		ret=insert_entries(pl,new);
	}
	free_entry(new);
	return ret;
}

int main(int argc,char **argv)
{
	int i;
	int opt;
	int have_type=0;
	struct path_list *pl;

	myname=argv[0];

	set_defaults();

	while((opt=getopt(argc,argv,":t:e:s:"))!=-1)
	{
		switch(opt)
		{
		case 't':
			if(have_type)
			{
				fprintf(stderr,"%s: Only one '-t' or '-e' allowed!\n",myname);
				shortusage();
				return EXIT_FAILURE;
			}
			if(set_type(optarg))
			{
				fprintf(stderr,"%s: Unknown type '%s'!\n",myname,optarg);
				shortusage();
				return EXIT_FAILURE;
			}
			have_type=1;
			break;
		case 'e':
			if(have_type)
			{
				fprintf(stderr,"%s: Only one '-t' or '-e' allowed!\n",myname);
				shortusage();
				return EXIT_FAILURE;
			}
			config.envname=optarg;
			have_type=1;
			break;
		case 's':
			if(optarg[1])
			{
				fprintf(stderr,"%s: Separator must be a single character!\n",myname);
				shortusage();
				return EXIT_FAILURE;
			}
			config.pathsep = optarg[0];
			break;
		case ':':
			fprintf(stderr,"%s: Option '%c' requires an argument\n",myname,optopt);
			shortusage();
			return EXIT_FAILURE;
		case '?':
			fprintf(stderr,"%s: Unrecognized option: %c\n",myname,optopt);
			shortusage();
			return EXIT_FAILURE;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	/* These lines allow us to treat argc and argv as though 
	 * any switches were not there
	 */
	argc-=optind;
	argv+=optind;

	if(argc<1)
	{
		usage();
		return 0;
	}

	pl=alloc_entry();
	if(!pl)
	{
		perror("alloc_entry");
		exit(EXIT_FAILURE);
	}

	for(i=0;i<argc;i++)
	{
		int ret=add_entry(pl,argv[i]);
		if(ret)
		{
			/*XXX This will get ugly if we have a long path
			    on the command line.  Is there a better way
			    to handle that?   --DV
			*/
			fprintf(stderr,"%s: Adding path entry '%s' failed!\n",myname,argv[i]);
			exit(EXIT_FAILURE);
		}
	}

	if(pl->num)
	{
		size_t idx;
		printf("%s",pl->entries[0]);
		for(idx=1;idx<pl->num;idx++)
			printf("%c%s",config.pathsep,pl->entries[idx]);
		printf("\n");
	}

	free_entry(pl);

	return 0;
}
