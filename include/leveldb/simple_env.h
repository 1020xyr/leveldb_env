#pragma once
#include <mutex>
#include <set>
#include "leveldb/env.h"
#include "leveldb/filesystem.h"

namespace leveldb {
class SimpleSequentialFile : public SequentialFile {
 public:
  SimpleSequentialFile(const std::string& fname) {
    file_ = SIMPLE_SYSTEM->NewDataFile(fname);
    index_ = 0;
  }
  Status Read(size_t n, Slice* result, char* scratch) override;
  Status Skip(uint64_t n) override;

 private:
  DataFile* file_;
  int index_;  // 当前读取位置
};

class SimpleRandomAccessFile : public RandomAccessFile {
 public:
  SimpleRandomAccessFile(const std::string& fname) { file_ = SIMPLE_SYSTEM->NewDataFile(fname); }
  Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const override {
    return file_->Read(offset, n, result, scratch);
  }

 private:
  DataFile* file_;
};

class SimpleWritableFile : public WritableFile {
 public:
  SimpleWritableFile(const std::string& fname) { file_ = SIMPLE_SYSTEM->NewDataFile(fname); }
  Status Append(const Slice& data) override { return file_->Append(data); }
  Status Close() override { return Status::OK(); };
  Status Flush() override { return Status::OK(); };
  Status Sync() override { return Status::OK(); };

 private:
  DataFile* file_;
};

class SimpleLockTable {
 public:
  bool Insert(const std::string& fname);
  void Remove(const std::string& fname);

 private:
  std::mutex mu_;
  std::set<std::string> locked_files_;
};

class SimpleFileLock : public FileLock {
 public:
  SimpleFileLock(std::string fname) : fname_(std::move(fname)) {}
  const std::string& fname() const { return fname_; }

 private:
  const std::string fname_;
};

class SimpleEnv : public EnvWrapper {
 public:
  static SimpleEnv* GetInstance() {  // 内部静态变量的懒汉单例
    static SimpleEnv instance;
    return &instance;
  }

  Status NewSequentialFile(const std::string& fname, SequentialFile** result) override {
    *result = new SimpleSequentialFile(fname);
    return Status::OK();
  }

  Status NewRandomAccessFile(const std::string& fname, RandomAccessFile** result) override {
    *result = new SimpleRandomAccessFile(fname);
    return Status::OK();
  };

  Status NewWritableFile(const std::string& fname, WritableFile** result) override {
    *result = new SimpleWritableFile(fname);
    return Status::OK();
  }

  Status NewAppendableFile(const std::string& fname, WritableFile** result) override {
    *result = new SimpleWritableFile(fname);
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
  SimpleEnv() : EnvWrapper(Env::Default()){};
  SimpleLockTable locks_;
};
}  // namespace leveldb