#ifndef VIENNACL_BACKEND_BLAS_HPP
#define VIENNACL_BACKEND_BLAS_HPP

/* =========================================================================
   Copyright (c) 2010-2013, Institute for Microelectronics,
                            Institute for Analysis and Scientific Computing,
                            TU Wien.
   Portions of this software are copyright by UChicago Argonne, LLC.

                            -----------------
                  ViennaCL - The Vienna Computing Library
                            -----------------

   Project Head:    Karl Rupp                   rupp@iue.tuwien.ac.at

   (A list of authors and contributors can be found in the PDF manual)

   License:         MIT (X11), see file LICENSE in the base directory
============================================================================= */

/** @file viennacl/backend/blas.hpp
    @brief Main interface routines for overriding some underlying BLAS functions
*/

#include <cassert>

#ifdef VIENNACL_WITH_CBLAS
#include "cblas.h"
#endif


#ifdef VIENNACL_WITH_CUBLAS
#include "cublas.h"
#endif

namespace viennacl
{
  namespace backend
  {

    namespace detail{

      template<class TransposeType>
      struct matrix_blas_wrapper{
          matrix_blas_wrapper(TransposeType _trans, TransposeType _notrans, bool is_row_major, bool is_transposed
                              ,vcl_size_t internal_size1, vcl_size_t internal_size2
                              ,vcl_size_t start1, vcl_size_t start2
                              ,vcl_size_t stride1, vcl_size_t stride2){
            trans = !(is_transposed^is_row_major)?_trans:_notrans;
            negtrans = (trans==_trans)?_notrans:_trans;
            ld = is_row_major?stride1*internal_size2:stride2*internal_size1;
            off = is_row_major?start1*internal_size2+start2:start2*internal_size1+start1;
          }
          vcl_size_t ld;
          vcl_size_t off;
          TransposeType trans;
          TransposeType negtrans;
      };


#ifdef VIENNACL_WITH_CBLAS
      template<class T>
      struct cblas_wrapper;

      template<>
      struct cblas_wrapper<float>{
        private:
          typedef detail::matrix_blas_wrapper<CBLAS_TRANSPOSE> wrapper;
        public:
          static bool gemm(bool C_row_major, bool A_row_major, bool B_row_major
                           ,bool is_A_trans, bool is_B_trans
                           ,const vcl_size_t M, const vcl_size_t N, const vcl_size_t K, const float alpha
                           ,float const *Ap, const vcl_size_t A_internal_size1, const vcl_size_t A_internal_size2
                           ,const vcl_size_t A_start1, const vcl_size_t A_start2, const vcl_size_t A_inc1, const vcl_size_t A_inc2
                           ,float const *Bp, const vcl_size_t B_internal_size1, const vcl_size_t B_internal_size2
                           ,const vcl_size_t B_start1, const vcl_size_t B_start2, const vcl_size_t B_inc1, const vcl_size_t B_inc2
                           ,const float beta, float *Cp, const vcl_size_t C_internal_size1, const vcl_size_t C_internal_size2
                           ,const vcl_size_t C_start1, const vcl_size_t C_start2, const vcl_size_t C_inc1, const vcl_size_t C_inc2)
          {
            if(A_inc1!=1 || A_inc2!=1 || B_inc1!=1 || B_inc2!=1 || C_inc1!=1 || C_inc2!=1)
              return false;

            wrapper A(CblasTrans, CblasNoTrans, A_row_major,is_A_trans,A_internal_size1, A_internal_size2, A_start1, A_start2, A_inc1, A_inc2);
            wrapper B(CblasTrans, CblasNoTrans, B_row_major,is_B_trans,B_internal_size1, B_internal_size2, B_start1, B_start2, B_inc1, B_inc2);
            wrapper C(CblasTrans, CblasNoTrans, C_row_major,false,C_internal_size1, C_internal_size2, C_start1, C_start2, C_inc1, C_inc2);

            if(C_row_major)
              cblas_sgemm(CblasColMajor,B.trans, A.trans, N, M, K, alpha, Bp+B.off, B.ld, Ap+A.off, A.ld, beta, Cp+C.off, C.ld);
            else
              cblas_sgemm(CblasColMajor,A.negtrans, B.negtrans, M, N, K, alpha, Ap+A.off, A.ld, Bp+B.off, B.ld, beta, Cp+C.off, C.ld);

            return true;
          }
      };


