#include <iostream>
#include <DRDSP/data/data_set.h>
#include <DRDSP/data/secants.h>
#include <DRDSP/projection/proj_secant.h>
#include <DRDSP/dynamics/model_reduced_producer.h>
#include <DRDSP/dynamics/generate_data.h>

#include "pendulum.h"

using namespace std;
using namespace DRDSP;

struct Options {
	Options() : numIterations(100), maxPoints(0), targetDimension(3), numRBFs(60) {}
	uint32_t numIterations, maxPoints;
	uint16_t targetDimension, numRBFs;
};

Options GetOptions( int argc, char** argv ) {
	Options options;

	if( argc >= 2 ) options.targetDimension = (uint16_t)atoi(argv[1]);
	if( argc >= 3 ) options.numRBFs = (uint16_t)atoi(argv[2]);
	if( argc >= 4 ) options.maxPoints = (uint32_t)atoi(argv[3]);
	if( argc >= 5 ) options.numIterations = (uint32_t)atoi(argv[4]);

	return options;
}

int main( int argc, char** argv ) {

	Options options = GetOptions(argc,argv);
/*
	// Create a new data set
	DataSystem data;
	data.maxPoints = options.maxPoints;

	// Load data from file
	bool success = data.Load("data/pendulum.txt");
	if( !success ) {
		return 0;
	}
*/
	// The pendulum example
	PendulumFlat pendulum;

	// Generate the data
	DataGenerator dataGenerator(pendulum.model);
	dataGenerator.pMin = 1.8;
	dataGenerator.pMax = 1.825;
	dataGenerator.pDelta = 0.005;
	dataGenerator.initial(0) = 0.3;
	dataGenerator.initial(1) = 0.3;
	dataGenerator.tStart = 10000;
	dataGenerator.tInterval = 4;
	dataGenerator.print = 200;
	dataGenerator.rk.dtmax = 0.001;

	DataSystem data = dataGenerator.GenerateDataSystem();
	
	// Embed the data
	DataSystem dataEmbedded = pendulum.embedding.EmbedData(data);
	
	// Pre-compute secants
	SecantsPreComputed* secants = new SecantsPreComputed [dataEmbedded.numParameters];
	SecantsPreComputed* newSecants = new SecantsPreComputed [dataEmbedded.numParameters];
	for(uint16_t i=0;i<dataEmbedded.numParameters;i++)
		secants[i].ComputeFromData( dataEmbedded.dataSets[i] );

	// Secant culling
	for(uint16_t i=0;i<dataEmbedded.numParameters;i++)
		newSecants[i] = secants[i].CullSecantsDegrees( 10.0 );

	delete[] secants;

	// Find a projection
	ProjSecant projSecant;
	projSecant.targetDimension = options.targetDimension;
	projSecant.targetMinProjectedLength = 0.7;

	// Compute initial condition
	projSecant.GetInitial(dataEmbedded);

	// Optimize over Grassmannian
	projSecant.Find(newSecants,dataEmbedded.numParameters);

	// Print some statistics
	projSecant.AnalyseSecants(newSecants,dataEmbedded.numParameters);

	delete[] newSecants;

	// Compute projected data
	cout << endl << "Computing Reduced Data..." << endl;
	ReducedDataSystem reducedData;
	reducedData.ComputeData(pendulum,data,projSecant.W);

	// Obtain the reduced model
	cout << endl << "Computing Reduced Model..." << endl;
	ModelReducedProducer producer;
	producer.numRBFs = options.numRBFs;
	ModelReduced reducedModel = producer.BruteForce(reducedData,data.parameterDimension,data.parameters,options.numIterations);

	cout << "Total Cost = " << producer.ComputeTotalCost(reducedModel,reducedData,data.parameters) << endl;

	reducedModel.OutputText("output/reduced.csv");
	//reducedData.WritePointsText("output/p2.5-points.csv");
	//reducedData.WriteVectorsText("output/p2.5-vectors.csv");
	projSecant.WriteBinary("output/projection.bin");
	projSecant.WriteText("output/projection.csv");

	

	system("PAUSE");
	return 0;
}
