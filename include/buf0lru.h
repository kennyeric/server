/******************************************************
The database buffer pool LRU replacement algorithm

(c) 1995 Innobase Oy

Created 11/5/1995 Heikki Tuuri
*******************************************************/

#ifndef buf0lru_h
#define buf0lru_h

#include "univ.i"
#include "ut0byte.h"
#include "buf0types.h"

/** The return type of buf_LRU_free_block() */
enum buf_lru_free_block_status {
	/** freed */
	BUF_LRU_FREED = 0,
	/** not freed because the caller asked to remove the
	uncompressed frame but the control block cannot be
	relocated */
	BUF_LRU_CANNOT_RELOCATE,
	/** not freed because of some other reason */
	BUF_LRU_NOT_FREED
};

/**********************************************************************
Tries to remove LRU flushed blocks from the end of the LRU list and put them
to the free list. This is beneficial for the efficiency of the insert buffer
operation, as flushed pages from non-unique non-clustered indexes are here
taken out of the buffer pool, and their inserts redirected to the insert
buffer. Otherwise, the flushed blocks could get modified again before read
operations need new buffer blocks, and the i/o work done in flushing would be
wasted. */
UNIV_INTERN
void
buf_LRU_try_free_flushed_blocks(void);
/*==================================*/
/**********************************************************************
Returns TRUE if less than 25 % of the buffer pool is available. This can be
used in heuristics to prevent huge transactions eating up the whole buffer
pool for their locks. */
UNIV_INTERN
ibool
buf_LRU_buf_pool_running_out(void);
/*==============================*/
				/* out: TRUE if less than 25 % of buffer pool
				left */

/*#######################################################################
These are low-level functions
#########################################################################*/

/* Minimum LRU list length for which the LRU_old pointer is defined */

#define BUF_LRU_OLD_MIN_LEN	80

#define BUF_LRU_FREE_SEARCH_LEN		(5 + 2 * BUF_READ_AHEAD_AREA)

/**********************************************************************
Invalidates all pages belonging to a given tablespace when we are deleting
the data file(s) of that tablespace. A PROBLEM: if readahead is being started,
what guarantees that it will not try to read in pages after this operation has
completed? */
UNIV_INTERN
void
buf_LRU_invalidate_tablespace(
/*==========================*/
	ulint	id);	/* in: space id */
/**********************************************************************
Gets the minimum LRU_position field for the blocks in an initial segment
(determined by BUF_LRU_INITIAL_RATIO) of the LRU list. The limit is not
guaranteed to be precise, because the ulint_clock may wrap around. */
UNIV_INTERN
ulint
buf_LRU_get_recent_limit(void);
/*==========================*/
			/* out: the limit; zero if could not determine it */
/************************************************************************
Insert a compressed block into buf_pool->zip_clean in the LRU order. */
UNIV_INTERN
void
buf_LRU_insert_zip_clean(
/*=====================*/
	buf_page_t*	bpage);	/* in: pointer to the block in question */

/**********************************************************************
Try to free a block.  If bpage is a descriptor of a compressed-only
page, the descriptor object will be freed as well.  If this function
returns BUF_LRU_FREED, it will not temporarily release
buf_pool_mutex. */
UNIV_INTERN
enum buf_lru_free_block_status
buf_LRU_free_block(
/*===============*/
				/* out: BUF_LRU_FREED if freed,
				BUF_LRU_CANNOT_RELOCATE or
				BUF_LRU_NOT_FREED otherwise. */
	buf_page_t*	bpage,	/* in: block to be freed */
	ibool		zip,	/* in: TRUE if should remove also the
				compressed page of an uncompressed page */
	ibool*		buf_pool_mutex_released,
				/* in: pointer to a variable that will
				be assigned TRUE if buf_pool_mutex
				was temporarily released, or NULL */
	ibool		have_LRU_mutex);
/**********************************************************************
Try to free a replaceable block. */
UNIV_INTERN
ibool
buf_LRU_search_and_free_block(
/*==========================*/
				/* out: TRUE if found and freed */
	ulint	n_iterations);	/* in: how many times this has been called
				repeatedly without result: a high value means
				that we should search farther; if
				n_iterations < 10, then we search
				n_iterations / 10 * buf_pool->curr_size
				pages from the end of the LRU list; if
				n_iterations < 5, then we will also search
				n_iterations / 5 of the unzip_LRU list. */
