/**
 * @file   lardataalg/Utilities/MappedContainer.h
 * @brief  Provides `MappedContainer` class.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   March 22, 2019
 *
 * This is a header-only library.
 */

#ifndef LARDATAALG_UTILITIES_MAPPEDCONTAINER_H
#define LARDATAALG_UTILITIES_MAPPEDCONTAINER_H

// LArSoft libraries
#include "larcorealg/CoreUtils/ContainerMeta.h" // util::collection_value_access_t
#include "larcorealg/CoreUtils/MetaUtils.h" // util::with_const_as_t

// C/C++ standard libraries
#include <string> // std::to_string()
#include <iterator> // std::iterator_category, std::size(), ...
#include <memory> // std::unique_ptr<>
#include <algorithm> // std::reference_wrapper<>
#include <stdexcept> // std::out_of_range
#include <limits> // std::numeric_limits<>
#include <cstddef> // std::size_t


namespace util {

  namespace details {

    // ---------------------------------------------------------------------------
    template <typename Cont> struct ContainerTraits;

    // ---------------------------------------------------------------------------
    template <typename Cont, typename = void> class ContainerStorage;

    // ---------------------------------------------------------------------------

  } // namespace details

  // ---------------------------------------------------------------------------
  /// Non-template base class for `MappedContainer`.
  struct MappedContainerBase {

    template <typename T = std::size_t>
    static constexpr T invalidIndex() { return std::numeric_limits<T>::max(); }

  }; // struct MappedContainerBase


  // ---------------------------------------------------------------------------
  /**
   * @brief A meta-container providing transparent mapping on top of another.
   * @tparam Cont type of the underlying container
   * @tparam Mapping type of the mapping to be applied
   *
   *
   * The mapped data is not permanently stored in memory, but rather the mapping
   * is applied anew on each request. If a permanent mapped container is
   * desired, a copy of this container into a standard one (e.g. a
   * `std::vector`) can be used.
   *
   * Note that the data itself is not modified by this container (although it
   * may be just a copy of the original one: see note about storage below).
   *
   * Example of usage:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * constexpr auto InvalidIndex = util::MappedContainerBase::invalidIndex();
   *
   * std::array<double, 4U> const data { 0.0, -1.0, -2.0, -3.0 };
   *
   * std::array<std::size_t, 6U> const mapping = {
   *   1U, 0U, InvalidIndex,
   *   3U, 2U, InvalidIndex,
   * };
   *
   * util::MappedContainer const mappedData
   *   (std::ref(data), mapping, 6U, std::numeric_limits<double>::quiet_NaN());
   *
   * for (std::size_t i = 0; i < 6U; ++i) {
   *   std::cout
   *     << "Mapped element #" << i << ": " << mappedData[i] << std::endl;
   * }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * which will print a list similar to: `-1.0`, `0.0`, `nan`, `-3.0`, `-2.0`
   * and `nan`.
   * Note that wrapping the original data into `std::ref` ensures that data is
   * referenced rather than copied, while (in this example) the mapping is
   * instead just copied. Wrapping it in `std::cref` would ensure the access to
   * be in the `const` fashion even if the data array were not constant.
   * The argument `6U` ensures that the container can address 6 indices.
   * In this example, it is needed only because then we specify a default value;
   * `MappedContainer` is otherwise able to use as size the size of the mapping,
   * if the mapping has one
   * (see `MappedContainer(DataContainer_t const&, Mapping_t const&)`).
   * Also note that in the example the C++17 template argument deduction is used
   * and it's not necessary to specify those argument explicitly (it would be
   * `util::MappedContainer<std::array<double, 4U> const, std::array<std::size_t, 6U> const>`).
   *
   *
   * Storage and ownership of the mapped data
   * -----------------------------------------
   *
   * If `Cont` is a C++ reference type, a C pointer, or a
   * `std::reference_wrapper` object, the original container is referenced.
   * Otherwise, a copy of it is internally stored with all data duplicated
   * according to the container copy constructor.
   *
   *
   * C++ standard
   * -------------
   *
   * This object fulfills most of the "Container" requirements, except for:
   * * container comparisons: they can be implemented if needed, but they would
   *     be slow
   * * `swap()`: did nor really bother to...
   *
   * It also fulfills most of the "SequentialContainer" requirements that are
   * not about construction or modification of the container size.
   *
   */
  template <typename Cont, typename Mapping>
  class MappedContainer: private MappedContainerBase {

