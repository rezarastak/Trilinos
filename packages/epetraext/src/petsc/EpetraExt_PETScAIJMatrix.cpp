//@HEADER
// ***********************************************************************
// 
//     EpetraExt: Epetra Extended - Linear Algebra Services Package
//                 Copyright (2001) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
//@HEADER

/*#############################################################################
# CVS File Information
#    Current revision: $Revision$
#    Last modified:    $Date$
#    Modified by:      $Author$
#############################################################################*/

#include "EpetraExt_ConfigDefs.h"
#ifdef HAVE_EXPERIMENTAL
#ifdef HAVE_PETSC

#include "Epetra_Import.h"
#include "Epetra_Export.h"
#include "Epetra_Vector.h"
#include "Epetra_MultiVector.h"
#include "EpetraExt_PETScAIJMatrix.h"

//==============================================================================
Epetra_PETScAIJMatrix::Epetra_PETScAIJMatrix(Mat Amat)
  : Epetra_Object("Epetra::PETScAIJMatrix"),
    Amat_(Amat),
    Values_(0),
    Indices_(0),
    MaxNumEntries_(-1),
    ImportVector_(0),
    NormInf_(-1.0),
    NormOne_(-1.0)
{
#ifdef HAVE_MPI
  Comm_ = new Epetra_MpiComm(Amat->comm);
#else
  Comm_ = new Epetra_SerialComm();
#endif  
  //TODO PETSc's CHKERRQ macro returns a value, which isn't allowed in ctors.
  int ierr;
  MatGetType(Amat, &MatType_);
  if ( strcmp(MatType_,MATSEQAIJ) != 0 && strcmp(MatType_,MATMPIAIJ) != 0 ) {
    char errMsg[80];
    sprintf(errMsg,"PETSc matrix must be either seqaij or mpiaij (but it is %s)",MatType_);
    //throw Comm_->ReportError("PETSc matrix must be either seqaij or mpiaij", -1);
    throw Comm_->ReportError(errMsg,-1);
  }
  petscMatrixType mt;
  Mat_MPIAIJ* aij=0;
  if (strcmp(MatType_,MATMPIAIJ) == 0) {
    mt = PETSC_MPI_AIJ;
    aij = (Mat_MPIAIJ*)Amat->data;
  }
  else if (strcmp(MatType_,MATSEQAIJ) == 0) {
    mt = PETSC_SEQ_AIJ;
  }
  int numLocalRows, numLocalCols;
  ierr = MatGetLocalSize(Amat,&numLocalRows,&numLocalCols);
  NumMyRows_ = numLocalRows;
  NumMyCols_ = numLocalCols; //numLocalCols is the total # of unique columns in the local matrix (the diagonal block)
  //TODO what happens if some columns are empty?
  if (mt == PETSC_MPI_AIJ)
    NumMyCols_ += aij->B->cmap.n;
  MatInfo info;
  ierr = MatGetInfo(Amat,MAT_LOCAL,&info);//CHKERRQ(ierr);
  if (ierr) {
    char errMsg[80];
    sprintf(errMsg,"MatGetInfo returned error code %d",ierr);
    throw Comm_->ReportError(errMsg,-1);
  }
  NumMyNonzeros_ = (int) info.nz_used; //PETSc stores nnz as double
  Comm_->SumAll(&(info.nz_used), &NumGlobalNonzeros_, 1);

  //FIXME The PETSc documentation warns that this may not be robust.
  //FIXME In particular, this will break if the ordering is not contiguous!
  int rowStart, rowEnd;
  ierr = MatGetOwnershipRange(Amat,&rowStart,&rowEnd);

  PetscRowStart_ = rowStart;
  PetscRowEnd_   = rowEnd;

  int* MyGlobalElements = new int[rowEnd-rowStart];
  for (int i=0; i<rowEnd-rowStart; i++)
    MyGlobalElements[i] = rowStart+i;

  ierr = MatGetInfo(Amat,MAT_GLOBAL_SUM,&info);//CHKERRQ(ierr);
  NumGlobalRows_ = (info.rows_global);

  DomainMap_ = new Epetra_Map(NumGlobalRows_, NumMyRows_, MyGlobalElements, 0, *Comm_);

  // get the GIDs of the non-local columns

  //FIXME what if the matrix is sequential?
  // TODO are these sorted and/or unique?

  int * ColGIDs = new int[NumMyCols_];
  for (int i=0; i<numLocalCols; i++) ColGIDs[i] = MyGlobalElements[i];
  for (int i=numLocalCols; i<NumMyCols_; i++) ColGIDs[i] = aij->garray[i-numLocalCols];

  ColMap_ = new Epetra_Map(-1, NumMyCols_, ColGIDs, 0, *Comm_);

  Importer_ = new Epetra_Import(*ColMap_, *DomainMap_);

  delete [] MyGlobalElements;
  delete [] ColGIDs;
} //Epetra_PETScAIJMatrix(Mat Amat)

