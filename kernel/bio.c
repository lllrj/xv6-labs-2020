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

#define NBUCKET 13
inline int HASH(int id){
  return id % NBUCKET;
}
struct hashbuf {
  struct buf head;       // 头节点
  struct spinlock lock;  // 锁
};

struct {
  struct buf buf[NBUF];
  struct hashbuf buckets[NBUCKET];  // 散列桶
} bcache;

void
binit(void)
{
  struct buf *b;
  for(int i = 0; i < NBUCKET; i++){

    initlock(&bcache.buckets[i].lock, "bcache");
    // Create linked list of buffers
    bcache.buckets[i].head.prev = &bcache.buckets[i].head;
    bcache.buckets[i].head.next = &bcache.buckets[i].head;
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.buckets[0].head.next;
    b->prev = &bcache.buckets[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.buckets[0].head.next->prev = b;
    bcache.buckets[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = HASH(blockno);
  acquire(&bcache.buckets[id].lock);

  // Is the block already cached?
  for(b = bcache.buckets[id].head.next; b != &bcache.buckets[id].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;

      acquire(&tickslock);
      b->lastuse = ticks;
      release(&tickslock);

      release(&bcache.buckets[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  b = 0;
  for(int j = HASH(id + 1); j != id; j = HASH(j + 1)){
    // printf("j % 13 = %d\n", j%NBUCKET);
    // printf("id = %d, j = %d\n",id,j);
    if(!holding(&bcache.buckets[j].lock)){
      acquire(&bcache.buckets[j].lock);
    }else continue;

    for (struct buf*temp = bcache.buckets[j].head.next; temp != &bcache.buckets[j].head; temp = temp->next) {
      if (temp->refcnt == 0 && (b == 0 || b->lastuse > temp->lastuse)) {
        b = temp;
      }
    }
    if(b) {
      
      // 断链
      b->next->prev = b->prev;
      b->prev->next = b->next;
      release(&bcache.buckets[j].lock);

      b->next = bcache.buckets[id].head.next;
      b->prev = &bcache.buckets[id].head;
      bcache.buckets[id].head.next->prev = b;
      bcache.buckets[id].head.next = b;
    

      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      acquire(&tickslock);
      b->lastuse = ticks;
      release(&tickslock);

      release(&bcache.buckets[id].lock);
      acquiresleep(&b->lock);
      return b;
    } else {
      // 在当前散列桶中未找到，则直接释放锁
        release(&bcache.buckets[j].lock);
        //release(&bcache.buckets[id].lock);
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
  int id = HASH(b->blockno);

  acquire(&bcache.buckets[id].lock);
  b->refcnt--;

  acquire(&tickslock);
  b->lastuse = ticks;
  release(&tickslock);
  release(&bcache.buckets[id].lock);
}

void
bpin(struct buf *b) {
  int id = HASH(b->blockno);
  acquire(&bcache.buckets[id].lock);
  b->refcnt++;
  release(&bcache.buckets[id].lock);
}

void
bunpin(struct buf *b) {
  int id = HASH(b->blockno);
  acquire(&bcache.buckets[id].lock);
  b->refcnt--;
  release(&bcache.buckets[id].lock);
}


