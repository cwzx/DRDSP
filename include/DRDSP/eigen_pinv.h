#ifndef INCLUDED_EIGEN_PINV
#define INCLUDED_EIGEN_PINV
#include <Eigen/Core>
#include <Eigen/SVD>
#include <algorithm>

using namespace Eigen;

namespace DRDSP {

	template<typename Derived>
	Matrix<typename Derived::Scalar,Derived::ColsAtCompileTime,Derived::RowsAtCompileTime> pseudoInverse( const MatrixBase<Derived>& A, typename NumTraits<typename Derived::Scalar>::Real epsilon = NumTraits<typename Derived::Scalar>::epsilon() ) {
		JacobiSVD<Derived> svd( A, ComputeThinU | ComputeThinV );

		NumTraits<Derived::Scalar>::Real tolerance = epsilon
			* std::max( A.cols(), A.rows() )
			* svd.singularValues().array().abs().maxCoeff();

		JacobiSVD<Derived>::SingularValuesType svInv = (svd.singularValues().array().abs() > tolerance).select( svd.singularValues().array().inverse(), 0 );

		return svd.matrixV() * svInv.asDiagonal() * svd.matrixU().adjoint();
	}

	template<typename Derived>
	Matrix<typename Derived::Scalar,-1,1> Vectorize( const MatrixBase<Derived>& A ) {
		Matrix<typename Derived::Scalar,-1,1> V( A.rows() * A.cols() );
		for(int64_t i=0;i<A.cols();++i) {
			V.segment(i*A.rows(),A.rows()) = A.col(i);
		}
		return V;
	}


}

#endif