/**********************************************************************
Returns a free block from the buf_pool.  The block is taken off the
free list.  If it is empty, returns NULL. */
UNIV_INTERN
buf_block_t*
buf_LRU_get_free_only(void);
/*=======================*/
				/* out: a free control block, or NULL
				if the buf_block->free list is empty */
/**********************************************************************
Returns a free block from the buf_pool. The block is taken off the
free list. If it is empty, blocks are moved from the end of the
LRU list to the free list. */
UNIV_INTERN
buf_block_t*
buf_LRU_get_free_block(
/*===================*/
				/* out: the free control block,
				in state BUF_BLOCK_READY_FOR_USE */
	ulint	zip_size);	/* in: compressed page size in bytes,
				or 0 if uncompressed tablespace */

/**********************************************************************
Puts a block back to the free list. */
UNIV_INTERN
void
buf_LRU_block_free_non_file_page(
/*=============================*/
	buf_block_t*	block,	/* in: block, must not contain a file page */
	ibool		have_page_hash_mutex);
/**********************************************************************
Adds a block to the LRU list. */
UNIV_INTERN
void
buf_LRU_add_block(
/*==============*/
	buf_page_t*	bpage,	/* in: control block */
	ibool		old);	/* in: TRUE if should be put to the old
				blocks in the LRU list, else put to the
				start; if the LRU list is very short, added to
				the start regardless of this parameter */
/**********************************************************************
Adds a block to the LRU list of decompressed zip pages. */
UNIV_INTERN
void
buf_unzip_LRU_add_block(
/*====================*/
	buf_block_t*	block,	/* in: control block */
	ibool		old);	/* in: TRUE if should be put to the end
				of the list, else put to the start */
/**********************************************************************
Moves a block to the start of the LRU list. */
UNIV_INTERN
void
buf_LRU_make_block_young(
/*=====================*/
	buf_page_t*	bpage);	/* in: control block */
/**********************************************************************
Moves a block to the end of the LRU list. */
UNIV_INTERN
void
buf_LRU_make_block_old(
/*===================*/
	buf_page_t*	bpage);	/* in: control block */
/************************************************************************
Update the historical stats that we are collecting for LRU eviction
policy at the end of each interval. */
UNIV_INTERN
void
buf_LRU_stat_update(void);
/*=====================*/

#if defined UNIV_DEBUG || defined UNIV_BUF_DEBUG
/**************************************************************************
Validates the LRU list. */
UNIV_INTERN
ibool
buf_LRU_validate(void);
/*==================*/
#endif /* UNIV_DEBUG || UNIV_BUF_DEBUG */
#if defined UNIV_DEBUG_PRINT || defined UNIV_DEBUG || defined UNIV_BUF_DEBUG
/**************************************************************************
Prints the LRU list. */
UNIV_INTERN
void
buf_LRU_print(void);
/*===============*/
#endif /* UNIV_DEBUG_PRINT || UNIV_DEBUG || UNIV_BUF_DEBUG */

/**********************************************************************
These statistics are not 'of' LRU but 'for' LRU.  We keep count of I/O
and page_zip_decompress() operations.  Based on the statistics we decide
if we want to evict from buf_pool->unzip_LRU or buf_pool->LRU. */

/** Statistics for selecting the LRU list for eviction. */
struct buf_LRU_stat_struct
{
	ulint	io;	/**< Counter of buffer pool I/O operations. */
	ulint	unzip;	/**< Counter of page_zip_decompress operations. */
};

typedef struct buf_LRU_stat_struct buf_LRU_stat_t;

/** Current operation counters.  Not protected by any mutex.
Cleared by buf_LRU_stat_update(). */
extern buf_LRU_stat_t	buf_LRU_stat_cur;

/** Running sum of past values of buf_LRU_stat_cur.
Updated by buf_LRU_stat_update().  Protected by buf_pool_mutex. */
extern buf_LRU_stat_t	buf_LRU_stat_sum;

/************************************************************************
Increments the I/O counter in buf_LRU_stat_cur. */
#define buf_LRU_stat_inc_io() buf_LRU_stat_cur.io++
/************************************************************************
Increments the page_zip_decompress() counter in buf_LRU_stat_cur. */
#define buf_LRU_stat_inc_unzip() buf_LRU_stat_cur.unzip++

#ifndef UNIV_NONINL
#include "buf0lru.ic"
#endif

#endif
