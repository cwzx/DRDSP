#ifndef INCLUDED_DYNAMICS_MODEL_REDUCED_PRODUCER
#define INCLUDED_DYNAMICS_MODEL_REDUCED_PRODUCER
#include "model.h"
#include "model_reduced.h"
#include "reduced_data_system.h"

namespace DRDSP {

	struct ModelReducedProducer {
		double fitWeight[2];
		uint16_t numRBFs;

		ModelReducedProducer();
		double ComputeTotalCost( ModelReduced& model, const ReducedDataSystem& data, const VectorXd* parameters ) const;
		ModelReduced ComputeModelReduced( const ReducedDataSystem& data, uint8_t parameterDimension, const VectorXd* parameters ) const;
		ModelReduced BruteForce( const ReducedDataSystem& data, uint8_t parameterDimension, const VectorXd* parameters, uint32_t numIterations ) const;
		void Fit( ModelReduced& reduced, const ReducedDataSystem& data, uint8_t parameterDimension, const VectorXd* parameters ) const;

	protected:
		void ComputeMatrices( const ModelRBF& model, const ReducedData& data, const VectorXd& parameter, MatrixXd& A, MatrixXd& B ) const;
	};

}

#endif

