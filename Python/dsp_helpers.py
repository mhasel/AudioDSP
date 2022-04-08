# audio DSP education, algorithm development, test and documentation module

def toMono(signal):
    '''
    Summary:
      Combine stereo channels to mono.
    Parameters:
      signal:       - array with discrete stereo samples
    Returns:
      The amplitude adjusted signal with summed channels
    '''
    if (signal.ndim >  1):
        length = signal.shape[0]
        signal = normalize((signal[0:length:1, 0] + signal[0:length:1, 1]))
    return signal


def normalize(ys, amp=0.99):
    """
    Taken from "Think DSP" by Allen B. Downey:
    https://greenteapress.com/wp/think-dsp/
    https://github.com/AllenDowney/ThinkDSPNormalizes 

    Summary:
      Normalize a discrete signal by dividing by scaling each sample
      with the highest or lowest value of the entire waveform.
    Parameters:
      ys:         - wave array
      amp:        - max amplitude (pos or neg) in result
    Returns: 
      a wave array so the maximum amplitude is +amp or -amp.
    """
    high, low = abs(max(ys)), abs(min(ys))
    return amp * (ys / max(high, low))

def time(signal, samplingRate):
    """
    Summary:
      Creates an evenly spaced floating point array containing
      exactly one time-value per sample value in the signal array
    Parameters:
      signal:           - wave array
      samplingRate:     - the amount of samples per second of the signal
    Returns:
      time-base array
    """
    from numpy import linspace
    #get length of signal in seconds by dividing the amount of samples by the sample rate
    signalLength = len(signal) / samplingRate
    # create evenly spaced ndarray
    return linspace(0, signalLength, len(signal))

# functions to convert scales
def dB_to_magnitude(dB):
    return 10 ** (dB / 20)   
def magnitude_to_dB(mag):
    from numpy import log10
    return 20 * log10(mag)  
    
def plot(signal, samplingrate, ax = None, start = None, end = None, **plt_kwargs):
    """  
    https://towardsdatascience.com/creating-custom-plotting-functions-with-matplotlib-1f4b8eba6aa1

    Summary:
      Allows creating a plot of a signal with passed parameters. 
    Parameters:
      signal:           - wave array
      samplingRate:     - the amount of samples per second of the signal    
      title:            - the title of the plot. None if omitted
      ax:               - pre-existing ax (optional) 
      start:            - starting point (optional)  
      end:              - ending point (optional) 
      **plt_kwagrs      - additional plot keywords (optional)
    Returns: 
      figure object of the relative signal amplitude plotted against time.
    """
    import matplotlib.pyplot as plt
    if ax is None:
        ax = plt.gca()

    x = time(signal, samplingrate)
    y = signal

    if start and end is not None:
        ax.plot(x[start:end], y[start:end], **plt_kwargs)
        return ax

    ax.plot(x, y, **plt_kwargs) 
    return ax

def annotate_plot(ax, title, xlabel = "Time, seconds", ylabel = "Signal amplitude, relative units"):
    ax.set(xlabel = xlabel, ylabel = ylabel, title = title)


def triangle(A, N):
    '''
    Summary:
      Create an array with values representing a triangle wave without
      phase shift or 'DC' offset.
    Parameters:
      A:            - amplitude
      N:            - number of samples
    Returns:
      wave array of a triangle wave    
    '''
    from numpy import zeros, abs
    i = 0
    x = zeros(N)
    while (i < N):        
        x[i] = 2 * abs((i % 2 * A) - A) - A
        i += 1
    return x
    
def triangle_alt(t):
    '''
    Summary:
      Create a single sample representing the normalized amplitude
      of a triangle wave at time t. The frequency can be controlled 
      by changing the time increments between samples.
    Parameters:
      t:           - time
    Returns:
      A sample with the normalized amplitude of a triangle signal 
      at a specific time value
    '''
    from numpy import arcsin, cos
    return arcsin(cos(t)) / 1.57079633
    
def sawtooth(A, N):
    '''
    Summary:
      Create an array with values representing a sawtooth wave with amplitude A
      and N samples. No phase shift or 'DC' offset.
    Parameters:
      A         - amplitude
      N         - number of samples
    Returns:
      An array containing N samples, resembling a sawtooth wave.
    '''
    from numpy import zeros
    i = 0
    x = zeros(N)
    while (i < N):
        x[i] = (i % (2 * A + 1)) - A
        i += 1
    return x

