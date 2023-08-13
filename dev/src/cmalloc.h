#include <stdlib.h>

#ifndef CMALLOC_H
#define CMALLOC_H

#ifndef POOL_SZ
#define POOL_SZ ( 5 * 1024 * 1024 )
#endif

#ifdef _ENBL_DESCRIPTOR
#define ADDR_RET void **
#else
#define ADDR_RET void *
#endif

#define arrcnt( _arr ) ( sizeof( _arr ) / *_arr )
#define NULL ((void*)0)

typedef unsigned int _uint32;
typedef unsigned long int _uint64;

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
typedef block_t descriptor_t[ memory_t / sizeof( block_t ) ];

#ifdef _ENBL_DESCRIPTOR
extern descriptor_t memtbl;
extern addr_t memtbl_ptr;
#endif /* _ENBL_DESCRIPTOR */

extern block_t mempool;
extern addr_t mempool_ptr;

#define get_ptr_idx( _tbl, _ptr ) ( _ptr - _tbl )

#define ceiling_to_nrst( _val, _mult ) \
    ( ( _val % _mult ) == 0 ? _val : _val + ( _val % _mult ) )

#define offset_addr( _addr, _sz ) \
    ( (addr_t)_addr + (addr_t)_sz )

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
#ifdef _ENBL_DESCRIPTOR
memset( memtbl, 0, sizeof( memtbl ) );
memtbl_ptr = 0;
#endif /* _ENBL_DESCRIPTOR */
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
if( !( mempool_ptr + sz < arrcnt( mempool ) ) return (ADDR_RET)NULL;

#ifdef _ENBL_DESCRIPTOR
memtbl[ memtbl_ptr ].addr = &mempool[ mempool_ptr ];
memtbl[ memtbl_ptr ].sz = sz;
mempool_ptr += sz;
return (ADDR_RET)&memtbl[ memtbl_ptr++ ].addr;
#else
mempool_ptr += sz;
return (ADDR_RET)&mempool[ mempool_ptr - sz ];
#endif /* _ENBL_DESCRIPTOR */
}

#ifdef _ENBL_DESCRIPTOR
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
#endif /* _ENBL_DESCRIPTOR */

#ifdef _ENBL_DESCRIPTOR
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
