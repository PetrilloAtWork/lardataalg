/**
 * @file   lardataalg/Utilities/CarefreePointer.h
 * @brief  Provides `util::CarefreePointer` class.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   June 3, 2020
 *
 * This is a header-only library.
 */

#ifndef LARDATAALG_UTILITIES_CAREFREEPOINTER_H
#define LARDATAALG_UTILITIES_CAREFREEPOINTER_H


// C/C++ standard libraries
#include <memory> // std::unique_ptr<>
#include <variant>
#include <utility> // std::move()
#include <type_traits> // std::conditional_t<>, std::is_void_v<>
#include <cassert>



// -----------------------------------------------------------------------------
namespace util {
  template <typename T, typename Deleter> class CarefreePointer;
  
  /// Contains `value` `true` if `T` is a `CarefreePointer`.
  template <typename T> struct is_carefree_pointer;
  
  /// `true` if `T` is a `CarefreePointer`.
  template <typename T>
  constexpr auto is_carefree_pointer_v = is_carefree_pointer<T>::value;
} // namespace util


/**
 * @brief A pointer which takes care of freeing its memory, _if_ owned.
 * @tparam T the type of data pointed to
 * @tparam Deleter the type of deleter being used
 * 
 * This "smart" pointer guarantees the correct disposal of the allocated memory
 * hosting its data. The disposal action may be to free that memory, if this
 * object owns the data, or it may be no action, if this object does not own
 * that data.
 * 
 * The ownership is established on construction, and it can't be changed after
 * that.
 * This object undergoes all the limitations of a `std::unique_ptr`: it can't
 * be copied but it can be moved. Moving is in fact the only way to change the
 * data ownership.
 * Also note that, like `std::unique_ptr`, this object does not allocate memory,
 * but it rather manages memory that was allocated beforehand.
 * 
 * Example of usage of non-owning pointer on a data array:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * std::vector<int> data { 10U, 4 };
 * util::CarefreePointer<int const[]> dataPtr { data.data() };
 * 
 * for (std::size_t i = 0; i < data.size(); ++i)
 *   std::cout << dataPtr[i] << std::endl;
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * At the end of the scope, `dataPtr` will not free its memory (`data` will, so
 * no memory leak will happen).
 * Note that the contained type is an array type with no specified size,
 * `int const[]`. Another way to initialize a non-owning pointer is directly
 * from an array, like in:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * int const data[3U] = { 4, 5, 6 };
 * util::CarefreePointer<int const[]> dataPtr { data };
 * 
 * for (std::size_t i = 0; i < 3U; ++i)
 *   std::cout << dataPtr[i] << std::endl;
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 * Initialization of an object owning its data always happens via unique
 * pointers. This is an example with a data array:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * util::CarefreePointer<int[]> dataPtr { std::make_unique<int[]>(10U) };
 * 
 * for (std::size_t i = 0; i < 10U; ++i) dataPtr[i] = i;
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 * While some deduction guides are in place that allow the correct instantiation
 * from a declaration like `util::CarefreePointer ptr{ ... };`, the important
 * case of the argument being a pointer is not supported. It is recommended that
 * the type is always specified, with special care to the presence of the array
 * specification (e.g. `int[]`).
 * 
 * @note Differently from normal pointers, a constant `CarefreePointer` allows
 *       only constant access to the pointed data.
 * 
 * On demand, the interface can be extended toward `std::unique_ptr` (`reset()`,
 * `release()`, `swap()`, comparisons, etc.).
 */
template <typename T, typename Deleter = void>
class util::CarefreePointer {
  
  /// Exact type this class is defined for.
  using Contained_t = T;
  
  /// The type of pointer used when owning the data.
  using OwningPointer_t = std::conditional_t<
    std::is_void_v<Deleter>,
    std::unique_ptr<Contained_t>, std::unique_ptr<Contained_t, Deleter>
    >;
  
  
    public:
  
  // --- BEGIN -- Types --------------------------------------------------------
  
  /// Type of data pointed to.
  using element_type = typename OwningPointer_t::element_type;
  
  /// Type of direct pointer to data.
  using pointer = typename OwningPointer_t::pointer;
  
  /// Actual type of deleter.
  using deleter_type = typename OwningPointer_t::deleter_type;
  
  // --- END -- Types ----------------------------------------------------------
  
  
  // --- BEGIN -- Constructors -------------------------------------------------
  