    /// Type of object used for storage.
    using Storage_t = details::ContainerStorage<Cont>;

    /// Type of object used for mapping storage.
    using MappingStorage_t = details::ContainerStorage<Mapping>;

    /// Type of contained value.
    using Value_t = typename Storage_t::value_type;

    using Size_t = std::size_t; ///< Type for describing container size.

    Storage_t fData; ///< Data to be mapped.

    MappingStorage_t fMapping; ///< Mapping of stored data into final one.

    Size_t fSize = 0U; ///< Nominal size of the container.

    // constantness needs to be removed in order for this object to be copiable
    ///< Value returned for elements not mapped.
    std::remove_cv_t<Value_t> fDefValue {};


    template <typename Container, typename Reference>
    class IteratorBase;


      public:

    using DataContainer_t = Cont; ///< Type of the original container.
    using Mapping_t = Mapping; ///< Type of the mapping object.

    /// Type of this class.
    using MappedContainer_t = MappedContainer<DataContainer_t, Mapping_t>;

    /// Type of the index passed to the original container.
    using DataIndex_t = util::collection_value_t<Mapping_t>;

    /// Type of the index passed to the mapping.
    using MappingIndex_t = std::size_t;


    // --- BEGIN -- C++ standard container definitions -------------------------
    /// @name C++ standard container definitions
    /// @{
    /// Type of datum in the container.
    using value_type = Value_t;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using reference = util::with_const_as_t<
      typename Storage_t::reference,
      util::collection_value_access_t<DataContainer_t>
      >;
    using const_reference = typename Storage_t::const_reference;

    using iterator = IteratorBase<MappedContainer_t, reference>;
    using const_iterator
      = IteratorBase<MappedContainer_t const, const_reference>;

    /// @}
    // --- END -- C++ standard container definitions ---------------------------


    /// Invalid index to be returned by the mapping when the required index is
    /// not mapped back to the original container; in that case, a
    /// `defaultValue()` is mapped instead.
    static constexpr DataIndex_t InvalidIndex = invalidIndex<DataIndex_t>();


    // --- BEGIN Constructors --------------------------------------------------
    /// @name Constructors
    /// @{

    /// Default constructor: container will be unusable until assigned to.
    MappedContainer() = default;

    /**
     * @brief Constructor: acquires data, mapping and a default value.
     * @param cont container with the data to be mapped
     * @param mapping the mapping to be used
     * @param size the size of the container after mapping
     * @param defValue value to be used as default
     *
     * The `defValue` value is returned for the requested elements which are not
     * mapped to the original container (`InvalidIndex`).
     */
    MappedContainer(
      DataContainer_t const& cont,
      Mapping_t const& mapping,
      size_type size,
      value_type defValue
      )
      : fData(cont), fMapping(mapping), fSize(size), fDefValue(defValue)
      {}

    /**
     * @brief Constructor: acquires data and mapping.
     * @param cont container with the data to be mapped
     * @param mapping the mapping to be used
     * @param size the size of the container after mapping
     *
     * The default value is a default-constructed `value_type` (`0` for numeric
     * types, `nullptr` for pointers).
     */
    MappedContainer
      (DataContainer_t const& cont, Mapping_t const& mapping, size_type size)
      : MappedContainer(cont, mapping, size, {})
      {}

    /**
     * @brief Constructor: acquires data and mapping.
     * @param cont container with the data to be mapped
     * @param mapping the mapping to be used
     *
     * The size of the container is declared to be the minimal one
     * (see `minimal_size()`).
     * The default value is a default-constructed `value_type` (`0` for numeric
     * types, `nullptr` for pointers).
     */
    MappedContainer(DataContainer_t const& cont, Mapping_t const& mapping)
      : MappedContainer(cont, mapping, minimal_size(cont, mapping))
      {}

