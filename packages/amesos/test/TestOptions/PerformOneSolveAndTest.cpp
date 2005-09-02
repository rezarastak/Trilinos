//
//  OUR_CHK_ERR always returns 1 on error.
//

#define OUR_CHK_ERR(a) { { int epetra_err = a; \
                      if (epetra_err != 0) { cerr << "Amesos ERROR " << epetra_err << ", " \
                           << __FILE__ << ", line " << __LINE__ << endl; \
relerror = 1.3e15; relresidual=1e15; return(1);}  }\
                   }

#include "Epetra_Comm.h"
#include "Teuchos_ParameterList.hpp"
#include "Amesos.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_Map.h"
#include "Epetra_Vector.h"
#include "Epetra_LinearProblem.h"
#include "PerformOneSolveAndTest.h"
#include "PartialFactorization.h"
#include "CreateTridi.h"
#include "NewMatNewMap.h" 
#include "Amesos_TestRowMatrix.h" 

//
//  Returns the number of failures.
//  Note:  If AmesosClass is not supported, PerformOneSolveAndTest() will 
//  always return 0
//
//  Still have to decide where we are going to check the residual.  
//
//  The following table shows the variable names that we use for 
//  each of the three phases:  
//     compute - which computes the correct value of b
//     solve - which solves for x in  A' A' A x = b 
//     check - which computes bcheck = A' A' A x 
//
//  For ill-conditioned matrices we restrict the test to one or two 
//  solves, by setting Levels to 1 or 2 on input to this routine.
//  When Levels is less than 3, some of the transformations
//  shown in the table as "->" and "<-" are not performed, instead 
//  a direct copy is made.
//
//  In the absence of roundoff, each item in a given column should 
//  be identical.  
//
//  If Levels = 3, we compute and solve A' A' A x = b and hence all 
//  of the transformations are performed
//
//  If Levels = 2, the transformations shown in the first column of 
//  transformations (labelled Levels>=3) are replaced by a direct copy.
//
//  If Levels = 1, only the transformations shown in the third column
//  are performed, the others being replaced by direct copies.
//  
//                           Levels>=3    Levels>=2
//                              A'         A'            A
//  compute             xexact  ->  cAx    ->     cAAx   ->       b 
//  solve               x       <-  sAx    <-     sAAx   <-       b
//  check               x       ->  kAx    ->     kAAx   ->  bcheck
//
//  Note that since Levels 2 and 3 use the same A, there is no need to 
//  call NumericFactorization() between the second and third call to Solve. 
//   

