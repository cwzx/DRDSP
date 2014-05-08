DRDSP
=====

Dimensionality Reduction for Dynamical Systems with Parameters

Introduction
------------

DRDSP is a C++ library for producing a low-dimensional dynamical system that models the long-term behaviour (attractors) of a given high-dimensional system. Systems with parameter spaces -- resulting in families of attractors -- can also be reduced.

The steps involved in the method are:

1. Generate some data from the attractors.
2. Compute the secants from the data sets.
3. Cull the secants (optional optimization).
4. Use the secants to find a projection.
5. Use the projection, the data, and the original model to compute some reduced data.
6. Use the reduced data to obtain the reduced model.


Extra Features
--------------

* DataGenerator -- Performs simulations for multiple parameter values. A custom integrator may be specified (RK4 by default).

* BifurcationDiagramGenerator -- Performs simulations for multiple parameter values and samples values to be plotted on a bifurcation diagram. The sampling condition and value functions must be specified. A custom integrator may also be specified (RK4 by default).


Usage
-----

For more detailed reference see `docs/`.

In order to use the library with your example system, you need to implement the dynamics of the system.

* Implement your model

A model is essentially a vector field.

A base class `Model` is provided in `DRDSP/dynamics/model.h`. Inherit from this and pass it the dimension of your state space in the constructor.

```cpp
struct ExampleModel : Model<> {
	double a, b, c;             // parameters and other data used to specify the model

	ExampleModel() :
		Model<>(10),            // set the dimension of the state space here
		a(1), b(1), c(1) {}     // initialize parameters to default values etc.
	
	// the vector field
	VectorXd operator()( const VectorXd& state ) const;
	
	// the partial derivatives of the vector field
	MatrixXd Partials( const VectorXd& state ) const;
};
```

* Implement a family

A family is a function object that takes a parameter and returns a model. This is where you choose which parameter(s) you're interested in investigating.

A base class `Family` is provided in `DRDSP/dynamics/model.h`. Inherit from this and pass it the dimension of your state space and parameter space in the constructor.

```cpp
struct ExampleFamily : Family<ExampleModel> {

	ExampleFamily() :
		Family<ExampleModel>(10,1)       // set the dimension of the state space and parameter space
	{}
	
	ExampleModel operator()( const VectorXd& parameter ) const {
		ExampleModel model;
		model.c = parameter[0];         // in this family we are investigating the `c` parameter.
		return model;
	}
};
```

### Optional -- Non-Euclidean state spaces

If the state space of your model is something other than R^n, you must embed the state space into R^n. Refer to the pendulum example to see this in action.

* Implement an embedding

```cpp
struct ExampleEmbedding : Embedding {

	ExampleEmbedding() :
		Embedding(10,12)       // the dimension of the state space and embedding space
	{}
	
	// Evaluate the embedding.
	VectorXd operator()( const VectorXd& state ) const;
	
	// The partial derivatives of the embedding.
	MatrixXd Derivative( const VectorXd& state ) const;
	
	// The adjoint of the derivative. If the coordinate bases are orthonormal this will be the matrix transpose of the derivative.
	MatrixXd DerivativeAdjoint( const VectorXd& state ) const;
	
	// The second partial derivatives of the ith component of the embedding.
	MatrixXd Derivative2( const VectorXd& state, uint32_t i ) const;
	
};
```

To use an embedding with the underlying model/family you can use these as your model/family: `ModelEmbedded<ExampleModel,ExampleEmbedding>` and `FamilyEmbedded<ExampleFamily,ExampleEmbedding>`.

* Implement a wrap function

A wrap function is applied after every time-step to modify the state vector. This allows you to keep angular variables in the range [-pi,pi) (for example).

```cpp
struct ExampleWrap {
	void operator()( VectorXd& x ) const {
		Wrap(x[3],-M_PI,M_PI);
	}
};
```

In order to use a custom wrap function with the provided RK4 integrator, you need to make a custom version of solver:

```cpp
typedef RKDynamicalSystem<ExampleModel,ExampleWrap> ExampleSolver;
```


### Data sets

The method requires data samples from the attractors of the original model. These data sets can either be loaded in from a file, or generated at run-time.

To generate a data set, the `DataGenerator` class will perform a simulation of the original model for multiple parameter values.

```cpp
auto parameters = ParameterList( 1.0, 2.0, 11 );   // Set of 11 parameter values in the range [1,2] (uniformly spaced)

DataGenerator<ExampleFamily> dataGenerator;
dataGenerator.initial.setRandom( 10 );   // Set the initial condition for the simulations
dataGenerator.tStart = 1000;             // Start recording data at this time (allow for transients to decay)
dataGenerator.tInterval = 100;           // Time interval over which to record data
dataGenerator.dtMax = 0.001;             // Maximum time-step to use in the simulation
dataGenerator.print = 1000;              // Number of data samples to take over this interval (uniformly spaced)

// Generate data for the given parameter values using 4 threads.
DataSystem data = dataGenerator.GenerateDataSystem( parameters, 4 ); 
```

To use a custom solver, use `DataGenerator<ExampleFamily,ExampleSolver>`.


### Secant-based Projection

Once data has been obtained, the secants are computed using the `Secants` class.

Secants can be culled using one of the `Secants::CullSecants*` methods. Culling reduces the total number of secants in order to lower the computational cost of finding a projection. This is an optional, but recommended, step.

The `ProjSecant` class determines a projection from a set of secants. The projection is encoded in an orthonormal matrix, `ProjSecant::W`.

### Computing Reduced Data

`ReducedData` and `ReducedDataSystem` classes take the original model, the data sets, and an orthonormal matrix and compute the data required to find the reduced model.

### Find Reduced Model

The `RBFModelProducer` class takes a `ReducedData` and produces a `BRFModel`.
The `RBFFamilyProducer` class takes a `ReducedDataSystem` and produces a `RBFFamily`.

Examples
--------

Examples are provided in `examples/`.

* `examples/rossler/` -- The Rossler system.
* `examples/pendulum/` -- A double pendulum with friction and external forcing.
* `examples/brusselator/` -- The Brusselator PDE on a 2D physical space with a 64x64 discretization.
* `examples/kuramoto/` -- The Kuramoto model. Coupled oscillators with external forcing.

Build
-----

The `include` directory contains the header files that should be copied to your include path.

The `build` directory contains project files for Visual Studio 2013, which will build a static library to `lib/` and executables for the examples to `bin/`. The static library should be linked in your build process.

At the moment there are no build files for other compilers/platforms. You will find the source for the library in `src/` if you wish to make your own.

### Dependencies and Requirements

DRDSP uses Eigen, a C++ template library for linear algebra. This is included in the repository.

A number of C++11 features are used by the library, and maybe one or two C++14 features.



