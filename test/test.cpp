#include <cassert>
#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

std::string RandStr(int min_len, int max_len) {  // 生成min_len-max_len之间大小的字符串
  int len = rand() % (max_len - min_len + 1) + min_len;
  std::string str;
  char c;
  int idx;
  /*循环向字符串中添加随机生成的字符*/
  for (idx = 0; idx < len; idx++) {
    c = 'a' + rand() % 26;
    str.push_back(c);
  }
  return str;
}

void PrintStats(leveldb::DB* db, std::string key) {
  std::string stats;
  if (!db->GetProperty(key, &stats)) {
    stats = "(failed)";
  }
  std::cout << key << std::endl << stats << std::endl;
}
int main() {
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;  // 不存在时创建数据库
  leveldb::Status status = leveldb::DB::Open(options, "./testdb", &db);

  std::vector<std::pair<std::string, std::string>> data;
  int data_size = 1 << 18;
  for (int i = 0; i < data_size; i++) {
    data.emplace_back(RandStr(10, 30), RandStr(100, 1000));
  }
  auto start = std::chrono::system_clock::now();
  printf("write part.\n");
  for (auto& kv : data) {
    leveldb::Status s = db->Put(leveldb::WriteOptions(), kv.first, kv.second);
    assert(status.ok());
  }
  std::random_shuffle(data.begin(), data.end());
  printf("read part.\n");
  for (int k = 0; k <= 100; k++) {
    for (int i = 0; i < data_size / 10; i++) {
      std::string value;
      leveldb::Status s = db->Get(leveldb::ReadOptions(), data[i].first, &value);
      assert(status.ok());
      assert(value == data[i].second);
    }
  }

  auto end = std::chrono::system_clock::now();
  printf("用时:%lds\n", std::chrono::duration_cast<std::chrono::seconds>(end - start).count());
  PrintStats(db, "leveldb.num-files-at-level0");
  PrintStats(db, "leveldb.num-files-at-level1");
  PrintStats(db, "leveldb.num-files-at-level2");
  PrintStats(db, "leveldb.num-files-at-level3");
  PrintStats(db, "leveldb.num-files-at-level4");
  PrintStats(db, "leveldb.num-files-at-level5");
  PrintStats(db, "leveldb.num-files-at-level6");
  PrintStats(db, "leveldb.stats");
  PrintStats(db, "leveldb.approximate-memory-usage");
  PrintStats(db, "leveldb.sstables");
  delete db;
  return 0;
}