    /**
     * @brief Constructor: steals data, acquires mapping and a default value.
     * @param cont container with the data to be mapped
     * @param mapping the mapping to be used
     * @param size the size of the container after mapping
     * @param defValue value to be used as default
     *
     * The `defValue` value is returned for the requested elements which are not
     * mapped to the original container (`InvalidIndex`).
     */
    MappedContainer(
      DataContainer_t&& cont,
      Mapping_t const& mapping,
      size_type size,
      value_type defValue
      )
      : fData(std::move(cont)), fMapping(mapping)
      , fSize(size), fDefValue(defValue)
      {}

    /**
     * @brief Constructor: steals data and acquires mapping.
     * @param cont container with the data to be mapped
     * @param mapping the mapping to be used
     * @param size the size of the container after mapping
     *
     * The default value is a default-constructed `value_type` (`0` for numeric
     * types, `nullptr` for pointers).
     */
    MappedContainer
      (DataContainer_t&& cont, Mapping_t const& mapping, size_type size)
      : MappedContainer(std::move(cont), mapping, size, {})
      {}

    /**
     * @brief Constructor: steals data and acquires mapping.
     * @param cont container with the data to be mapped
     * @param mapping the mapping to be used
     *
     * The size of the container is declared to be the minimal one
     * (see `minimal_size()`).
     * The default value is a default-constructed `value_type` (`0` for numeric
     * types, `nullptr` for pointers).
     */
    MappedContainer(DataContainer_t&& cont, Mapping_t const& mapping)
      : MappedContainer(std::move(cont), mapping, minimal_size(cont, mapping))
      {}

    /// @}
    // --- END Constructors ----------------------------------------------------


    // --- BEGIN Container information -----------------------------------------
    /**
     * @name Container information
     *
     * The size of the container after mapping is not strictly defined, since
     * there might be elements not present in the original container, as well
     * as those elements might appear more than once.
     *
     * On top of this, this object does not need to know the size to correctly
     * operate, because no storage is used for the container data after mapping.
     *
     * Nevertheless, users may rightfully wonder and ask how many elements of
     * the container are valid after the mapping.
     *
     * This class allows an answer to be provided at construction time, relying
     * on the superior wisdom of the user. If the user does not care to impart
     * such wisdom, a guess is made with using the minimal size needed to
     * accommodate all the elements after mapping (see `minimal_size()`).
     */
    /// @{

    /**
     * @brief Returns the nominal size of the container (after mapping).
     *
     * This is the value provided at construction time, or the `minimal_size()`
     * at that time if no value was provided.
     */
    size_type size() const { return fSize; }

    /**
     * @brief Returns the minimum size to include all mapped values.
     *
     * This method is available only if the mapping type (`Mapping_t`) answers
     * a `std::size()` call, which is expected to return the number of elements
     * the original data can be mapped into.
     */
    size_type minimal_size() const;

    /// Returns the size of the largest possible container of this type.
    size_type max_size() const { return std::numeric_limits<size_type>::max(); }

    /// Returns whether the container has no elements.
    bool empty() const { return size() == 0U; }


    // @{
    /**
     * @brief Returns the default value for elements with no original content.
     *
     * Note that changing it will change at the same time the value returned
     * for all unmapped elements afterwards.
     */
    reference       defaultValue()       { return fDefValue; }
    const_reference defaultValue() const { return fDefValue; }
    // @}


    /// Changes the default value.
    /// The new value will be used for all following mappings.
    void setDefaultValue(value_type defValue) { fDefValue = defValue; }


    /// @}
    // --- END Container information -------------------------------------------


    // --- BEGIN Random access to elements -------------------------------------
    /// @name Random access to elements
    /// @{

    // @{
    /**
     * @brief Returns the content corresponding to the specified `index`.
     * @param index the index of the data to be retrieved
     * @return the mapped value (some decoration of `value_type`)
     *
     * The content requested for `index` is fetched from the original data, at
     * the element resulting from the mapping of the `index` argument.
     */
    decltype(auto) operator[] (MappingIndex_t index) const
      { return map_element(index); }
    decltype(auto) operator[] (MappingIndex_t index)
      { return map_element(index); }
    // @}

