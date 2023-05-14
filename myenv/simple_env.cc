#include <mutex>
#include <set>
#include "leveldb/env.h"
#include "myenv/filesystem.h"
namespace leveldb {

class SimpleSequentialFile : public SequentialFile {  // 顺序读文件
 public:
  SimpleSequentialFile(DataFile* file) : file_(file), index_(0) { file_->Ref(); }
  ~SimpleSequentialFile() override { file_->Unref(); }
  Status Read(size_t n, Slice* result, char* scratch) override;
  Status Skip(uint64_t n) override;

 private:
  DataFile* file_;
  uint64_t index_;  // 当前读取位置
};

class SimpleRandomAccessFile : public RandomAccessFile {  // 随机读文件
 public:
  SimpleRandomAccessFile(DataFile* file) : file_(file) { file_->Ref(); }
  ~SimpleRandomAccessFile() override { file_->Unref(); }
  Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const override {
    return file_->Read(offset, n, result, scratch);
  }

 private:
  DataFile* file_;
};

class SimpleWritableFile : public WritableFile {  // 可写文件
 public:
  SimpleWritableFile(DataFile* file) : file_(file) { file_->Ref(); }
  ~SimpleWritableFile() override { file_->Unref(); }
  Status Append(const Slice& data) override { return file_->Append(data); }
  Status Close() override { return Status::OK(); };
  Status Flush() override { return Status::OK(); };
  Status Sync() override { return Status::OK(); };

 private:
  DataFile* file_;
};

class SimpleEnv : public EnvWrapper {
 public:
  static SimpleEnv* GetInstance() {  // 内部静态变量的懒汉单例
    static SimpleEnv instance;
    return &instance;
  }

  Status NewSequentialFile(const std::string& fname, SequentialFile** result) override {
    DataFile* file = SIMPLE_SYSTEM->GetDataFile(fname);
    if (file == nullptr) {  // 若文件不存在则返回错误
      *result = nullptr;
      return Status::IOError(fname, "File not found");
    }
    *result = new SimpleSequentialFile(file);
    return Status::OK();
  }

  Status NewRandomAccessFile(const std::string& fname, RandomAccessFile** result) override {
    DataFile* file = SIMPLE_SYSTEM->GetDataFile(fname);
    if (file == nullptr) {  // 若文件不存在则返回错误
      *result = nullptr;
      return Status::IOError(fname, "File not found");
    }
    *result = new SimpleRandomAccessFile(file);
    return Status::OK();
  };

  Status NewWritableFile(const std::string& fname, WritableFile** result) override {
    DataFile* file = SIMPLE_SYSTEM->GetDataFile(fname);
    if (file == nullptr) {  // 文件不存在则创建文件
      file = SIMPLE_SYSTEM->NewDataFile(fname);
    } else {  // 文件存在则清空文件数据
      file->Truncate();
    }
    *result = new SimpleWritableFile(file);
    return Status::OK();
  }

  Status NewAppendableFile(const std::string& fname, WritableFile** result) override {
    DataFile* file = SIMPLE_SYSTEM->GetDataFile(fname);
    if (file == nullptr) {  // 文件不存在则创建文件
      file = SIMPLE_SYSTEM->NewDataFile(fname);
    }
    *result = new SimpleWritableFile(file);
    return Status::OK();
  }

  bool FileExists(const std::string& fname) override { return SIMPLE_SYSTEM->FileExists(fname); }

  Status GetChildren(const std::string& dir, std::vector<std::string>* result) override {
    return SIMPLE_SYSTEM->GetChildren(dir, result);
  };

  Status RemoveFile(const std::string& fname) override { return SIMPLE_SYSTEM->RemoveFile(fname); }

  Status CreateDir(const std::string& dirname) { return SIMPLE_SYSTEM->CreateDir(dirname); }

  Status RemoveDir(const std::string& dirname) { return SIMPLE_SYSTEM->RemoveDir(dirname); }

  Status GetFileSize(const std::string& fname, uint64_t* file_size) {
    return SIMPLE_SYSTEM->GetFileSize(fname, file_size);
  }

  Status RenameFile(const std::string& src, const std::string& target) {
    return SIMPLE_SYSTEM->RenameFile(src, target);
  };

  Status LockFile(const std::string& fname, FileLock** lock) { return Status::OK(); }

  Status UnlockFile(FileLock* lock) { return Status::OK(); }

 private:
  SimpleEnv() : EnvWrapper(Env::Default()){};  // 选择默认env实现初始化target
};

Status SimpleSequentialFile::Skip(uint64_t n) {
  uint64_t len = file_->GetFileLen();
  if (index_ > len) {
    return Status::IOError("pos_ > file_->Size()");
  }
  index_ = std::min(index_ + n, len);
  return Status::OK();
}

Status SimpleSequentialFile::Read(size_t n, Slice* result, char* scratch) {
  Status status = file_->Read(index_, n, result, scratch);
  if (status.ok()) {  // 更新读取位置
    index_ += result->size();
  }
  return status;
}

Env* Env::GetSimpleEnv() {  // 实现env.h中定义接口
  Env::GetMemEnv();
  return SimpleEnv::GetInstance();
}

}  // namespace leveldb