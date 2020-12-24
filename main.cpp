#include <iostream>
#include <vector>
#include <complex>
#include <rtl-sdr.h>

using namespace std;

uint32_t FS_AUDIO = 44100;
uint32_t FS = FS_AUDIO * 4 * 6;
const int NUM_FM_CHANNELS = 101;

vector<int> get_valid_channels() {
    vector<int> valid_channels(NUM_FM_CHANNELS);
    for (int i = 0; i < NUM_FM_CHANNELS; i++) {
        valid_channels[i] = 87900000 + 200000*i;
    }
    return valid_channels;
}


vector<int> get_gains(rtlsdr_dev* sdr) {
    /*Return valid gain values for the given RTL-SDR device.

    Args:
        sdr (rtlsdr_dev*): RTL-SDR device.

    Returns:
        vector<int>: Valid gains in tenths of a dB.
    */
    int num_gains = rtlsdr_get_tuner_gains(sdr, NULL);
    if (num_gains <= 0) {
        return vector<int> ();
    }
    vector<int> gains(num_gains);
    if (rtlsdr_get_tuner_gains(sdr, gains.data()) != num_gains)
        return std::vector<int>();

    return gains;
}


int configure(rtlsdr_dev* sdr, float channel_MHz, string gain_mode, float gain, int agc_en) {
    /*Configure the RTL-SDR frequency and gain. All other parameters are fixed.

    Args:
        sdr (rtlsdr_dev*): RTL-SDR device.
        channel_MHz (uint32_t): Frequency channel to listen to in MHz (e.g. 88.3)
        gain_mode (string): "auto" or "manual".
        gain (uint32_t): Gain in dB. Only set if gain mode is manual.
        agc_en (int): Enable automatic gain control.

    Returns:
        int: 0 if configuration succeeded, otherwise -1.
    */

    int r;
    uint32_t freq = (uint32_t) (channel_MHz * 1000000); // MHz -> Hz
    uint32_t bw = (FS / 10) * 8; // BW in Hz = 80% of sample rate
    uint32_t gain_01dB = (uint32_t) (gain * 10); // Convert to units of tenths of a dB

    r = rtlsdr_set_sample_rate(sdr, FS);
    if (r < 0) {
        cout << "Failed to set sample rate." << endl;
        return -1;
    }

    r = rtlsdr_set_tuner_bandwidth(sdr, bw);
    if (r < 0) {
        cout << "Failed to set tuner bandwidth." << endl;
        return -1;
    }

    // Check that the channel is valid
    vector<int> valid_channels = get_valid_channels();
    if (!count(valid_channels.begin(), valid_channels.end(), freq)) {
        cout << "Channel " << channel_MHz << " MHz is invalid. ";
        cout << "Valid FM channels in the US range from 87.9 MHz to 107.9 MHz.";
        return -1;
    }
    r = rtlsdr_set_center_freq(sdr, freq - FS/4);
    if (r < 0) {
        cout << "Failed to set center frequency." << endl;
        return -1;
    }

    if (gain_mode == "manual") {
        r = rtlsdr_set_tuner_gain_mode(sdr, 1); // 1 = "manual"
        if (r < 0) {
            cout << "Failed to set gain mode to manual." << endl;
            return -1;
        }
        // Check the given gain value is valid
        vector<int> gains = get_gains(sdr);
        if (!count(gains.begin(), gains.end(), gain_01dB)) {
            cout << "Gain " << gain << " dB is invalid. " << endl;
            cout << "Valid gains:" << endl;
            for (int g: gains) {
                cout << g * 0.1 << " ";
            }
            cout << endl;
            return -1;
        }
        r = rtlsdr_set_tuner_gain(sdr, gain_01dB);
        if (r < 0) {
            cout << "Failed to set tuner gain to ." << gain << endl;
            return -1;
        }
    } else {
        r = rtlsdr_set_tuner_gain_mode(sdr, 0); // 0 = "auto"
        if (r < 0) {
            cout << "Failed to set tuner gain mode to auto." << endl;
            return -1;
        }
    }

    r = rtlsdr_set_agc_mode(sdr, agc_en);
    if (r < 0) {
        cout << "Failed to set AGC mode." << endl;
        return -1;
    }

    r = rtlsdr_reset_buffer(sdr); // Reset buffer to prepare for streaming
    if (r < 0) {
        cout << "Failed to reset buffer." << endl;
        return -1;
    }
    return 0;
}


int main(int argc, char** argv) {

    // Define some params that will eventually be passed as CLI args
    float channel = 88.3;
    string tuner_gain_mode = "auto";
    float gain = 0.0;
    int agc_en = 0;
    uint32_t device_index = 0;
    uint32_t block_size = 512;
    uint32_t buffer_size = 2*block_size; // Buffer holds 2 pieces of data (I/Q) for every sample

    // Define variables used to configure and read from the RTL-SDR
    int r; // Used for results of rtlsdr functions.
    int n_read; // Tracks the number of samples read from rtlsdr_read_sync(...)
    vector<int8_t> buf(buffer_size); // Buffer for samples directly from RTL-SDR
    vector< complex<int32_t> > iq(block_size); // Buffer for complex data

    // Make sure a device is plugged in
    uint32_t dev_cnt = rtlsdr_get_device_count();
    if (device_index >= dev_cnt) {
        cout << "No devices found." << endl;
        exit(0);
    }

    rtlsdr_dev* sdr; // Initialize an rtlsdr device

    r = rtlsdr_open(&sdr, device_index);

    if (r < 0) {
        cout << "Failed to open RTL-SDR device." << endl;
        exit(0);
    }

    char manufacturer[256], product[256], serial[256];
    rtlsdr_get_usb_strings(sdr, manufacturer, product, serial);

    cout << "Opened " << manufacturer << " " << product;
    cout << " (Serial " << serial << ")" << endl;

    r = configure(sdr, channel, tuner_gain_mode, gain, agc_en);
    if (r < 0) {
        cout << "Failed to configure RTL-SDR." << endl;
        exit(0);
    }

    r = rtlsdr_read_sync(sdr, buf.data(), buffer_size, &n_read);
    if (r < 0) {
        cout << "Failed to read from RTL-SDR." << endl;
        cout << "rtlsdr_read_sync() returned " << r << "." << endl;
    } else if (n_read != buffer_size) {
        cout << "Underflow: Read " << n_read << " of ";
        cout << buffer_size << " samples." << endl;
        }

    for (int i=0; i < iq.size(); i++) {
        iq[i] = complex<int32_t>(buf[2*i], buf[2*i+1]); // Deinterleave IQ data
        cout << iq[i] << endl; // DEBUG
    }

    if (sdr) {
        rtlsdr_close(sdr);
    }
}
