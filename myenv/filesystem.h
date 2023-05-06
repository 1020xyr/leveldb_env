#pragma once
#include <vector>
#include <string>
#include <atomic>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include "leveldb/status.h"

namespace leveldb {
const size_t kBlockSize = 1 << 12;    // 块大小
const size_t kBlockNumber = 1 << 18;  // 块总数
using blk_no_t = int;                 // 块号类型
const blk_no_t kInvaildBlock = -1;    // 无效块

// 数据块
struct Block {
  char data[kBlockSize];
};

// 块层
class BlockLayer {
 public:
  friend class SimpleFileSystem;

  // 禁用复制构造函数与赋值函数
  BlockLayer(const BlockLayer&) = delete;
  const BlockLayer& operator=(const BlockLayer&) = delete;

  // atomic_fetch_add 原子自增，返回旧值
  blk_no_t AllocBlock() { return std::atomic_fetch_add(&cur_index_, 1); }  // 分配单个块
  std::vector<blk_no_t> AllocBlocks(int num);                              // 分配多个块

  void WriteBlock(blk_no_t blk_no, const char* data, int offset, int len) {
    assert(blk_no < kBlockNumber);
    assert(data != nullptr);

    memcpy(blocks_[blk_no].data + offset, data, len);
  }  // 向块写入数据
  void ReadBlock(blk_no_t blk_no, char* data, int offset, int len) {
    assert(blk_no < kBlockNumber);
    assert(data != nullptr);

    memcpy(data, blocks_[blk_no].data + offset, len);
  }  // 读取块数据

 private:
  BlockLayer() : cur_index_(0) {}
  ~BlockLayer() {}

  std::atomic<blk_no_t> cur_index_;  // 当前分配索引
  Block blocks_[kBlockNumber];       // 块层所有数据块
};

class File {
 public:
  File() : ref_(1) {}
  // 禁止拷贝
  File(const File&) = delete;
  File& operator=(const File&) = delete;
  void Ref();
  void Unref();
  virtual ~File() = default;

 private:
  std::mutex ref_mutex_;
  int ref_;
};

enum class FileType { Directory, DataFile };        // 文件类型分为目录文件与数据文件
using FileInfo = std::pair<std::string, FileType>;  // 文件名-文件类型

class Directory : public File {
 public:
  Directory() = default;
  void AddChildren(FileInfo info);               // 添加子文件
  std::vector<std::string> GetAllChildren();     // 获取全部子文件
  bool DeleteChildren(const std::string& path);  // 删除子文件

 private:
  std::mutex data_mu_;
  std::vector<FileInfo> childrens_;  // 目录下的所有文件
};

class DataFile : public File {
 public:
  DataFile() : buf_pos_(0) {}
  Status Read(uint64_t offset, size_t n, Slice* result, char* scratch);     // 读取文件数据
  Status Append(const Slice& data);                                         // 在文件尾部追加数据
  void Truncate();                                                          // 清空文件数据
  uint64_t GetFileLen() { return blocks_.size() * kBlockSize + buf_pos_; }  // 获取文件长度

 private:
  std::mutex data_mu_;
  // 文件数据前面数据存在于数据块中，剩余数据存在于缓冲区中
  char buf_[kBlockSize];          // 文件缓冲区
  size_t buf_pos_;                // 缓冲区索引
  std::vector<blk_no_t> blocks_;  // 文件包含数据块
};

class Helper {
 public:
  static std::string GetParentDir(const std::string& path);  // 获取路径的父目录
};

class SimpleFileSystem {
 public:
  static SimpleFileSystem* GetInstance();  // 获取文件系统实例
  // 禁用复制构造函数与赋值函数
  SimpleFileSystem(const SimpleFileSystem&) = delete;
  const SimpleFileSystem& operator=(const SimpleFileSystem&) = delete;

  BlockLayer* GetBlockLayer() { return &block_layer_; }  // 获取块层指针
  DataFile* NewDataFile(const std::string& fname);       // 创建数据文件
  Directory* NewDirectory(const std::string& dirname);   // 创建目录文件
  DataFile* GetDataFile(const std::string& fname);       // 用文件名获取数据文件指针
  Directory* GetDirectory(const std::string& dirname);   // 用文件名获取目录文件指针

  bool FileExists(const std::string& fname);
  Status GetFileSize(const std::string& fname, uint64_t* file_size);
  Status GetChildren(const std::string& dirname, std::vector<std::string>* result);
  Status RemoveFile(const std::string& fname);
  Status CreateDir(const std::string& dirname);
  Status RemoveDir(const std::string& dirname);
  Status RenameFile(const std::string& src, const std::string& target);

 private:
  SimpleFileSystem() = default;
  ~SimpleFileSystem();

  BlockLayer block_layer_;

  std::mutex mu_;
  std::unordered_map<std::string, Directory*> dirs_;  // 文件名-目录文件映射
  std::unordered_map<std::string, DataFile*> files_;  // 文件名-数据文件映射
};

#define SIMPLE_SYSTEM SimpleFileSystem::GetInstance()
};  // namespace leveldb