#include "memibot.h"
#include <valarray>
/*
Net(
(linear1) : Linear(in_features = 9, out_features = 4, bias = True)
(linear2) : Linear(in_features = 4, out_features = 4, bias = True)
(linear3) : Linear(in_features = 4, out_features = 1, bias = True)
)
last_loop = 1188
sum = 22510 / 25000
percentage = 0.9004
sum = 4502 / 5000
percentage = 0.9004
sum = 4123 / 4480
percentage = 0.9203125
linear1.weight
tensor([[-6.8270e-01, -5.0403e-01, -9.6975e-01, -1.8453e-01, -5.0873e-01,
	-6.7935e-01, 1.5981e+00, -1.1815e-01, 4.4695e-01],
	[-6.9925e-43, -6.9925e-43, 1.2051e-43, -6.9925e-43, -1.4574e-43,
	8.2677e-44, 3.6574e-43, -6.9925e-43, -6.9925e-43],
	[4.6074e-01, 7.0527e-01, 2.1650e+00, 2.6704e-01, 8.2598e-01,
	1.2211e+00, 1.5166e+00, -7.3566e-01, -2.2526e+00],
	[-6.8854e-01, -7.8955e-01, -2.3101e+00, 5.8044e-02, -7.6844e-01,
	-1.2742e+00, -3.6821e+00, 6.7418e-01, 2.2733e+00]])
	linear1.bias
	tensor([-1.1911e-01, 2.3822e-43, 8.7253e-01, 4.2953e-01])
	linear2.weight
	tensor([[9.1025e-01, -4.0217e-43, 5.3139e-01, -6.3018e-01],
		[9.6690e-44, -5.3670e-43, -6.9925e-43, -6.9925e-43],
		[-6.9925e-43, 6.9925e-43, -6.9925e-43, -6.9925e-43],
		[6.7262e-44, -6.9925e-43, -6.9785e-43, -5.2969e-43]])
	linear2.bias
	tensor([1.7449e+00, -6.9925e-43, -6.9925e-43, -6.2218e-43])
	linear3.weight
	tensor([[1.3356e+00, 6.5721e-43, -6.9925e-43, 3.6013e-43]])
	linear3.bias
	tensor([-1.5990])
*/

static const inline double relu(double x) {
	return std::max(x, 0.0);
}

static const inline double sigmoid(double x) {
	return std::exp(x) / (1 + std::exp(x));
}

// 1에 가까울수록 패배, 0에 가까울수록 승리.
float MEMIBot::PredictWinrate(int stalker, int immortal, int marine, int marauder, int siegetank, int medivac, int viking, int cyclone, int battlecruiser) {

	//l1
	const std::valarray<double> l1r1({ -0.6827, -0.50403, -0.96975, -0.18453, -0.50873, -0.67935, 1.5981, -0.11815, 0.44695 });
	const std::valarray<double> l1r2({ 0.46074, 0.70527, 2.165, 0.26704, 0.82598, 1.2211, 1.5166, -0.73566, -2.2526 });
	const std::valarray<double> l1r3({ -0.68854, -0.78955, -2.3101, 0.058044, -0.76844, -1.2742, -3.6821, 0.67418, 2.2733 });

	const double l1b1 = -0.11911;
	const double l1b2 = 0.87253;
	const double l1b3 = 0.42953;

	//l2
	const std::valarray<double> l2r1 ({ 0.91025, 0.53139, -0.63018 });

	const double l2b1 = 1.7449;

	//l3
	const double l3r1 = 1.3356;

	const double l3b1 = -1.599;


	std::valarray<double> i1c1(9);
	std::valarray<double> i2c1(3);
	double i3c1;
	double output;
	i1c1[0] = marine;
	i1c1[1] = marauder;
	i1c1[2] = siegetank;
	i1c1[3] = medivac;
	i1c1[4] = viking;
	i1c1[5] = cyclone;
	i1c1[6] = battlecruiser;
	i1c1[7] = stalker;
	i1c1[8] = immortal;

	i2c1[0] = relu((l1r1 * i1c1).sum() + l1b1);
	i2c1[1] = relu((l1r2 * i1c1).sum() + l1b2);
	i2c1[2] = relu((l1r3 * i1c1).sum() + l1b3);

	i3c1 = relu((l2r1 * i2c1).sum() + l2b1);
	output = sigmoid(l3r1 * i3c1 + l3b1);

	return (float)output;
}

