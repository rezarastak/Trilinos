// @HEADER
//
// ***********************************************************************
//
//             Xpetra: A linear algebra interface package
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef XPETRA_TPETRAMAP_DECL_HPP
#define XPETRA_TPETRAMAP_DECL_HPP

/* this file is automatically generated - do not edit (see script/tpetra.py) */

#include "Xpetra_TpetraConfigDefs.hpp"

#include <Tpetra_Map.hpp>

#include "Xpetra_Map.hpp"
#include "Xpetra_Utils.hpp"

#include "Xpetra_Exceptions.hpp"

namespace Xpetra {

  template <class LocalOrdinal = Map<>::local_ordinal_type,
            class GlobalOrdinal = typename Map<LocalOrdinal>::global_ordinal_type,
            class Node = typename Map<LocalOrdinal, GlobalOrdinal>::node_type>
  class TpetraMap
    : public Map<LocalOrdinal,GlobalOrdinal,Node> {

  public:

    //! @name Constructors and destructor
    //@{


    //! Constructor with Tpetra-defined contiguous uniform distribution.
#ifdef TPETRA_ENABLE_DEPRECATED_CODE
    TPETRA_DEPRECATED
    TpetraMap (global_size_t numGlobalElements,
               GlobalOrdinal indexBase,
               const Teuchos::RCP< const Teuchos::Comm< int > > &comm,
               LocalGlobal lg,
               const Teuchos::RCP< Node > & /* node */);
#endif // TPETRA_ENABLE_DEPRECATED_CODE


    TpetraMap (global_size_t numGlobalElements,
               GlobalOrdinal indexBase,
               const Teuchos::RCP< const Teuchos::Comm< int > > &comm,
               LocalGlobal lg=GloballyDistributed);


    //! Constructor with a user-defined contiguous distribution.
#ifdef TPETRA_ENABLE_DEPRECATED_CODE
    TPETRA_DEPRECATED
    TpetraMap (global_size_t numGlobalElements,
               size_t numLocalElements,
               GlobalOrdinal indexBase,
               const Teuchos::RCP< const Teuchos::Comm< int > > &comm,
               const Teuchos::RCP< Node > & /* node */);
#endif // TPETRA_ENABLE_DEPRECATED_CODE


    TpetraMap (global_size_t numGlobalElements,
               size_t numLocalElements,
               GlobalOrdinal indexBase,
               const Teuchos::RCP< const Teuchos::Comm< int > > &comm);


    //! Constructor with user-defined arbitrary (possibly noncontiguous) distribution.
#ifdef TPETRA_ENABLE_DEPRECATED_CODE
    TPETRA_DEPRECATED
    TpetraMap (global_size_t numGlobalElements,
               const Teuchos::ArrayView< const GlobalOrdinal > &elementList,
               GlobalOrdinal indexBase,
               const Teuchos::RCP< const Teuchos::Comm< int > > &comm,
               const Teuchos::RCP< Node > & /* node */);
#endif // TPETRA_ENABLE_DEPRECATED_CODE


    TpetraMap (global_size_t numGlobalElements,
               const Teuchos::ArrayView< const GlobalOrdinal > &elementList,
               GlobalOrdinal indexBase,
               const Teuchos::RCP< const Teuchos::Comm< int > > &comm);


#ifdef HAVE_XPETRA_KOKKOS_REFACTOR
#ifdef HAVE_XPETRA_TPETRA
    //! Constructor with user-defined arbitrary (possibly noncontiguous) distribution passed as a Kokkos::View.
    TpetraMap (global_size_t numGlobalElements,
               const Kokkos::View<const GlobalOrdinal*, typename Node::device_type>& indexList,
               GlobalOrdinal indexBase,
               const Teuchos::RCP< const Teuchos::Comm< int > > &comm);
#endif
#endif

    //! Destructor
    ~TpetraMap();


    //! @name Attributes
    //@{

    //! The number of elements in this Map.
    global_size_t getGlobalNumElements() const;

    //! The number of elements belonging to the calling node.
    size_t getNodeNumElements() const;

    //! The index base for this Map.
    GlobalOrdinal getIndexBase() const;

    //! The minimum local index.
    LocalOrdinal getMinLocalIndex() const;

    //! The maximum local index on the calling process.
    LocalOrdinal getMaxLocalIndex() const;

    //! The minimum global index owned by the calling process.
    GlobalOrdinal getMinGlobalIndex() const;

    //! The maximum global index owned by the calling process.
    GlobalOrdinal getMaxGlobalIndex() const;

    //! The minimum global index over all processes in the communicator.
    GlobalOrdinal getMinAllGlobalIndex() const;

    //! The maximum global index over all processes in the communicator.
    GlobalOrdinal getMaxAllGlobalIndex() const;

    //! The local index corresponding to the given global index.
    LocalOrdinal getLocalElement(GlobalOrdinal globalIndex) const;

    //! The global index corresponding to the given local index.
    GlobalOrdinal getGlobalElement(LocalOrdinal localIndex) const;

    //! Return the process IDs and corresponding local IDs for the given global IDs.
    LookupStatus getRemoteIndexList(const Teuchos::ArrayView< const GlobalOrdinal > &GIDList, const Teuchos::ArrayView< int > &nodeIDList, const Teuchos::ArrayView< LocalOrdinal > &LIDList) const;

    //! Return the process IDs for the given global IDs.
    LookupStatus getRemoteIndexList(const Teuchos::ArrayView< const GlobalOrdinal > &GIDList, const Teuchos::ArrayView< int > &nodeIDList) const;

    //! Return a view of the global indices owned by this node.
    Teuchos::ArrayView< const GlobalOrdinal > getNodeElementList() const;

    //@}

    //! @name Boolean tests
    //@{

    //! True if the local index is valid for this Map on this node, else false.
    bool isNodeLocalElement(LocalOrdinal localIndex) const;

    //! True if the global index is found in this Map on this node, else false.
    bool isNodeGlobalElement(GlobalOrdinal globalIndex) const;

    //! True if this Map is distributed contiguously, else false.
    bool isContiguous() const;

    //! Whether this Map is globally distributed or locally replicated.
    bool isDistributed() const;

    //! True if and only if map is compatible with this Map.
    bool isCompatible(const Map< LocalOrdinal, GlobalOrdinal, Node > &map) const;

    //! True if and only if map is identical to this Map.
    bool isSameAs(const Map< LocalOrdinal, GlobalOrdinal, Node > &map) const;

    //@}

    //! @name
    //@{

    //! Get this Map's Comm object.
    Teuchos::RCP< const Teuchos::Comm< int > >  getComm() const;

#ifdef TPETRA_ENABLE_DEPRECATED_CODE
    //! Get this Map's Node object.
    Teuchos::RCP< Node >  getNode() const;
#endif // TPETRA_ENABLE_DEPRECATED_CODE

    //@}

    //! @name
    //@{

    //! Return a simple one-line description of this object.
    std::string description() const;

    //! Print this object with the given verbosity level to the given FancyOStream.
    void describe(Teuchos::FancyOStream &out, const Teuchos::EVerbosityLevel verbLevel=Teuchos::Describable::verbLevel_default) const;

    RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > removeEmptyProcesses () const;
    RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > replaceCommWithSubset (const Teuchos::RCP<const Teuchos::Comm<int> >& newComm) const;

#ifdef XPETRA_ENABLE_DEPRECATED_CODE
    template<class Node2>
    RCP<Map<LocalOrdinal, GlobalOrdinal, Node2> > XPETRA_DEPRECATED clone(const RCP<Node2> &node2) const;
#endif

    //@}

    //! @name Xpetra specific
    //@{

    //! TpetraMap constructor to wrap a Tpetra::Map object
    TpetraMap(const Teuchos::RCP<const Tpetra::Map<LocalOrdinal, GlobalOrdinal, Node > > &map);

    //! Get the library used by this object (Tpetra or Epetra?)    
    UnderlyingLib lib() const;

    //! Get the underlying Tpetra map
    RCP< const Tpetra::Map< LocalOrdinal, GlobalOrdinal, Node > > getTpetra_Map() const;

#ifdef HAVE_XPETRA_KOKKOS_REFACTOR
#ifdef HAVE_XPETRA_TPETRA
    using local_map_type = typename Map<LocalOrdinal, GlobalOrdinal, Node>::local_map_type;
    /// \brief Get the local Map for Kokkos kernels.
    local_map_type getLocalMap () const;
#endif
#endif

    //@}

  protected:

    RCP< const Tpetra::Map< LocalOrdinal, GlobalOrdinal, Node > > map_;

  }; // TpetraMap class


