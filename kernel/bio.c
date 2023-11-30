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

struct {
  struct spinlock lockList[NBUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf headList[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

//   initlock(&bcache.lock, "bcache");

//   // Create linked list of buffers
//   bcache.head.prev = &bcache.head;
//   bcache.head.next = &bcache.head;
//   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
//     b->next = bcache.head.next;
//     b->prev = &bcache.head;
//     initsleeplock(&b->lock, "buffer");
//     bcache.head.next->prev = b;
//     bcache.head.next = b;
  for (int i = 0; i < NBUCKET; ++i){
    initlock(&bcache.lockList[i], "bcache.bucket");
    bcache.headList[i].prev = &bcache.headList[i];
    bcache.headList[i].next = &bcache.headList[i];  
  }
  
  for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
    b->prev = &bcache.headList[0];
    b->next = bcache.headList[0].next;
    initsleeplock(&b->lock, "buffer");
    bcache.headList[0].next->prev = b;
    bcache.headList[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b, *head, *free;

  acquire(&bcache.lockList[hashid(blockno)]);
  head = &bcache.headList[hashid(blockno)];

  // Is the block already cached?
  for(b = head->next; b != head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lockList[hashid(blockno)]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(int i = 1; i < NBUCKET; i ++){
    acquire(&bcache.lockList[hashid(blockno + i)]);
    free = &bcache.headList[hashid(blockno + i)];
    for (b = free->prev; b != free; b = b->prev){
        if (b-> refcnt != 0) continue;
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        // remove from free
        b->next->prev = b->prev;
        b->prev->next = b->next;
        // insert to head
        b->next = head->next;
        b->prev = head;
        head -> next -> prev = b;
        head -> next = b;
        
        release(&bcache.lockList[hashid(blockno + i)]);
        release (&bcache.lockList[hashid(blockno)]);
        acquiresleep(&b->lock);
        return b;
    }
    release(&bcache.lockList[hashid(blockno + i)]);
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

  acquire(&bcache.lockList[hashid(b->blockno)]);
  b -> refcnt--;
  if (b-> refcnt == 0){
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = &bcache.headList[hashid(b->blockno)];
    b->prev = bcache.headList[hashid(b->blockno)].prev;
    bcache.headList[hashid(b->blockno)].prev->next = b;
    bcache.headList[hashid(b->blockno)].prev = b;
  }
  release(&bcache.lockList[hashid(b->blockno)]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lockList[hashid(b->blockno)]);
  b->refcnt++;
  release(&bcache.lockList[hashid(b->blockno)]);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lockList[hashid(b->blockno)]);
  b->refcnt--;
  release(&bcache.lockList[hashid(b->blockno)]);
}