    // @{
    /**
     * @brief Returns the content corresponding to the specified `index`.
     * @param index the index of the data to be retrieved
     * @return the mapped value (some decoration of `value_type`)
     * @throw std::out_of_range if the index is not in the container
     */
    decltype(auto) at(MappingIndex_t index) const;
    decltype(auto) at(MappingIndex_t index);
    // @}


    // @{
    /**
     * @brief Returns the first element in the container.
     * @return the first element in the container, undefined if `empty()`
     */
    decltype(auto) front() const { return map_element(0U); }
    decltype(auto) front()       { return map_element(0U); }
    // @}

    // @{
    /**
     * @brief Returns the last element in the container.
     * @return the last element in the container, undefined if `empty()`
     */
    decltype(auto) back() const { return map_element(size() - 1); }
    decltype(auto) back()       { return map_element(size() - 1); }
    // @}


    // @{
    /**
     * @brief Returns the index in the original data which is mapped to `index`.
     * @param index the index to be mapped (like e.g. in `at()`)
     * @return the index in the original data container, or `InvalidIndex`
     */
    decltype(auto) map_index(MappingIndex_t index) const;
    decltype(auto) map_index(MappingIndex_t index);
    // @}


    /// @}
    // --- END Random access to elements ---------------------------------------


    // --- BEGIN Iteration -----------------------------------------------------
    /// @name Iteration
    /// @{

    /// Returns a constant iterator to the first mapped element.
    const_iterator cbegin() const { return { *this, 0U }; }

    /// Returns a constant iterator to the first mapped element.
    const_iterator begin() const { return cbegin(); }

    /// Returns an iterator to the first mapped element.
    iterator begin() { return { *this, 0U }; }

    /// Returns a constant iterator past the last mapped element.
    const_iterator cend() const { return { *this, size() }; }

    /// Returns a constant iterator past the last mapped element.
    const_iterator end() const { return cend(); }

    /// Returns an iterator past the last mapped element.
    iterator end() { return { *this, size() }; }

    /// @}
    // --- END Iteration -------------------------------------------------------

      protected:

    /// Returns the minimum size to include all mapped values.
    static size_type minimal_size
      (DataContainer_t const& cont, Mapping_t const& mapping);


      private:

    /// Returns the value mapped to the specified `index`.
    decltype(auto) map_element(MappingIndex_t index);

    /// Returns the value mapped to the specified `index`.
    decltype(auto) map_element(MappingIndex_t index) const;


  }; // class MappedContainer<>


  //----------------------------------------------------------------------------
  /**
   * @brief Returns a container-like object mapping the content of `cont`.
   * @tparam Cont type of the original container
   * @tparam Mapping type of mapping object
   * @param cont the container
   * @param mapping the mapping to be applied
   * @return a `util::MappedContainer` object transparently applying the mapping
   *
   * See `util::MappedContainer` for the details on the format of the mapping,
   * the ownership of the container and the supported operations.
   */
  template <typename Cont, typename Mapping>
  auto mapContainer(Cont cont, Mapping mapping)
    { return MappedContainer<Cont, Mapping>(cont, mapping); }


  //----------------------------------------------------------------------------


} // namespace util


//------------------------------------------------------------------------------
//---  template implementation
//------------------------------------------------------------------------------
namespace util {

  namespace details {

    //--------------------------------------------------------------------------
    template <typename T>
    T& NullRef() { T* nullTptr = nullptr; return *nullTptr; }


    //--------------------------------------------------------------------------
    //---  ContainerStorage
    //--------------------------------------------------------------------------
    template <typename Cont>
    struct ContainerStorageBase {

      using Container_t = Cont;
      using Traits_t = details::ContainerTraits<Container_t>;

      using index_type      = typename Traits_t::size_type;

      using value_type      = typename Traits_t::value_type;
      using size_type       = typename Traits_t::size_type;
      using difference_type = typename Traits_t::difference_type;
      using reference       = typename Traits_t::reference;
      using const_reference = typename Traits_t::const_reference;

      Container_t fCont;

