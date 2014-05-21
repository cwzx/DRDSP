#include <DRDSP/projection/proj_secant.h>
#include <DRDSP/dynamics/rbf_family_producer.h>
#include <DRDSP/dynamics/data_generator.h>
#include <DRDSP/misc.h>
#include "pendulum.h"

using namespace std;
using namespace DRDSP;

struct Options {
	uint32_t targetDimension, numRBFs, numIterations, numThreads;

	Options() : targetDimension(4), numRBFs(40), numIterations(1000), numThreads(4) {}
	
	Options( int argc, char** argv ) : Options() {
		if( argc >= 2 ) targetDimension = (uint32_t)atoi(argv[1]);
		if( argc >= 3 ) numRBFs = (uint32_t)atoi(argv[2]);
		if( argc >= 4 ) numIterations = (uint32_t)atoi(argv[3]);
		if( argc >= 5 ) numThreads = (uint32_t)atoi(argv[4]);
	}
};

typedef Multiquadratic RadialType;

int main( int argc, char** argv ) {

	Options options(argc,argv);

	// The pendulum example
	FamilyEmbedded<PendulumFamily,FlatEmbedding> pendulum;

	auto parameters = ParameterList( 1.42, 1.43, 11 );

	// Generate the data
	cout << "Generating data..." << endl;
	DataGenerator<PendulumFamily,PendulumSolver> dataGenerator;
	dataGenerator.initial.setZero(pendulum.family.dimension);
	dataGenerator.initial(1) = -0.422;
	dataGenerator.tStart = 1500;
	dataGenerator.tInterval = 12;
	dataGenerator.print = 200;
	dataGenerator.dtMax = 0.001;

	DataSystem data = dataGenerator.GenerateDataSystem( parameters, options.numThreads );
	data.WriteDataSetsCSV("output/orig",".csv");
	
	// Embed the data
	cout << "Embedding data..." << endl;
	DataSystem dataEmbedded = EmbedData( pendulum.embedding, data, options.numThreads );

	// Pre-compute secants
	cout << "Computing secants..." << endl;
	vector<SecantsPreComputed> secants = ComputeSecants( dataEmbedded, options.numThreads );

	// Secant culling
	cout << "Culling secants..." << endl;
	vector<SecantsPreComputed> newSecants = CullSecants( secants, 10.0, options.numThreads );

	secants = vector<SecantsPreComputed>();

	// Find a projection
	cout << "Finding projection..." << endl;
	ProjSecant projSecant( options.targetDimension );
	
	projSecant.ComputeInitial( dataEmbedded )   // Compute initial condition
	          .Find( newSecants )               // Optimize over Grassmannian
	          .AnalyseSecants( newSecants )     // Print some statistics
	          .WriteBinary("output/projection.bin")
	          .WriteCSV("output/projection.csv");

	newSecants = vector<SecantsPreComputed>();

	// Compute projected data
	cout << "Computing Reduced Data..." << endl;
	ReducedDataSystem reducedData;
	reducedData.ComputeDataEmbedded( pendulum, data, projSecant.W, options.numThreads )
	           .WritePointsCSV("output/p","-points.csv")
	           .WriteVectorsCSV("output/p","-vectors.csv");

	// Obtain the reduced model
	cout << "Computing Reduced Model..." << endl;
	RBFFamilyProducer<RadialType> producer(options.numRBFs);
	auto reducedModel = producer.BruteForce( reducedData,
	                                         data.parameterDimension,
	                                         data.parameters,
	                                         options.numIterations );

	cout << "Total Cost = "
		 << producer.ComputeTotalCost( reducedModel, reducedData, data.parameters )
		 << endl;

	reducedModel.WriteCSV("output/reduced.csv");

	// Generate the data
	cout << "Simulating the reduced model..." << endl;
	DataGenerator<RBFFamily<RadialType>> rdataGenerator(reducedModel);
	rdataGenerator.MatchSettings(dataGenerator);
	rdataGenerator.tStart = 0.0;
	DataSystem rdata = rdataGenerator.GenerateUsingInitials( parameters, reducedData, options.numThreads );
	rdata.WriteDataSetsCSV("output/rdata",".csv");

	Compare( reducedData, rdata );

	cout << "Press any key to continue . . . "; cin.get();
}
