#include "Epetra_BLAS.h"
#include "ExecuteTestProblems.h"
#include "BuildTestProblems.h"
#include "Epetra_Comm.h"
  int MatrixTests(const Epetra_BlockMap & Map, const Epetra_LocalMap & LocalMap, int NumVectors,
		      bool verbose)
  {
    const Epetra_Comm & Comm = Map.Comm();
    int ierr = 0, i;
    int IndexBase = 0;
    double *residual = new double[NumVectors];

    /* get ID of this processor */

    int MyPID   = Comm.MyPID();


    // Test GEMM first.  7 cases:
    
    //                                       Num
    //     OPERATIONS                        case  Notes
    // 1) C(local) = A^X(local) * B^X(local)  4   (X=Trans or Not, No Comm needed) 
    // 2) C(local) = A^T(distr) * B  (distr)  1   (2D dot product, replicate C)
    // 3) C(distr) = A  (distr) * B^X(local)  2   (2D vector update, no Comm needed)

    // ==================================================================
    // Case 1 through 4 (A, B, C all local) Strided and non-strided cases
    // ==================================================================

    // Construct MultiVectors

  {
    Epetra_MultiVector A(LocalMap, NumVectors);
    Epetra_MultiVector B(LocalMap, NumVectors);
    Epetra_LocalMap  Map2d(NumVectors, IndexBase, Comm);
    Epetra_MultiVector C(Map2d, NumVectors);
    Epetra_MultiVector C_GEMM(Map2d, NumVectors);

    double **App, **Bpp, **Cpp;
    
    Epetra_MultiVector *Ap, *Bp, *Cp;

    // For testing non-strided mode, create MultiVectors that are scattered throughout memory

    App = new double *[NumVectors];
    Bpp = new double *[NumVectors];
    Cpp = new double *[NumVectors];
    for (i=0; i<NumVectors; i++) App[i] = new double[A.MyLength()+i];
    for (i=0; i<NumVectors; i++) Bpp[i] = new double[B.MyLength()+i];
    for (i=0; i<NumVectors; i++) Cpp[i] = new double[C.MyLength()+i];
    
    Epetra_MultiVector A1(View, LocalMap, App, NumVectors);
    Epetra_MultiVector B1(View, LocalMap, Bpp, NumVectors);
    Epetra_MultiVector C1(View, Map2d, Cpp, NumVectors);

    for (int strided = 0; strided<2; strided++){
    int ierr;
    // Loop through all trans cases using a variety of values for alpha and beta
    for (i=0; i<4; i++){
	ierr = 0;
	char transa = 'N'; if (i>1) transa = 'T';
	char transb = 'N'; if (i%2!=0) transb = 'T';
	double alpha = (double) i+1;
	double beta  = (double) (i/2);
	ierr += C.Random();  // Fill C with random numbers
	ierr += BuildMatrixTests(C,transa, transb, alpha, A, B, beta, C_GEMM );
	if (strided)
	  {
	    Ap = &A; Bp = &B; Cp = &C;
	  }
	else
	  {
	    A.ExtractCopy(App); Ap = &A1;
	    B.ExtractCopy(Bpp); Bp = &B1;
	    C.ExtractCopy(Cpp); Cp = &C1;
	  }
	  
	ierr += Cp->Multiply(transa, transb, alpha, *Ap, *Bp, beta);
	ierr += Cp->Update(-1.0, C_GEMM, 1.0);
	ierr += Cp->Norm2(residual);

	if (verbose && ierr==0)
	  {
	    cout << "XXXXX Replicated Local MultiVector GEMM tests";
	    if (strided)
	    cout << " (Strided Multivectors)" << endl;
	    else
	    cout << " (Non-Strided Multivectors)" << endl;
	    cout << "  alpha = " << alpha << ",  beta = " << beta <<", transa = "<<transa
		 <<", transb = " << transb;
	  }
	if (ierr==0 && BadResidual(verbose,residual, NumVectors)) return(-1);
      }

      }
    for (i=0; i<NumVectors; i++)
      {
	delete [] App[i];
	delete [] Bpp[i];
	delete [] Cpp[i];
      }
    delete [] App;
    delete [] Bpp;
    delete [] Cpp;
  }
      
    // ====================================
    // Case 5  (A, B distributed C  local)
    // ====================================

    // Construct MultiVectors
  {
    Epetra_MultiVector A(Map, NumVectors);
    Epetra_MultiVector B(Map, NumVectors);
    Epetra_LocalMap Map2d(NumVectors, IndexBase, Comm);
    Epetra_MultiVector C(Map2d, NumVectors);
    Epetra_MultiVector C_GEMM(Map2d, NumVectors);

    char transa = 'T';
    char transb = 'N';
    double alpha = 2.0;
    double beta  = 1.0;
    ierr += C.Random();  // Fill C with random numbers
    ierr += BuildMatrixTests(C, transa, transb, alpha, A, B, beta, C_GEMM );
    ierr += C.Multiply(transa, transb, alpha, A, B, beta);
    ierr += C.Update(-1.0, C_GEMM, 1.0);
    ierr += C.Norm2(residual);

    if (verbose && ierr==0)
      {
	cout << "XXXXX Generalized 2D dot product via GEMM call     " << endl;
	cout << "  alpha = " << alpha << ",  beta = " << beta <<", transa = "<<transa
	     <<", transb = " << transb;
      }
    if (BadResidual(verbose,residual, NumVectors)) return(-1);
    
    
  }      
    // ====================================
    // Case 6-7  (A, C distributed, B local)
    // ====================================

    // Construct MultiVectors
  {
    Epetra_MultiVector A(Map, NumVectors);
    Epetra_LocalMap Map2d(NumVectors, IndexBase, Comm);
    Epetra_MultiVector B(Map2d, NumVectors);
    Epetra_MultiVector C(Map, NumVectors);
    Epetra_MultiVector C_GEMM(Map, NumVectors);

    for (i=0; i<2; i++)
      {
	char transa = 'N';
	char transb = 'N'; if (i>0) transb = 'T';
	double alpha = 2.0;
	double beta  = 1.1;
	ierr += C.Random();  // Fill C with random numbers
	ierr += BuildMatrixTests(C,transa, transb, alpha, A, B, beta, C_GEMM );
	ierr += C.Multiply(transa, transb, alpha, A, B, beta);
	ierr += C.Update(-1.0, C_GEMM, 1.0);
	ierr += C.Norm2(residual);
	
	if (verbose)
	  {
	    cout << "XXXXX Generalized 2D vector update via GEMM call     " << endl;
	    cout << "  alpha = " << alpha << ",  beta = " << beta <<", transa = "<<transa
		 <<", transb = " << transb;
	  }
	if (BadResidual(verbose,residual, NumVectors)) return(-1);
      }

    delete [] residual;
    
    return(ierr);
  }
  }