      ContainerStorageBase() = default;
      explicit ContainerStorageBase(Container_t const& cont): fCont(cont) {}
      explicit ContainerStorageBase(Container_t&& cont)
        : fCont(std::move(cont)) {}

      decltype(auto) container() const
        { return util::collection_from_reference(fCont); }
      decltype(auto) container()
        { return util::collection_from_reference(fCont); }

      auto size() const { using std::size; return size(container()); }

      decltype(auto) operator[] (index_type index)
        { return container()[index]; }
      decltype(auto) operator[] (index_type index) const
        { return container()[index]; }

    }; // struct ContainerStorageBase


    //--------------------------------------------------------------------------
    template <typename Cont, typename /* = void */>
    class ContainerStorage: public ContainerStorageBase<Cont> {

      using Base_t = ContainerStorageBase<Cont>;
      
        public:
      // inherit all constructors
//       using Base_t::Base_t;

      ContainerStorage() = default;
      explicit ContainerStorage(Cont const& cont): Base_t(cont) {}
      explicit ContainerStorage(Cont&& cont): Base_t(std::move(cont)) {}
      
    }; // struct ContainerStorage


    template <typename Cont>
    class ContainerStorage
      <Cont, std::enable_if_t<util::is_reference_wrapper_v<Cont>>>
      : public ContainerStorageBase<Cont>
    {
      using Base_t = ContainerStorageBase<Cont>;
      using DataContainer_t = typename Cont::type;

        public:
      // inherit all constructors
      using Base_t::Base_t;

      // sad hack :-(
      // calling the inherited constructor with a `reference_wrapper`
      // (`Base_t::Container_t`) referencing an invalid reference to
      // the wrapped data type (which is `DataContainer_t`).
      ContainerStorage()
        : Base_t(typename Base_t::Container_t{NullRef<DataContainer_t>()})
        {}

    }; // struct ContainerStorage


    //--------------------------------------------------------------------------


  } // namespace details


  //----------------------------------------------------------------------------
  //--- MappedContainer::IteratorBase
  //----------------------------------------------------------------------------
  template <typename Cont, typename Mapping>
  template <typename Container, typename Reference>
  class MappedContainer<Cont, Mapping>::IteratorBase {

    using Container_t = Container;
    using Reference_t = Reference;

    /// This type.
    using Iterator_t = IteratorBase<Container_t, Reference_t>;

    Container_t* fCont = nullptr; ///< Pointer to the container.
    MappingIndex_t fIndex = InvalidIndex; ///< Current index in container.

      public:

    // --- BEGIN Traits --------------------------------------------------------
    /// @name Traits
    /// @{

    using value_type        = std::remove_cv_t<typename Container_t::value_type>;
    using difference_type   = typename Container_t::difference_type;
    using size_type         = typename Container_t::size_type;
    using reference         = Reference_t;
    using pointer           = decltype(&std::declval<reference>());
    using iterator_category = std::input_iterator_tag;

    /// @}
    // --- END Traits ----------------------------------------------------------


    /// Constructor: an invalid iterator.
    IteratorBase() = default;

    /// Constructor: iterator pointing to element `index` of `cont`.
    IteratorBase(Container_t& cont, MappingIndex_t index)
      : fCont(&cont), fIndex(index)
      {}

    /// Copy constructor.
    IteratorBase(Iterator_t const&) = default;

    /// Copy constructor: from a different container type.
    template <typename OC, typename OR>
    IteratorBase(IteratorBase<OC, OR> const& from)
      : fCont(from.cont), fIndex(from.index)
      {}

    /// Copy assignment.
    Iterator_t& operator= (Iterator_t const&) = default;

    /// Assignment from a different container type.
    template <typename OC, typename OR>
    Iterator_t& operator= (IteratorBase<OC, OR> const& from)
      { fCont = from.fCont; fIndex = from.fIndex; return *this; }


    // --- BEGIN Dereferencing -------------------------------------------------
    /// @name Dereferencing
    /// @{
    /// Returns the mapped item the iterator currently points to.
    reference operator*() const { return (*fCont)[fIndex]; }

