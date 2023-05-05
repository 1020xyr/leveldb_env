#include "leveldb/filesystem.h"

namespace leveldb {
std::vector<blk_no_t> BlockLayer::AllocBlocks(int num) {
  int pre_index = std::atomic_fetch_add(&cur_index_, num);
  std::vector<blk_no_t> blocks;
  for (blk_no_t i = pre_index; i < pre_index + num; i++) {
    blocks.emplace_back(i);
  }
  return blocks;
}

std::vector<std::string> Directory::GetAllChildren() {
  std::vector<std::string> ans;
  for (FileInfo info : childrens_) {
    ans.emplace_back(info.first);
  }
  return ans;
}

bool Directory::DeleteChildren(const std::string& path) {
  for (auto iter = childrens_.begin(); iter != childrens_.end(); ++iter) {
    if (iter->first == path) {
      childrens_.erase(iter);
      return true;
    }
  }
  return false;
}

Status DataFile::Read(uint64_t offset, size_t n, Slice* result, char* scratch) {
  Status status = Status::OK();
  BlockLayer* block_layer = SIMPLE_SYSTEM->GetBlockLayer();
  size_t read_size = 0;
  int index = offset / kBlockSize;       // 块索引
  int blk_offset = offset % kBlockSize;  // 块偏移
  while (read_size < n) {
    if (index > blocks_.size()) {  // 超出文件长度
      break;
    }
    if (index < blocks_.size()) {  // 数据在数据块中
      int size = std::min<int>(n - read_size, kBlockSize - blk_offset);
      block_layer->ReadBlock(blocks_[index], scratch + read_size, blk_offset, size);

      index++;
      blk_offset = 0;
      read_size += size;
    } else {  // 数据在缓冲区中
      int size = std::min<int>(n - read_size, buf_pos_ - blk_offset);
      memcpy(scratch + read_size, buf_ + blk_offset, size);

      index++;
      blk_offset = 0;
      read_size += size;
    }
  }
  *result = Slice(scratch, read_size);
  return status;
}

Status DataFile::Append(const Slice& data) {
  BlockLayer* block_layer = SIMPLE_SYSTEM->GetBlockLayer();
  const char* write_data = data.data();
  size_t write_size = data.size();
  // 第一步：将数据写入缓冲区
  size_t size = std::min(write_size, kBlockSize - buf_pos_);
  memcpy(buf_ + buf_pos_, write_data, size);

  write_data += size;
  write_size -= size;
  buf_pos_ += size;
  // 若缓冲区已满，写入数据块
  if (buf_pos_ == kBlockSize) {
    blk_no_t no = block_layer->AllocBlock();
    block_layer->WriteBlock(no, buf_, 0, kBlockSize);
    blocks_.emplace_back(no);
    buf_pos_ = 0;
  }

  // 第二步：将数据写入数据块
  while (write_size >= kBlockSize) {
    blk_no_t no = block_layer->AllocBlock();
    block_layer->WriteBlock(no, write_data, 0, kBlockSize);
    blocks_.emplace_back(no);

    write_data += kBlockSize;
    write_size -= kBlockSize;
  }

  // 第三步：将数据写入缓冲区
  if (write_size > 0) {
    memcpy(buf_, write_data, write_size);
    buf_pos_ += write_size;
  }
  return Status::OK();
}

std::string Helper::GetParentDir(const std::string& path) {
  std::size_t pos = path.find_last_of('/');
  if (pos == std::string::npos) {
    printf("path:%s directoy: null\n", path.c_str());
    return "";
  }
  std::string dir = path.substr(0, pos);  // 最后一个/前的字符串
  // printf("path:%s directoy:%s\n", path.c_str(), dir.c_str());
  return dir;
}

SimpleFileSystem* SimpleFileSystem::GetInstance() {  // 内部静态变量的懒汉单例
  static SimpleFileSystem instance;
  return &instance;
}

DataFile* SimpleFileSystem::NewDataFile(const std::string& fname) {
  printf("create data file:%s\n", fname.c_str());
  files_.emplace(fname, DataFile{});  // 添加文件名-数据文件映射
  auto dir = Helper::GetParentDir(fname);
  if (dirs_.count(dir) > 0) {  // 在父目录下添加文件信息
    dirs_[dir].AddChildren({fname, FileType::DataFile});
  }
  return &files_[fname];
}

Directory* SimpleFileSystem::NewDirectory(const std::string& dirname) {
  printf("create directory:%s\n", dirname.c_str());
  dirs_.emplace(dirname, Directory{});  // 添加文件名-目录文件映射
  auto dir = Helper::GetParentDir(dirname);
  if (dirs_.count(dir) > 0) {  // 在父目录下添加文件信息
    dirs_[dir].AddChildren({dirname, FileType::Directory});
  }
  return &dirs_[dirname];
}

DataFile* SimpleFileSystem::GetDataFile(const std::string& fname) {
  if (files_.count(fname) > 0) {
    return &files_[fname];
  }
  return nullptr;
}

Directory* SimpleFileSystem::GetDirectory(const std::string& dirname) {
  if (dirs_.count(dirname) > 0) {
    return &dirs_[dirname];
  }
  return nullptr;
}

Status SimpleFileSystem::GetChildren(const std::string& dirname, std::vector<std::string>* result) {
  result->clear();
  if (dirs_.count(dirname) == 0) {
    return Status::IOError("directory isn't exist.");
  }
  Directory directory = dirs_[dirname];
  *result = directory.GetAllChildren();
  return Status::OK();
}

Status SimpleFileSystem::RemoveFile(const std::string& fname) {
  printf("delete file %s\n", fname.c_str());
  std::string parent = Helper::GetParentDir(fname);
  if (dirs_.count(parent) == 0) {
    return Status::IOError("parent directory isn't exist.");
  }
  bool ret = dirs_[parent].DeleteChildren(fname);
  if (!ret) {
    return Status::IOError("can't find children.");
  }
  files_.erase(fname);
  return Status::OK();
}

Status SimpleFileSystem::CreateDir(const std::string& dirname) {
  NewDirectory(dirname);
  return Status::OK();
}
Status SimpleFileSystem::RemoveDir(const std::string& dirname) {
  printf("delete directory %s\n", dirname.c_str());
  std::string parent = Helper::GetParentDir(dirname);
  if (dirs_.count(parent) == 0) {
    return Status::IOError("parent directory isn't exist.");
  }
  bool ret = dirs_[parent].DeleteChildren(dirname);
  if (!ret) {
    return Status::IOError("can't find children.");
  }
  dirs_.erase(dirname);
  return Status::OK();
}

Status SimpleFileSystem::GetFileSize(const std::string& fname, uint64_t* file_size) {
  if (files_.count(fname) == 0) {
    return Status::IOError("file isn't exist.");
  }
  *file_size = files_[fname].GetFileLen();
  return Status::OK();
}

Status SimpleFileSystem::RenameFile(const std::string& src, const std::string& target) {
  if (files_.count(src) == 0) {
    return Status::IOError("file isn't exist.");
  }
  files_.emplace(target, files_[src]);  // 添加文件名-数据文件映射
  auto dir = Helper::GetParentDir(src);
  if (dirs_.count(dir) > 0) {  // 在父目录下添加文件信息
    dirs_[dir].DeleteChildren(src);
    dirs_[dir].AddChildren({target, FileType::DataFile});
  }
  return Status::OK();
}

}  // namespace leveldb