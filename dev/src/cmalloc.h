#include <stdlib.h>
#include <string.h>

#ifndef CMALLOC_H
#define CMALLOC_H

#ifndef POOL_SZ
#define POOL_SZ ( 5 * 1024 * 1024 )
#endif

#ifdef _ENBL_DESCRIPTOR
#define ADDR_RET void **
#define ADDR addr_t**
#else
#define ADDR_RET void *
#define ADDR addr_t*
#endif

#define arrcnt( _arr ) ( sizeof( _arr ) / *_arr )
#define NULL ((void*)0)

typedef unsigned int _uint32;
typedef unsigned long int _uint64;
typedef signed long int _sint64;

#ifdef __64BIT
typedef _uint64 addr_t;
typedef _uint64 sz_t;
#else
typedef _uint32 addr_t;
typedef _uint32 sz_t;
#endif

typedef struct
    {
    addr_t * addr;
    sz_t sz;
    } block_t;

typedef addr_t memory_t[ POOL_SZ / sizeof( addr_t ) ];
typedef block_t descriptor_t[ sizeof( memory_t ) / sizeof( block_t ) ];

extern descriptor_t memtbl;
extern addr_t memtbl_ptr;

extern memory_t mempool;
extern addr_t mempool_ptr;

#define get_ptr_idx( _tbl, _ptr ) ( _ptr - _tbl )

#define ceiling_to_nrst( _val, _mult ) \
    ( ( _val % _mult ) == 0 ? _val : _val + ( _val % _mult ) )

inline addr_t * offset_addr( addr_t * addr, _sint64 sz )
{
return (addr_t*)( (_uint64)addr + sz );
}

inline sz_t blksz( sz_t sz )
{
return ceiling_to_nrst( sz, sizeof( sz_t ) ) / sizeof( sz_t );
}

inline sz_t bytesz( sz_t sz )
{
return sz * sizeof( sz_t );
}

inline void cmalloc_init( void )
{
memset( memtbl, 0, sizeof( memtbl ) );
memtbl_ptr = 0;
memset( mempool, 0, sizeof( mempool ) );
mempool_ptr = 0;
}


/*
* Allocate memory of the defined size.
*
* NOTE: While the size that is passed in is not a multiple of sz_t,
*       the size is converted to "blocks", such that sz is a multiple
*       of sz_t.
*/
inline ADDR_RET cmalloc( sz_t sz )
{
sz = blksz( sz );
if( !( mempool_ptr + sz < arrcnt( mempool ) ) ) return (ADDR_RET)NULL;
memtbl[ memtbl_ptr ].addr = &mempool[ mempool_ptr ];
memtbl[ memtbl_ptr ].sz = sz;
mempool_ptr += sz;
return (ADDR_RET)&memtbl[ memtbl_ptr++ ].addr;
}


inline void cfree( ADDR base_addr )
{
_uint32 tblptr;
#ifdef _ENBL_DESCRIPTOR
tblptr = get_ptr_idx( memtbl, base_addr );
#else
for( int i = 0; i < memtbl_ptr; i++ ) {
    if( memtbl[ i ].addr == base_addr ) {
        tblptr = i;
        break;
    }
}
#endif

#ifdef _ENBL_DESCRIPTOR
_uint32 tbliter;
_uint32 mvsz = 0;
addr_t * from, * to;
for( tbliter = tblptr + 1; tbliter < memtbl_ptr; tbliter++ ) {
    if( memtbl[ tbliter ].addr != NULL ) {
        mvsz += memtbl[ tbliter ].sz;
        memtbl[ tbliter ].addr = offset_addr( memtbl[ tbliter ].addr, bytesz( memtbl[ tblptr ].sz ) );
        }
    }
if( mvsz != 0 ) {
    to = memtbl[ tblptr ].addr;
    from = offset_addr( to, bytesz( memtbl[ tblptr ].sz ) );
    memmove( to, from, bytesz( mvsz ) );
    }
#endif /* _ENBL_DESCRIPTOR */
memset( &memtbl[ tblptr ], 0, sizeof( memtbl[ tblptr ] ) );
if( tblptr == memtbl_ptr - 1 ) {
    while( memtbl[ --memtbl_ptr ].addr == NULL );
    }
}


#ifdef _ENBL_DESCRIPTOR
inline void cfree_sz( ADDR base_addr, sz_t sz )
{
_uint32 tblptr = get_ptr_idx( memtbl, base_addr );
_uint32 mvsz = 0;
_uint32 tbliter;
addr_t * from, * to;
sz = blksz( sz );
for( tbliter = tblptr + 1; tbliter < memtbl_ptr; tbliter++ ) {
    if( memtbl[ tbliter ].addr != NULL ) {
        mvsz += memtbl[ tbliter ].sz;
        memtbl[ tbliter ].addr = offset_addr( memtbl[ tbliter ].addr, -bytesz( sz ) );
        }
    }
if( mvsz != 0 ) {
    from = offset_addr( memtbl[ tblptr ].addr, bytesz( memtbl[ tblptr ].sz ) );
    to = offset_addr( from, -bytesz( sz ) );
    memset( to, 0, bytesz( sz ) );
    memmove( to, from, bytesz( mvsz ) );
    memset( offset_addr( memtbl[ memtbl_ptr - 1 ].addr, bytesz( sz ) ), 0, bytesz( memtbl[ memtbl_ptr ].sz ) );
    }
}


/*
* Increase a previously allocated memory pool.
*/
inline void incr_cmalloc( addr_t ** base_addr, sz_t sz )
{
_uint32 tblptr = get_ptr_idx( memtbl, base_addr );
_uint32 memptr = get_ptr_idx( mempool, *base_addr );
_uint32 tblptr_iter;
sz_t mvsz;
sz = blksz( sz );
memptr += sz;
for( mvsz = 0, tblptr_iter = tblptr + 1;
     tblptr_iter < memtbl_ptr;
     mvsz += memtbl[ tblptr_iter ].sz, memtbl[ tblptr_iter ].addr = offset_addr( memtbl[ tblptr_iter ].addr, bytesz( sz ) ), tblptr_iter++ );
memmove( mempool[ memptr ], mempool[ memptr - sz ] , bytesz( mvsz ) );
memset( mempool[ memptr - sz ], 0, bytesz( sz ) );
mempool_ptr += sz;
}


/*
* Assumes memory has already been increased.
*
* NOTE: Although this library defines memory in blocks, we cannot
*       assume the corresponding types project-side are aligned.
*       Need to calculate the actual addresses (not the block addrs)
*       for which memory is being moved around.
*/
inline void _cinsert( addr_t** base_addr, sz_t sz, _uint32 idx )
{
_uint32 tblptr = get_ptr_idx( memtbl, base_addr );
_uint32 memptr = get_ptr_idx( mempool, *base_addr );
addr_t * memfrom = (addr_t*)offset_addr( *base_addr, sz * idx );
addr_t * memto = (addr_t*)offset_addr( memfrom, sz );
_uint32 mvsz = memtbl[ tblptr ].sz - idx - 1;
mvsz = blksz( mvsz );
memmove( memfrom, memto, mvsz );
memset( memfrom, 0, sz );
}
#define cinsert( _addr, idx ) _cinsert( _addr, sizeof( **_addr ), idx )
#endif /* _ENBL_DESCRIPTOR */

#endif /* CMALLOC_H */
 