  template <class LocalOrdinal, class GlobalOrdinal, class Node>
  const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> & toTpetra(const Map<LocalOrdinal,GlobalOrdinal,Node> &map) {
    // TODO: throw exception
    const TpetraMap<LocalOrdinal,GlobalOrdinal,Node> & tpetraMap = dynamic_cast<const TpetraMap<LocalOrdinal,GlobalOrdinal,Node> &>(*map.getMap());
    return *tpetraMap.getTpetra_Map();
  }

  template <class LocalOrdinal, class GlobalOrdinal, class Node>
  const RCP< const Tpetra::Map< LocalOrdinal, GlobalOrdinal, Node > > toTpetra(const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &map) {
    typedef TpetraMap<LocalOrdinal, GlobalOrdinal, Node> TpetraMapClass;
    if (map != Teuchos::null) {
      XPETRA_RCP_DYNAMIC_CAST(const TpetraMapClass, map->getMap(), tpetraMap, "toTpetra");
      return tpetraMap->getTpetra_Map();
    }
    return Teuchos::null;
  }

  // In some cases (for instance, in MueLu adapter to Tpetra operator), we need to return a reference. This is only possible if
  // we assume that the map argument is nonzero
  template <class LocalOrdinal, class GlobalOrdinal, class Node>
  const RCP< const Tpetra::Map< LocalOrdinal, GlobalOrdinal, Node > > toTpetraNonZero(const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &map) {
    TEUCHOS_TEST_FOR_EXCEPTION(map.is_null(), std::invalid_argument, "map must be nonzero");
    typedef TpetraMap<LocalOrdinal, GlobalOrdinal, Node> TpetraMapClass;
    XPETRA_RCP_DYNAMIC_CAST(const TpetraMapClass, map->getMap(), tpetraMap, "toTpetra");
    return tpetraMap->getTpetra_Map();
  }

