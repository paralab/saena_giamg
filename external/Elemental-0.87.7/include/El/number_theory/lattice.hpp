/*
   Copyright (c) 2009-2016, Jack Poulson,
   All rights reserved.

   Copyright (c) 2016, Ron Estrin,
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#ifndef EL_LATTICE_HPP
#define EL_LATTICE_HPP

namespace El {

// Deep insertion
// ==============
template<typename F>
void DeepColSwap( Matrix<F>& B, Int i, Int k );
template<typename F>
void DeepRowSwap( Matrix<F>& B, Int i, Int k );

// Lattice Log Potential
// =====================
// NOTE: This operates on the Gaussian Normal Form (R from QR)
template<typename F>
Base<F> LatticeLogPotential( const Matrix<F>& R );

} // namespace El

#include <El/number_theory/lattice/LLL.hpp>

namespace El {

// Fill M with its (quasi-reduced) image of B, and fill K with the
// LLL-reduced basis for the kernel of B.
//
// This is essentially Algorithm 2.7.1 from Cohen's
// "A course in computational algebraic number theory". The main difference
// is that we avoid solving the normal equations and call a least squares
// solver.
// 
template<typename F>
void LatticeImageAndKernel
( const Matrix<F>& B,
        Matrix<F>& M,
        Matrix<F>& K,
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

// Fill K with the LLL-reduced basis for the image of B.
// TODO: Make this faster than LatticeImageAndKernel
template<typename F>
void LatticeImage
( const Matrix<F>& B,
        Matrix<F>& M,
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

// Fill K with the LLL-reduced basis for the kernel of B.
// This will eventually mirror Algorithm 2.7.2 from Cohen's
// "A course in computational algebraic number theory".
template<typename F>
void LatticeKernel
( const Matrix<F>& B,
        Matrix<F>& K,
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

// Search for Z-dependence
// =======================
// Search for Z-dependence of a vector of real or complex numbers, z, via
// the quadratic form
//
//   Q(a) = || a ||_2^2 + N | z^T a |^2,
//
// which is generated by the basis matrix
//
//   
//   B = [I; sqrt(N) z^T],
//
// as Q(a) = a^T B^T B a = || B a ||_2^2. Cohen has advice for the choice of
// the (large) parameter N within subsection 2.7.2 within his book. 
//
// The number of (nearly) exact Z-dependences detected is returned.
//
template<typename F>
Int ZDependenceSearch
( const Matrix<F>& z,
        Base<F> NSqrt,
        Matrix<F>& B,
        Matrix<F>& U, 
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

// Search for an algebraic relation
// ================================
// Search for the (Gaussian) integer coefficients of a polynomial of alpha
// that is (nearly) zero.
template<typename F>
Int AlgebraicRelationSearch
( F alpha,
  Int n,
  Base<F> NSqrt,
  Matrix<F>& B,
  Matrix<F>& U, 
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

} // namespace El

#include <El/number_theory/lattice/Enumerate.hpp>
#include <El/number_theory/lattice/BKZ.hpp>

namespace El {

// Lattice coordinates
// ===================
// Seek the coordinates x in Z^n of a vector y within a lattice B, i.e.,
//
//     B x = y.
//
// Return 'true' if such coordinates could be found and 'false' otherwise.
template<typename F>
bool LatticeCoordinates( const Matrix<F>& B, const Matrix<F>& y, Matrix<F>& x );

// Enrich a lattice with a particular vector
// =========================================
// Push B v into the first column of B via a unimodular transformation
template<typename F>
void EnrichLattice( Matrix<F>& B, const Matrix<F>& v );
template<typename F>
void EnrichLattice( Matrix<F>& B, Matrix<F>& U, const Matrix<F>& v );

// Closest vector problem
// ======================

template<typename F>
void NearestPlane
( const Matrix<F>& B,
  const Matrix<F>& T,
        Matrix<F>& X,
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

template<typename F>
void NearestPlane
( const Matrix<F>& B,
  const Matrix<F>& QR,
  const Matrix<F>& t,
  const Matrix<Base<F>>& d,
  const Matrix<F>& T,
        Matrix<F>& X,
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

} // namespace El

#include <El/number_theory/lattice/NearestPlane.hpp>
#include <El/number_theory/lattice/Enrich.hpp>

#endif // ifndef EL_LATTICE_HPP