def square_wave(t):
    '''
    Summary:
      Return a single sample representing the normalized amplitude
      of a square wave at time t. The frequency can be controlled 
      by changing the time increments between samples.
    Parameters:
      t         - time
    Returns:
      1         if sin(t) is positive
      -1        otherwise
    '''
    from numpy import sin
    x = sin(t)
    if (x >= 0.0):
        return 1
    else:
        return -1

def sine_wave(A, f, Fs, s):
    '''
    Summary:
      Generates a sine wave. 
    Parameters:
      A         - amplitude
      f         - frequency
      Fs        - sampling frequency / sampling rate
      s         - total duration of the returned signal in seconds
    Returns:
      An array containing Fs * s samples, resembling a sine wave.

    '''
    from numpy import pi, linspace, zeros, sin
    w = f / 2 * pi
    t = linspace(0, s, Fs * s)
    x = zeros(len(t))
    x = A * sin(w * t)
    return x

def cos_wave(A, f, Fs, s):
    '''
    Summary:
      Generates a cosine wave. 
    Parameters:
      A         - amplitude
      f         - frequency
      Fs        - sampling frequency / sampling rate
      s         - total duration of the returned signal in seconds
    Returns:
      An array containing Fs * s samples, resembling a cosine wave.

    '''
    from numpy import pi, linspace, zeros, cos
    w = f / 2 * pi
    t = linspace(0, s, Fs * s)
    x = zeros(len(t))
    x = A * cos(w * t)
    return x

def overdrive(signal, threshold = 1/3):
    '''
    Summary:
      Creates a soft-clipped version of the passed wave array.
    Parameters:
      signal:           - the wave array to process
      threshold:        - determines how steep the clipped wave form is. 
                          values exceeding 0.4 will no longer sound ok.
    Returns:
      the modified wave array
    '''
    from numpy import zeros
    N = len(signal)
    y = zeros(N)
    signal = toMono(signal)

    for i in range(1, N):
        if abs(signal[i]) < threshold:
            y[i] = 2 * signal[i]
        if abs(signal[i]) > 2 * threshold:
            if signal[i] > 0:
                y[i] = 1
            if signal[i] < 0:
                y[i] = -1
        elif abs(signal[i]) >= threshold:
            if signal[i] > 0:
                y[i] = (3 - (2 - signal[i] * 3) **2) / 3
            if signal[i] < 0:
                y[i] = - (3 - (2 - abs(signal[i]) * 3) ** 2) / 3
        
    return y

def fuzz(signal, gain = 11, mix = 0.2):
    from numpy import abs, exp, sign
    '''
    Summary:
      Creates a distorted version of the passed wave array.
    Parameters:
      signal:       - the wave array to apply the effect to
      gain:         - determines the clipping range. higher values result in
                      steeper wave forms.
      mix:          - the ratio of the wet signal to dry signal. a value of 1 will
                      return only the distorted part.
    Returns:
      the modified wave array
    '''
    
    signal = toMono(signal)
    # downscale samples and multiply with gain value
    q = signal * gain / max(abs(signal))    
	# invert sign and multiply with e-function
    z = sign(-q) * (1 - exp(sign(-q)*q))
	#  blend dry and upscaled wet signal
    return mix * z * max(abs(signal))/max(abs(z))+(1-mix)*signal

def tremolo(signal, rate = 0.5, depth = 0.5):
    '''
    Algorithm translated to python from:
    https://wiki.analog.com/resources/tools-software/sharc-audio-module/baremetal/tremelo-effect-tutorial
    
    Summary:
      Amplitude modulation with low-frequency carrier wave. Results in iconic, wobbly sounding audio
    Parameters:
      signal:       - the wave array to apply the effect to
      rate:         - rate of change. this factors into the modulating signal's frequency
      depth:        - modulation depth. 
    Returns:
      the modified wave array
    '''

    from numpy import zeros, pi, sin
    assert (rate >= 0) and (rate <= 1) and (depth >= 0) and (depth <= 1) 
    
    signal = toMono(signal)

    t = 0.0
    N = len(signal)
    out = zeros(N)
    for i in range(N):
        # calculate modulation factor for this sample
        factor = 1 - (depth * 0.5 * sin(t) + 0.5)
        t += rate * 0.002
        if (t > 2 * pi):
            t -= 2 * pi
        out[i] = factor * signal[i]
    return out