  /// Constructor: a non-owning, null pointer.
  CarefreePointer(): CarefreePointer(nullptr) {}
  
  /**
   * @brief Constructor: points to `dataPtr` _not owning its data_.
   * @param dataPtr pointer to the existing data
   */
  CarefreePointer(pointer dataPtr): fPtr(dataPtr) {}
  
  /// Constructor: steals `dataPtr`, _owning its data_.
  CarefreePointer(std::unique_ptr<Contained_t, deleter_type>&& dataPtr)
    : fPtr(std::move(dataPtr)) {}
  
  /// Constructor: moves `data` into our _owned data structure_.
  CarefreePointer(element_type&& data)
    : fPtr{ std::make_unique<element_type>(std::move(data)) } {}
  
  // --- END -- Constructors ---------------------------------------------------
  
  
  // --- BEGIN -- Access -------------------------------------------------------
  
  /// Returns a constant pointer to the beginning of data.
  element_type const* get() const;
  
  /// Returns a pointer to the beginning of data.
  element_type* get();
  
  /// Returns a constant pointer to the beginning of data.
  operator element_type const* () const { return get(); }
  
  /// Returns a pointer to the beginning of data.
  operator element_type* () { return get(); }
  
  /// Returns a reference to the pointed object.
  element_type& operator*() { return *get(); }
  
  /// Returns a constant reference to the pointed object.
  element_type const& operator*() const { return *get(); }
  
  /// Returns a reference to the pointed object.
  element_type* operator->() { return get(); }
  
  /// Returns a constant reference to the pointed object.
  element_type const* operator->() const { return get(); }
  
  // --- END -- Access ---------------------------------------------------------
  
  
  // --- BEGIN -- Pointer information ------------------------------------------
  /**
   * @name Queries to the smart pointer
   * 
   * As opposed to the contained data or the plain C pointer to that data.
   */
  /// @{
  
  /// Returns if the pointer currently owns the data it points to.
  bool is_owning() const
    { return std::holds_alternative<OwningPointer_t>(fPtr); }
  
  /// @}
  // --- END -- Pointer information --------------------------------------------
  
    private:
  
  /// The type of pointer used when not owning the data.
  using NonOwningPointer_t = pointer;
  
  /// Container of the actual pointer to the data.
  std::variant<OwningPointer_t, NonOwningPointer_t> fPtr;
  
  
}; // class util::CarefreePointer<>


namespace util {
  
  //
  // util::CarefreePointer deduction guide
  //
  
  template <typename T, std::size_t N>
  CarefreePointer(T(&)[N]) -> CarefreePointer<std::remove_reference_t<T>[]>;
  
} // namespace util



//------------------------------------------------------------------------------
//--- util::CarefreePointer
//------------------------------------------------------------------------------
template <typename T, typename Deleter> class CarefreePointer;

namespace util {
  
  namespace details {
    
    template <typename Coll, typename = void>
    struct is_carefree_pointer_impl: std::false_type {};
    
    template <typename... Args>
    struct is_carefree_pointer_impl<CarefreePointer<Args...>>
      : std::true_type
    {};
    
  } // namespace details
  
  template <typename T>
  struct is_carefree_pointer: details::is_carefree_pointer_impl<T> {};
  
  
} // namespace util



//------------------------------------------------------------------------------
template <typename T, typename Deleter>
auto util::CarefreePointer<T, Deleter>::get() const -> element_type const* {
  if (auto pPtr = std::get_if<OwningPointer_t>(&fPtr))    return pPtr->get();
  if (auto pPtr = std::get_if<NonOwningPointer_t>(&fPtr)) return *pPtr;
  assert(false);
  return nullptr; // quiet the compiler
} // util::CarefreePointer<T, Deleter>::get() const


//------------------------------------------------------------------------------
template <typename T, typename Deleter>
auto util::CarefreePointer<T, Deleter>::get() -> element_type* {
  if (auto pPtr = std::get_if<OwningPointer_t>(&fPtr))    return pPtr->get();
  if (auto pPtr = std::get_if<NonOwningPointer_t>(&fPtr)) return *pPtr;
  assert(false);
  return nullptr; // quiet the compiler
} // util::CarefreePointer<T, Deleter>::get()


//------------------------------------------------------------------------------


#endif // LARDATAALG_UTILITIES_CAREFREEPOINTER_H
