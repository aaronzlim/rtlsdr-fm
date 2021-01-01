#include <vector>
#include <complex>

class QuarterRateDecimationFilter {

public:

    /**
     * @brief Construct a new Quarter Rate Decimation Filter object
     *
     * @param num_taps Number of taps used to create the filter.
     * @param cutoff Filter cutoff frequency normalized to the sample rate.
     */
    QuarterRateDecimationFilter(int num_taps, float cutoff);


    /**
     * @brief Construct a new Quarter Rate Decimation Filter object
     *
     * @param coefficients Filter coefficients.
     * @param is_baseband If true, the cofficients will be shifted up to FS/4.
     */
    QuarterRateDecimationFilter(std::vector< std::complex<int32_t> > coefficients, bool is_baseband);


    /**
     * Apply a bandpass filter centered at FS/4 and decimate by 4.
     * This aliases FS/4 to baseband.
     */
    void filter(const std::vector< std::complex<int32_t> >& iq_in,
                std::vector< std::complex<int32_t> >& iq_out);

};