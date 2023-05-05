#include <typeinfo>
#include <iostream>

class Person {
 public:
  virtual ~Person() {}
};

class Employee : public Person {};

int main() {
  Person person;
  Employee employee;
  Person* ptr = &employee;
  Person& ref = employee;

  std::cout << typeid(int).name() << std::endl;
  std::cout << typeid(Employee).name() << std::endl;
  std::cout << typeid(*ptr).name() << std::endl;
  std::cout << typeid(ptr).name() << std::endl;
  std::cout << typeid(ref).name() << std::endl;

  return 0;
}