//==============================================================================

Epetra_PETScAIJMatrix::~Epetra_PETScAIJMatrix(){
  if (ImportVector_!=0) delete ImportVector_;
  delete Importer_;
  delete ColMap_;
  delete DomainMap_;
  delete Comm_;

  if (Values_!=0) {delete [] Values_; Values_=0;}
  if (Indices_!=0) {delete [] Indices_; Indices_=0;}
} //Epetra_PETScAIJMatrix dtor

//==========================================================================

extern "C" {
  PetscErrorCode CreateColmap_MPIAIJ_Private(Mat);
}

int Epetra_PETScAIJMatrix::ExtractMyRowCopy(int Row, int Length, int & NumEntries, double * Values,
                     int * Indices) const 
{
  int nz;
  PetscInt *gcols, *lcols, ierr;
  PetscScalar *vals;

  // PETSc assumes the row number is global, whereas Trilinos assumes it's local.
  int globalRow = PetscRowStart_ + Row;
  assert(globalRow < PetscRowEnd_);
  //printf("pid %d: getting global row %d (local row %d, range: [%d-%d]\n",Comm_->MyPID(),globalRow,Row, PetscRowStart_,PetscRowEnd_); fflush(stdout);
  ierr=MatGetRow(Amat_, globalRow, &nz, (const PetscInt**) &gcols, (const PetscScalar **) &vals);CHKERRQ(ierr);

  // I ripped this bit of code from PETSc's MatSetValues_MPIAIJ() in mpiaij.c.  The PETSc getrow returns everything in
  // global numbering, so we must convert to local numbering.
  if (strcmp(MatType_,MATMPIAIJ) == 0) {
    Mat_MPIAIJ  *aij = (Mat_MPIAIJ*)Amat_->data;
    lcols = (PetscInt *) malloc(nz * sizeof(int));
    if (!aij->colmap) {
      ierr = CreateColmap_MPIAIJ_Private(Amat_);CHKERRQ(ierr);
    }
    /*
      A PETSc parallel aij matrix uses two matrices to represent the local rows.
      The first matrix, A, is square and contains all local columns.
      The second matrix, B, is rectangular and contains all non-local columns.

      Matrix A:
      Local column ID's are mapped to global column id's by adding cmap.rstart.
      Given the global ID of a local column, the local ID is found by
      subtracting cmap.rstart.

      Matrix B:
      Non-local column ID's are mapped to global column id's by the local-to-
      global map garray.  Given the global ID of a local column, the local ID is
      found by the global-to-local map colmap.  colmap is either an array or
      hash table, the latter being the case when PETSC_USE_CTABLE is defined.
    */
    int offset = Amat_->cmap.n-1; //offset for non-local column indices

    //printf("cmap.rstart = %d, cmap.rend = %d, cmap.n = %d, cmap.N = %d\n",
    //        Amat_->cmap.rstart,Amat_->cmap.rend, Amat_->cmap.n,Amat_->cmap.N);

    for (int i=0; i<nz; i++) {
/*
      ierr = PetscTableFind(aij->colmap,gcols[i]+1,lcols+i);CHKERRQ(ierr);
      if (lcols[i] != 0) {lcols[i]--; printf("lcols[%d] = %d\n",i,lcols[i]);}
      else               {lcols[i] = gcols[i] - Amat_->cmap.rstart;
                          printf("lcols[%d] = %d (%d - %d)\n",i,lcols[i],gcols[i],Amat_->cmap.rstart);
      }
      printf("       %d -> %d\n",gcols[i],lcols[i]);
*/
      if (gcols[i] >= Amat_->cmap.rstart && gcols[i] < Amat_->cmap.rend) {
        lcols[i] = gcols[i] - Amat_->cmap.rstart;
        //printf("       %d -> %d (local)\n",gcols[i],lcols[i]);
      } else {
#       ifdef PETSC_USE_CTABLE
        ierr = PetscTableFind(aij->colmap,gcols[i]+1,lcols+i);CHKERRQ(ierr);
        lcols[i] = lcols[i] + offset;
#       else
        lcols[i] = aij->colmap[gcols[i]] + offset;
#       endif
        //printf("       %d -> %d (ghost)\n",gcols[i],lcols[i]);
      }

    } //for i=0; i<nz; i++)
  }
  else lcols = gcols;

  NumEntries = nz;
  if (NumEntries > Length) return(-1);
  for (int i=0; i<NumEntries; i++) {
    Indices[i] = lcols[i];
    Values[i] = vals[i];
  }
  MatRestoreRow(Amat_,globalRow,&nz,(const PetscInt**) &gcols, (const PetscScalar **) &vals);
  return(0);
} //ExtractMyRowCopy()