int MultiVectorTests(const Epetra_BlockMap & Map, int NumVectors, bool verbose)
{
  const Epetra_Comm & Comm = Map.Comm();
  int ierr = 0, i;
  double *residual = new double[NumVectors];
  
  Epetra_BLAS BLAS;
  /* get number of processors and the name of this processor */
  
  // int NumProc = Comm.getNumProc();
  int MyPID   = Comm.MyPID();
  
  // Construct MultiVectors
  
  Epetra_MultiVector A(Map, NumVectors);
  Epetra_MultiVector sqrtA(Map, NumVectors);
  Epetra_MultiVector B(Map, NumVectors);
  Epetra_MultiVector C(Map, NumVectors);
  Epetra_MultiVector C_alphaA(Map, NumVectors);
  Epetra_MultiVector C_alphaAplusB(Map, NumVectors);
  Epetra_MultiVector C_plusB(Map, NumVectors);
  Epetra_MultiVector Weights(Map, NumVectors);
  
  // Construct double vectors
  double *dotvec_AB   = new double[NumVectors];
  double *norm1_A     = new double[NumVectors];
  double *norm2_sqrtA = new double[NumVectors];
  double *norminf_A = new double[NumVectors];
  double *normw_A = new double[NumVectors];
  double *minval_A = new double[NumVectors];
  double *maxval_A = new double[NumVectors];
  double *meanval_A = new double[NumVectors];
  
  // Generate data 

  
  C.Random(); // Fill C with random numbers.
  double alpha = 2.0;
  BuildMultiVectorTests (C,alpha, A, sqrtA, B, C_alphaA, C_alphaAplusB,
			     C_plusB, dotvec_AB, norm1_A, norm2_sqrtA, norminf_A, 
			     normw_A, Weights, minval_A, maxval_A, meanval_A);
  
  if (verbose) cout << "XXXXX Testing alpha * A     ";
  // Test alpha*A
  Epetra_MultiVector alphaA(A); // Copy of A
  ierr += alphaA.Scale(alpha);
  ierr += alphaA.Update(-1.0, C_alphaA, 1.0);
  ierr += alphaA.Norm2(residual);
  
  if (ierr!=0 && verbose) 
    cout << "Error in alpha * A MultiVector testing";
  if (ierr) return(-2);
  if (verbose)
	{
	  cout << "  alpha = " << alpha;
	}
	if (BadResidual(verbose,residual, NumVectors)) return(-1);
  
  if (verbose) cout << "XXXXX Testing C = alpha * A + B      ";
  // Test alpha*A + B
  Epetra_MultiVector alphaAplusB(A); // Copy of A
  ierr += alphaAplusB.Update(1.0, B, alpha, A, 0.0);
  ierr += alphaAplusB.Update(-1.0, C_alphaAplusB, 1.0);
  ierr += alphaAplusB.Norm2(residual);
  
  if (ierr!=0 && verbose) 
    cout << "Error in alpha * A + B MultiVector testing";
  if (ierr) return(-2);
  if (verbose) cout << "  alpha = " << alpha;
  if (BadResidual(verbose,residual, NumVectors)) return(-1);
  
  if (verbose) cout << "XXXXX Testing C += B      ";
  // Test + B
  Epetra_MultiVector plusB(C); // Copy of C
  ierr += plusB.Update(1.0, B, 1.0);
  ierr += plusB.Update(-1.0, C_plusB, 1.0);
  ierr += plusB.Norm2(residual);
  
  if (ierr!=0 && verbose) 
    cout << "Error in + B MultiVector testing";
  if (ierr) return(-2);
  if (BadResidual(verbose,residual, NumVectors)) return(-1);
  
  if (verbose) cout << "XXXXX Testing A.dotProd(B)     ";
  // Test A.dotvec(B)
  double *dotvec = residual;
  ierr += A.Dot(B,dotvec);
  BLAS.AXPY(NumVectors,-1.0,dotvec_AB,dotvec);
  
  if (ierr!=0 && verbose) 
    cout << "Error dotvec MultiVector testing";
  if (ierr) return(-2);
  if (BadResidual(verbose,residual, NumVectors)) return(-1);
  
  
  if (verbose) cout << "XXXXX Testing norm1_A      ";
  // Test A.norm1()
  double *norm1 = residual;
  ierr += A.Norm1(norm1);
  BLAS.AXPY(NumVectors,-1.0,norm1_A,norm1);
  
  if (ierr!=0 && verbose)
    cout << "Error in norm1 MultiVector testing";
  if (ierr) return(-2);
  if (BadResidual(verbose,residual, NumVectors)) return(-1);
	  
  
  if (verbose) cout << "XXXXX Testing norm2_sqrtA     ";
  // Test sqrtA.norm2()
  double *norm2 = residual;
  ierr += sqrtA.Norm2(norm2);
  BLAS.AXPY(NumVectors,-1.0,norm2_sqrtA,norm2);
  
  if (ierr!=0 && verbose)
    cout << "Error in norm2 MultiVector testing";
  if (ierr) return(-2);
  if (BadResidual(verbose,residual, NumVectors)) return(-1);
	
  
  if (verbose) cout << "XXXXX Testing norminf_A     ";
  // Test A.norminf()
  double *norminf = residual;
  ierr += A.NormInf(norminf);
  BLAS.AXPY(NumVectors,-1.0,norminf_A,norminf);
  
  if (ierr!=0 && verbose)
    cout << "Error in NormInf MultiVector testing";
  if (ierr) return(-2);
  if (BadResidual(verbose,residual, NumVectors)) return(-1);
	
  
  if (verbose) cout << "XXXXX Testing normw_A     ";
  // Test A.NormWeighted()
  double *normw = residual;
  ierr += A.NormWeighted(Weights, normw);
  BLAS.AXPY(NumVectors,-1.0,normw_A,normw);
  
  if (ierr!=0 && verbose)
    cout << "Error in NormWeighted MultiVector testing";
  if (ierr) return(-2);
  if (BadResidual(verbose,residual, NumVectors)) return(-1);
	
  
  if (verbose) cout << "XXXXX Testing minval_A     ";
  // Test A.MinValue()
  double *minval = residual;
  ierr += A.MinValue(minval);
  BLAS.AXPY(NumVectors,-1.0,minval_A,minval);
  
  if (ierr!=0 && verbose)
    cout << "Error in MinValue MultiVector testing";
  if (ierr) return(-2);
  if (BadResidual(verbose,residual, NumVectors)) return(-1);
	
  
  if (verbose) cout << "XXXXX Testing maxval_A     ";
  // Test A.MaxValue()
  double *maxval = residual;
  ierr += A.MaxValue(maxval);
  BLAS.AXPY(NumVectors,-1.0,maxval_A,maxval);
  
  if (ierr!=0 && verbose)
    cout << "Error in MaxValue MultiVector testing";
  if (ierr) return(-2);
  if (BadResidual(verbose,residual, NumVectors)) return(-1);
	  
  if (verbose) cout << "XXXXX Testing meanval_A     ";
  // Test A.MeanValue()
  double *meanval = residual;
  ierr += A.MeanValue(meanval);
  BLAS.AXPY(NumVectors,-1.0,meanval_A,meanval);
  
  if (ierr!=0 && verbose)
    cout << "Error in MeanValue MultiVector testing";
  if (ierr) return(-2);
  if (BadResidual(verbose,residual, NumVectors)) return(-1);
	
  
  if (verbose) cout << "XXXXX Testing abs_A     ";
  // Test A.Abs()
  Epetra_MultiVector Abs_A = A;
  ierr += Abs_A.Abs(A);
  ierr += Abs_A.Update(1.0, A, -1.0); // Abs_A = A - Abs_A (should be zero since A > 0)
  ierr += Abs_A.Norm2(residual);
  if (ierr!=0 && verbose)
    cout << "Error in Absolute value MultiVector testing";
  if (ierr) return(-2);
  if (BadResidual(verbose,residual, NumVectors)) return(-1);
	
  
  // Delete everything
  
  delete [] dotvec_AB;
  delete [] norm1_A;
  delete [] norm2_sqrtA;
  delete [] norminf_A;
  delete [] normw_A;
  delete [] minval_A;
  delete [] maxval_A;
  delete [] meanval_A;
  delete [] residual;
  
  return(ierr);
}

int BadResidual(bool verbose, double * Residual, int NumVectors)
{
  double threshold = 5.0E-6;
  int ierr = 0;
  for (int i=0; i<NumVectors; i++) {
    if (Residual[i]>threshold) {
      ierr = 1;
      if (verbose) cout << endl << "     Residual[" << i <<"] = " << Residual[i];
    }
  }
  if (verbose)
    if (ierr==0) cout << "\t Checked OK" << endl;
    else cout << endl << "Test failed" << endl;
  
  return(ierr);
}
