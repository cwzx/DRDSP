#pragma warning ( disable : 4503 )
#include <DRDSP/projection/proj_secant.h>
#include <DRDSP/dynamics/rbf_family_producer.h>
#include <DRDSP/dynamics/data_generator.h>
#include <DRDSP/misc.h>
#include "dynamo_solver.h"

using namespace std;
using namespace DRDSP;

struct Options {
	uint32_t targetDimension, numRBFs, numIterations, numThreads;

	Options() : targetDimension(2), numRBFs(40), numIterations(1000), numThreads(3) {}
	
	Options( int argc, char** argv ) : Options() {
		if( argc >= 2 ) targetDimension = (uint32_t)atoi(argv[1]);
		if( argc >= 3 )         numRBFs = (uint32_t)atoi(argv[2]);
		if( argc >= 4 )   numIterations = (uint32_t)atoi(argv[3]);
		if( argc >= 5 )      numThreads = (uint32_t)atoi(argv[4]);
	}
};

typedef PolyharmonicSpline3 RadialType;

int main( int argc, char** argv ) {
	Options options(argc,argv);
	
	DynamoFamily dynamo;

	auto parameters = ParameterList( 1.5, 2.1, 3 );
	
	// Generate the data
	cout << "Generating data..." << endl;
	DataGenerator<DynamoFamily,DynamoSolver> dataGenerator;
	dataGenerator.initial = dynamo(parameters[0]).InitialCondition();
	dataGenerator.tStart = 4;
	dataGenerator.tInterval = 0.11;
	dataGenerator.print = 100;

	DataSystem data = dataGenerator.GenerateDataSystem( parameters, options.numThreads );
	data.WriteDataSetsBinary("output/orig",".bin");

	// Pre-compute secants
	cout << "Computing secants..." << endl;
	vector<SecantsPreComputed> secants = ComputeSecants( data, options.numThreads );

	// Secant culling
	cout << "Culling secants..." << endl;
	vector<SecantsPreComputed> newSecants = CullSecants( secants, 10.0, options.numThreads );

	secants = vector<SecantsPreComputed>();

	// Find a projection
	cout << "Finding projection..." << endl;
	ProjSecant projSecant( options.targetDimension );

	projSecant.ComputeInitial(data)            // Compute initial condition
	          .Find( newSecants )              // Optimize over Grassmannian
			  .AnalyseSecants( newSecants )    // Print some statistics
			  .WriteBinary("output/projection.bin")
			  .WriteCSV("output/projection.csv");

	newSecants = vector<SecantsPreComputed>();

	// Compute projected data
	cout << "Computing Reduced Data..." << endl;
	ReducedDataSystem reducedData;
	reducedData.ComputeData( dynamo, data, projSecant.W, options.numThreads )
	           .WritePointsCSV("output/p","-points.csv")
	           .WriteVectorsCSV("output/p","-vectors.csv")
			   .WriteDerivativesCSV("output/p","-derivs.csv");

	// Obtain the reduced model
	cout << "Computing Reduced Model..." << endl;

	{
	
		RBFFamilyProducer<ThinPlateSpline> producer(options.numRBFs);
		auto reducedModel = producer.BruteForce(reducedData,data.parameterDimension,data.parameters,options.numIterations);
	
		cout << "Total Cost = " << producer.ComputeTotalCost(reducedModel,reducedData,data.parameters) << endl;
	
		reducedModel.WriteCSV("output/reduced1.csv");

		// Generate the data
		cout << "Generating Reduced data..." << endl;
		DataGenerator<RBFFamily<ThinPlateSpline>> rdataGenerator(reducedModel);
		rdataGenerator.MatchSettings(dataGenerator);
		rdataGenerator.tStart = 0.0;

		DataSystem rdata = rdataGenerator.GenerateUsingInitials( parameters, reducedData, options.numThreads );
		rdata.WriteDataSetsCSV("output/rdata1",".csv");

	}

	{
	
		RBFFamilyProducer<PolyharmonicSpline3> producer(options.numRBFs);
		auto reducedModel = producer.BruteForce(reducedData,data.parameterDimension,data.parameters,options.numIterations);
	
		cout << "Total Cost = " << producer.ComputeTotalCost(reducedModel,reducedData,data.parameters) << endl;
	
		reducedModel.WriteCSV("output/reduced2.csv");

		// Generate the data
		cout << "Generating Reduced data..." << endl;
		DataGenerator<RBFFamily<PolyharmonicSpline3>> rdataGenerator(reducedModel);
		rdataGenerator.MatchSettings(dataGenerator);
		rdataGenerator.tStart = 0.0;

		DataSystem rdata = rdataGenerator.GenerateUsingInitials( parameters, reducedData, options.numThreads );
		rdata.WriteDataSetsCSV("output/rdata2",".csv");

	}

	{
	
		RBFFamilyProducer<Multiquadratic> producer(options.numRBFs);
		auto reducedModel = producer.BruteForce(reducedData,data.parameterDimension,data.parameters,options.numIterations);
	
		cout << "Total Cost = " << producer.ComputeTotalCost(reducedModel,reducedData,data.parameters) << endl;
	
		reducedModel.WriteCSV("output/reduced3.csv");

		// Generate the data
		cout << "Generating Reduced data..." << endl;
		DataGenerator<RBFFamily<Multiquadratic>> rdataGenerator(reducedModel);
		rdataGenerator.MatchSettings(dataGenerator);
		rdataGenerator.tStart = 0.0;

		DataSystem rdata = rdataGenerator.GenerateUsingInitials( parameters, reducedData, options.numThreads );
		rdata.WriteDataSetsCSV("output/rdata3",".csv");

	}

	//Compare( reducedData, rdata );

	cout << "Press any key to continue . . . "; cin.get();
}

