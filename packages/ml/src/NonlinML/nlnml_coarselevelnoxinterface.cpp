/*
#@HEADER
# ************************************************************************
#
#               ML: A Multilevel Preconditioner Package
#                 Copyright (2002) Sandia Corporation
#
# Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the U.S. Government.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# Questions? Contact Jonathan Hu (jhu@sandia.gov) or Ray Tuminaro 
# (rstumin@sandia.gov).
#
# ************************************************************************
#@HEADER
*/
/* ******************************************************************** */
/* See the file COPYRIGHT for a complete copyright notice, contact      */
/* person and disclaimer.                                               */
/* ******************************************************************** */
// ML-headers
#include "ml_common.h"
#if defined(HAVE_ML_NOX) && defined(HAVE_ML_EPETRA) && defined(HAVE_ML_AZTECOO) && defined(HAVE_ML_TEUCHOS) && defined(HAVE_ML_IFPACK) && defined(HAVE_ML_AMESOS) && defined(HAVE_ML_EPETRAEXT)

// ----------   Includes   ----------
#include <ctime>
#include <iostream>
#include "nlnml_coarselevelnoxinterface.H"

/*----------------------------------------------------------------------*
 |  ctor (public)                                             m.gee 3/06|
 *----------------------------------------------------------------------*/
NLNML::NLNML_CoarseLevelNoxInterface::NLNML_CoarseLevelNoxInterface(
                 RefCountPtr<NLNML::NLNML_FineLevelNoxInterface> finterface,
                 int level,int outlevel,
                 RefCountPtr< vector< RefCountPtr<Epetra_CrsMatrix> > > P,
                 const Epetra_BlockMap& this_bmap,
                 int maxlevel) :
level_(level),
maxlevel_(maxlevel),
outputlevel_(outlevel),
isFASmodfied_(false),
nFcalls_(0),
fineinterface_(finterface),
P_(P),
fbar_(NULL),
fxbar_(NULL)
{
  this_bmap_ = rcp(new Epetra_BlockMap(this_bmap));
  
  // create a series of working vectors to be used on prolongation/restriction
  wvec_.clear();
  if (Level())
  {
    wvec_.resize(Level()+1);
    for (int i=0; i<Level(); ++i)
      wvec_[i] = rcp(new Epetra_Vector((*P)[i+1]->OperatorRangeMap(),false));
    wvec_[Level()] = rcp(new Epetra_Vector((*P)[Level()]->OperatorDomainMap(),false));
  }
  
  
  
  return;
}


/*----------------------------------------------------------------------*
 |  ctor (public)                                             m.gee 3/06|
 *----------------------------------------------------------------------*/
void NLNML::NLNML_CoarseLevelNoxInterface::recreate()
{

  return;
}


/*----------------------------------------------------------------------*
 |  dtor (public)                                             m.gee 3/06|
 *----------------------------------------------------------------------*/
NLNML::NLNML_CoarseLevelNoxInterface::~NLNML_CoarseLevelNoxInterface()
{
  return;
}


/*----------------------------------------------------------------------*
 |  evaluate nonlinear function (public, derived)             m.gee 3/06|
 *----------------------------------------------------------------------*/
bool NLNML::NLNML_CoarseLevelNoxInterface::computeF(
                                 const Epetra_Vector& x, Epetra_Vector& F, 
			         const FillType fillFlag)
{
  return true;
}

/*----------------------------------------------------------------------*
 |  restrict from fine to this level (public)                 m.gee 3/06|
 *----------------------------------------------------------------------*/
Epetra_Vector*  NLNML::NLNML_CoarseLevelNoxInterface::restrict_fine_to_this(
                                                 const Epetra_Vector& xfine)
{
  if (!Level())
  {
    Epetra_Vector* xcoarse = new Epetra_Vector(xfine);
    return xcoarse;
  }
  else
  {
    // Note that the GIDs in xfine match those of the fineinterface and
    // might be different from those in P_[1]->OperatorRangeMap().
    // The LIDs and the map match, so we have to copy xfine to xfineP
    // using LIDs
    Epetra_Vector* xfineP = wvec_[0].get(); // RangeMap of P_[1]
    if (xfine.MyLength() != xfineP->MyLength() || xfine.GlobalLength() != xfineP->GlobalLength())
    {
        cout << "**ERR**: NLNML::NLNML_CoarseLevelNoxInterface::restrict_fine_to_this:\n"
             << "**ERR**: mismatch in dimension of xfine and xfineP\n"
             << "**ERR**: file/line: " << __FILE__ << "/" << __LINE__ << "\n"; throw -1;
    }
    const int mylength = xfine.MyLength();
    for (int i=0; i<mylength; i++)
      (*xfineP)[i] = xfine[i];      
    
    // loop from the finest level to this level and
    // apply series of restrictions (that is transposed prolongations)
    Epetra_Vector* fvec = xfineP;
    for (int i=0; i<Level()-1; i++)
    {
      Epetra_Vector* cvec = wvec_[i+1].get();
      (*P_)[i+1]->Multiply(true,*fvec,*cvec);
      fvec = cvec;
    }
    Epetra_Vector* out = new Epetra_Vector((*P_)[Level()]->OperatorDomainMap(),false); 
    (*P_)[Level()]->Multiply(true,*fvec,*out);
    return out;   
  }
}

/*----------------------------------------------------------------------*
 |  prolongate from this level to fine (public)               m.gee 3/06|
 *----------------------------------------------------------------------*/
void NLNML::NLNML_CoarseLevelNoxInterface::prolong_this_to_fine()
{
  return;
}

/*----------------------------------------------------------------------*
 |  restrict from this to next coarser level (public)         m.gee 3/06|
 *----------------------------------------------------------------------*/
void NLNML::NLNML_CoarseLevelNoxInterface::restrict_to_next_coarser_level()
{
  return;
}


/*----------------------------------------------------------------------*
 |  prolongate from next coarser level to this level (public) m.gee 3/06|
 *----------------------------------------------------------------------*/
void NLNML::NLNML_CoarseLevelNoxInterface::prolong_to_this_level()
{
  return;
}

/*----------------------------------------------------------------------*
 |  set ptr to all prolongation operators (public)            m.gee 3/06|
 *----------------------------------------------------------------------*/
void NLNML::NLNML_CoarseLevelNoxInterface::setP()
{
  return;
}


/*----------------------------------------------------------------------*
 |  set modified system (public)                              m.gee 3/06|
 *----------------------------------------------------------------------*/
void NLNML::NLNML_CoarseLevelNoxInterface::setModifiedSystem()
{
  return;
}

/*----------------------------------------------------------------------*
 |  make application apply all constraints (public)           m.gee 3/06|
 *----------------------------------------------------------------------*/
void NLNML::NLNML_CoarseLevelNoxInterface::ApplyAllConstraints()
{
  return;
}


/*----------------------------------------------------------------------*
 |  return this levels blockmap (public)                      m.gee 3/06|
 *----------------------------------------------------------------------*/
void NLNML::NLNML_CoarseLevelNoxInterface::BlockMap()
{
  return;
}









#endif