      template<>
      struct cblas_wrapper<double>{
        private:
          typedef detail::matrix_blas_wrapper<CBLAS_TRANSPOSE> wrapper;
        public:
          static bool gemm(bool C_row_major, bool A_row_major, bool B_row_major
                           ,bool is_A_trans, bool is_B_trans
                           ,const vcl_size_t M, const vcl_size_t N, const vcl_size_t K, const double alpha
                           ,double const *Ap, const vcl_size_t A_internal_size1, const vcl_size_t A_internal_size2
                           ,const vcl_size_t A_start1, const vcl_size_t A_start2, const vcl_size_t A_inc1, const vcl_size_t A_inc2
                           ,double const *Bp, const vcl_size_t B_internal_size1, const vcl_size_t B_internal_size2
                           ,const vcl_size_t B_start1, const vcl_size_t B_start2, const vcl_size_t B_inc1, const vcl_size_t B_inc2
                           ,const double beta, double *Cp, const vcl_size_t C_internal_size1, const vcl_size_t C_internal_size2
                           ,const vcl_size_t C_start1, const vcl_size_t C_start2, const vcl_size_t C_inc1, const vcl_size_t C_inc2)
          {
            if(A_inc1!=1 || A_inc2!=1 || B_inc1!=1 || B_inc2!=1 || C_inc1!=1 || C_inc2!=1)
              return false;

            wrapper A(CblasTrans, CblasNoTrans, A_row_major,is_A_trans,A_internal_size1, A_internal_size2, A_start1, A_start2, A_inc1, A_inc2);
            wrapper B(CblasTrans, CblasNoTrans, B_row_major,is_B_trans,B_internal_size1, B_internal_size2, B_start1, B_start2, B_inc1, B_inc2);
            wrapper C(CblasTrans, CblasNoTrans, C_row_major,false,C_internal_size1, C_internal_size2, C_start1, C_start2, C_inc1, C_inc2);

            if(C_row_major)
              cblas_dgemm(CblasColMajor,B.trans, A.trans, N, M, K, alpha, Bp+B.off, B.ld, Ap+A.off, A.ld, beta, Cp+C.off, C.ld);
            else
              cblas_dgemm(CblasColMajor,A.negtrans, B.negtrans, M, N, K, alpha, Ap+A.off, A.ld, Bp+B.off, B.ld, beta, Cp+C.off, C.ld);

            return true;
          }
      };
#endif


#ifdef VIENNACL_WITH_CUBLAS
      template<class T>
      struct cublas_wrapper;

      template<>
      struct cublas_wrapper<float>{
        private:
          typedef detail::matrix_blas_wrapper<char> wrapper;
        public:
          static bool gemm(bool C_row_major, bool A_row_major, bool B_row_major
                           ,bool is_A_trans, bool is_B_trans
                           ,const vcl_size_t M, const vcl_size_t N, const vcl_size_t K, const float alpha
                           ,float const *Ap, const vcl_size_t A_internal_size1, const vcl_size_t A_internal_size2
                           ,const vcl_size_t A_start1, const vcl_size_t A_start2, const vcl_size_t A_inc1, const vcl_size_t A_inc2
                           ,float const *Bp, const vcl_size_t B_internal_size1, const vcl_size_t B_internal_size2
                           ,const vcl_size_t B_start1, const vcl_size_t B_start2, const vcl_size_t B_inc1, const vcl_size_t B_inc2
                           ,const float beta, float *Cp, const vcl_size_t C_internal_size1, const vcl_size_t C_internal_size2
                           ,const vcl_size_t C_start1, const vcl_size_t C_start2, const vcl_size_t C_inc1, const vcl_size_t C_inc2)
          {
            if(A_inc1!=1 || A_inc2!=1 || B_inc1!=1 || B_inc2!=1 || C_inc1!=1 || C_inc2!=1)
              return false;

            wrapper A('T', 'N', A_row_major, is_A_trans, A_internal_size1, A_internal_size2, A_start1, A_start2, A_inc1, A_inc2);
            wrapper B('T', 'N', B_row_major, is_B_trans, B_internal_size1, B_internal_size2, B_start1, B_start2, B_inc1, B_inc2);
            wrapper C('T', 'N', C_row_major, false, C_internal_size1, C_internal_size2, C_start1, C_start2, C_inc1, C_inc2);

            if(C_row_major)
              cublasSgemm(B.trans, A.trans, N, M, K, alpha, Bp+B.off, B.ld, Ap+A.off, A.ld, beta, Cp+C.off, C.ld);
            else
              cublasSgemm(A.negtrans, B.negtrans, M, N, K, alpha, Ap+A.off, A.ld, Bp+B.off, B.ld, beta, Cp+C.off, C.ld);

            return true;
          }
      };


