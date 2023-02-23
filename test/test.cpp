#include <cassert>
#include <iostream>
#include <string>
#include <chrono>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

int main() {
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;  // 不存在时创建数据库
  leveldb::Status status = leveldb::DB::Open(options, "./testdb", &db);
  leveldb::WriteBatch batch;
  batch.Put("aaa", "bbb");
  batch.Put("ccc", "ddd");
  leveldb::Status s = db->Write(leveldb::WriteOptions(), &batch);
  assert(status.ok());
  delete db;
  return 0;
}