  template <class LocalOrdinal, class GlobalOrdinal, class Node>
  const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > toXpetra(const RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> >& map) {
    if (!map.is_null())
      return rcp(new TpetraMap<LocalOrdinal, GlobalOrdinal, Node>(map));

    return Teuchos::null;
  }

  template <class LocalOrdinal, class GlobalOrdinal, class Node>
  const RCP<Map<LocalOrdinal,GlobalOrdinal,Node> > toXpetraNonConst(const RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> >& map) {
    if (!map.is_null())
      return rcp(new TpetraMap<LocalOrdinal,GlobalOrdinal,Node>(map));

    return Teuchos::null;
  }


  namespace useTpetra {

    //! Non-member function to create a locally replicated Map with a specified node.
#ifdef TPETRA_ENABLE_DEPRECATED_CODE
    template <class LocalOrdinal, class GlobalOrdinal, class Node>
    TPETRA_DEPRECATED
    Teuchos::RCP< const TpetraMap<LocalOrdinal,GlobalOrdinal,Node> >
    createLocalMapWithNode(size_t numElements, const Teuchos::RCP< const Teuchos::Comm< int > > &comm, const Teuchos::RCP< Node > & /* node */)
    {
      return createLocalMapWithNode<LocalOrdinal,GlobalOrdinal,Node>
                   (numElements, comm);
    }
#endif // TPETRA_ENABLE_DEPRECATED_CODE
    template <class LocalOrdinal, class GlobalOrdinal, class Node>
    Teuchos::RCP< const TpetraMap<LocalOrdinal,GlobalOrdinal,Node> >
    createLocalMapWithNode(size_t numElements, const Teuchos::RCP< const Teuchos::Comm< int > > &comm)
    {
      XPETRA_MONITOR("useTpetra::createLocalMapWithNode");

      return rcp(new TpetraMap<LocalOrdinal,GlobalOrdinal,Node>(Tpetra::createLocalMapWithNode<LocalOrdinal,GlobalOrdinal,Node>(numElements, comm)));
    }

    //! Non-member function to create a (potentially) non-uniform, contiguous Map with the default node.
    template <class LocalOrdinal, class GlobalOrdinal>
    Teuchos::RCP< const TpetraMap<LocalOrdinal,GlobalOrdinal> >
    createContigMap(global_size_t numElements, size_t localNumElements, const Teuchos::RCP< const Teuchos::Comm< int > > &comm) {
      XPETRA_MONITOR("useTpetra::createContigMap");

      return rcp(new TpetraMap<LocalOrdinal,GlobalOrdinal>(Tpetra::createContigMap<LocalOrdinal,GlobalOrdinal>(numElements, localNumElements, comm)));
    }

    //! Non-member function to create a (potentially) non-uniform, contiguous Map with a user-specified node.
#ifdef TPETRA_ENABLE_DEPRECATED_CODE
    template <class LocalOrdinal, class GlobalOrdinal, class Node>
    TPETRA_DEPRECATED
    Teuchos::RCP< const TpetraMap<LocalOrdinal,GlobalOrdinal,Node> >
    createContigMapWithNode(global_size_t numElements, size_t localNumElements,
                            const Teuchos::RCP< const Teuchos::Comm< int > > &comm, const TPETRA_DEPRECATED Teuchos::RCP< Node > & /* node */ )
    {
      return createContigMapWithNode<LocalOrdinal,GlobalOrdinal,Node>
                   (numElements, localNumElements, comm);
    }
#endif // TPETRA_ENABLE_DEPRECATED_CODE
    template <class LocalOrdinal, class GlobalOrdinal, class Node>
    Teuchos::RCP< const TpetraMap<LocalOrdinal,GlobalOrdinal,Node> >
    createContigMapWithNode(global_size_t numElements, size_t localNumElements,
                            const Teuchos::RCP< const Teuchos::Comm< int > > &comm)
    {
      XPETRA_MONITOR("useTpetra::createContigMap");
      return rcp(new TpetraMap<LocalOrdinal,GlobalOrdinal,Node>(Tpetra::createContigMapWithNode<LocalOrdinal,GlobalOrdinal,Node>(numElements, localNumElements, comm)));
    }
  } // useTpetra namespace

} // Xpetra namespace
#endif // XPETRA_TPETRAMAP_DECL_HPP

