#ifndef __Panzer_Intrepid_ConstBasis_hpp__
#define __Panzer_Intrepid_ConstBasis_hpp__

#include "Intrepid2_Basis.hpp"

namespace panzer {
  
  template<class ExecutionSpace, class OutputValueType, class PointValueType> 
  class Basis_Constant: public Intrepid2::Basis<ExecutionSpace,OutputValueType,PointValueType> {
private:
  
  /** \brief  Initializes <var>tagToOrdinal_</var> and <var>ordinalToTag_</var> lookup arrays.
   */
  void initializeTags();
  
public:

    using outputViewType = typename Intrepid2::Basis<ExecutionSpace,OutputValueType,PointValueType>::outputViewType;
    using pointViewType = typename Intrepid2::Basis<ExecutionSpace,OutputValueType,PointValueType>::pointViewType;
  
  /** \brief  Constructor.
   */
  Basis_Constant(const shards::CellTopology & ct);  
  
  
  /** \brief  Evaluation of a FEM basis on a <strong>reference Triangle</strong> cell. 
  
              Returns values of <var>operatorType</var> acting on FEM basis functions for a set of
              points in the <strong>reference Triangle</strong> cell. For rank and dimensions of
              I/O array arguments see Section \ref basis_md_array_sec .

      \param  outputValues      [out] - variable rank array with the basis values
      \param  inputPoints       [in]  - rank-2 array (P,D) with the evaluation points
      \param  operatorType      [in]  - the operator acting on the basis functions    
   */
  void getValues(      outputViewType outputValues,
                 const pointViewType  inputPoints,
                 const Intrepid2::EOperator operatorType) const;
  
  
  /**  \brief  FVD basis evaluation: invocation of this method throws an exception.
   */
  void getValues(      outputViewType outputValues,
                 const pointViewType  inputPoints,
                 const pointViewType  cellVertices,
                 const Intrepid2::EOperator operatorType = Intrepid2::OPERATOR_VALUE) const;

};

}// namespace panzer

#include "Panzer_Intrepid_ConstBasis_impl.hpp"

#endif
