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

int real_merge_size(int* list1, int* list2) {
    int list1_size = list1[0];
    int list2_size = list2[0];
    int tmp_merge_size = list1_size + list2_size;
    int test = 0;

    int* ptr_list1 = list1 + 1;
    int* ptr_list2 = list2 + 1;

    for (int i = 0; i < tmp_merge_size; i++) {
        // Our ptr is at the end, so WLOG set it to be max 
        if (list1_size == 0 && list2_size == 0) {
            return test;
        }

        if ((*(ptr_list1) < *(ptr_list2) && list1_size > 0) || (list2_size == 0 && list1_size > 0)) {
            // Increment the ptr for list1
            ptr_list1++;
            list1_size--;
        } else if ((*(ptr_list1) > *(ptr_list2) && list2_size > 0) || (list1_size == 0 && list2_size > 0)) {
            // Increment the ptr for list2
            ptr_list2++;
            list2_size--;
        } else {
            // If the two lists are equal at the same position
            // Then we only incorporate one value to prevent duplicates
            // Increment both pointers to remove duplicates
            ptr_list1++;
            ptr_list2++;
            list1_size--;
            list2_size--;
        }
        test++;
    }

    return test;
}


// Checks if sorted list2 is contained in sorted list1
bool check_list_contained(int* list1, int* list2) {
    int real_size = real_merge_size(list1, list2);
    int list2_size = list2[0];

    int* ptr_list1 = list1 + 1;
    int* ptr_list2 = list2 + 1;

    int equal_cnt = 0; 

    for (int i = 0; i < real_size; i++) {
        if (*(ptr_list1) < *(ptr_list2)) {
            ptr_list1++;
        } else if (*(ptr_list1) > *(ptr_list2)) {
            ptr_list2++;
        } else {
            ptr_list1++;
            ptr_list2++;
            equal_cnt++;
        }
    }
    
    if (equal_cnt == list2_size) {
        return true;
    } else {
        return false;
    }
}


// Merges two sorted integer lists into a larger sorter list
void mergesort_list(int* list1, int* list2, int* sorted_list) {

    int list1_size = list1[0];
    int list2_size = list2[0];
    int sorted_list_size = sorted_list[0];

    int* ptr_list1 = list1 + 1;
    int* ptr_list2 = list2 + 1;
    int* ptr_sorted = sorted_list + 1;

    int ptr_list1_tracker = 0;
    int ptr_list2_tracker = 0;

    for (int i = 0; i < sorted_list_size; i++) {
        // Our ptr is at the end, so WLOG set it to be max 
        if ((*(ptr_list1) < *(ptr_list2) && list1_size > 0) || (list2_size == 0 && list1_size > 0)) {
            // Increment the ptr for list1
            sorted_list[i + 1] = *ptr_list1;
            //printf("list1 %d\n", *ptr_list1);
            ptr_list1++;
            list1_size--;
        } else if ((*(ptr_list1) > *(ptr_list2) && list2_size > 0) || (list1_size == 0 && list2_size > 0)) {
            // Increment the ptr for list2
            sorted_list[i + 1] = *ptr_list2;
            //printf("list2 %d\n", *ptr_list2);
            ptr_list2++;
            list2_size--;
        } else {
            // If the two lists are equal at the same position
            // Then we only incorporate one value to prevent duplicates
            // Increment both pointers to remove duplicates
            sorted_list[i + 1] = *ptr_list1;
            ptr_list1++;
            ptr_list2++;
            list1_size--;
            list2_size--;
        }
    }
}

// Unions two sorted integer lists
int* naive_union(int* list1, int* list2){
    int union_list_size = real_merge_size(list1, list2);
    if(union_list_size == 0){
        printf("wack %d %d\n", list1[0], list2[0]);
    }
    int* union_list = malloc(sizeof(int) * (union_list_size + 1));
    union_list[0] = union_list_size;
    
    mergesort_list(list1, list2, union_list);

    return union_list;
}

uint64_t total_tfi_count(int nObjs, int nIns, int nLatches, int nOuts, int nAnds, int* pObjs) {
    const int start = (nIns + 1) * 2;
    // nIns + 1 because we include the CONST0

    // Initialize all the inputs to have a list of size 1
    int** tfi_set = malloc(sizeof(int*) * (nIns + nAnds + nOuts + 1));

    // Initialize the CONST0 node to have a list of size 0
    tfi_set[0] = calloc(1, sizeof(int));

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

    total_tfi_count(nObjs, nIns, nLatches, nOuts, nAnds, pObjs);

    free(pObjs);
}


