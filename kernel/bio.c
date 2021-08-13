// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBKT 13

struct {
  // struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBKT];
  struct spinlock bktlock[NBKT];
} bcache;

void
binit(void)
{
  struct buf *b;

  // initlock(&bcache.lock, "bcache");

  for(int i=0; i<NBKT; i++){
    initlock(&bcache.bktlock[i], "bcache");
    // Create linked list of buffers for each bucket
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

  // In the beginning, average allocate
  for(int i=0; i<NBUF; i++){
    b = &bcache.buf[i];
    int bkt = i%NBKT;
    // insert between head and head.next
    b->next = bcache.head[bkt].next;
    b->prev = &bcache.head[bkt];
    initsleeplock(&b->lock, "buffer");
    bcache.head[bkt].next->prev = b;
    bcache.head[bkt].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // acquire(&bcache.lock);

  int bkt = blockno % NBKT;
  acquire(&bcache.bktlock[bkt]);

  // Is the block already cached?
  for(b = bcache.head[bkt].next; b != &bcache.head[bkt]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bktlock[bkt]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  
  for(b = bcache.head[bkt].prev; b != &bcache.head[bkt]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bktlock[bkt]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  
  // steal from other bkt
  release(&bcache.bktlock[bkt]);
  
  int find = 0;
  for(int i=0; i<NBKT; i++){
    if(i==bkt) continue;
    acquire(&bcache.bktlock[i]);
    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0) {
        find = 1;
        // remove from bucket i
        b->prev->next = b->next;
        b->next->prev = b->prev;
        break;
      }
    }
    release(&bcache.bktlock[i]);
    if(find) break;
  }

  // insert to bucket
  acquire(&bcache.bktlock[bkt]);
  b->next = bcache.head[bkt].next;
  b->prev = &bcache.head[bkt];
  bcache.head[bkt].next->prev = b;
  bcache.head[bkt].next = b;

  // Is the block already cached?
  for(b = bcache.head[bkt].next; b != &bcache.head[bkt]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bktlock[bkt]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  
  for(b = bcache.head[bkt].prev; b != &bcache.head[bkt]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bktlock[bkt]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    // read from disk to bufs
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bkt = b->blockno % NBKT;

  acquire(&bcache.bktlock[bkt]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[bkt].next;
    b->prev = &bcache.head[bkt];
    bcache.head[bkt].next->prev = b;
    bcache.head[bkt].next = b;
  }
  
  release(&bcache.bktlock[bkt]);
}

void
bpin(struct buf *b) {
  int bkt = b->blockno % NBKT;
  acquire(&bcache.bktlock[bkt]);
  b->refcnt++;
  release(&bcache.bktlock[bkt]);
}

void
bunpin(struct buf *b) {
  int bkt = b->blockno % NBKT;
  acquire(&bcache.bktlock[bkt]);
  b->refcnt--;
  release(&bcache.bktlock[bkt]);
}


