#include "myenv/filesystem.h"

namespace leveldb {
std::vector<blk_no_t> BlockLayer::AllocBlocks(int num) {  // 分配多个块
  int pre_index = std::atomic_fetch_add(&cur_index_, num);
  std::vector<blk_no_t> blocks;
  for (blk_no_t i = pre_index; i < pre_index + num; i++) {
    blocks.emplace_back(i);
  }
  return blocks;
}

void File::Ref() {
  std::lock_guard<std::mutex> lk(ref_mutex_);
  ref_++;
}

void File::Unref() {
  bool do_delete = false;
  {
    std::lock_guard<std::mutex> lk(ref_mutex_);
    ref_--;
    if (ref_ <= 0) {  // 引用计数减至0，需删除
      do_delete = true;
    }
  }
  if (do_delete) {
    delete this;
  }
}

void Directory::AddChildren(FileInfo info) {
  std::lock_guard<std::mutex> lk(data_mu_);
  childrens_.emplace_back(info);
}

std::vector<std::string> Directory::GetAllChildren() {
  std::lock_guard<std::mutex> lk(data_mu_);
  std::vector<std::string> ans;
  for (FileInfo info : childrens_) {
    ans.emplace_back(info.first);
  }
  return ans;
}

bool Directory::DeleteChildren(const std::string& path) {
  std::lock_guard<std::mutex> lk(data_mu_);
  for (auto iter = childrens_.begin(); iter != childrens_.end(); ++iter) {
    if (iter->first == path) {
      childrens_.erase(iter);
      return true;
    }
  }
  return false;
}

Status DataFile::Read(uint64_t offset, size_t n, Slice* result, char* scratch) {
  std::lock_guard<std::mutex> lk(data_mu_);
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
  std::lock_guard<std::mutex> lk(data_mu_);
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

  // 第三步：将剩余数据写入缓冲区
  if (write_size > 0) {
    memcpy(buf_, write_data, write_size);
    buf_pos_ += write_size;
  }
  return Status::OK();
}

void DataFile::Truncate() {
  std::lock_guard<std::mutex> lk(data_mu_);
  blocks_.clear();
  buf_pos_ = 0;
}
uint64_t DataFile::GetFileLen() {
  std::lock_guard<std::mutex> lk(data_mu_);
  return blocks_.size() * kBlockSize + buf_pos_;
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
  std::lock_guard<std::mutex> lk(mu_);
  if (files_.count(fname) == 0) {  // 若不存在则创建新文件
    printf("create data file:%s\n", fname.c_str());
    DataFile* file = new DataFile();
    files_.emplace(fname, file);  // 添加文件名-数据文件映射

    // 在父目录下添加文件信息
    auto dir = Helper::GetParentDir(fname);
    if (dirs_.count(dir) > 0) {
      dirs_[dir]->AddChildren({fname, FileType::DataFile});
    }
  }
  return files_[fname];
}

Directory* SimpleFileSystem::NewDirectory(const std::string& dirname) {
  std::lock_guard<std::mutex> lk(mu_);
  if (dirs_.count(dirname) == 0) {  // 若不存在则创建新目录
    printf("create directory:%s\n", dirname.c_str());
    Directory* dir = new Directory();
    dirs_.emplace(dirname, dir);  // 添加文件名-目录文件映射

    // 在父目录下添加文件信息
    auto parent_dir = Helper::GetParentDir(dirname);
    if (dirs_.count(parent_dir) > 0) {
      dirs_[parent_dir]->AddChildren({dirname, FileType::Directory});
    }
  }
  return dirs_[dirname];
}

DataFile* SimpleFileSystem::GetDataFile(const std::string& fname) {
  std::lock_guard<std::mutex> lk(mu_);
  if (files_.count(fname) > 0) {
    return files_[fname];
  }
  return nullptr;
}

Directory* SimpleFileSystem::GetDirectory(const std::string& dirname) {
  std::lock_guard<std::mutex> lk(mu_);
  if (dirs_.count(dirname) > 0) {
    return dirs_[dirname];
  }
  return nullptr;
}
bool SimpleFileSystem::FileExists(const std::string& fname) {
  std::lock_guard<std::mutex> lk(mu_);
  return files_.count(fname) > 0;
}

Status SimpleFileSystem::GetChildren(const std::string& dirname, std::vector<std::string>* result) {
  std::lock_guard<std::mutex> lk(mu_);
  result->clear();
  if (dirs_.count(dirname) == 0) {
    return Status::IOError("directory isn't exist.");
  }
  Directory* directory = dirs_[dirname];
  *result = directory->GetAllChildren();
  return Status::OK();
}

Status SimpleFileSystem::RemoveFile(const std::string& fname) {
  std::lock_guard<std::mutex> lk(mu_);
  printf("delete file %s\n", fname.c_str());
  // 在父目录中删除该文件信息
  std::string parent = Helper::GetParentDir(fname);
  if (dirs_.count(parent) == 0) {
    return Status::IOError("parent directory isn't exist.");
  }
  if (!dirs_[parent]->DeleteChildren(fname)) {
    return Status::IOError("can't find children.");
  }

  // 删除实际文件
  if (files_.count(fname) == 0) {
    return Status::IOError(fname, "File not found");
  }
  files_[fname]->Unref();
  files_.erase(fname);
  return Status::OK();
}

Status SimpleFileSystem::CreateDir(const std::string& dirname) {
  NewDirectory(dirname);
  return Status::OK();
}

Status SimpleFileSystem::RemoveDir(const std::string& dirname) {
  std::lock_guard<std::mutex> lk(mu_);
  printf("delete directory %s\n", dirname.c_str());
  // 在父目录下删除目录信息
  std::string parent = Helper::GetParentDir(dirname);
  if (dirs_.count(parent) == 0) {
    return Status::IOError("parent directory isn't exist.");
  }

  if (!dirs_[parent]->DeleteChildren(dirname)) {
    return Status::IOError("can't find children.");
  }

  // 删除实际目录
  if (dirs_.count(dirname) == 0) {
    return Status::IOError(dirname, "directory not found");
  }
  dirs_[dirname]->Unref();  // 减小引用计数
  dirs_.erase(dirname);
  return Status::OK();
}

Status SimpleFileSystem::GetFileSize(const std::string& fname, uint64_t* file_size) {
  std::lock_guard<std::mutex> lk(mu_);
  if (files_.count(fname) == 0) {
    return Status::IOError("file isn't exist.");
  }
  *file_size = files_[fname]->GetFileLen();
  return Status::OK();
}

Status SimpleFileSystem::RenameFile(const std::string& src, const std::string& target) {
  std::lock_guard<std::mutex> lk(mu_);
  printf("%s rename to %s\n", src.c_str(), target.c_str());
  if (files_.count(src) == 0) {
    return Status::IOError("file isn't exist.");
  }
  files_.emplace(target, files_[src]);  // 添加文件名-数据文件映射
  files_.erase(src);                    // 删除旧文件名-数据文件映射

  // 在父目录下修改文件信息
  auto dir = Helper::GetParentDir(src);
  if (dirs_.count(dir) > 0) {
    dirs_[dir]->DeleteChildren(src);
    dirs_[dir]->AddChildren({target, FileType::DataFile});
  }
  return Status::OK();
}

SimpleFileSystem::~SimpleFileSystem() {  // 降低文件与目录的引用计数
  for (auto [name, p] : files_) {
    p->Unref();
  }

  for (auto [name, p] : dirs_) {
    p->Unref();
  }
}

}  // namespace leveldb