//==========================================================================

int Epetra_PETScAIJMatrix::NumMyRowEntries(int Row, int & NumEntries) const 
{
  int globalRow = PetscRowStart_ + Row;
  MatGetRow(Amat_, globalRow, &NumEntries, PETSC_NULL, PETSC_NULL);
  MatRestoreRow(Amat_,globalRow,&NumEntries, PETSC_NULL, PETSC_NULL);
  return(0);
}

//==============================================================================

int Epetra_PETScAIJMatrix::ExtractDiagonalCopy(Epetra_Vector & Diagonal) const
{

  //TODO optimization: only get this diagonal once
  Vec petscDiag;
  double *vals=0;
  int length;

  int ierr=VecCreate(Comm_->Comm(),&petscDiag);CHKERRQ(ierr);
  VecSetSizes(petscDiag,NumMyRows_,NumGlobalRows_);
# ifdef HAVE_MPI
  ierr = VecSetType(petscDiag,VECMPI);CHKERRQ(ierr);
# else //TODO untested!!
  VecSetType(petscDiag,VECSEQ);
# endif

  MatGetDiagonal(Amat_, petscDiag);
  VecGetArray(petscDiag,&vals);
  VecGetLocalSize(petscDiag,&length);
  for (int i=0; i<length; i++) Diagonal[i] = vals[i];
  VecRestoreArray(petscDiag,&vals);
  VecDestroy(petscDiag);
  return(0);
}

//=============================================================================

