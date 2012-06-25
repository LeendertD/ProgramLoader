#!/usr/bin/env python
import os
from pylab import plot,figure,subplot,savefig,log,legend

#
# Leendert van Duijn
#
# Script for turning logfiles into graphs
#

def ops(fname):
  """ Opens a file, reads array of strings, closes the file, returns the strings"""
  fil = open(fname,"r")
  strs = fil.readlines()
  fil.close()
  return strs

def grepun(stri, strm):
  """ Filter, returns if not found: -1 """
  try:
    return stri.index(strm)
  except:
    return -1

def grep(starr, strm):
  """ Filters an array, omitting any non matching entries """
  a = list()
  for b in starr:
    if -1 < grepun(b, strm):
      a.append(b)
  return a

def converty(strd):
  """ Turns the string form '<Clocks>?,?,?...</Clocks>' into an array of floats """
  start = strd.index("<Clocks>") + len("<Clocks>")
  end = strd.index("</Clocks>")
  strp = strd[start:end]
  #print(strp)
  arr = strp.split(",")
  darr = map(lambda x:float(x), arr)
  #darr = map(lambda x:0.000000000000001+float(x), arr)
  return darr

def perent(arr):
  """ For all entries in the input, turn them into float array form """
  res = list()
  #print(arr)
  if len(arr) > 0:
    for a in range(0,len(arr[0])):
      res.append(map(lambda x:x[a], arr))
    return res

#Prepare a legend, for graphical output
i_plotnum = 1
leg = (
"Pid",
"Core",
"CoreCount",
"CreateTick",
"TicksToDetach",
"TicksToEnd",
"TicksToCleaned"
)
def plotmad(arr_fn):
  """
  Plot an image,
  feed this an array of filenames (sans extension, relative to the ./logs/ directory),
  it then generates an output filename based on those filenames,
  it of course opens and plots the timing contentsof those files,
  using the png file format
  """
  #Ensure a new figure is made
  global i_plotnum
  fname = ""
  figure(i_plotnum, figsize=(15,15))
  i_plotnum += 1

  #Prepare the to plot data
  arr_resp = list()
  for fn in arr_fn:
    fname += fn
    data = ops("./logs/"+ fn + ".log")
    filt = grep(data, "Clock")
    if (len(filt) < 2):
      return
    res = map(converty, filt)
    resp = perent(res)
    arr_resp.append((fn,resp))

  #Prepare the line styles
  fig = list()
  fig.append("x-")
  fig.append("o-")
  fig.append("+-")

  #For all datasets, plot all graphs, including a legend
  int_fign = 0
  for resp_ in arr_resp:
    fn, resp = resp_
    ll = len(resp)
    for a in range(ll):
      subplot(4,2,ll-a)
      plot(resp[a],fig[int_fign], label=fn)
      #plot(resp[a][2:-1],"x-")
      #plot(log(resp[a]))
      #legend(frameon=False,shadow=False,title=leg[a], loc="upper center")
      legend(frameon=False,shadow=False,title=leg[a], loc="best")
    int_fign += 1

  #If any data was plotted, store the result
  if 0 < len(arr_resp):
    savefig("imgs/" + fname + ".png")

#Run on several input files, and combinations
plotmad(["nulls"])
plotmad(["nulls_noL2"])
plotmad(["nulls_noL2","nulls"])
plotmad(["bulksec"])
plotmad(["bulksec_noL2"])
plotmad(["bulksec_noL2", "bulksec"])
plotmad(["bulktest"])
plotmad(["cory"])
plotmad(["funn"])
plotmad(["printy"])
plotmad(["shared"])
plotmad(["sparmy"])
plotmad(["sparmy_noL2", "sparmy"])
plotmad(["sparmy_noL2"])
plotmad(["spawny"])
plotmad(["tiny"])
plotmad(["tshared"])

