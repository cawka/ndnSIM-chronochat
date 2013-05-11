library(grid)
library(scales)
suppressMessages (library(ggplot2))
suppressMessages (library(plyr))
source ("graph-style.R")

output1 = 'delay-cdf.pdf'

## numClients = 26
numClients = 52
recalculate = FALSE
## recalculate = TRUE
folder = "losses-adaptive-retransmission"
## folder = "losses-adaptive-no-single"
## folder = "losses-no-single-reduced"
## folder = "losses-vs-retx"

if (recalculate) {
  
  seqnos.file <- paste(sep='', folder, '/seqnos-numClients-',numClients,'.txt')
  seqnos <- read.table (seqnos.file, sep="\t",
                        col.names=c("Run", "Type", "Loss", "RandomLoss", "Time", "Node1", "Node2", "SeqNo"))
  
  ## seqnos = subset(seqnos, RandomLoss < 0.1)
  seqnos$RandomLoss = factor(seqnos$RandomLoss, levels=c(0, 0.01, 0.05, 0.1), ordered=TRUE)
  
  
  levels(seqnos$Type)[levels(seqnos$Type)=="IP"] = "IRC"
  levels(seqnos$Type)[levels(seqnos$Type)=="NDN"] = "Chronos"
  
  seqnos$Run = factor(seqnos$Run)
  seqnos$SeqNo = factor(seqnos$SeqNo)
  seqnos$Node1 = factor(seqnos$Node1)
  seqnos$Node2 = factor(seqnos$Node2)
  ## seqnos$RandomLoss = factor(seqnos$RandomLoss)
  
  self = subset(seqnos, Node1==Node2)
  ## ggplot(self, aes(x=TimeReal))+geom_point(aes(y=SeqNo, color=Node2))
  
  self$TimeReal = self$Time
  self$Time <- NULL
  self$Node1 <- NULL
  
  seqnos.with.delays <- join (seqnos, self)
  seqnos.with.delays$Delay = seqnos.with.delays$Time - seqnos.with.delays$TimeReal
  
  seqnos.with.delays = subset(seqnos.with.delays, Node1 != Node2)
  
  save (seqnos.with.delays, file=paste(sep="", folder,"/seqnos.rda"))
} else {
  load (paste(sep="", folder,"/seqnos.rda"))
}


delays = c(0, 2^seq(-8,5,0.1))

cdfFun <- function(d) {
  data.frame (
              Delay =    delays, 
              DelayCdf = ecdf(d$Delay)(delays)
              )
}

seqnos.with.delays.ecdf <- ddply (seqnos.with.delays, .(RandomLoss, Type), cdfFun)
seqnos.with.delays.ecdf <- subset (seqnos.with.delays.ecdf, DelayCdf<1.0)

g1 <- ggplot (seqnos.with.delays.ecdf) +
  geom_step (aes(x=Delay, y=DelayCdf, linetype=RandomLoss, color=RandomLoss), size=0.2) +
  geom_point (aes(x=Delay, y=DelayCdf, shape=RandomLoss, color=RandomLoss), size=1.2) +
  facet_wrap (~ Type, nrow=1, ncol=2) +
  scale_y_continuous ("CDF"
                      , trans = trans_new ("test"
                          , function(x) {15^x}
                          , function(x) {log(x,15)}
                          , format=function(x){paste(sep="",round(x*100),"%")})
                      , limits = c(0,1)
                      , minor_breaks = function(x) {r = seq(min(x),max(x),diff(range(x))/20); print(r); return(15^r)}
                      ) +
  scale_x_continuous ("Delay"
                      , trans = trans_new ("test"
                          , function(x) {log(x)}
                          , function(x) {exp(x)}
                          , breaks=function(x){2^seq(-8,5,2)}
                          , format=function(x) {
                            ret<-paste(sep="",round(x),"s")
                            ret[!is.na(x) & x<1] = paste(sep="",round(x[!is.na(x) & x<1],1),"s")
                            ret[!is.na(x) & x<0.1] = paste(sep="",round(1000*x[!is.na(x) & x<0.1]),"ms")
                            return (ret)
                          }
                          )
                          , minor_breaks = function(x) {
                            b <- 2^seq(-8,5,2)
                            breaks <- NULL
                            length(breaks) = (length(b)-1) * 10
                            
                            for(i in c(1:(length(b)-1))) {
                              breaks[(i*10-9):(i*10)] = seq(b[i], b[i+1], (b[i+1]-b[i])/9)
                            }
                            return (log(breaks))
                          }
                      ) +
  theme_custom () +
  scale_shape_discrete ("Random loss"
                        , breaks=c(0, 0.01, 0.05, 0.1)
                        , labels=c("0%", "1%", "5%", "10%")
                        ) +
  scale_linetype_discrete ("Random loss"
                           , breaks=c(0, 0.01, 0.05, 0.1)
                           , labels=c("0%", "1%", "5%", "10%")
                           ) +
  scale_color_manual ("Random loss"
                       , values=c("black", "red", "blue", "darkgreen")
                       , breaks=c(0, 0.01, 0.05, 0.1)
                       , labels=c("0%", "1%", "5%", "10%")
                       ) +
  opts (legend.position = "bottom", #c(0.8,0.75),
        legend.direction="horizontal",
        legend.background = theme_rect(fill="white", colour="black", size=0.1),
        plot.margin = unit(c(0, 0.15, 0, 0), "lines"),
        panel.margin = unit(c(0.15, 0, 0, 0), "lines")
        )


cat ("Writing", paste(sep="/", folder,output1), "\n")
pdf (paste(sep="/", folder,output1), width=6, height=3)
g1
x = dev.off ()


