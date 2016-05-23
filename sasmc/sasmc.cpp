
#include "libsasmc.h"

#include <algorithm>
#include <cstdio>

/*
	Command-line arguments parsing functions
*/
char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

/*
	Main
*/
int main (int argc, char** argv)
{
	bool silent=false;
	bool clean=false;
	bool verbose=false;
	
	if(cmdOptionExists(argv, argv+argc, "-i")==false)
	{
		printf("SASM Compiler\n ver %s by Gi@cky98\n\n", VERSION_STR);
		printf("Usage: %s -i input.sasm [-o output.sbi] [-s] [-cl]\n\n", (char*)argv[0]);
		printf("-i input.sasm\t\tSASM assembly file to compile\n");
		printf("-o output.sbi\t\tSBI output filename\n");
		printf("-s\t\t\tSilent mode\n");
		printf("-v\t\t\tVerbose mode\n");
		printf("-cl\t\t\tClean non-SBI files used during compilation\n");
		return 1;
	}


	
	const char* inname = getCmdOption(argv, argv + argc, "-i");
    char* outname;

	if(cmdOptionExists(argv, argv + argc, "-o"))
		outname = getCmdOption(argv, argv + argc, "-o");
	else
		outname = (char*)"out.sbi";

	if(cmdOptionExists(argv, argv + argc, "-s")) silent = true;
	if(cmdOptionExists(argv, argv + argc, "-cl")) clean = true;
	if(cmdOptionExists(argv, argv + argc, "-v")) verbose = true;
	
	return sasmc (inname, outname, silent, clean, verbose);
}
