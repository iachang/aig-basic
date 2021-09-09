#include "depend.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

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
    
    /*
    for ( i = 0; i < 2 * nObjs; i++) {
	    printf("%d : %d\n", i, pObjs[i]);
    }*/

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

int lit_to_ulit(int lit) {
    return lit >> 1;
}

uint64_t longest_path_in_aig(int nObjs, int nIns, int nLatches, int nOuts, int nAnds, int* pObjs) {
    uint64_t* pathDP = (uint64_t*) calloc(nIns + nAnds + 1, sizeof(uint64_t));
    
    int longest_path = 0;
    const int start = (nIns + 1) * 2;
    // nIns + 1 because we include the CONST0
    
    for (int i = 0; i < nAnds; i++) {
        int in1 = pObjs[start + 2 * i];
        int in2 = pObjs[start + 2 * i + 1];
    
        pathDP[nIns + 1 + i] = max(pathDP[lit_to_ulit(in1)], pathDP[lit_to_ulit(in2)]) + 1;
        // nIns + 1 because we include the CONST0

        longest_path = max(longest_path, pathDP[nIns + 1 + i]);
    } 
    
	return longest_path;
}

int* naive_union(int* list1, int* list2){
    int list1_size = list1[0];
    int list2_size = list2[0];

    //printf("List1 size: %d, List2 size: %d\n", list1_size, list2_size);

    int included_count = 0;
    for (int i = 0; i < list1_size; i++) {
        bool is_included = false;
        for (int j = 0; j < list2_size; j++) {
            if (list1[i + 1] == list2[j + 1]) {
                is_included = true;
                included_count += 1;
            }
        }
    }

    int union_list_size = list1_size + list2_size - included_count;
    
    int* union_list = malloc(sizeof(int) * (union_list_size + 1));
    union_list[0] = union_list_size;
    for (int i = 0; i < list1_size; i++) {
        union_list[i + 1] = list1[i + 1];
    }

    int actual_i = 0;
    for (int i = 0; i < list2_size; i++) {
        bool is_included = false;
        for (int j = 0; j < list1_size; j++) {
            if (list2[i + 1] == list1[j + 1]) {
                is_included = true;
            }
        }

        if (!is_included) {
            union_list[list1_size + actual_i + 1] = list2[i + 1];
            actual_i += 1;
        }
    }

    // Will this cause bugs? Maybe not because topological?
    //free(list1);
    //free(list2);

    /*
    printf("Size: %d\n", union_list[0]);
    for (int i = 0; i < union_list_size; i++) {
        printf("%d ", union_list[i + 1]);
    }
    printf("\n");*/

    return union_list;
}

uint64_t total_tfi_count(int nObjs, int nIns, int nLatches, int nOuts, int nAnds, int* pObjs) {
    const int start = (nIns + 1) * 2;
    // nIns + 1 because we include the CONST0

    // Initialize all the inputs to have a list of size 1
    int** tfi_set = malloc(sizeof(int*) * (nIns + nAnds + nOuts + 1));

    // Initialize the CONST0 node to have a list of size 0
    tfi_set[0] = calloc(2, sizeof(int));

    for (int i = 1; i < nIns + 1; i++) {
        tfi_set[i] = malloc(sizeof(int) * 2);
        tfi_set[i][0] = 1;
        tfi_set[i][1] = i;
    }
    
    for (int i = 0; i < nAnds; i++) {
        int in1 = pObjs[start + 2 * i];
        int in2 = pObjs[start + 2 * i + 1];
        tfi_set[nIns + 1 + i] = naive_union(tfi_set[lit_to_ulit(in1)], tfi_set[lit_to_ulit(in2)]);
    }

    int total_count = 0;
    //int* current_list;
    for (int i = 0; i < nOuts; i++) {
        int out = pObjs[(nIns + 1 + nAnds + i) * 2];
        tfi_set[nIns + 1 + nAnds + i] = tfi_set[lit_to_ulit(out)];

        /*
        if (i == 0)
            current_list = tfi_set[nIns + 1 + nAnds + i];
        else
            current_list = naive_union(tfi_set[nIns + 1 + nAnds + i], current_list);*/
        
        total_count += tfi_set[lit_to_ulit(out)][0];
    }

    printf("Total TFI count: %d\n", total_count);
    //printf("Total TFI union count: %d\n", current_list[0]);
    return total_count;
    
}

void read_aig_wrapper(const char* filename) {
    int nObjs, nIns, nLatches, nOuts, nAnds, *pObjs = Aiger_Read(filename, &nObjs, &nIns, &nLatches, &nOuts, &nAnds );
    if ( pObjs == NULL ) {
		printf("Error opening the file %s", filename);
		return;
	}

	int longest_path = longest_path_in_aig(nObjs, nIns, nLatches, nOuts, nAnds, pObjs);
    printf("Longest Path: %d\n", longest_path);

    /*
    printf("Performing the unit test on union_list:\n");
    int* list1_test = malloc(sizeof(int) * 6);
    list1_test[0] = 5;
    list1_test[1] = 1;
    list1_test[2] = 2;
    list1_test[3] = 3;
    list1_test[4] = 4;
    list1_test[5] = 5;

    int* list2_test = malloc(sizeof(int) * 5);
    list2_test[0] = 4;
    list2_test[1] = 3;
    list2_test[2] = 4;
    list2_test[3] = 5;
    list2_test[4] = 6;*/

    //naive_union(list1_test, list2_test);

    total_tfi_count(nObjs, nIns, nLatches, nOuts, nAnds, pObjs);

    free(pObjs);
}


