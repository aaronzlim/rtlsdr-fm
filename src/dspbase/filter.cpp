#include <iostream>

#include "filter.hpp"

using namespace std;

QuarterRateDecimationFilter::QuarterRateDecimationFilter(int num_taps, float cutoff){

}


QuarterRateDecimationFilter::QuarterRateDecimationFilter(vector< complex<int32_t> > coefficients,
                                                         bool is_baseband){

}


void QuarterRateDecimationFilter::filter(const vector< complex<int32_t> >& iq_in,
                                    vector< complex<int32_t> >& iq_out){

}