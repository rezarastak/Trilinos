/*
// @HEADER
// ***********************************************************************
//
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
// @HEADER
*/

#include "Tpetra_TestingUtilities.hpp"
#include "Tpetra_FECrsGraph.hpp"
#include "Tpetra_CrsGraph.hpp"
#include "Tpetra_Assembly_Helpers.hpp"
#include "Tpetra_Details_getNumDiags.hpp"

namespace { // (anonymous)

using Tpetra::TestingUtilities::getDefaultComm;
using Tpetra::createContigMapWithNode;
using Tpetra::StaticProfile;
using Teuchos::RCP;
using Teuchos::ArrayRCP;
using Teuchos::rcp;
using Teuchos::outArg;
using Teuchos::Comm;
using Teuchos::Array;
using Teuchos::ArrayView;
using Teuchos::tuple;
using std::endl;
typedef Tpetra::global_size_t GST;


template<class LO, class GO, class Node>
class GraphPack {
public:
  RCP<const Tpetra::Map<LO,GO,Node> > uniqueMap;
  RCP<const Tpetra::Map<LO,GO,Node> > overlapMap;
  std::vector<std::vector<GO> > element2node;

  void print(int rank, std::ostream & out) {
    using std::endl;
    out << "["<<rank<<"] Unique Map  : ";
    for(size_t i=0; i<uniqueMap->getNodeNumElements(); i++)
      out << uniqueMap->getGlobalElement(i) << " ";
    out<<endl;      

    out << "["<<rank<<"] Overlap Map : ";
    for(size_t i=0; i<overlapMap->getNodeNumElements(); i++)
      out << overlapMap->getGlobalElement(i) << " ";
    out<<endl;

    out << "["<<rank<<"] element2node: ";
    for(size_t i=0; i<(size_t)element2node.size(); i++) {
      out <<"(";
      for(size_t j=0; j<(size_t)element2node[i].size(); j++)
        out<<element2node[i][j] << " ";
      out<<") ";
    }
    out<<endl;
  }
};


template<class LO, class GO, class Node>
void generate_fem1d_graph(size_t numLocalNodes, RCP<const Comm<int> > comm , GraphPack<LO,GO,Node> & pack) {
  const GST INVALID = Teuchos::OrdinalTraits<GST>::invalid();
  int rank    = comm->getRank();
  int numProc = comm->getSize();
  size_t numOverlapNodes  = (rank == numProc-1) ? numLocalNodes : numLocalNodes + 1;
  size_t numLocalElements = (rank == numProc-1) ? numLocalNodes -1 : numLocalNodes;
  //  printf("CMS numOverlapNodes = %d numLocalElements = %d\n",numOverlapNodes,numLocalElements);

  pack.uniqueMap = createContigMapWithNode<LO,GO,Node>(INVALID,numLocalNodes,comm);

  Teuchos::Array<GO> overlapIndices(numOverlapNodes);
  for(size_t i=0; i<numLocalNodes; i++) {
    overlapIndices[i] = pack.uniqueMap->getGlobalElement(i);
  }
  if(rank != numProc -1)  overlapIndices[numOverlapNodes-1] = overlapIndices[numLocalNodes-1] +1;

  pack.overlapMap = rcp(new Tpetra::Map<LO,GO,Node>(INVALID,overlapIndices,0,comm));

  pack.element2node.resize(numLocalElements);
  for(size_t i=0; i<numLocalElements; i++) {
    pack.element2node[i].resize(2);
    pack.element2node[i][0] = pack.uniqueMap->getGlobalElement(i);
    pack.element2node[i][1] = pack.uniqueMap->getGlobalElement(i) + 1;
  }
}

//
// UNIT TESTS
//






////
TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL( FECrsGraph, Diagonal, LO, GO, Node )
{
    typedef Tpetra::FECrsGraph<LO,GO,Node> FEG;
    typedef Tpetra::CrsGraph<LO,GO,Node> CG;
    const GST INVALID = Teuchos::OrdinalTraits<GST>::invalid();

    // get a comm
    RCP<const Comm<int> > comm = getDefaultComm();

    // create a Map
    const size_t numLocal = 10;
    RCP<const Tpetra::Map<LO,GO,Node> > map = createContigMapWithNode<LO,GO,Node>(INVALID,numLocal,comm);


    // Trivial test that makes sure a diagonal graph can be built
    CG g1(map,1,StaticProfile);
    FEG g2(map,map,1);

    Tpetra::beginFill(g2);
    for(size_t i=0; i<numLocal; i++) {
      GO gid = map->getGlobalElement(i);
      g1.insertGlobalIndices(gid,1,&gid);
      g2.insertGlobalIndices(gid,1,&gid);
    }
    Tpetra::endFill(g2);
    g1.fillComplete();

    // FIXME: Use graph comparison here


    TPETRA_GLOBAL_SUCCESS_CHECK(out,comm,true)
}


TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL( FECrsGraph, Assemble1D, LO, GO, Node )
{
  typedef Tpetra::FECrsGraph<LO,GO,Node> FEG;
  typedef Tpetra::CrsGraph<LO,GO,Node> CG;
  
  // get a comm
  RCP<const Comm<int> > comm = getDefaultComm();
  
  // create a Map
  const size_t numLocal = 10;

  // Generate a mesh
  GraphPack<LO,GO,Node> pack;
  generate_fem1d_graph(numLocal,comm,pack);
  //  pack.print(comm->getRank(),std::cout);

  // Comparative assembly
  // FIXME: We should be able to get away with 4 for StaticProfile here, but we need 4 since duplicates are
  // not being handled correctly.
  CG g1(pack.uniqueMap,4,StaticProfile);
  FEG g2(pack.uniqueMap,pack.overlapMap,4);

  g2.beginFill();
  for(size_t i=0; i<(size_t)pack.element2node.size(); i++) {
    for(size_t j=0; j<pack.element2node[i].size(); j++) {
      GO gid_j = pack.element2node[i][j];
      for(size_t k=0; k<pack.element2node[i].size(); k++) {
        GO gid_k = pack.element2node[i][k];
        //        printf("Inserting (%d,%d)\n",gid_j,gid_k);
        g1.insertGlobalIndices(gid_j,1,&gid_k);
        g2.insertGlobalIndices(gid_j,1,&gid_k);
      }
    }
  }
  g1.fillComplete();
  g2.endFill();

  // FIXME: Use graph comparison here
  TPETRA_GLOBAL_SUCCESS_CHECK(out,comm,true)
}

//
// INSTANTIATIONS
//

#define UNIT_TEST_GROUP( LO, GO, NODE ) \
      TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( FECrsGraph, Diagonal, LO, GO, NODE ) \
      TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( FECrsGraph, Assemble1D, LO, GO, NODE )

  TPETRA_ETI_MANGLING_TYPEDEFS()

  TPETRA_INSTANTIATE_LGN( UNIT_TEST_GROUP )

} // end namespace (anonymous)


