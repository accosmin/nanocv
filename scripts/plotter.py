import os
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages

def get_csv(spath):
        # state file with the following format:
        #  (epoch, {train, valid, test} x {criterion, loss{average, variance, maximum}, error{average, variance, maximum}}, time)+
        name = os.path.basename(spath).replace(".state", "")
        name = name.replace(name[0 : name.find("_") + 1], "")
        data = mlab.csv2rec(spath, delimiter = ' ', names = None)
        return name, data

def get_csvs(spaths):
        datas = []
        names = []
        for spath in spaths:
                name, data = get_csv(spath)
                datas.append(data)
                names.append(name)
        return names, datas

def plot_one(spath, ppath):
        title, data = get_csv(spath)
        with PdfPages(ppath) as pdf:
                for col in (0, 1, 4):
                        # x axis - epoch/iteration index
                        xname = data.dtype.names[0]
                        xlabel = xname
                        # y axis - train/validation/test datasets
                        yname0 = data.dtype.names[col + 1]
                        yname1 = data.dtype.names[col + 8]
                        yname2 = data.dtype.names[col + 15]
                        ylabel = yname0.replace("train_", "")
                        # plot
                        plt.xlabel(xlabel, fontsize = "smaller")
                        plt.ylabel(ylabel, fontsize = "smaller")
                        plt.title(title, weight = "bold")
                        plt.plot(data[xname], data[yname0], "r--", label = yname0)
                        plt.plot(data[xname], data[yname1], "g:", label = yname1)
                        plt.plot(data[xname], data[yname2], "b-", label = yname2)
                        plt.legend(fontsize = "smaller")
                        pdf.savefig()
                        plt.close()

def plot_many_wrt(spaths, names, datas, pdf, xcol, ycol):
        colnames = datas[0].dtype.names
        title = colnames[ycol + 1]
        # x axis - epoch/iteration index
        xname = colnames[xcol]
        xlabel = xname
        # y axis - train/validation/test datasets
        yname = colnames[ycol + 1]
        ylabel = yname.replace("train_", "").replace("valid_", "").replace("test_", "")
        # plot
        plt.xlabel(xlabel, fontsize = "smaller")
        plt.ylabel(ylabel, fontsize = "smaller")
        plt.title(title, weight = "bold")
        for i, spath in enumerate(spaths):
                plt.plot(datas[i][xname], datas[i][yname], label = names[i])
        plt.legend(fontsize = "smaller")
        pdf.savefig()
        plt.close()

def plot_many(spaths, ppath):
        names, datas = get_csvs(spaths)
        with PdfPages(ppath) as pdf:
                for col in (0, 1, 4, 7, 8, 11, 14, 15, 18, 22):
                        # plot wrt epoch/iteration number
                        plot_many_wrt(spaths, names, datas, pdf, 0, col)
                        # plot wrt time
                        plot_many_wrt(spaths, names, datas, pdf, 22, col)