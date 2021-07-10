#include "depend.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"

// Simple AIG Parser entirely written by Alan Mishchenko (2021) from ABC
static unsigned Aiger_ReadUnsigned( FILE * pFile )
{
    unsigned x = 0, i = 0;
    unsigned char ch;
    while ((ch = fgetc(pFile)) & 0x80)
        x |= (ch & 0x7f) << (7 * i++);
    return x | (ch << (7 * i));
}

static void Aiger_WriteUnsigned( FILE * pFile, unsigned x )
{
    unsigned char ch;
    while (x & ~0x7f)
    {
        ch = (x & 0x7f) | 0x80;
        fputc( ch, pFile );
        x >>= 7;
    }
    ch = x;
    fputc( ch, pFile );
}

int* Aiger_Read( char * pFileName, int * pnObjs, int * pnIns, int * pnLatches, int * pnOuts, int * pnAnds )
{
    int i, Temp, nTotal, nObjs, nIns, nLatches, nOuts, nAnds, * pObjs;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Aiger_Read(): Cannot open the output file \"%s\".\n", pFileName );
        return NULL;
    }
    if ( fgetc(pFile) != 'a' || fgetc(pFile) != 'i' || fgetc(pFile) != 'g' )
    {
        fprintf( stdout, "Aiger_Read(): Can only read binary AIGER.\n" );
        fclose( pFile );
        return NULL;
    }
    if ( fscanf(pFile, "%d %d %d %d %d", &nTotal, &nIns, &nLatches, &nOuts, &nAnds) != 5 )
    {
        fprintf( stdout, "Aiger_Read(): Cannot read the header line.\n" );
        fclose( pFile );
        return NULL;
    }
    if ( nTotal != nIns + nLatches + nAnds )
    {
        fprintf( stdout, "The number of objects does not match.\n" );
        fclose( pFile );
        return NULL;
    }
    nObjs = 1 + nIns + 2*nLatches + nOuts + nAnds;
    //pObjs = ABC_CALLOC( int, nObjs * 2 );
    pObjs = calloc( sizeof(int), nObjs * 2 );
    // read flop input literals
    for ( i = 0; i < nLatches; i++ )
    {
        while ( fgetc(pFile) != '\n' );
        fscanf( pFile, "%d", &Temp );
        pObjs[2*(nObjs-nLatches+i)+0] = Temp;
        pObjs[2*(nObjs-nLatches+i)+1] = Temp;
    }
    // read output literals
    for ( i = 0; i < nOuts; i++ )
    {
        while ( fgetc(pFile) != '\n' );
        fscanf( pFile, "%d", &Temp );
        pObjs[2*(nObjs-nOuts-nLatches+i)+0] = Temp;
        pObjs[2*(nObjs-nOuts-nLatches+i)+1] = Temp;
    }
    // read the binary part
    while ( fgetc(pFile) != '\n' );
    for ( i = 0; i < nAnds; i++ )
    {
        int uLit = ((1 + nIns + nLatches + i) << 1);
        int uLit1 = uLit  - Aiger_ReadUnsigned( pFile );
        int uLit0 = uLit1 - Aiger_ReadUnsigned( pFile );
        pObjs[2*(1+nIns+i)+0] = uLit0;
        pObjs[2*(1+nIns+i)+1] = uLit1;
    }

    for ( i = 0; i < 2 * nObjs; i++) {
	printf("%d : %d\n", i, pObjs[i]);
    }

    fclose( pFile );
    if ( pnObjs )    *pnObjs = nObjs;
    if ( pnIns )     *pnIns  = nIns;
    if ( pnLatches ) *pnLatches = nLatches;
    if ( pnOuts )    *pnOuts = nOuts;
    if ( pnAnds )    *pnAnds = nAnds;
    return pObjs;
}

void greetings() {
	printf("Ian's World!\n");
}

bool aiger_filename_check(const char* filename) {
	// Check that filename length is at least 4
	if (strlen(filename) < 4) {
		return false;
	}

	char* last_four_filename = filename + strlen(filename) - 4;
	
	// Check that file ends in .aig and that the file exists
	if (strcmp(last_four_filename, ".aig") || access(filename, F_OK) != 0) {
		return false;
	}

	return true;
}

uint64_t longest_path_in_aig(int nObjs, int nIns, int nLatches, int nOuts, int nAnds, int* pObjs) {
	return 0;
}

void read_aig_wrapper(const char* filename) {
    int nObjs, nIns, nLatches, nOuts, nAnds, *pObjs = Aiger_Read(filename, &nObjs, &nIns, &nLatches, &nOuts, &nAnds );
    if ( pObjs == NULL ) {
		printf("Error opening the file %s", filename);
		return;
	}

	longest_path_in_aig(nObjs, nIns, nLatches, nOuts, nAnds, pObjs);
        
    free(pObjs);
}


