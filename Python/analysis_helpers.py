class Plot:
    '''
    modified version of PlotCsv
    original source code supplied by Leopold Götsch     
    '''

    # fields
    source = ''
    header = ''
    sep = ''

    def __init__(self, source: str, header: int, sep = ',') -> None:
        """
        Class constructor:
        Instantiates an object of this class. 
        """
        self.source = source
        self.header = header
        self.sep = sep

    # methods
    def Bode (
        self, destination: str, mode: str,
        figsize = (9, 6), dpi = 100
        ):
   
        import pandas as pd
        import matplotlib.pyplot as plt

        if (mode == "magnitude"):
                title = 'Magnitude'
                yAxisLabel = 'dB'
                y = 'Channel 2 Magnitude (dB)'
                x = 'Frequency (Hz)'
                xAxisLabel = 'Frequency [Hz]'
                scale = 'log'
        
        if (mode == 'phase'):
            title = 'Phase'
            yAxisLabel = 'Degrees'
            y = 'Channel 2 Phase (deg)'
            x = 'Frequency (Hz)'
            xAxisLabel = 'Frequency [Hz]'
            scale = 'log'  

        dataframe = pd.read_csv(self.source, self.sep, header = self.header)
        
        plt.rcParams["figure.figsize"] = figsize
        plt.rcParams["figure.dpi"] = dpi
        plt.plot(dataframe[x], dataframe[y])
        plt.title(title)
        plt.xlabel(xAxisLabel)
        plt.ylabel(yAxisLabel)
        plt.xscale(scale)        
        plt.savefig(ResultsPath() + destination)
        plt.show()


def ResultsPath():
    '''
    Helper for ipynb files, which cannot access the os module.
    Gets path of the currently executing assembly and creates
    a 'Results' subdirectory, if it doesn't already exists.
    '''
    from os import makedirs
    from os.path import dirname, join, isdir
    script_dir = dirname(__file__)
    results_dir = join(script_dir, 'Results\\')
    if not isdir(results_dir):
        makedirs(results_dir)
    return results_dir

    