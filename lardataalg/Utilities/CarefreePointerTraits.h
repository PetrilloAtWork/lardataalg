/**
 * @file   lardataalg/Utilities/CarefreePointerTraits.h
 * @brief  Specialization `util::details::ContainerTraits` of for
 *         `util::CarefreePointer` class.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   June 3, 2020
 *
 * This is a header-only library.
 * 
 * It is needed when using `util::CarefreePointer` inside
 * `util::MappedContainer`.
 */

#ifndef LARDATAALG_UTILITIES_CAREFREEPOINTERTRAITS_H
#define LARDATAALG_UTILITIES_CAREFREEPOINTERTRAITS_H


// LArSoft libraries
#include "lardataalg/Utilities/CarefreePointer.h"
#include "larcorealg/CoreUtils/ContainerMeta.h" // make_collection_reference_impl


namespace util {
  
  //----------------------------------------------------------------------------
  template <typename... Args> // shortcut
  struct collection_value_type<CarefreePointer<Args...>>
    : collection_value_type<typename CarefreePointer<Args...>::element_type*>
  {};
  
  template <typename... Args> // shortcut
  struct collection_value_access_type<CarefreePointer<Args...>>
    : collection_value_type<typename CarefreePointer<Args...>::element_type*>
  {};
  
  template <typename... Args> // shortcut
  struct collection_value_constant_access_type<CarefreePointer<Args...>>
    : collection_value_constant_access_type
      <typename CarefreePointer<Args...>::element_type*>
  {};
  
  //----------------------------------------------------------------------------
  namespace details {
    
    //--------------------------------------------------------------------------
    template <typename Coll>
    struct make_collection_reference_impl<
      Coll,
      std::enable_if_t<util::is_carefree_pointer_v<std::remove_reference_t<Coll>>>
      >
    {
      
      using Coll_t = std::remove_reference_t<Coll>;
      
      using type = Coll_t;
      
      static type make(Coll_t& coll) { return std::move(coll); }
      static type make(Coll_t&& coll) { return std::move(coll); }
      
    }; // make_collection_reference_impl<std::reference_wrapper>
    
    
    //--------------------------------------------------------------------------
    
  } // namespace details
  
  //----------------------------------------------------------------------------

} // namespace util


// -----------------------------------------------------------------------------
// this should allow specialization even before ContainerTraits is known
namespace util::details { template <typename Cont> struct ContainerTraits; }


namespace util::details {
  
  template <typename... Args> // shortcut
  struct ContainerTraits<CarefreePointer<Args...>>
    : ContainerTraits<typename CarefreePointer<Args...>::element_type*>
  {};
  
} // namespace util::details


// -----------------------------------------------------------------------------


#endif // LARDATAALG_UTILITIES_CAREFREEPOINTERTRAITS_H