int PerformOneSolveAndTest( const char* AmesosClass,
			    int EpetraMatrixType,
			    const Epetra_Comm &Comm, 
			    bool transpose, 
			    bool verbose, 
			    Teuchos::ParameterList ParamList, 
			    Epetra_CrsMatrix *& InMat, 
			    int Levels, 
			    const double Rcond,
			    double& relerror,
			    double& relresidual )
{

  bool TrustMe = ParamList.get( "TrustMe", false );

  bool MyVerbose = false ; 

  RefCountPtr<Epetra_CrsMatrix> MyMat ; 
  RefCountPtr<Epetra_CrsMatrix> MyMatWithDiag ; 

  MyMat = rcp( new Epetra_CrsMatrix( *InMat ) ); 

  Amesos_TestRowMatrix ATRW( &*MyMat ) ; 
  
  Epetra_RowMatrix* MyRowMat ; 
  
  assert ( EpetraMatrixType >= 0 && EpetraMatrixType <= 2 );
  switch ( EpetraMatrixType ) {
  case 0:
    MyRowMat = &*MyMat ; 
    break;
  case 1:
    MyRowMat = &ATRW;
    break;
  case 2:
    MyMat->OptimizeStorage(); 
    MyRowMat = &*MyMat ; 
    bool OptStorage = MyMat->StorageOptimized();
    assert( OptStorage) ; 
    break;
  }
  bool OptStorage = MyMat->StorageOptimized();
  
  Epetra_CrsMatrix* MatPtr = &*MyMat ;

  OUR_CHK_ERR ( PartialFactorization( AmesosClass, Comm, transpose, verbose, 
				      ParamList, MatPtr, Rcond ) );

  if (ParamList.isParameter("AddToDiag")) { 
    //
    //  If AddToDiag is set, create a matrix which is numerically identical, but structurally 
    //  has no missing diagaonal entries.   In other words, every diagonal element in MyMayWithDiag 
    //  has an entry in the matrix, though that entry will be zero if InMat has no entry for that
    //  particular diagonal element.  
    //
    MyMatWithDiag = NewMatNewMap( *InMat, 2, 0, 0, 0, 0 );  //  Ensure that all diagonal entries exist ;

    //
    //  Now add AddToDiag to each diagonal element.  
    //
    double AddToDiag = ParamList.get("AddToDiag", 0.0 );
    Epetra_Vector Diag( MyMatWithDiag->RowMap() );
    Epetra_Vector AddConstVecToDiag( MyMatWithDiag->RowMap() );
    AddConstVecToDiag.PutScalar( AddToDiag );
    assert( MyMatWithDiag->ExtractDiagonalCopy( Diag ) == 0 );
    Diag.Update( 1.0, AddConstVecToDiag, 1.0 ) ; 
    assert(MyMatWithDiag->ReplaceDiagonalValues( Diag ) == 0 ) ; 
  } else { 
    MyMatWithDiag = rcp( new Epetra_CrsMatrix( *InMat ) ); 
  }
  //  Epetra_CrsMatrix*& Amat = &*MyMat ; 

  if ( MyVerbose ) cout << " Partial Factorization complete " << endl ; 

  relerror = 0 ; 
  relresidual = 0 ; 

  assert( Levels >= 1 && Levels <= 3 ) ; 

  int iam = Comm.MyPID() ; 
  int errors = 0 ; 

  const Epetra_Map *Map = &MyMat->RowMap() ; 
  const Epetra_Map *RangeMap = 
    transpose?&MyMat->OperatorDomainMap():&MyMat->OperatorRangeMap() ; 
  const Epetra_Map *DomainMap = 
    transpose?&MyMat->OperatorRangeMap():&MyMat->OperatorDomainMap() ; 

  Epetra_Vector xexact(*DomainMap);
  Epetra_Vector x(*DomainMap);

  Epetra_Vector cAx(*DomainMap);
  Epetra_Vector sAx(*DomainMap);
  Epetra_Vector kAx(*DomainMap);

  Epetra_Vector cAAx(*DomainMap);
  Epetra_Vector sAAx(*DomainMap);
  Epetra_Vector kAAx(*DomainMap);

  Epetra_Vector FixedLHS(*DomainMap);
  Epetra_Vector FixedRHS(*RangeMap);

  Epetra_Vector b(*RangeMap);
  Epetra_Vector bcheck(*RangeMap);

  Epetra_Vector DomainDiff(*DomainMap);
  Epetra_Vector RangeDiff(*RangeMap);

  Epetra_LinearProblem Problem;
  Amesos_BaseSolver* Abase ; 
  Amesos Afactory;




  Abase = Afactory.Create( AmesosClass, Problem ) ; 

  relerror = 0 ; 
  relresidual = 0 ; 

  if ( Abase == 0 ) 
    assert( false ) ; 
  else {

    //
    //  Phase 1:  Compute b = A' A' A xexact
    //
    Problem.SetOperator( MyRowMat );
    Epetra_CrsMatrix* ECM = dynamic_cast<Epetra_CrsMatrix*>(MyRowMat) ; 
    
    //
    //  We only set transpose if we have to - this allows valgrind to check
    //  that transpose is set to a default value before it is used.
    //
    if ( transpose ) OUR_CHK_ERR( Abase->SetUseTranspose( transpose ) ); 
    if (MyVerbose) ParamList.set( "DebugLevel", 1 );
    if (MyVerbose) ParamList.set( "OutputLevel", 1 );
    OUR_CHK_ERR( Abase->SetParameters( ParamList ) ); 

    if ( TrustMe ) {
      Problem.SetLHS( &FixedLHS );
      Problem.SetRHS( &FixedRHS );
      assert( OptStorage) ;
    }
    OUR_CHK_ERR( Abase->SymbolicFactorization(  ) ); 
    OUR_CHK_ERR( Abase->NumericFactorization(  ) ); 

    int ind[1];
    double val[1];
    ind[0] = 0;
    xexact.Random();
    xexact.PutScalar(1.0);

    //
    //  Compute cAx = A' xexact
    //
    double Value = 1.0 ;
    if ( Levels == 3 ) 
      {
	val[0] = Value ; 
	if ( MyMatWithDiag->MyGRID( 0 ) ) { 
	  MyMatWithDiag->SumIntoMyValues( 0, 1, val, ind ) ; 
	}
	MyMatWithDiag->Multiply( transpose, xexact, cAx ) ; 

	val[0] = - Value ; 
	if ( MyMatWithDiag->MyGRID( 0 ) )
	  MyMatWithDiag->SumIntoMyValues( 0, 1, val, ind ) ; 
      }
    else
      {
	cAx = xexact ;
      }

    //
    //  Compute cAAx = A' cAx
    //
    if ( Levels >= 2 ) 
      {
	val[0] =  Value ; 
	if ( MyMatWithDiag->MyGRID( 0 ) )
	  MyMatWithDiag->SumIntoMyValues( 0, 1, val, ind ) ; 
	MyMatWithDiag->Multiply( transpose, cAx, cAAx ) ; //  x2 = A' x1

	val[0] = - Value ; 
	if ( MyMatWithDiag->MyGRID( 0 ) )
	  MyMatWithDiag->SumIntoMyValues( 0, 1, val, ind ) ; 
      }
    else
      {
	cAAx = cAx ;
      }

    if ( MyVerbose ) cout << " Compute  b = A x2 = A A' A'' xexact  " << endl ; 

    MyMatWithDiag->Multiply( transpose, cAAx, b ) ;  //  b = A x2 = A A' A'' xexact
 

    //  Phase 2:  Solve A' A' A x = b 
    //
    //
    //  Solve A sAAx = b 
    //
    if ( TrustMe ) { 
      FixedRHS = b;
    } else { 
      Problem.SetLHS( &sAAx );
      Problem.SetRHS( &b );
    }


    OUR_CHK_ERR( Abase->SymbolicFactorization(  ) ); 
    OUR_CHK_ERR( Abase->SymbolicFactorization(  ) );     // This should be irrelevant, but should nonetheless be legal 
    OUR_CHK_ERR( Abase->NumericFactorization(  ) ); 
    OUR_CHK_ERR( Abase->Solve(  ) ); 
    if ( TrustMe ) sAAx = FixedLHS ; 
    if ( MyVerbose ) cerr << __FILE__ << "::" << __LINE__ << endl ; 

    if ( Levels >= 2 ) 
      {
        OUR_CHK_ERR( Abase->SetUseTranspose( transpose ) ); 
	if ( TrustMe ) { 
	  FixedRHS = sAAx ;
	} else { 
	  Problem.SetLHS( &sAx );
	  Problem.SetRHS( &sAAx );
	}
	val[0] =  Value ; 
	if ( MyMat->MyGRID( 0 ) )
	  MyMat->SumIntoMyValues( 0, 1, val, ind ) ; 
	OUR_CHK_ERR( Abase->NumericFactorization(  ) ); 
	
	Teuchos::ParameterList* NullList = (Teuchos::ParameterList*) 0 ;  
	//      We do not presently handle null lists.
	//	OUR_CHK_ERR( Abase->SetParameters( *NullList ) );   // Make sure we handle null lists 
	OUR_CHK_ERR( Abase->Solve(  ) ); 
	if ( TrustMe ) sAx = FixedLHS ; 
	if ( MyVerbose ) cerr << __FILE__ << "::" << __LINE__ << endl ; 
	
      }
    else
      {
	sAx = sAAx ;
      }

    if ( Levels >= 3 ) 
      {
	if ( TrustMe ) { 
	  FixedRHS = sAx ;
	} else { 
	  Problem.SetLHS( &x );
	  Problem.SetRHS( &sAx );
	}
	OUR_CHK_ERR( Abase->Solve(  ) ); 
	if ( TrustMe ) x = FixedLHS ;
	if ( MyVerbose ) cerr << __FILE__ << "::" << __LINE__ << endl ; 
      }
    else
      {
	x = sAx ; 
      }

    if ( Levels >= 2 ) 
      {
	val[0] =  -Value ; 
	if ( MyMat->MyGRID( 0 ) ) {
	  if ( MyMat->SumIntoMyValues( 0, 1, val, ind ) ) { 
	    cout << " TestOptions requires a non-zero entry in A(1,1) " << endl ; 
	  }
	}
      }

    //
    //  Phase 3:  Check the residual: bcheck = A' A' A x 
    //


    if ( Levels >= 3 ) 
      {
	val[0] =  Value ; 
	if ( MyMatWithDiag->MyGRID( 0 ) )
	  MyMatWithDiag->SumIntoMyValues( 0, 1, val, ind ) ; 
	MyMatWithDiag->Multiply( transpose, x, kAx ) ;
	val[0] =  -Value ; 
	if ( MyMatWithDiag->MyGRID( 0 ) )
	  MyMatWithDiag->SumIntoMyValues( 0, 1, val, ind ) ; 
      }
    else
      {
	kAx = x ; 
      }

    if ( Levels >= 2 ) 
      {
	val[0] =  Value ; 
	if ( MyMatWithDiag->MyGRID( 0 ) )
	  MyMatWithDiag->SumIntoMyValues( 0, 1, val, ind ) ; 
	MyMatWithDiag->Multiply( transpose, kAx, kAAx ) ;
	val[0] =  -Value ; 
	if ( MyMatWithDiag->MyGRID( 0 ) )
	  MyMatWithDiag->SumIntoMyValues( 0, 1, val, ind ) ; 
      }
    else
      {
	kAAx = kAx ; 
      }

    if ( MyVerbose ) cerr << __FILE__ << "::" << __LINE__ << endl ; 
    MyMatWithDiag->Multiply( transpose, kAAx, bcheck ) ; //  temp = A" x2
    if ( MyVerbose ) cerr << __FILE__ << "::" << __LINE__ << endl ; 

    if ( MyVerbose ) cout << " Levels =  " << Levels << endl ; 
    if ( MyVerbose ) cout << " Rcond =  " << Rcond << endl ; 

    double norm_diff ;
    double norm_one ;

    DomainDiff.Update( 1.0, sAAx, -1.0, cAAx, 0.0 ) ;
    DomainDiff.Norm2( &norm_diff ) ; 
    sAAx.Norm2( &norm_one ) ; 
    if (MyVerbose) cout << " norm( sAAx - cAAx ) / norm(sAAx ) = " 
		      << norm_diff /norm_one << endl ; 


    DomainDiff.Update( 1.0, sAx, -1.0, cAx, 0.0 ) ;
    DomainDiff.Norm2( &norm_diff ) ; 
    sAx.Norm2( &norm_one ) ; 
    if (MyVerbose) cout << " norm( sAx - cAx ) / norm(sAx ) = " 
		      << norm_diff /norm_one << endl ; 


    DomainDiff.Update( 1.0, x, -1.0, xexact, 0.0 ) ;
    DomainDiff.Norm2( &norm_diff ) ; 
    x.Norm2( &norm_one ) ; 
    if (MyVerbose) cout << " norm( x - xexact ) / norm(x) = " 
		      << norm_diff /norm_one << endl ; 

    relerror = norm_diff / norm_one ; 

    DomainDiff.Update( 1.0, sAx, -1.0, kAx, 0.0 ) ;
    DomainDiff.Norm2( &norm_diff ) ; 
    sAx.Norm2( &norm_one ) ; 
    if (MyVerbose) cout << " norm( sAx - kAx ) / norm(sAx ) = " 
		      << norm_diff /norm_one << endl ; 


    DomainDiff.Update( 1.0, sAAx, -1.0, kAAx, 0.0 ) ;
    DomainDiff.Norm2( &norm_diff ) ; 
    sAAx.Norm2( &norm_one ) ; 
    if (MyVerbose) cout << " norm( sAAx - kAAx ) / norm(sAAx ) = " 
		      << norm_diff /norm_one << endl ; 


    RangeDiff.Update( 1.0, bcheck, -1.0, b, 0.0 ) ;
    RangeDiff.Norm2( &norm_diff ) ; 
    bcheck.Norm2( &norm_one ) ; 
    if (MyVerbose) cout << " norm( bcheck - b ) / norm(bcheck ) = " 
		      << norm_diff /norm_one << endl ; 

    relresidual = norm_diff / norm_one ; 


    if (iam == 0 ) {
      if ( relresidual * Rcond < 1e-16 ) {
	if (MyVerbose) cout << " Test 1 Passed " << endl ;
      } else {
	cout <<  __FILE__ << "::"  << __LINE__ << 
	  " relresidual = " << relresidual <<
	  " TEST 1 FAILED " << endl ;
	errors += 1 ; 
      }
    }

    delete Abase;
  }

  return errors;
  
}


