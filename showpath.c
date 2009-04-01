#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>	/*for getopt*/

char **entries;
size_t num,max;

void grow(void)
{
	if(num >= max)
	{
		void *t=realloc(entries,2*max*sizeof *entries);
		if(!t)
		{
			perror("realloc");
			exit(EXIT_FAILURE);
		}
		entries=t;
		max*=2;
	}
}

/*strdup is *nix, and glibc with gcc -ansi -pedantic doesn't give a
    declaration for it.
*/
char *strdup(const char *s);

int add_entry(const char *new)
{
	size_t i;

	for(i=0;i<num;i++)
		if(strcmp(entries[i],new)==0)
			return 0;

	grow();
	entries[num]=strdup(new);
	if(!entries[num])
	{
		perror("strdup");
		exit(EXIT_FAILURE);
	}
	num++;
	return 0;
}

static char *types[]={"exec","man"};
static char *envs[]={"PATH","MANPATH"};
static size_t num_envs=2;
static char *envname="PATH";

int set_type(const char *type)
{
	size_t i;
	for(i=0;i<num_envs;i++)
	{
		if(strcmp(types[i],type)==0)
		{
			envname=envs[i];
			return 0;
		}
	}

	return 1;
}

int add_path(void)
{
	const char *pp=getenv(envname);
	char *p,*q;
	if(!pp)
	{
		perror("getenv: can't get $PATH");
		return -1;
	}
	/*getenv doesn't necessarily give us a string we can modify*/
	p=strdup(pp);
	if(!p)
	{
		perror("strdup");
		fputs("Failed getting $PATH!\n",stderr);
		return -1;
	}
	q=strtok(p,":");
	while(q)
	{
		int ret=add_entry(q);
		if(ret)
			return ret;
		q=strtok(NULL,":");
	}

	return 0;
}

static char *myname;

void shortusage(void)
{
	printf("Usage: %s [-t type] [-v env_name] path-entry ...\n",myname);
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

int main(int argc,char **argv)
{
	int i;
	int opt;
	int have_type=0;

	myname=argv[0];

	while((opt=getopt(argc,argv,":t:v:"))!=-1)
	{
		switch(opt)
		{
		case 't':
			if(have_type)
			{
				fprintf(stderr,"%s: Only one '-t' or '-v' allowed!\n",myname);
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
		case 'v':
			if(have_type)
			{
				fprintf(stderr,"%s: Only one '-t' or '-v' allowed!\n",myname);
				shortusage();
				return EXIT_FAILURE;
			}
			envname=optarg;
			have_type=1;
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
	argc-=optind;
	argv+=optind;

	if(argc<1)
	{
		usage();
		return 0;
	}

	entries=malloc(sizeof *entries);
	if(!entries)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	max=1;

	for(i=0;i<argc;i++)
	{
		int ret;
		if(strcmp(argv[i],"%current")==0)
			ret=add_path();
		else
			ret=add_entry(argv[i]);
		if(ret)
		{
			fprintf(stderr,"%s: Adding path entry '%s' failed!\n",argv[0],argv[i]);
			exit(EXIT_FAILURE);
		}
	}

	if(num)
	{
		size_t idx;
		printf("%s",entries[0]);
		for(idx=1;idx<num;idx++)
			printf(":%s",entries[idx]);
		printf("\n");
	}

	return 0;
}