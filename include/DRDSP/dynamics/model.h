#ifndef INCLUDED_DYNAMICS_MODEL
#define INCLUDED_DYNAMICS_MODEL
#include "embedding.h"
#include "dynamicalSystem.h"

namespace DRDSP {

	/**
	 * \brief Base for model without parameters.
	 */
	template<typename State = VectorXd>
	struct Model {
		typedef State State;
		uint32_t stateDim; ///< Dimension of the model's state space

		Model() : stateDim(0) {}
		explicit Model( uint32_t stateDim ) : stateDim(stateDim) {}
	};

	/**
	 * \brief Base for model with parameters.
	 */
	template<typename Model,typename Parameter = VectorXd>
	struct Family {
		typedef Model Model;
		typedef Parameter Parameter;
		uint32_t stateDim,  ///< Dimension of the model's state space
		         paramDim;  ///< Dimension of the model's parameter space

		Family() : stateDim(0), paramDim(0) {}
		Family( uint32_t stateDim, uint32_t paramDim ) : stateDim(stateDim), paramDim(paramDim) {}
	};
	
	/**
	 * \brief A model without parameters whose state space is embedded into R^n.
	 */
	template<typename M,typename Embedding>
	struct ModelEmbedded : Model<typename M::State> {
		M model;              ///< The underlying model
		Embedding embedding;  ///< An embedding into R^n

		ModelEmbedded() {
			dimension = model.stateDim;
		}

		ModelEmbedded( const M& m, const Embedding& e ) :
			Model<State>(m.stateDim),
			model(m),
			embedding(e)
		{}

		VectorXd operator()( const State& state ) const {
			return embedding.Derivative(state) * model(state);
		}

		MatrixXd Partials( const State& state ) const {
			MatrixXd result;
			result.setZero(embedding.embedDim,embedding.sourceDim);
			VectorXd vector = model(state);
			for(uint32_t i=0;i<embedding.embedDim;++i) {
				result.row(i) += embedding.Derivative2(state,i) * vector;
			}
			result += embedding.Derivative(state) * model.Partials(state);
			return result;
		}

	};

	/**
	 * \brief A model with parameters whose state space is embedded into R^n.
	 */
	template<typename F,typename Embedding>
	struct FamilyEmbedded : Family<typename F::Model,typename F::Parameter> {
		F family;        ///< The underlying family
		Embedding embedding;  ///< An embedding into R^n

		FamilyEmbedded() {
			stateDim = embedding.embedDim;
			paramDim = family.paramDim;
		}
		
		FamilyEmbedded( const F& f, const Embedding& e ) :
			Family<Model,Parameter>(e.embedDim,f.paramDim),
			family(f),
			embedding(e)
		{}

		ModelEmbedded<Model,Embedding> operator()( const Parameter& parameter ) const {
			return ModelEmbedded<Model,Embedding>( family(parameter), embedding );
		}

	};

	/**
	 * \brief A parameter map applied to a family.
	 */
	template<typename F,typename PMap>
	struct PMapFamily : Family<typename F::Model,typename F::Parameter> {
		F family;       ///< The underlying family
		PMap pmap;      ///< A map between parameter spaces

		PMapFamily() {
			stateDim = family.stateDim;
			paramDim = pmap.sourceDim;
		}

		PMapFamily( const F& f, const PMap& p ) :
			Family<Model,Parameter>(f.stateDim,p.sourceDim),
			family(f),
			pmap(p)
		{}

		Model operator()( const Parameter& parameter ) const {
			return family(pmap(parameter));
		}

	};

}

#endif
