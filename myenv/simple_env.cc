#include "leveldb/simple_env.h"
namespace leveldb {
Status SimpleSequentialFile::Skip(uint64_t n) {
  index_ += n;
  if (n >= file_->GetFileLen()) {
    Status::IOError("skip error");
  }
  return Status::OK();
}

Status SimpleSequentialFile::Read(size_t n, Slice* result, char* scratch) {
  Status status = file_->Read(index_, n, result, scratch);
  index_ += result->size();
  return status;
}

bool SimpleLockTable::Insert(const std::string& fname) {
  mu_.lock();
  bool succeeded = locked_files_.insert(fname).second;
  mu_.unlock();
  return succeeded;
}

void SimpleLockTable::Remove(const std::string& fname) {
  mu_.lock();
  locked_files_.erase(fname);
  mu_.unlock();
}

}  // namespace leveldb