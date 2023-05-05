class Test {
 public:
  int info() { return a; }  // 内联函数
  void print();             // 非内联函数
 private:
  int a;
};

void Test::print() {
  int a;
  int b = a + 1;
}