#include <conf.h>
#include <stdio.h>
#include <kernel.h>
#include <lab1.h>

double pow(double x, int y) {
	double result = 1.0;
	if (y == 0) {
		return result;
	}
	
	while (y != 0) {
		result = result * x;
		y --;
	}
	return result;
}

double log(double x) {
	double result = 0;
	double term = (double) (x - 1);
	int n = 100;
	double neg_term = 1.0;
	
	int i;
	for (i = 1; i < n; i ++) {
		double val = pow(term, i);
		val *= (double) neg_term;
		val = val / (double) i;
		result = result + val;
		neg_term *= -1;
	}
	return result;
}

double expdev(double lambda) {
	double dummy;
	double RAND_MAX = 077777;
    do {
		dummy = (double) rand() / RAND_MAX;
	} while (dummy == 0.0);
    return -log(dummy) / lambda;
}
