library(grid)
library(scales)
suppressMessages (library(ggplot2))
suppressMessages (library(plyr))
source ("graph-style.R")

output1 = 'delay-histogram-52.pdf'

numClients = 52

seqnos.file <- paste(sep='', 'delay-histogram/seqnos-numClients-',numClients,'.txt')
seqnos <- read.table (seqnos.file, sep="\t",
                      col.names=c("Run", "Type", "Loss", "RandomLoss", "Time", "Node1", "Node2", "SeqNo"))

levels(seqnos$Type)[levels(seqnos$Type)=="IP"] = "IRC"
levels(seqnos$Type)[levels(seqnos$Type)=="NDN"] = "Chronos"

runs = unique (seqnos$Run)

for (run in runs) {
  input.generate <- paste(sep='', '../run-input/run-',run,'-numClients-',numClients,'.txt');
  data.generate <- read.table (input.generate, skip=numClients+2, col.names=c("Node2", "TimeReal", "Size"))
  data.generate$SeqNo = -1

  ### Assign sequence numbers to input
  sec <- vector()
  sec[1:52] = -1
  for (i in c(1:length(data.generate$Node2))) {
    sec [data.generate$Node2[i] + 1] <- sec [data.generate$Node2[i] + 1] + 1
    
    data.generate$SeqNo[ i ] = sec [data.generate$Node2[i] + 1]
  }
  data.generate$Node2 = factor(data.generate$Node2)
  data.generate$Run = run
  
  if (run==runs[1]) {
    delays = data.generate
  }
  else {
    delays = rbind (delays, data.generate)
  } 
}

seqnos$Run = factor(seqnos$Run)
seqnos$SeqNo = factor(seqnos$SeqNo)
seqnos$Node1 = factor(seqnos$Node1)
seqnos$Node2 = factor(seqnos$Node2)

delays$Run = factor(delays$Run)
delays$SeqNo = factor(delays$SeqNo)

seqnos.with.delays <- join(seqnos, delays) #, by=c("Node2", "SeqNo", "Run"))
seqnos.with.delays$Delay = abs(seqnos.with.delays$Time - seqnos.with.delays$TimeReal)

seqnos.with.delays = subset (seqnos.with.delays, Node1 != Node2)

fmt <- function(x, ...) {
  paste (x*1000, "ms", sep='')
}

s = 1000 * 51 * 20

fmt2 <- function(x, ...) {
  degree = trunc (log(x, 10))
  print (x)
  ## x = x / 10^degree
  ## ## 
  ## parse(text = bquote(paste("xxx", 10^3)))
  out <- list()
  length(out) = length(x)

  for (i in c(1:length(x))) {
    if (is.na(x[i])) {
      out[[i]] = NA
    }
    else
      {
        ## out[[i]] = degree[i]
        if (x[i] == 0 | degree[i] == 0) {
          out[[i]] = paste(x[i], " ")
        } else {
          val = x[i] / 1000
          out[[i]] = paste(sep="", val, "k\n(", round(100*x[i]/s, digits=-1), "%)")
          ## val = x[i] / 10^degree[i]
          ## deg = degree[i]
          ## out[[i]] = substitute(x %.% 10^y, list(x=val, y=deg))
        }
      }
  }
  return (out)
}


g1 <- ggplot (seqnos.with.delays) +
  geom_histogram (aes(x=Delay, y=..count.., fill=Type), size=0.2, colour="black", binwidth=0.02, position="dodge") +
  ## geom_line (aes(x=Links, y=Mean, colour=Type)) +
  ## geom_errorbar (aes(x=Links, ymin=Ymin, ymax=Ymax), width=0.5, size=0.2, colour="gray60") +
  scale_y_continuous ("Number of message\ndeliveries (20 runs)", labels=fmt2 ) +
  scale_x_continuous ("Delay of message delivery",
                      labels=fmt
                      , breaks=seq(0,0.25, 0.04),
                      minor_breaks=seq(0,0.25,0.02),
                      limits=c(-0.001,0.25)
                      ) +
  ## scale_colour_manual (values=c("black", "red")) +
  scale_fill_brewer (palette="Spectral") +
  theme_custom () +
  opts (legend.position =c(1, 1),
        legend.direction="vertical",
        legend.justification=c(0.9,0.9),
        legend.background = theme_rect(fill="white", colour="black", size=0.1),
        plot.margin = unit(c(0.1, 0.1, 0, 0), "lines"),
        legend.title = theme_blank()
        )

cat ("Writing", output1, "\n")
pdf (output1, width=3.4, height=2)
g1
x = dev.off ()
