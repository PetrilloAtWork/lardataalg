/**
 * @file    CarefreePointer_test.cc
 * @brief   Tests the classes in `CarefreePointer.h`.
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * @date    March 22, 2019
 * @version 1.0
 * @see     `lardataalg/Utilities/CarefreePointer.h`
 */

// Boost libraries
#define BOOST_TEST_MODULE ( CarefreePointer_test )
#include <cetlib/quiet_unit_test.hpp> // BOOST_AUTO_TEST_CASE()
#include <boost/test/test_tools.hpp> // BOOST_CHECK(), BOOST_CHECK_EQUAL()

// LArSoft libraries
#include "lardataalg/Utilities/CarefreePointer.h"
#include "larcorealg/CoreUtils/counter.h"

// C/C++ standard libraries
#include <iostream>
#include <ostream>
#include <algorithm> // std::fill(), std::count()
#include <memory>
#include <vector>
#include <array>
#include <type_traits>
#include <cstddef> // std::size_t


//------------------------------------------------------------------------------
//--- Test code
//
struct TestObject {
  
  int id;
  
  static int count;
  
  TestObject() noexcept: id(count++) {}
  ~TestObject() noexcept { --count; }
  
  TestObject(TestObject const& from): id(from.id) { ++count; }
  TestObject(TestObject&& from): id(from.id) { ++count; }
  
}; // TestObject

std::ostream& operator<< (std::ostream& out, TestObject const& obj)
  { return out << "TestObject[" << ((void*) &obj) << "]{#" << obj.id << "}"; }

int TestObject::count = 0;


//------------------------------------------------------------------------------
template <bool Owned, typename CarefreePtr, typename Data>
void ObjectTest(CarefreePtr&& ptr, Data* dataPtr, int id) {
  
  BOOST_TEST_MESSAGE("Now testing with ownership: " << Owned << ".");
  
  // pointer is assumed to be non-null:
  BOOST_CHECK(ptr);
  BOOST_CHECK(bool(ptr));
  
  BOOST_CHECK_EQUAL(ptr.is_owning(), Owned);
  
  BOOST_CHECK_EQUAL(ptr.get(), dataPtr);
  
  BOOST_CHECK_EQUAL(ptr->id, id);
  
  if constexpr(!std::is_const_v<std::remove_reference_t<decltype(ptr)>>) {
    BOOST_TEST_MESSAGE("  -> now testing constant access");
    ObjectTest<Owned>(std::as_const(ptr), dataPtr, id);
  }
  
} // ObjectTest()


//------------------------------------------------------------------------------
template <bool Owned, typename CarefreePtr, typename Data>
void ArrayTest(CarefreePtr&& ptr, Data* dataPtr, std::size_t N) {
  
  BOOST_TEST_MESSAGE("Now testing with ownership: " << Owned << ".");
  
  // pointer is assumed to be non-null:
  BOOST_CHECK(ptr);
  BOOST_CHECK(bool(ptr));
  
  BOOST_CHECK_EQUAL(ptr.is_owning(), Owned);
  
  BOOST_CHECK_EQUAL(ptr.get(), dataPtr);
  
  for (std::size_t i: util::counter(N))
    BOOST_CHECK_EQUAL(ptr[i], dataPtr[i]);
  
  if constexpr(!std::is_const_v<std::remove_reference_t<decltype(ptr)>>) {
    BOOST_TEST_MESSAGE("  -> now testing constant access");
    ArrayTest<Owned>(std::as_const(ptr), dataPtr, N);
  }
  
} // ArrayTest()


