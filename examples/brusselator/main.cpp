#include <iostream>
#include <memory>
#include <DRDSP/data/data_set.h>
#include <DRDSP/data/secants.h>
#include <DRDSP/projection/proj_secant.h>
#include <DRDSP/dynamics/rbf_family_producer.h>
#include <DRDSP/dynamics/data_generator.h>
#include "brusselator.h"

using namespace std;
using namespace DRDSP;

struct Options {
	uint32_t numIterations, maxPoints, targetDimension, numRBFs;

	Options() : numIterations(1000), maxPoints(0), targetDimension(2), numRBFs(30) {}
	
	Options( int argc, char** argv ) : Options() {
		if( argc >= 2 ) targetDimension = (uint32_t)atoi(argv[1]);
		if( argc >= 3 ) numRBFs = (uint32_t)atoi(argv[2]);
		if( argc >= 4 ) maxPoints = (uint32_t)atoi(argv[3]);
		if( argc >= 5 ) numIterations = (uint32_t)atoi(argv[4]);
	}
};

void Compare( const ReducedDataSystem& reducedData, const DataSystem& rdata ) {

	ofstream out("output/comparison.csv");
	out << "Parameter,RMS,Max,MaxMin,Differences" << endl;
	for(uint32_t i=0;i<reducedData.numParameters;++i) {
		DataComparisonResult r = CompareData( reducedData.reducedData[i].points, rdata.dataSets[i].points );
		cout << "Parameter " << rdata.parameters[i] << endl;
		cout << "RMS: " << r.rmsDifference << endl;
		cout << "Max: " << r.maxDifference << endl;
		cout << "MaxMin: " << r.maxMinDifference << endl;

		out << rdata.parameters[i] << ",";
		out << r.rmsDifference << ",";
		out << r.maxDifference << ",";
		out << r.maxMinDifference << ",";
		for( const auto& x : r.differences )
			out << x << ",";
		out << endl;
	}

}

typedef Multiquadratic RadialType;

int main( int argc, char** argv ) {

	Options options(argc,argv);

	// Dynamics
	BrusselatorFamily brusselator;

	auto parameters = ParameterList( 2.1, 2.9, 9 );

	// Generate the data
	cout << "Generating data..." << endl;
	DataGenerator<BrusselatorFamily> dataGenerator;
	dataGenerator.initial.setRandom(brusselator.dimension);
	dataGenerator.tStart = 50;
	dataGenerator.tInterval = 7.2;
	dataGenerator.print = 200;
	dataGenerator.dtMax = 0.001;

	DataSystem data = dataGenerator.GenerateDataSystem( parameters );

	// Pre-compute secants
	vector<SecantsPreComputed> secants( data.numParameters );
	for(uint16_t i=0;i<data.numParameters;++i)
		secants[i].ComputeFromData( data.dataSets[i] );

	// Secant culling
	vector<SecantsPreComputed> newSecants( data.numParameters );
	for(uint16_t i=0;i<data.numParameters;++i)
		newSecants[i] = secants[i].CullSecantsDegrees( 10.0 );

	secants = vector<SecantsPreComputed>();

	// Find a projection
	ProjSecant projSecant;
	projSecant.targetDimension = options.targetDimension;
	projSecant.targetMinProjectedLength = 0.7;

	// Compute initial condition
	projSecant.GetInitial( data );

	// Optimize over Grassmannian
	projSecant.Find( newSecants );

	// Print some statistics
	projSecant.AnalyseSecants( newSecants );

	newSecants = vector<SecantsPreComputed>();

	projSecant.WriteBinary("output/projection.bin");
	projSecant.WriteCSV("output/projection.csv");
	
	// Compute projected data
	cout << endl << "Computing Reduced Data..." << endl;
	ReducedDataSystem reducedData;
	reducedData.ComputeData( brusselator, data, projSecant.W );
	reducedData.WritePointsCSV("output/p","-points.csv");
	reducedData.WriteVectorsCSV("output/p","-vectors.csv");

	// Obtain the reduced model
	cout << endl << "Computing Reduced Model..." << endl;
	RBFFamilyProducer<RadialType> producer(options.numRBFs);
	auto reducedModel = producer.BruteForce(reducedData,data.parameterDimension,data.parameters.data(),options.numIterations);

	cout << "Total Cost = " << producer.ComputeTotalCost(reducedModel,reducedData,data.parameters.data()) << endl;

	reducedModel.WriteCSV("output/reduced.csv");
	
	// Generate the data
	cout << "Simulating the reduced model..." << endl;
	DataGenerator<RBFFamily<RadialType>> rdataGenerator(reducedModel);
	rdataGenerator.MatchSettings(dataGenerator);
	rdataGenerator.tStart = 0.0;
	DataSystem rdata = rdataGenerator.GenerateUsingInitials( parameters, reducedData );
	rdata.WriteDataSetsCSV("output/rdata",".csv");

	Compare( reducedData, rdata );

	system("PAUSE");
}
