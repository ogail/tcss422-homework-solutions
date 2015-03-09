#include <minix/blockdriver.h>
#include <stdlib.h>
#include <zlib.h>

//-----------------------------------------------------------------------------
// Useful constants and global variables.
//-----------------------------------------------------------------------------

// Ramdisk size, 2 MB.
#define RAMDISK_SIZE 0x200000
// The size of each block within the ramdisk, 4 KB.
#define RAMDISK_BLOCK_SIZE 0x1000
// The number of blocks within the ramdisk, 512.
#define RAMDISK_BLOCKS (RAMDISK_SIZE / RAMDISK_BLOCK_SIZE)
// A buffer for holding compressed data.
static unsigned char * compressed_data_buffer;
// The size of the buffer for compressed data.
static int compressed_data_buffer_size;
// A buffer for holding uncompressed data, exactly one block's worth of data.
static unsigned char uncompressed_data_buffer[RAMDISK_BLOCK_SIZE];
// An array of pointers to compressed ramdisk blocks.
// Initially, all elements are NULL.
static unsigned char * compressed_blocks[RAMDISK_BLOCKS];
// A parallel array of the sizes of each compressed ramdisk block.
// Initially, all elements are 0.
static unsigned long compressed_block_sizes[RAMDISK_BLOCKS];

//-----------------------------------------------------------------------------

/* Holds device information, e.g., ramdisk size. */
static struct device m_geom;
/* Tracks how many open requests are currently active for the device. */
static int open_counter;

static void sef_local_startup(void);
static int sef_cb_init_fresh(int type, sef_init_info_t *info);
static struct device *m_block_part(dev_t minor);
static int m_block_open(dev_t minor, int access);
static int m_block_close(dev_t minor);
static int m_block_transfer(dev_t minor, int do_write, u64_t position, endpoint_t endpt, iovec_t *iov, unsigned int nr_req, int flags);

/* Holds the callback functions for this device driver. */
static struct blockdriver m_bdtab = {
	BLOCKDRIVER_TYPE_DISK, /* handle partition requests */
	m_block_open,          /* open or mount */
	m_block_close,         /* close */
	m_block_transfer,      /* do the I/O */
	NULL,                  /* no I/O control */
	NULL,                  /* no need to clean up */
	m_block_part,          /* return partition information */
	NULL,                  /* no geometry */
	NULL,                  /* no interrupt processing */
	NULL,                  /* no alarm processing */
	NULL,                  /* no processing of other messages */
	NULL                   /* no threading support */
};


int main(void) {
	sef_local_startup();
	blockdriver_task(&m_bdtab);
	return OK;
}

static void sef_local_startup() {
	sef_setcb_init_fresh(sef_cb_init_fresh);
	sef_setcb_init_lu(sef_cb_init_fresh);
	sef_setcb_init_restart(sef_cb_init_fresh);
	sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
	sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
	sef_startup();
}

static int sef_cb_init_fresh(int UNUSED(type), sef_init_info_t *UNUSED(info)) {
	m_geom.dv_base = cvul64(0);
	m_geom.dv_size = cvul64(RAMDISK_SIZE);
	memset(compressed_blocks, 0, RAMDISK_BLOCKS * sizeof(unsigned char *));
	memset(compressed_block_sizes, 0, RAMDISK_BLOCKS * sizeof(int));
	compressed_data_buffer_size = compressBound(RAMDISK_BLOCK_SIZE);
	compressed_data_buffer = malloc(compressed_data_buffer_size);
	open_counter = 0;
	return OK;
}

static struct device *m_block_part(dev_t UNUSED(minor)) {
	return &m_geom;
}

static int m_block_open(dev_t UNUSED(minor), int UNUSED(access)) {
	open_counter++;
	return OK;
}

static int m_block_close(dev_t UNUSED(minor)) {
	if(open_counter < 1) {
		printf("CRD: closing unopened device.\n");
		return(EINVAL);
	}
	open_counter--;
	return OK;
}