int Epetra_PETScAIJMatrix::Multiply(bool TransA,
                               const Epetra_MultiVector& X,
                               Epetra_MultiVector& Y) const
{
  (void)TransA;
  int NumVectors = X.NumVectors();
  if (NumVectors!=Y.NumVectors()) EPETRA_CHK_ERR(-1);  // X and Y must have same number of vectors

  double ** xptrs;
  double ** yptrs;
  X.ExtractView(&xptrs);
  Y.ExtractView(&yptrs);
  if (RowMatrixImporter()!=0) {
    if (ImportVector_!=0) {
      if (ImportVector_->NumVectors()!=NumVectors) { delete ImportVector_; ImportVector_= 0;}
    }
    if (ImportVector_==0) ImportVector_ = new Epetra_MultiVector(RowMatrixColMap(),NumVectors);
    ImportVector_->Import(X, *RowMatrixImporter(), Insert);
    ImportVector_->ExtractView(&xptrs);
  }

  Vec petscDiag;
  double *vals=0;
  int length;
  Vec petscX, petscY;
/* new */
//VecCreateMPIWithArray(MPI_Comm comm,PetscInt n,PetscInt N,const PetscScalar array[],Vec *vv)
/* new */
/*
  int ierr = VecCreate(Comm_->Comm(),&petscX);CHKERRQ(ierr);
  ierr = VecCreate(Comm_->Comm(),&petscY);CHKERRQ(ierr);
  ierr = VecSetSizes(petscX,X.MyLength(),X.GlobalLength()); CHKERRQ(ierr);
  ierr = VecSetSizes(petscY,Y.MyLength(),Y.GlobalLength()); CHKERRQ(ierr);
#ifdef HAVE_MPI
  VecSetType(petscX,VECMPI);
  VecSetType(petscY,VECMPI);
#else
  VecSetType(petscX,VECSEQ);
  VecSetType(petscY,VECSEQ);
#endif
  int *localIndices = new int[X.MyLength()];
  for (int i=0; i<X.MyLength(); i++) localIndices[i] = i;
*/
  int ierr;
  for (int i=0; i<NumVectors; i++) {
#   ifdef HAVE_MPI
    ierr=VecCreateMPIWithArray(Comm_->Comm(),X.MyLength(),X.GlobalLength(),xptrs[i],&petscX); CHKERRQ(ierr);
    ierr=VecCreateMPIWithArray(Comm_->Comm(),Y.MyLength(),Y.GlobalLength(),yptrs[i],&petscY); CHKERRQ(ierr);
#   else //FIXME  I suspect this will bomb in serial (i.e., w/o MPI
    ierr=VecCreateSeqWithArray(Comm_->Comm(),X.MyLength(),X.GlobalLength(),xptrs[i],&petscX); CHKERRQ(ierr);
    ierr=VecCreateSeqWithArray(Comm_->Comm(),Y.MyLength(),Y.GlobalLength(),yptrs[i],&petscY); CHKERRQ(ierr);
#   endif
/*
    ierr = VecSetValuesLocal(petscX,X.MyLength(),localIndices,xptrs[i],INSERT_VALUES);CHKERRQ(ierr);
    ierr = VecAssemblyBegin(petscX); VecAssemblyEnd(petscX);CHKERRQ(ierr);
    ierr = VecSetValuesLocal(petscY,Y.MyLength(),localIndices,yptrs[i],INSERT_VALUES);CHKERRQ(ierr);
    ierr = VecAssemblyBegin(petscY); VecAssemblyEnd(petscY);CHKERRQ(ierr);
*/

    ierr = MatMult(Amat_,petscX,petscY);CHKERRQ(ierr);

    ierr = VecGetArray(petscY,&vals);CHKERRQ(ierr);
    ierr = VecGetLocalSize(petscY,&length);CHKERRQ(ierr);
    for (int j=0; j<length; j++) yptrs[i][j] = vals[j];
    ierr = VecRestoreArray(petscY,&vals);CHKERRQ(ierr);
  }
  
  double flops = NumGlobalNonzeros();
  flops *= 2.0;
  flops *= (double) NumVectors;
  UpdateFlops(flops);
  return(0);
} //Multiply()

//=============================================================================

int Epetra_PETScAIJMatrix::Solve(bool Upper, bool Trans, bool UnitDiagonal, 
                            const Epetra_MultiVector& X,
                            Epetra_MultiVector& Y) const
{
  (void)Upper;
  (void)Trans;
  (void)UnitDiagonal;
  (void)X;
  (void)Y;
  return(-1); // Not implemented
}