//------------------------------------------------------------------------------
void BorrowedObjectTest() {
  
  BOOST_TEST_MESSAGE("Now testing on borrowed data.");
  unsigned int count = TestObject::count;
  {
    
    TestObject obj;
    BOOST_CHECK_EQUAL(TestObject::count, count + 1);
    
    {
      
      // this construction semantics implies borrowing
      util::CarefreePointer<TestObject> ptr { &obj };
      BOOST_CHECK_EQUAL(TestObject::count, count + 1);
      
      ObjectTest<false>(ptr, &obj, obj.id);
      BOOST_CHECK_EQUAL(TestObject::count, count + 1);
      
    }
    // carefree pointer just destroyed
    BOOST_CHECK_EQUAL(TestObject::count, count + 1);
  }
  
  // test object just destroyed
  BOOST_CHECK_EQUAL(TestObject::count, count);
  
} // BorrowedObjectTest()


//------------------------------------------------------------------------------
void OwnedObjectTest() {
  
  BOOST_TEST_MESSAGE("Now testing on owned data.");
  unsigned int count = TestObject::count;
  
  {
    
    auto dataPtr = new TestObject;
    
    // this construction semantics implies owning
    util::CarefreePointer<TestObject> ptr
      { std::unique_ptr<TestObject>(dataPtr) };
    
    ObjectTest<true>(ptr, dataPtr, dataPtr->id);
    
  }
  
  BOOST_CHECK_EQUAL(TestObject::count, count);
  
} // OwnedObjectTest()


//------------------------------------------------------------------------------
void NullObjectTest() {
  
  BOOST_TEST_MESSAGE("Now testing default-constructed pointer.");
  
  // start with a null pointer
  util::CarefreePointer<TestObject> ptr;
  
  BOOST_CHECK(!ptr);
  BOOST_CHECK(!ptr.is_owning());
  BOOST_CHECK_EQUAL(ptr, nullptr);
  
  BOOST_TEST_MESSAGE("Now moving a non-null pointer in.");
  auto dataPtr = new TestObject;
  
  ptr = std::unique_ptr<TestObject>(dataPtr);
  ObjectTest<true>(ptr, dataPtr, dataPtr->id);
  
} // NullObjectTest()


//------------------------------------------------------------------------------
void BorrowedArrayTest() {
  
  BOOST_TEST_MESSAGE("Now testing on borrowed array.");
  
  constexpr std::size_t N = 10U;
  constexpr int Value = 2;
  
  std::array<int, N> data;
  data.fill(Value);
  
  int* dataPtr = data.data();
  {
    
    // this construction semantics implies borrowing
    util::CarefreePointer<int> ptr { dataPtr };
    
    ArrayTest<false>(ptr, dataPtr, data.size());
    
  }
  
  // we have to verify that the memory from `data` was not deleted;
  // probably such an attempt on the stack would cause some major protest;
  // but just in case, we try to trigger a segmentation violation
  // if that address is not mapped any more
  BOOST_CHECK_EQUAL(std::count(dataPtr, dataPtr + N, Value), N);
  data.fill(Value + 1);
  BOOST_CHECK_EQUAL(std::count(dataPtr, dataPtr + N, Value + 1), N);
  
} // BorrowedObjectTest()


//------------------------------------------------------------------------------
void OwnedArrayTest() {
  
  BOOST_TEST_MESSAGE("Now testing on owned array.");
  
  constexpr std::size_t N = 10U;
  constexpr int Value = 2;
  
  int* dataPtr = new int[N];
  std::fill(dataPtr, dataPtr + N, Value);
  
  {
    
    // this construction semantics implies borrowing
    util::CarefreePointer<int[]> ptr { std::unique_ptr<int[]>(dataPtr) };
    
    ArrayTest<true>(ptr, dataPtr, N);
    
  }
  
  // we should verify out that the memory from `data` was deleted: good luck.
  
} // OwnedArrayTest()


