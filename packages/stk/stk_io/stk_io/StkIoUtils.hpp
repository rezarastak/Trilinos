// Copyright (c) 2013, Sandia Corporation.
 // Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 // the U.S. Government retains certain rights in this software.
 // 
 // Redistribution and use in source and binary forms, with or without
 // modification, are permitted provided that the following conditions are
 // met:
 // 
 //     * Redistributions of source code must retain the above copyright
 //       notice, this list of conditions and the following disclaimer.
 // 
 //     * Redistributions in binary form must reproduce the above
 //       copyright notice, this list of conditions and the following
 //       disclaimer in the documentation and/or other materials provided
 //       with the distribution.
 // 
 //     * Neither the name of Sandia Corporation nor the names of its
 //       contributors may be used to endorse or promote products derived
 //       from this software without specific prior written permission.
 // 
 // THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 // "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 // LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 // A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 // OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 // SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 // LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 // DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 // THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 // (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 // OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef STKIOUTILS_HPP_
#define STKIOUTILS_HPP_

// #######################  Start Clang Header Tool Managed Headers ########################
// clang-format off
#include <stddef.h>                                       // for size_t
#include <map>                                            // for map, etc
#include <string>                                         // for string, etc
#include <vector>                                         // for vector
#include "stk_mesh/base/Types.hpp"                        // for EntityRank, etc
#include "stk_mesh/baseImpl/elementGraph/GraphTypes.hpp"
namespace stk { namespace io { class StkMeshIoBroker; } }
namespace stk { namespace mesh { class BulkData; } }
namespace stk { namespace mesh { class MetaData; } }
namespace stk { namespace mesh { class Part; } }
namespace stk { namespace mesh { class Selector; } }
namespace stk { namespace mesh { struct Entity; } }
// clang-format on
// #######################   End Clang Header Tool Managed Headers  ########################

namespace stk { namespace io { class MetaData; } }
namespace stk { namespace io { class BulkData; } }

namespace stk {
namespace io {

size_t get_entities(stk::mesh::Part &part,
                    stk::mesh::EntityRank type,
                    const stk::mesh::BulkData &bulk,
                    stk::mesh::EntityVector &entities,
                    bool include_shared,
                    const stk::mesh::Selector *subset_selector);

stk::mesh::EntityRank part_primary_entity_rank(const stk::mesh::Part &part);

typedef std::map<std::string, std::vector<std::string>> IossBlockMembership;

IossBlockMembership get_block_memberships(stk::io::StkMeshIoBroker& stkIo);

const stk::mesh::Part* getElementBlockSelectorForElement(const stk::mesh::BulkData& bulkData, stk::mesh::Entity element);

void fill_block_parts_given_names(const std::vector<std::string>& side_block_names,
                                              stk::mesh::MetaData& meta,
                                              std::vector<const stk::mesh::Part*>& blocks);

void create_bulkdata_sidesets(stk::mesh::BulkData& bulkData);

bool isSidesetSupported(const stk::mesh::BulkData &bulk, const stk::mesh::EntityVector &sides, const stk::mesh::impl::ParallelPartInfo &parallelPartInfo);

stk::mesh::FieldVector get_transient_fields(stk::mesh::MetaData &meta);
stk::mesh::FieldVector get_transient_fields(stk::mesh::MetaData &meta, const stk::mesh::EntityRank rank);

}}


#endif /* STKIOUTILS_HPP_ */