static int m_block_transfer(
		dev_t UNUSED(minor), /* minor device number */
		int do_write,        /* read or write? */
		u64_t pos64,         /* offset on device to read or write */
		endpoint_t endpt,    /* process doing the request */
		iovec_t *iov,        /* pointer to read or write request vector */
		unsigned int nr_req, /* length of request vector */
		int UNUSED(flags)    /* transfer flags */
) {
	unsigned count;
	vir_bytes vir_offset = 0;
	int r;
	uLongf compression_length;
	off_t position;
	ssize_t total = 0;

	if (ex64hi(pos64) != 0)
		return OK;	/* Beyond EOF */
	position = cv64ul(pos64);

	while (nr_req > 0) {
		count = iov->iov_size;

//-----------------------------------------------------------------------------
// At this point in the code, the meaningful data transfer parameters are held
// in two variables, "position" and "count".  Position holds the index into
// the ramdisk at which the transfer will begin.  Count holds the number of
// bytes to be transfered.
//-----------------------------------------------------------------------------

		// If the transfer position is invalid, stop the transfer here.
		if (position >= m_geom.dv_size)
			return total;
		// If the transfer would extend beyond the end of the ramdisk, lower
		// the number of bytes to transfer so that the transfer stops at the
		// end of the ramdisk.
		if (position + count > m_geom.dv_size)
			count = m_geom.dv_size - position;
		// Calculate which block within the ramdisk the position refers to.
		int block_address = position / RAMDISK_BLOCK_SIZE;
		// Calculate the offset within the block that position refers to.
		int block_offset = position % RAMDISK_BLOCK_SIZE;
		// If the transfer would span multiple blocks, lower the number of
		// bytes to transfer so that only one block is involved.  Subsequent
		// iterations of the loop will transfer the rest of the data.
		if (count > RAMDISK_BLOCK_SIZE - block_offset)
			count = RAMDISK_BLOCK_SIZE - block_offset;
		if (!do_write) {
			// Reading from the ramdisk.
			if (compressed_blocks[block_address] == NULL)
				// The block doesn't exist, so we'll treat it as all 0's.
				memset(uncompressed_data_buffer, 0, RAMDISK_BLOCK_SIZE);
			else {
				// Copy the block's data to the buffer.
				memcpy(uncompressed_data_buffer, compressed_blocks[block_address], RAMDISK_BLOCK_SIZE);
				// Uncompress the read data
				r = uncompress(uncompressed_data_buffer, &compression_length, uncompressed_data_buffer, RAMDISK_BLOCK_SIZE);
				if (Z_OK != r || compression_length > RAMDISK_BLOCK_SIZE)
					panic("CRD: uncompression failed: %d", r);
			}

			// Transfer data from the ramdisk to another process.
			r = sys_safecopyto(endpt, iov->iov_addr, vir_offset, (vir_bytes)(uncompressed_data_buffer + block_offset), count);
		} else {
			// Writing to the ramdisk.
			if (count < RAMDISK_BLOCK_SIZE)
				// If the transfer is less than an entire block, we need the
				// block's current data since it will be only partially
				// overwritten.
				if (compressed_blocks[block_address] == NULL)
					// The block doesn't exist, so we'll treat it as all 0's.
					memset(uncompressed_data_buffer, 0, RAMDISK_BLOCK_SIZE);
				else
					// Copy the block's data to the buffer.
					memcpy(uncompressed_data_buffer, compressed_blocks[block_address], RAMDISK_BLOCK_SIZE);
			// Transfer data from another process to the ramdisk.
			r = sys_safecopyfrom(endpt, iov->iov_addr, vir_offset, (vir_bytes)(uncompressed_data_buffer + block_offset), count);
			// Free any previous storage for the block.
			free(compressed_blocks[block_address]);
			// Allocate new storage for the block.
			compressed_blocks[block_address] = malloc(RAMDISK_BLOCK_SIZE);
			// Compress the buffer
			r = compress(uncompressed_data_buffer, &compression_length, uncompressed_data_buffer, RAMDISK_BLOCK_SIZE);
			if (Z_OK != r || compression_length > RAMDISK_BLOCK_SIZE)
				panic("CRD: compression failed: %d", r);
			// Copy the buffer's data to the block.
			memcpy(compressed_blocks[block_address], uncompressed_data_buffer, RAMDISK_BLOCK_SIZE);

//-----------------------------------------------------------------------------
		}
		if (r != OK)
			panic("CRD: I/O copy failed: %d", r);

		/* Book the number of bytes transferred. */
		position += count;
		vir_offset += count;
		total += count;
		if ((iov->iov_size -= count) == 0) {
			iov++;
			nr_req--;
			vir_offset = 0;
		}
	}
	return total;
}
