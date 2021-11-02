#include "depend.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"
#include "time.h"

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

int ulit_to_plit(int uLit) {
    return uLit << 1;
}

int lit_to_plit(int lit) {
    return lit >> 1 << 1;
}

// Helper function to count the support sizes for a unique Travel ID
static int recursive_supp_helper(int* tid_nodes, int tid, int lit, int* pObjs) {
    if (lit < 2 || tid_nodes[lit_to_ulit(lit)] == tid) {
        return 0;
    }

    tid_nodes[lit_to_ulit(lit)] = tid;

    //Check if our node is a ci
    if (pObjs[lit] == 0) {
        return 1;
    }

    return recursive_supp_helper(tid_nodes, tid, pObjs[lit], pObjs)
        + recursive_supp_helper(tid_nodes, tid, pObjs[lit^1], pObjs);
}

int recursive_total_tfi_count(int nObjs, int nIns, int nLatches, int nOuts, int nAnds, int* pObjs) {
    int count = 0;
    int tid = 0;
    
    // Initialize our travel id tracker array
    int* tid_nodes = calloc(nIns + nAnds + nOuts + 1, sizeof(int));

    // Iterate over all outputs
    for (int i = 0; i < nOuts; i++) {
        int lit = pObjs[(nIns + 1 + nAnds + i) * 2];
        tid++;

        count += recursive_supp_helper(tid_nodes, tid, lit, pObjs);
    }
    
    printf("Total TFI count (Recursive): %d\n", count);
    return count;
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

int real_merge_size(int* list1, int* list2) {
    int list1_size = list1[0];
    int list2_size = list2[0];
    int real_biglist_size = 0;

    int* list1_start = list1 + 1;
    int* list2_start = list2 + 1;

    int* list1_end = list1 + list1_size + 1;
    int* list2_end = list2 + list2_size + 1;

    while (list1_start < list1_end && list2_start < list2_end)  {
        if (*list1_start == *list2_start) {
            list1_start++;
            list2_start++;
        } else if (*list1_start < *list2_start) {
            list1_start++;
        } else {
            list2_start++;
        }
        real_biglist_size++;
    }
    
    while (list1_start < list1_end) {
        list1_start++;
        real_biglist_size++;
    }

    while (list2_start < list2_end) {
        list2_start++;
        real_biglist_size++;
    }

    return real_biglist_size;
}


// Checks if sorted list2 is contained in sorted list1
bool check_list_contained(int* list1, int* list2) {
    int real_size = real_merge_size(list1, list2);
    if (real_size == list1[0])
        return true;
    else
        return false;
}

// Checks if sorted list is contained in sorted biglist
bool check_biglist_contained(int* smalllist, int* biglist) {
    if (biglist[0] == smalllist[0]) {
        return true;
    }
    else
        return false;
}


// Merges two sorted integer lists into a larger sorter list
void mergesort_list(int* list1, int* list2, int* biglist) {
    int list1_size = list1[0];
    int list2_size = list2[0];
    int biglist_size = biglist[0];

    int* list1_start = list1 + 1;
    int* list2_start = list2 + 1;
    int* biglist_start = biglist + 1;

    int* list1_end = list1 + list1_size + 1;
    int* list2_end = list2 + list2_size + 1;

    while(list1_start < list1_end && list2_start < list2_end)  {
        if (*list1_start == *list2_start) {
            *biglist_start++ = *list1_start++;
            list2_start++;
        } else if (*list1_start < *list2_start) {
            *biglist_start++ = *list1_start++;
        } else {
            *biglist_start++ = *list2_start++;
        }
    }

    while (list1_start < list1_end) {
        *biglist_start++ = *list1_start++;
    }

    while (list2_start < list2_end) {
        *biglist_start++ = *list2_start++;
    }

    assert(biglist_size >= list1_size);
    assert(biglist_size >= list2_size);
}

// Merges two sorted integer lists into a larger sorter list, but with no size known for biglist
void mergesort_sizeless_list(int* list1, int* list2, int* biglist) {
    int list1_size = list1[0];
    int list2_size = list2[0];

    int* list1_start = list1 + 1;
    int* list2_start = list2 + 1;
    int* biglist_start = biglist + 1;

    int* list1_end = list1 + list1_size + 1;
    int* list2_end = list2 + list2_size + 1;

    while(list1_start < list1_end && list2_start < list2_end)  {
        if (*list1_start == *list2_start) {
            *biglist_start++ = *list1_start++;
            list2_start++;
        } else if (*list1_start < *list2_start) {
            *biglist_start++ = *list1_start++;
        } else {
            *biglist_start++ = *list2_start++;
        }
    }

    while (list1_start < list1_end) {
        *biglist_start++ = *list1_start++;
    }

    while (list2_start < list2_end) {
        *biglist_start++ = *list2_start++;
    }

    int biglist_size = biglist_start - (biglist + 1);

    assert(biglist_size >= list1_size);
    assert(biglist_size >= list2_size);

    biglist[0] = biglist_size;
}

void print_list(int* list) {
    int list_size = list[0];
    printf("Printing list of size %d\n", list_size);

    for (int i = 1; i <= list_size; i++) {
        printf("%d ", list[i]);
    }
    printf("\n");
}

// Unions two sorted integer lists
int* naive_union(int* list1, int* list2){
    if (check_list_contained(list1, list2)) {
        return list1;
    } 
    else if (check_list_contained(list2, list1)) {
        return list2;
    } 

    int union_list_size = real_merge_size(list1, list2);
    int* union_list = malloc(sizeof(int) * (union_list_size + 1));
    union_list[0] = union_list_size;
    mergesort_list(list1, list2, union_list);
    return union_list;
}

// Uses a bounce buffer to merge tmp sets, and we perform ops on this tmp merged set
int* efficient_union(int* list1, int* list2, int* tmp_union_list){
    mergesort_sizeless_list(list1, list2, tmp_union_list);

    if (check_biglist_contained(list1, tmp_union_list)) {
        return list1;
    } else if (check_biglist_contained(list2, tmp_union_list)) {
        return list2;
    }

    int* real_union_list = malloc(sizeof(int) * (tmp_union_list[0] + 1));
    real_union_list[0] = tmp_union_list[0];
    for(int i = 1; i <= real_union_list[0]; i++){
        real_union_list[i] = tmp_union_list[i];
    }

    return real_union_list;
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

uint64_t total_tfi_eff_count(int nObjs, int nIns, int nLatches, int nOuts, int nAnds, int* pObjs) {
    const int start = (nIns + 1) * 2;
    // nIns + 1 because we include the CONST0

    // Initialize all the inputs to have a list of size 1
    int** tfi_set = malloc(sizeof(int*) * (nIns + nAnds + nOuts + 1));

    // Initialize the CONST0 node to have a list of size 0
    tfi_set[0] = calloc(2, sizeof(int));

    // Initialize the largest possible union list once and we just use it all the time
    int* tmp_union_list = malloc(sizeof(int) * (nIns + 1));

    for (int i = 1; i < nIns + 1; i++) {
        tfi_set[i] = malloc(sizeof(int) * 2);
        tfi_set[i][0] = 1;
        tfi_set[i][1] = i;
    }
    
    for (int i = 0; i < nAnds; i++) {
        int in1 = pObjs[start + 2 * i];
        int in2 = pObjs[start + 2 * i + 1];
        tfi_set[nIns + 1 + i] = efficient_union(tfi_set[lit_to_ulit(in1)], tfi_set[lit_to_ulit(in2)], tmp_union_list);
    }

    free(tmp_union_list);

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
    list2_test[4] = 6;
    if (check_list_contained(list1_test, list2_test)) {
        printf("list 2 contained in list 1!\n");
    } else {
        printf("Nope!\n");
    }
    int* combined_test = naive_union(list1_test, list2_test);
    printf("Combined test: \n");
    for (int i = 0; i < combined_test[0]; i++) {
        printf("%d ", combined_test[i + 1]);
    }
    printf("\n");

    printf("Performing the unit test on union_list:\n");
    int* list3_test = malloc(sizeof(int) * 6);
    list3_test[0] = 4;
    list3_test[1] = 1;
    list3_test[2] = 2;
    list3_test[3] = 3;
    list3_test[4] = 4;
    int* list4_test = malloc(sizeof(int) * 2);
    list4_test[0] = 0;
    list4_test[1] = 0;
    if (check_list_contained(list3_test, list4_test)) {
        printf("list 4 contained in list 3!\n");
    } else {
        printf("Nope!\n");
    }
    int* combined_test_2 = naive_union(list3_test, list4_test);
    printf("Combined test 2: \n");
    for (int i = 0; i < combined_test_2[0]; i++) {
        printf("%d ", combined_test_2[i + 1]);
    }
    printf("\n");*/

    float iter_tfi_start_time = (float)clock()/CLOCKS_PER_SEC;
    total_tfi_count(nObjs, nIns, nLatches, nOuts, nAnds, pObjs);
    float iter_tfi_end_time = (float)clock()/CLOCKS_PER_SEC;
    printf("Iterative TFI count time (sec): %0.5f\n\n", iter_tfi_end_time - iter_tfi_start_time);

    float iter_tfi_eff_start_time = (float)clock()/CLOCKS_PER_SEC;
    total_tfi_eff_count(nObjs, nIns, nLatches, nOuts, nAnds, pObjs);
    float iter_tfi_eff_end_time = (float)clock()/CLOCKS_PER_SEC;
    printf("Iterative TFI efficient count time (sec): %0.5f\n\n", iter_tfi_eff_end_time - iter_tfi_eff_start_time);

    float recur_tfi_start_time = (float)clock()/CLOCKS_PER_SEC;
    recursive_total_tfi_count(nObjs, nIns, nLatches, nOuts, nAnds, pObjs);
    float recur_tfi_end_time = (float)clock()/CLOCKS_PER_SEC;
    printf("Recursive TFI count time (sec): %0.5f\n", recur_tfi_end_time - recur_tfi_start_time);

    free(pObjs);
}