    /// Returns a member of the mapped item the iterator currently points to.
    reference operator->() const { return &(operator*(fIndex)); }

    /// Returns the mapped item `n` steps ahead of what the iterator currentlt
    /// points to.
    reference operator[] (difference_type n) const
      { return (*fCont)[fIndex + n]; }

    /// @}
    // --- END Dereferencing ---------------------------------------------------


    // --- BEGIN Transformation ------------------------------------------------
    /// @name Transformation
    /// @{
    /// Increments this iterator and returns it incremented.
    Iterator_t& operator++() { ++fIndex; return *this; }

    /// Increments this iterator and returns its old value.
    Iterator_t operator++(int) { auto it = *this; this->operator++(); return it; }

    /// Decrements this iterator and returns it decremented.
    Iterator_t& operator--() { --fIndex; return *this; }

    /// Decrements this iterator and returns its old value.
    Iterator_t operator--(int) { auto it = *this; this->operator--(); return it; }

    /// Increments this iterator by `n` steps and returns it incremented.
    Iterator_t& operator+= (difference_type n) { fIndex += n; return *this; }

    /// Decrements this iterator by `n` steps and returns it decremented.
    Iterator_t& operator-= (difference_type n) { fIndex -= n; return *this; }

    /// Returns an iterator pointing `n` steps ahead of this one.
    Iterator_t operator+ (difference_type n) const
      { auto it = *this; it += n; return it; }

    /// Returns an iterator pointing `n` steps behind this one.
    Iterator_t operator- (difference_type n) const
      { auto it = *this; it -= n; return it; }

    /// Returns the number of steps this iterator is ahead of `other`.
    difference_type operator- (Iterator_t& other) const
      { return fIndex - other.fIndex; }

    /// @}
    // --- END Transformation --------------------------------------------------


    // --- BEGIN Comparisons ---------------------------------------------------
    /// @name Comparisons
    /// @{
    /// Returns whether this iterator is equal to `other`.
    template <typename OC, typename OR>
    bool operator== (IteratorBase<OC, OR> const& other) const
      { return (fCont == other.fCont) && (fIndex == other.fIndex); }

    /// Returns whether this iterator is not equal to `other`.
    template <typename OC, typename OR>
    bool operator!= (IteratorBase<OC, OR> const& other) const
      { return (fCont != other.fCont) || (fIndex != other.fIndex); }

    /// Returns whether this iterator is behind of or equal to `other`.
    template <typename OC, typename OR>
    bool operator<= (IteratorBase<OC, OR> const& other) const
      { return (fCont == other.fCont) && (fIndex <= other.fIndex); }

    /// Returns whether this iterator is strictly behind of `other`.
    template <typename OC, typename OR>
    bool operator< (IteratorBase<OC, OR> const& other) const
      { return (fCont == other.fCont) && (fIndex < other.fIndex); }

    /// Returns whether this iterator is ahead of or equal to `other`.
    template <typename OC, typename OR>
    bool operator>= (IteratorBase<OC, OR> const& other) const
      { return (fCont == other.fCont) && (fIndex >= other.fIndex); }

    /// Returns whether this iterator is strictly ahead of `other`.
    template <typename OC, typename OR>
    bool operator> (IteratorBase<OC, OR> const& other) const
      { return (fCont == other.fCont) && (fIndex > other.fIndex); }

    /// @}
    // --- END Comparisons -----------------------------------------------------

  }; // IteratorBase


  // it hurts the eye
  template <
    typename Cont, typename Mapping,
    typename Container, typename Reference
    >
  typename MappedContainer<Cont, Mapping>::template IteratorBase<Container, Reference>
  operator+
    (
    typename MappedContainer<Cont, Mapping>::template IteratorBase<Container, Reference>::difference_type n,
    typename MappedContainer<Cont, Mapping>::template IteratorBase<Container, Reference> const& it
    )
    { return it + n; }


} // namespace util



namespace util::details {


  //----------------------------------------------------------------------------
  template <typename Cont>
  struct ContainerTraitsImpl {
    using value_type      = typename Cont::value_type;
    using difference_type = typename Cont::difference_type;
    using size_type       = typename Cont::size_type;
    using const_reference = typename Cont::const_reference;
    using reference       = typename Cont::reference;
    using const_iterator  = typename Cont::const_iterator;
    using iterator        = typename Cont::iterator;
  }; // ContainerTraitsImpl<>