//=============================================================================
// Utility routine to get the specified row and put it into Values_ and Indices_ arrays
int Epetra_PETScAIJMatrix::MaxNumEntries() const {
  int NumEntries;

  if (MaxNumEntries_==-1) {
    for (int i=0; i < NumMyRows_; i++) {
      NumMyRowEntries(i, NumEntries);
      if (NumEntries>MaxNumEntries_) MaxNumEntries_ = NumEntries;
    }
  }
  return(MaxNumEntries_);
}

//=============================================================================
// Utility routine to get the specified row and put it into Values_ and Indices_ arrays
int Epetra_PETScAIJMatrix::GetRow(int Row) const {

  int NumEntries;
  int MaxNumEntries = Epetra_PETScAIJMatrix::MaxNumEntries();

  if (MaxNumEntries>0) {
    if (Values_==0) Values_ = new double[MaxNumEntries];
    if (Indices_==0) Indices_ = new int[MaxNumEntries];
  }
  Epetra_PETScAIJMatrix::ExtractMyRowCopy(Row, MaxNumEntries, NumEntries, Values_, Indices_);
  
  return(NumEntries);
}

//=============================================================================
int Epetra_PETScAIJMatrix::InvRowSums(Epetra_Vector& x) const {
//
// Put inverse of the sum of absolute values of the ith row of A in x[i].
//

  if (!OperatorRangeMap().SameAs(x.Map())) EPETRA_CHK_ERR(-2); // x must have the same distribution as the range of A

  int ierr = 0;
  int i, j;
  for (i=0; i < NumMyRows_; i++) {
    int NumEntries = GetRow(i); // Copies ith row of matrix into Values_ and Indices_
    double scale = 0.0;
    for (j=0; j < NumEntries; j++) scale += fabs(Values_[j]);
    if (scale<Epetra_MinDouble) {
      if (scale==0.0) ierr = 1; // Set error to 1 to signal that zero rowsum found (supercedes ierr = 2)
      else if (ierr!=1) ierr = 2;
      x[i] = Epetra_MaxDouble;
    }
    else
      x[i] = 1.0/scale;
  }
  UpdateFlops(NumGlobalNonzeros());
  EPETRA_CHK_ERR(ierr);
  return(0);
}

//=============================================================================
//=============================================================================
int Epetra_PETScAIJMatrix::InvColSums(Epetra_Vector& x) const {
//
// Put inverse of the sum of absolute values of the jth column of A in x[j].
//

  if (!Filled()) EPETRA_CHK_ERR(-1); // Matrix must be filled.
  if (!OperatorDomainMap().SameAs(x.Map())) EPETRA_CHK_ERR(-2); // x must have the same distribution as the domain of A
  

  Epetra_Vector * xp = 0;
  Epetra_Vector * x_tmp = 0;
  

  // If we have a non-trivial importer, we must export elements that are permuted or belong to other processors
  if (RowMatrixImporter()!=0) {
    x_tmp = new Epetra_Vector(RowMatrixColMap()); // Create import vector if needed
    xp = x_tmp;
  }
  int ierr = 0;
  int i, j;

  for (i=0; i < NumMyCols_; i++) (*xp)[i] = 0.0;

  for (i=0; i < NumMyRows_; i++) {
    int NumEntries = GetRow(i);// Copies ith row of matrix into Values_ and Indices_
    for (j=0; j < NumEntries; j++) (*xp)[Indices_[j]] += fabs(Values_[j]);
  }

  if (RowMatrixImporter()!=0){
    x.Export(*x_tmp, *RowMatrixImporter(), Add); // Fill x with Values from import vector
    delete x_tmp;
    xp = &x;
  }
  // Invert values, don't allow them to get too large
  for (i=0; i < NumMyRows_; i++) {
    double scale = (*xp)[i];
    if (scale<Epetra_MinDouble) {
      if (scale==0.0) ierr = 1; // Set error to 1 to signal that zero rowsum found (supercedes ierr = 2)
      else if (ierr!=1) ierr = 2;
      (*xp)[i] = Epetra_MaxDouble;
    }
    else
      (*xp)[i] = 1.0/scale;
  }
  UpdateFlops(NumGlobalNonzeros());
  EPETRA_CHK_ERR(ierr);
  return(0);
}