      template<>
      struct cublas_wrapper<double>{
        private:
          typedef detail::matrix_blas_wrapper<char> wrapper;
        public:
          static bool gemm(bool C_row_major, bool A_row_major, bool B_row_major
                           ,bool is_A_trans, bool is_B_trans
                           ,const vcl_size_t M, const vcl_size_t N, const vcl_size_t K, const double alpha
                           ,double const *Ap, const vcl_size_t A_internal_size1, const vcl_size_t A_internal_size2
                           ,const vcl_size_t A_start1, const vcl_size_t A_start2, const vcl_size_t A_inc1, const vcl_size_t A_inc2
                           ,double const *Bp, const vcl_size_t B_internal_size1, const vcl_size_t B_internal_size2
                           ,const vcl_size_t B_start1, const vcl_size_t B_start2, const vcl_size_t B_inc1, const vcl_size_t B_inc2
                           ,const double beta, double *Cp, const vcl_size_t C_internal_size1, const vcl_size_t C_internal_size2
                           ,const vcl_size_t C_start1, const vcl_size_t C_start2, const vcl_size_t C_inc1, const vcl_size_t C_inc2)
          {
            if(A_inc1!=1 || A_inc2!=1 || B_inc1!=1 || B_inc2!=1 || C_inc1!=1 || C_inc2!=1)
              return false;

            wrapper A('T', 'N', A_row_major,is_A_trans,A_internal_size1, A_internal_size2, A_start1, A_start2, A_inc1, A_inc2);
            wrapper B('T', 'N', B_row_major,is_B_trans,B_internal_size1, B_internal_size2, B_start1, B_start2, B_inc1, B_inc2);
            wrapper C('T', 'N', C_row_major,false,C_internal_size1, C_internal_size2, C_start1, C_start2, C_inc1, C_inc2);

            if(C_row_major)
              cublasDgemm(B.trans, A.trans, N, M, K, alpha, Bp+B.off, B.ld, Ap+A.off, A.ld, beta, Cp+C.off, C.ld);
            else
              cublasDgemm(A.negtrans, B.negtrans, M, N, K, alpha, Ap+A.off, A.ld, Bp+B.off, B.ld, beta, Cp+C.off, C.ld);

            return true;
          }
      };
#endif

    }

    template<class T, class PtrType = T*, class ConstPtrType = T const *>
    struct blas_function_types{
        typedef T value_type;
        typedef PtrType pointer_type;
        typedef ConstPtrType const_pointer_type;

        typedef bool (*gemm)(bool /*C_row_major*/, bool /*A_row_major*/, bool /*B_row_major*/
                             ,bool /*is_A_trans*/, bool /*is_B_trans*/
                             ,const vcl_size_t /*M*/, const vcl_size_t /*N*/, const vcl_size_t /*K*/, const T /*alpha*/
                             ,ConstPtrType /*A*/ , const vcl_size_t /*A_internal_size1*/, const vcl_size_t /*A_internal_size2*/
                             ,const vcl_size_t /*A_start1*/, const vcl_size_t /*A_start2*/, const vcl_size_t /*A_inc1*/, const vcl_size_t /*A_inc2*/
                             ,ConstPtrType /*B*/, const vcl_size_t /*B_internal_size1*/, const vcl_size_t /*B_internal_size2*/
                             ,const vcl_size_t /*B_start1*/, const vcl_size_t /*B_start2*/, const vcl_size_t /*B_inc1*/, const vcl_size_t /*B_inc2*/
                             ,const T /*beta*/, PtrType /*C*/, const vcl_size_t /*C_internal_size1*/, const vcl_size_t /*C_internal_size2*/
                             ,const vcl_size_t /*C_start1*/, const vcl_size_t /*C_start2*/, const vcl_size_t /*C_inc1*/, const vcl_size_t /*C_inc2*/);
    };

    namespace result_of{
      template<class T> struct host_blas_functions {  typedef blas_function_types<T, T*, T const *> type; };
      template<class T> struct cuda_blas_functions {  typedef blas_function_types<T, T*, T const *> type; };
    }

#define HAS_MEM_FUNC(func, name)                                        \
  template<typename T, typename Sign>                                 \
  struct name {                                                       \
  typedef char yes[1];                                            \
  typedef char no [2];                                            \
  template <typename U, U> struct type_check;                     \
  template <typename _1> static yes &chk(type_check<Sign, &_1::gemm> *); \
  template <typename   > static no  &chk(...);                    \
  static bool const value = sizeof(chk<T>(0)) == sizeof(yes);     \
  }



    template<class FunctionsType>
    class blas{
      public:
        typedef FunctionsType functions_type;
        typedef typename functions_type::value_type value_type;
      private:
        typedef typename functions_type::gemm gemm_t;

        /** @brief SFINAE Check of the existence of a GEMM function with the proper signature */
        template<typename T, typename Sign>
        struct init_gemm{                                                                                      \
            template <typename U, U> struct type_check;
            template <typename _1> static Sign chk(type_check<Sign, &_1::gemm> *){ return _1::gemm; }
            template <typename   > static Sign chk(...){ return NULL; }
            static Sign value() { return chk<T>(0); }
        };

      public:
        blas() : gemm_(NULL) {
#ifdef VIENNACL_WITH_CBLAS
          gemm_ = init_gemm< viennacl::backend::detail::cblas_wrapper<value_type>, gemm_t>::value();
#endif
#ifdef VIENNACL_WITH_CUBLAS
          gemm_ = init_gemm< viennacl::backend::detail::cublas_wrapper<value_type>, gemm_t>::value();
#endif
        }


        gemm_t gemm() const { return gemm_; }
        void gemm(gemm_t fptr) { gemm_ = fptr; }

      private:
        gemm_t gemm_;
    };

  }

}

#endif
