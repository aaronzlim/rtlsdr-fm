#!/usr/bin/env python

from argparse import ArgumentParser
from pathlib import Path
from types import MappingProxyType

from matplotlib import pyplot as plt
import numpy as np
from numpy.fft import fft, fftshift, fftfreq

VALID_DATA_TYPES = MappingProxyType({
    'int8': np.int8,
    'int16': np.int16,
    'int32': np.int32,
    'str': None
})

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('file', type=str, help='The binary file to read.')
    parser.add_argument('samplerate', type=float, help='Sample rate of the data contined in <file>.')
    parser.add_argument('--type', '-t', type=str, help='Data type (e.g. int8, int16, etc). Defaults to str.',
                        choices=list(VALID_DATA_TYPES.keys()), default='str')

    args = parser.parse_args()

    filepath = Path(args.file)
    fs = args.samplerate
    data_type = VALID_DATA_TYPES[args.type]

    if not filepath.exists():
        raise IOError(f'No such file or directory {filepath.resolve()}')

    if data_type is None:
        with open(filepath, 'r') as f:
            x = np.array([int(s) for s in f.readlines()])
    else:
        x = np.fromfile(filepath.resolve(), dtype=data_type)
    iq = x[::2] + 1j*x[1::2] # deinterleave IQ data


    IQ = 20 * np.log10(np.abs(fftshift(fft(iq)/len(iq)))) # power spectrum

    t = np.arange(len(iq)) / fs
    f = fftshift(fftfreq(len(iq), d=1/fs))

    _, ax = plt.subplots(2, 1)

    ax[0].plot(t*1e3, iq.real)
    ax[0].plot(t*1e3, iq.imag)
    ax[0].set_title('Time Domain')
    ax[0].set_xlabel('Time (ms)')
    ax[0].set_ylabel('Amplitude')

    ax[1].plot(f/1e3, IQ)
    ax[1].set_title('Power Spectrum')
    ax[1].set_xlabel('Frequency (kHz)')
    ax[1].set_ylabel('Log Mag (dB)')

    plt.tight_layout()
    plt.show()