//------------------------------------------------------------------------------
void DocTestConstructor1() {
/*
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * std::vector<int> data { 10U, 4 };
 * util::CarefreePointer<int const[]> dataPtr { data.data() };
 * 
 * for (std::size_t i = 0; i < data.size(); ++i)
 *   std::cout << dataPtr[i] << std::endl;
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
 
  std::vector<int> data(10U, 4);
  util::CarefreePointer<int const[]> dataPtr { data.data() };
  
  for (std::size_t i = 0; i < data.size(); ++i)
    std::cout << dataPtr[i] << std::endl;
  
  // ---------------------------------------------------------------------------
  std::vector<int> checked;
  for (auto const i: util::counter(data.size())) checked.push_back(dataPtr[i]);
  
  BOOST_CHECK(std::equal(cbegin(checked), cend(checked), data.cbegin()));

} // DocTestConstructor1()


//------------------------------------------------------------------------------
void DocTestConstructor2() {
/*
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * int const data[3U] = { 4, 5, 6 };
 * util::CarefreePointer<int const[]> dataPtr { data };
 * 
 * for (std::size_t i = 0; i < 3U; ++i)
 *   std::cout << dataPtr[i] << std::endl;
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
  
  int const data[3U] = { 4, 5, 6 };
  util::CarefreePointer<int const[]> dataPtr { data };
  
  for (std::size_t i = 0; i < 3U; ++i)
    std::cout << dataPtr[i] << std::endl;
  
  // ---------------------------------------------------------------------------
  static_assert(std::is_same_v<
    std::remove_reference_t<decltype(dataPtr)>,
    util::CarefreePointer<int const[]>
    >);
  
  std::vector<int> checked;
  for (auto const i: util::counter(std::extent_v<decltype(data)>))
    checked.push_back(dataPtr[i]);
  
  BOOST_CHECK(std::equal(cbegin(checked), cend(checked), data));

} // DocTestConstructor2()


//------------------------------------------------------------------------------
void DocTestConstructor3() {
/*
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * util::CarefreePointer dataPtr { std::make_unique<int[]>(10U) };
 * 
 * for (std::size_t i = 0; i < 10U; ++i) dataPtr[i] = i;
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
 
  util::CarefreePointer dataPtr { std::make_unique<int[]>(10U) };
  
  for (std::size_t i = 0; i < 10U; ++i) dataPtr[i] = i;
  
  // ---------------------------------------------------------------------------
  for (auto const i: util::counter(10U))
    BOOST_CHECK_EQUAL(dataPtr[i], i);
  
} // DocTestConstructor3()


//------------------------------------------------------------------------------
// static checks
void DeductionChecks() {
  
  static_assert(std::is_same_v<
    decltype(util::CarefreePointer{ std::make_unique<int[]>(10U) }),
    util::CarefreePointer<int[]>
    >);
  
  static_assert(std::is_same_v<
    decltype(util::CarefreePointer{ std::make_unique<int>(10U) }),
    util::CarefreePointer<int>
    >);
  
  int array5[5U];
  static_assert(std::is_same_v<
    decltype(util::CarefreePointer{ array5 }),
    util::CarefreePointer<int[]>
    >);
  
  /* // this type of deduction is not supported
  int* ptr;
  static_assert(std::is_same_v<
    decltype(util::CarefreePointer{ ptr }),
    util::CarefreePointer<int>
    >);
  */
  
} // StaticChecks()


//------------------------------------------------------------------------------
//--- registration of tests
//

BOOST_AUTO_TEST_CASE(StaticTestCase) {
  
  // not that a Boost test case is needed for static checks...
  DeductionChecks();
  
} // StaticTestCase

BOOST_AUTO_TEST_CASE(ObjectTestCase) {
  
  BorrowedObjectTest();
  OwnedObjectTest();
  NullObjectTest();
  
} // ObjectTestCase

BOOST_AUTO_TEST_CASE(ArrayTestCase) {
  
  BorrowedArrayTest();
  OwnedArrayTest();
  
} // ArrayTestCase

BOOST_AUTO_TEST_CASE(DocumentationTestCase) {
  
  DocTestConstructor1();
  DocTestConstructor2();
  DocTestConstructor3();
  
} // DocumentationTestCase