  template <typename T>
  struct ContainerTraitsImpl<T*> {
    using value_type      = T; // should it be devoid of const/mutable?
    using difference_type = std::ptrdiff_t;
    using size_type       = std::size_t;
    using const_reference = T const&;
    using reference       = T&;
    using const_iterator  = T const*;
    using iterator        = T*;
  }; // ContainerTraitsImpl<T*>

  // this is not as powerful as it should... hoping users do not nest
  // crazily `unique_ptr`, `reference_wrapper` and such
  template <typename Cont>
  struct ContainerTraits
    : ContainerTraitsImpl<
        std::remove_reference_t<
          util::collection_from_reference_t<
            util::strip_referenceness_t<Cont>
          >
        >
      >
    {};


  //----------------------------------------------------------------------------

} // namespace util::details


//------------------------------------------------------------------------------
//--- util::MappedContainer
//------------------------------------------------------------------------------
template <typename Cont, typename Mapping>
auto util::MappedContainer<Cont, Mapping>::minimal_size() const -> size_type
  { return minimal_size(fData.container(), fMapping.container()); }


//------------------------------------------------------------------------------
template <typename Cont, typename Mapping>
decltype(auto) util::MappedContainer<Cont, Mapping>::map_element
  (MappingIndex_t index) const
{
  auto const dataIndex = map_index(index);
  return (dataIndex == InvalidIndex)? defaultValue(): fData[dataIndex];
} // util::MappedContainer<>::map_element() const


//------------------------------------------------------------------------------
template <typename Cont, typename Mapping>
decltype(auto) util::MappedContainer<Cont, Mapping>::map_element
  (MappingIndex_t index)
{
  auto const dataIndex = map_index(index);
  return (dataIndex == InvalidIndex)? defaultValue(): fData[dataIndex];
} // util::MappedContainer<>::map_element()


//------------------------------------------------------------------------------
template <typename Cont, typename Mapping>
decltype(auto) util::MappedContainer<Cont, Mapping>::at
  (MappingIndex_t index) const
{
  if (index >= size()) {
    throw std::out_of_range(
      "MappedContainer::at(" + std::to_string(index) + "): out of range (size: "
      + std::to_string(size()) + ")"
      );
  }
  return map_element(index);
} // util::MappedContainer<>::at() const


//------------------------------------------------------------------------------
template <typename Cont, typename Mapping>
decltype(auto) util::MappedContainer<Cont, Mapping>::at
  (MappingIndex_t index)
{
  if (index >= size()) {
    throw std::out_of_range(
      "MappedContainer::at(" + std::to_string(index) + "): out of range (size: "
      + std::to_string(size()) + ")"
      );
  }
  return map_element(index);
} // util::MappedContainer<>::at()


//------------------------------------------------------------------------------
template <typename Cont, typename Mapping>
auto util::MappedContainer<Cont, Mapping>::minimal_size
  (DataContainer_t const&, Mapping_t const& mapping) -> size_type
{
  /*
   * Obscure compiler errors are expected if the `Mapping_t` type does not
   * support `std::size`. In that case, very simply, `minimal_size()` can't be
   * used and the caller must specify the size of the container.
   */

  using std::size;
  return size(util::collection_from_reference(mapping));
} // util::MappedContainer<>::minimal_size(DataContainer_t, Mapping_t)


//------------------------------------------------------------------------------
template <typename Cont, typename Mapping>
decltype(auto) util::MappedContainer<Cont, Mapping>::map_index
  (MappingIndex_t index) const
  { return fMapping[index]; }


//------------------------------------------------------------------------------
template <typename Cont, typename Mapping>
decltype(auto) util::MappedContainer<Cont, Mapping>::map_index
  (MappingIndex_t index)
  { return fMapping[index]; }


//------------------------------------------------------------------------------


#endif // LARDATAALG_UTILITIES_MAPPEDCONTAINER_H