def ring_modulator(signal, modulator, rate = 0.5, blend = 0.5):
    '''
    Algorithm translated to python from:
    https://wiki.analog.com/resources/tools-software/sharc-audio-module/baremetal/ring-modulator-effect-tutorial
    
    Summary:
      DSB-SC modulation with low-frequency carrier wave. Can be hard to find settings that sound well.
    Parameters:
      signal:       - the wave array to apply the effect to.
      modulator:    - the type of modulating wave form.
      rate:         - rate of change. this factors into the modulating signal's frequency.
      blend:        - the ratio of modulated signal to unmodulated signal. 
    Returns:
      the modified wave array
    '''

    from numpy import zeros, pi, sin
    assert (rate >= 0) and (rate <= 1) and (blend >= 0) and (blend <= 1) and (modulator >= 0) and (modulator < 3)
  
    t = 0.0
    N = len(signal)
    out = zeros(N)    

    signal = toMono(signal)

    for i in range(N):       
        
        if (modulator == 0):
            factor = sin(t)
        elif (modulator == 1):
            factor = triangle_alt(t)
        elif (modulator == 2):
            factor = square_wave(t)

        t += rate * 0.02        

        if (t > 2 * pi):
            t -= 2 * pi
        
        out[i] = (1 - blend) * signal[i] + blend * factor * signal[i]
    
    return out

def FIR_delay(signal, delayMilliseconds, delayAmplitude, Fs = 48000):
    '''
    Summary:
      Applies a single echo to the signal by convoluting it with a simple
      impulse response. Suitable for offline processing.
    Parameters:
      signal:               - the wave array to apply the effect to.
      delayMilliseconds:    - the echo delay in milliseconds
      delayAmplitude:       - the echo amplitude
      Fs:                   - sampling frequency/sampling rate
    Returns:
      the modified wave array        
    '''
    from numpy import zeros, convolve
    # Delay in seconds * sample rate = delay in samples
    delay = delayMilliseconds / 1000    
    delayInSamples = round(delay * Fs)
    
    signal = toMono(signal)

    # Create simple delay impulse response. Sample value is relative amplitude.
    # First sample is 1, last sample equals the delay amplitude, all samples in between are 0.
    # This results in an echo with scaled amplitude on the out-signal after the set delay time
    impulseResponse = zeros(delayInSamples)
    impulseResponse[0] = 1
    impulseResponse[-1] = delayAmplitude

    # Returns the discrete, linear convolution of two one-dimensional sequences.
    # https://numpy.org/doc/stable/reference/generated/numpy.convolve.html
    return convolve(signal, impulseResponse)

def Reverb(ir_samplingRate, impulseResponse, signal, Fs):
    '''
    Summary:
      Creates a reverberation effect by convoluting the signal with an impulse response.
      Sampling rate of impulse response and dry signal should match, or at least be close.
      The resulting sound depends solely on the impulse response passed into the function.
    Parameters:
      ir_samplingRate:             - sampling rate of the impulse response
      impulseResponse:             - impulse response wave array
      signal:                      - dry signal wave array
      Fs:                          - sampling frequency/rate
    Returns:
      the modified wave array          
    '''
    from numpy import concatenate, zeros
    from scipy.signal import fftconvolve

    impulseResponse = toMono(impulseResponse)
    assert impulseResponse.ndim == 1

    # lighten the computational load by excluding all 0 values from further calculation
    impulseResponse = impulseResponse[impulseResponse != 0]

    signal = toMono(signal)
    assert signal.ndim == 1

    # apply convolution, using the tubby impulse response. scipy.signal allows applying convolution in the frequency domain
    # audio that has an effect applied to it is also called a "wet signal" (as opposed to a clean (dry) signal)
    wetSignal = fftconvolve(signal, impulseResponse)
    # normalize the signal (avoid distortion)
    wetSignal = normalize(wetSignal)

    # the wet signal is longer than the original signal. the original signal needs to be padded to be able to combine them
    assert len(wetSignal) > len((signal))
    # add some 0 to the end of the input signal to make up the difference
    drySignal = concatenate((signal, zeros(len(wetSignal) - len(signal))))
    assert len(wetSignal) == len(drySignal)
    # normalize dry signal
    drySignal = normalize(drySignal)

    # combine dry signal with the wet signal. lower the magnitude of the wet signal by 6dB (approx. half the volume)
    outSignal = drySignal + dB_to_magnitude(-3) * wetSignal
    # normalize again to avoid clipping
    return normalize(outSignal)