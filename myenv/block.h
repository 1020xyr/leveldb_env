#include <cstring>
namespace MyEnv {
const int kBlockSize = 1 << 12;
const int kBlockNumber = 1 << 18;
using blk_no_t = int;

struct Block {
  char data[kBlockSize];
};

class BlockLayer {
  BlockLayer() : cur_index_(0) {}
  blk_no_t AllocBlock() { return cur_index_++; }
  void WriteBlock(blk_no_t blk_no, char* data, int len) { memcpy(blocks_[blk_no].data, data, len); }
  void ReadBlock(blk_no_t blk_no, char* data, int len) { memcpy(data, blocks_[blk_no].data, len); }

 private:
  blk_no_t cur_index_;
  Block blocks_[kBlockNumber];
};

}  // namespace MyEnv