//=============================================================================
int Epetra_PETScAIJMatrix::LeftScale(const Epetra_Vector& x) {
//
// This function scales the ith row of A by x[i].
//
//TODO Does PETSc have an ApplyPermutation method for matrices?!

  return(-1); //FIXME

  if (!Filled()) EPETRA_CHK_ERR(-1); // Matrix must be filled.
  if (!OperatorRangeMap().SameAs(x.Map())) EPETRA_CHK_ERR(-2); // x must have the same distribution as the range of A

  int i, j;
  int * bindx;
  double * val;
/*
  FIXME
  int * bindx = Amat_->bindx;
  double * val = Amat_->val;
*/


  for (i=0; i < NumMyRows_; i++) {
    
    int NumEntries = bindx[i+1] - bindx[i];
    double scale = x[i];
    val[i] *= scale;
    double * Values = val + bindx[i];
    for (j=0; j < NumEntries; j++)  Values[j] *= scale;
  }
  NormOne_ = -1.0; // Reset Norm so it will be recomputed.
  NormInf_ = -1.0; // Reset Norm so it will be recomputed.
  UpdateFlops(NumGlobalNonzeros());
  return(0);
}

//=============================================================================
int Epetra_PETScAIJMatrix::RightScale(const Epetra_Vector& x) {
//
// This function scales the jth row of A by x[j].
//
//TODO Does PETSc have an ApplyPermutation method for matrices?!

  return(-1); //FIXME

  if (!Filled()) EPETRA_CHK_ERR(-1); // Matrix must be filled.
  if (!OperatorDomainMap().SameAs(x.Map())) EPETRA_CHK_ERR(-2); // x must have the same distribution as the domain of A

  int * bindx;
  double * val;
/*
  FIXME
  int * bindx = Amat_->bindx;
  double * val = Amat_->val;
*/
  Epetra_Vector * xp = 0;
  Epetra_Vector * x_tmp = 0;

  // If we have a non-trivial importer, we must import elements that are permuted or are on other processors
  if (RowMatrixImporter()!=0) {
    x_tmp = new Epetra_Vector(RowMatrixColMap()); // Create import vector if needed
    x_tmp->Import(x,*RowMatrixImporter(), Insert); // x_tmp will have all the values we need
    xp = x_tmp;
  }

  int i, j;

  for (i=0; i < NumMyRows_; i++) {
    int NumEntries = bindx[i+1] - bindx[i];
    double scale = (*xp)[i];
    val[i] *= scale;
    double * Values = val + bindx[i];
    int * Indices = bindx + bindx[i];
    for (j=0; j < NumEntries; j++)  Values[j] *=  (*xp)[Indices[j]];
  }
  if (x_tmp!=0) delete x_tmp;
  NormOne_ = -1.0; // Reset Norm so it will be recomputed.
  NormInf_ = -1.0; // Reset Norm so it will be recomputed.
  UpdateFlops(NumGlobalNonzeros());
  return(0);
}

//=============================================================================

double Epetra_PETScAIJMatrix::NormInf() const {

  if (NormInf_>-1.0) return(NormInf_);

  MatNorm(Amat_, NORM_INFINITY,&NormInf_);
  UpdateFlops(NumGlobalNonzeros());

  return(NormInf_);
}

//=============================================================================

double Epetra_PETScAIJMatrix::NormOne() const {

  if (NormOne_>-1.0) return(NormOne_);
  if (!Filled()) EPETRA_CHK_ERR(-1); // Matrix must be filled.

  MatNorm(Amat_, NORM_1,&NormOne_);
  UpdateFlops(NumGlobalNonzeros());

  return(NormOne_);
}

//=============================================================================

void Epetra_PETScAIJMatrix::Print(ostream& os) const {
  return;
}

#endif /*HAVE_PETSC*/
#endif /*HAVE_EXPERIMENTAL*/
