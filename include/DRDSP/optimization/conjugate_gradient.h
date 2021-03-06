#ifndef INCLUDED_OPTIMIZATION_CONJUGATE_GRADIENT
#define INCLUDED_OPTIMIZATION_CONJUGATE_GRADIENT
#include "linesearch.h"

namespace DRDSP {
	
	template<typename Geodesic>
	struct ConjugateGradient {
		typedef typename Geodesic::Metric Metric;
		typedef typename Metric::Point Point;
		typedef typename Metric::InnerProduct::Vector Vector;

		Metric metric;
		Geodesic geodesic;
		LineSearch lineSearch;

		uint32_t n = 0,
		         maxSteps = 1000;

		enum modType { FR, PR, HS } modifier = HS;

		template<typename S,typename DS>
		bool Step( Point& x, const S& cost, const DS& gradient ) {
			lastGrad = grad;
			grad = gradient(x);

			Vector Lambda = -grad;
			if( n > 0 )
				Lambda += geodesic.ParallelTranslate(geodesic.velocity,lineSearch.alpha) * Modifier(x);

			geodesic.Set(x,Lambda);

			double alpha = lineSearch.Search(
				[&]( double t ){ return cost( geodesic(t) ); },
				[&]( double t ){
					Point xt = geodesic(t);
					return metric(xt)(
						gradient(xt),
						geodesic.ParallelTranslate( geodesic.velocity, t )
					);
				}
			);

			if( alpha <= 0.0 ) return false;

			lastX = x;
			x = geodesic(alpha);

			return true;
		}

		template<typename S,typename DS>
		bool Optimize( Point& x, const S& cost, const DS& gradient ) {
			n = 0;
			if( lineSearch.alpha <= 0.0 )
				lineSearch.alpha = 1.0e-2;
			for(uint32_t i=0;i<maxSteps;++i) {
				if( !Step( x, cost, gradient ) )
					return true;
				++n;
				cout << n << "\t" << lineSearch.S0 << "\t" << lineSearch.alpha << endl;
			}
			return false;
		}

	protected:
		Point lastX;
		Vector grad, lastGrad;

		double Modifier( const Point& x ) {
			if( n==0 ) return 0.0;
			Vector ptlastGrad, GmptlastG;
			typename Metric::InnerProduct IPX = metric(x);
			switch( modifier ) {
				case FR:
					return IPX.Norm2(grad) / metric(lastX).Norm2(lastGrad);
				break;
				case PR:
					ptlastGrad = geodesic.ParallelTranslate(lastGrad,lineSearch.alpha);
					return  IPX(grad,grad-ptlastGrad) / IPX.Norm2(ptlastGrad);
				break;
				case HS:
					ptlastGrad = geodesic.ParallelTranslate(lastGrad,lineSearch.alpha);
					GmptlastG = grad - ptlastGrad;
					return IPX(grad,GmptlastG) / IPX(ptlastGrad,GmptlastG);
				break;
			}
			return 0.0;
		}
	};

}

#endif

