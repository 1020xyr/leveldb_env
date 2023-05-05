#include <atomic>
#include <iostream>
int main() {
  std::atomic<int> a(0);
  std::cout << std::atomic_fetch_add(&a, 1) << " " << std::atomic_load(&a) << std::endl;
}