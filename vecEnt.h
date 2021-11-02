
// This is a page-based vector of integer-based entries.

// "Page-based" means that the vector is allocated page by page 
// without ever needing to reallocate memory. By comparison, a conventional
// vector has a potential need for reallocing when the size reaches a limit.

// "Integer-based" means that the smallest entry stored in the vector
// is a 4-byte int. The vector is designed to store arrays of such entries.

// Each entry in the vector is represented by a unique "entry handle", 
// which is an offset in the array where the entry is located.
// The entry handle has two parts: the page number and the offset within a page.
// See ec_EntRead() below for how the handle is used to reach the entry.

// One argument given to Vec_EntAlloc() is Log2PageSize, which is a base-2 
// logarithm of the maximum number of 4-byte integers stored in one page.

#ifdef _WIN32
#define inline __inline
#endif

typedef struct Vec_Ent_t_ Vec_Ent_t;
struct Vec_Ent_t_ 
{
    int    Log2PageSize;  // log2 of page size (the max number of 4-byte ints in a page)
    int    PageMask;      // bit-mask needed to derive the in-page offset from an entry handle
    int    iNext;         // the total number of 4-byte ints stored (also, the handle of the next free entry in the vector)
    int    nPagesAlloc;   // the number of allocated pages
    int ** ppPages;       // memory pages
};

// initializes the page-based vector
static inline Vec_Ent_t * Vec_EntAlloc( int Log2PageSize )
{
    Vec_Ent_t * p   = (Vec_Ent_t*)calloc( sizeof(Vec_Ent_t), 1 );
    p->Log2PageSize = Log2PageSize;
    p->PageMask     = (1<<Log2PageSize)-1;
    return p;
}
// returns the entry with a given handle
static inline int Vec_EntRead( Vec_Ent_t * p, int Handle )
{
    return p->ppPages[Handle >> p->Log2PageSize][Handle & p->PageMask];
}
// returns the pointer to an existing entry with a given handle
static inline int * Vec_EntReadPtr( Vec_Ent_t * p, int Handle )
{
    return p->ppPages[Handle >> p->Log2PageSize] + (Handle & p->PageMask);
}
// returns the pointer to where the next array of entries (whose size is Size) can be saved
static inline int * Vec_EntFetch( Vec_Ent_t * p, int Size )
{
    int iPage = (p->iNext + Size) >> p->Log2PageSize;
    if ( iPage == p->nPagesAlloc )
    {
        int nPages = p->nPagesAlloc ? 2*p->nPagesAlloc : 4;
        p->ppPages = (int**)(p->ppPages ? realloc(p->ppPages, sizeof(int*)*nPages) : malloc(sizeof(int*)*nPages));
        memset( p->ppPages + p->nPagesAlloc, 0, sizeof(int*)*(nPages - p->nPagesAlloc) );
        p->nPagesAlloc = nPages;
    }
    assert( iPage < p->nPagesAlloc && Size <= (1 << p->Log2PageSize) );
    if ( p->iNext == 0 || (iPage > (p->iNext >> p->Log2PageSize) && p->ppPages[iPage] == NULL) )
    {
        p->ppPages[iPage] = (int *)malloc( sizeof(int)*(1 << p->Log2PageSize) );
        p->iNext = iPage * (1 << p->Log2PageSize);
    }
    return Vec_EntReadPtr( p, p->iNext );
}
// to be called after Vec_EntFetch() if we commit to using the fetched memory
static inline void Vec_EntCommit( Vec_Ent_t * p, int Size )
{
    assert( (p->iNext & p->PageMask) + Size <= (1 << p->Log2PageSize) );
    p->iNext += Size;
}
// deallocates the vector
static inline void Vec_EntFree( Vec_Ent_t * p )
{
    int i;
    for ( i = 0; i < p->nPagesAlloc; i++ )
        if ( p->ppPages[i] ) 
            free( p->ppPages[i] );
    free( p->ppPages );
    free( p );
}

