library(scales)
library(grid)
library(ggplot2)
library(plyr)
source ("graph-style.R")

output1 = 'packet-concentration-numClients-52-cdf-total-losses.pdf'
output2 = 'packet-concentration-numClients-52-total-losses.pdf'


run = 0

numClients = 26
numClients = 52
recalculate = TRUE
folder = "losses-adaptive-retransmission"
## folder = "losses-with-singlehomed"
## folder = "losses-adaptive-no-single"
## folder = "losses-no-single-reduced"
## folder = "../run-output"
## numberOfLinks = 94
folder = "losses-vs-retx"

numberOfLinks = 84

if (recalculate) {

  ## data.file = paste(sep='','packet-concentration/packets-numClients-', numClients, '.txt')
  data.file = paste(sep='',folder,'/packets-numClients-', numClients, '.txt')
  data = read.table (data.file,
    col.names=c("Time", "Type", "Run", "Loss", "RandomLoss", "Pkt", "ChannelID", "NodeFrom", "NodeTo", "CountData", "CountInterests"), nrows = 5)
  classes <- sapply(data, class)
  data = read.table (data.file,
    col.names=c("Time", "Type", "Run", "Loss", "RandomLoss", "Pkt", "ChannelID", "NodeFrom", "NodeTo", "CountData", "CountInterests"), colClasses=classes)

  cat ("Done reading\n")
  ## data = subset(data, RandomLoss < 0.1)
  data$RandomLoss = factor(data$RandomLoss, levels=c(0, 0.01, 0.05, 0.1), ordered=TRUE)
  
  ## data = subset(data, Pkt=="OK")
  
  fun <- function(d) {
    data.frame (CountData = sum(d$CountData),
                CountInterests = sum(d$CountInterests)
                )
  }
  
  data = ddply(data, .(Type, Run, Loss, RandomLoss, Pkt, ChannelID), fun)
  
  save (data, file=paste(sep="", folder, "/data.rda"))
} else {
  load (paste(sep="", folder,"/data.rda"))
}


levels(data$Type)[levels(data$Type)=="IP"] = "IRC"
levels(data$Type)[levels(data$Type)=="NDN"] = "Chronos"

data = subset (data, ChannelID <= numberOfLinks-1)
data$Count = data$CountData + data$CountInterests

getCdf <- function(d) {
  ## print (cumsum(d$Count[order(-d$Count)]))
  vec = d$Count[order(-d$Count)]
  length(vec) = numberOfLinks
  vec[is.na(vec)] = 0
  
  data.frame (
              NumLinks = c(1:numberOfLinks),
              Count = cumsum(vec)
              )
}

data2 = subset(data, Pkt=="OK")
data.cdf <- ddply (data2, .(Type, Run, RandomLoss), getCdf)

fmt <- function(x, ...) {
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
          out[[i]] = paste(sep="", round(val,1), "k")
          ## val = x[i] / 10^degree[i]
          ## deg = degree[i]
          ## out[[i]] = substitute(x %.% 10^y, list(x=val, y=deg))
        }
      }
  }
  return (out)
}

conf <- function (d) {
  ret = data.frame (
              Mean = mean (d$Count),
              Conf975 = qt(0.975, length(d$Count)-1) * sd(d$Count) / sqrt (length(d$Count))
  )
  
  ret$Ymin = ret$Mean - ret$Conf975
  ret$Ymax = ret$Mean + ret$Conf975

  return (ret)
}

data.sum = ddply (data.cdf, .(Type, NumLinks, RandomLoss), conf)

## data.sum1 = subset(data.sum, NumLinks %in% c(1,2,3,seq(4,numberOfLinks,3)))

## g1 <- ggplot (subset(data.sum, NumLinks %in% seq(1,82,2)),
g1 <- ggplot (data.sum,
              aes(x=NumLinks, y=Mean, color=RandomLoss, shape=RandomLoss, ymin=Ymin, ymax=Ymax)) +
              ## aes(x=NumLinks, y=Mean, color=RandomLoss, ymin=Ymin, ymax=Ymax, shape=Type)) +
  geom_point (size=1) +
  facet_wrap (~ Type) +
  geom_line (size=0.1) +
  geom_errorbar (width=1, size=0.2, colour="gray30", stat="identity") +
  scale_y_continuous ("Total number of packets", labels=fmt) +
  scale_x_continuous ("Number of links counted in total") +
  scale_shape_discrete ("Loss rate", breaks=c(0, 0.01, 0.05, 0.1), labels=c("0%","1%","5%","10%")) + 
  scale_color_brewer ("Loss rate", palette="Dark2", breaks=c(0, 0.01, 0.05, 0.1), labels=c("0%","1%","5%","10%")) +
  ## scale_colour_manual (values=c("black", "red")) +
  theme_custom () +
  opts (legend.position = c(1, 0),
        legend.direction="vertical",
        legend.justification=c(0.9,0.1),
        legend.background = theme_rect(fill="white", colour="black", size=0.1),
        plot.margin = unit(c(0.1, 0.1, 0, 0), "lines")
        ## ,
        ## legend.title = theme_blank()
        )
## g1


getOrderedSets <- function(d) {
  vec = d$Count[order(-d$Count)]
  length(vec) = numberOfLinks
  vec[is.na(vec)] = 0
  
  data.frame (
              Link = c(1:numberOfLinks),
              Count = vec
              )
}
data.ordered = ddply (data, .(Type, Run, RandomLoss), getOrderedSets)

data.ordered.sum = ddply (data.ordered, .(Type, Link, RandomLoss), conf)
data.ordered.sum$Ymin[data.ordered.sum$Ymin < 0] = 0

data.ordered.sum.subset = subset(data.ordered.sum, ((Type=="IRC" & Link %in% c(1,2,3,4,seq(5,numberOfLinks-1,2))) | (Type=="Chronos" & Link %in% seq(1,numberOfLinks-1,2))))


g2.1 <- ggplot (subset(data.ordered.sum, Type=="IRC"),
              ## aes(x=Link, y=Mean, fill=Type, color=RandomLoss, ymin=Ymin, ymax=Ymax, shape=Type)) +
              aes(x=Link, y=Mean, color=RandomLoss, shape=RandomLoss, ymin=Ymin, ymax=Ymax)) +
  geom_point (size=1) +
  ## geom_boxplot () +
  geom_line (size=0.1) +
  geom_errorbar (width=1, size=0.1, colour="gray40", stat="identity") +
  facet_wrap (~ Type, scale="free_y", ncol=1, nrow=2) +
  scale_y_continuous ("Total number of packets", #\n(sqrt scale)",
                      labels=fmt
                      ## ,
                      ## trans = "sqrt"
                      ## ,
                      ## breaks=trans_breaks(function(x) {round(sqrt(x),1)},function(x) {round(x^2,1)})
                      ,limits = c(0,130000)
                      ) +
  scale_x_continuous ("Different link IDs") + #/*, labels=NA) +
  scale_shape_discrete ("Loss rate", breaks=c(0, 0.01, 0.05, 0.1), labels=c("0%","1%","5%","10%")) + 
  scale_color_brewer ("Loss rate", palette="Dark2", breaks=c(0, 0.01, 0.05, 0.1), labels=c("0%","1%","5%","10%")) +
  theme_custom () +
  opts (legend.position = c(1, 1),
        legend.direction="vertical",
        legend.justification=c(0.9,0.9),
        legend.background = theme_rect(fill="white", colour="black", size=0.1),
        plot.margin = unit(c(0.1, 0.1, 0, 0), "lines")
        ## ,
        ## legend.title = theme_blank()
        )

g2.2 <- ggplot (subset(data.ordered.sum, Type=="Chronos"),
              ## aes(x=Link, y=Mean, fill=Type, color=RandomLoss, ymin=Ymin, ymax=Ymax, shape=Type)) +
              aes(x=Link, y=Mean, color=RandomLoss, shape=RandomLoss, ymin=Ymin, ymax=Ymax)) +
  geom_point (size=1) +
  ## geom_boxplot () +
  geom_line (size=0.1) +
  geom_errorbar (width=1, size=0.1, colour="gray40", stat="identity") +
  facet_wrap (~ Type, scale="free_y", ncol=1, nrow=2) +
  scale_y_continuous ("Total number of packets", #\n(sqrt scale)",
                      labels=fmt
                      ## ,
                      ## trans = "sqrt"
                      ## ,
                      ## breaks=trans_breaks(function(x) {round(sqrt(x),1)},function(x) {round(x^2,1)})
                      ,limits = c(0,7000)
                      ) +
  scale_x_continuous ("Different link IDs") + #/*, labels=NA) +
  scale_shape_discrete ("Loss rate", breaks=c(0, 0.01, 0.05, 0.1), labels=c("0%","1%","5%","10%"), guide="none") + 
  scale_color_brewer ("Loss rate", palette="Dark2", breaks=c(0, 0.01, 0.05, 0.1), labels=c("0%","1%","5%","10%"), guide="none") +
  theme_custom () +
  opts (plot.margin = unit(c(0.1, 0.1, 0, 0), "lines")
        )

total.packets <- subset(data.cdf,NumLinks==numberOfLinks)
g3 <- ggplot(total.packets) +
  geom_boxplot (aes(x=RandomLoss, y=Count, fill=Type), size=0.2, outlier.size=1) +
  scale_y_continuous ("Total number of packets", labels=fmt, limits=c(200000,1000000)) +
  scale_x_discrete ("Random loss", labels=c("0%","1%","5%","10%")) +
  scale_fill_brewer (palette="Spectral") +
  theme_custom () +
  opts (legend.position = "right", #c(1, 1),
        legend.direction="vertical"
        ## ,
        ## legend.justification=c(0.9,0.9),
        ## legend.background = theme_rect(fill="white", colour="black", size=0.1),
        ## plot.margin = unit(c(0.1, 0.1, 0, 0), "lines")
        )


  ## facet_wrap (~ Type, ncol=1, nrow=2)

cat ("Writing", paste(sep="/", folder, output1), "\n")
pdf (paste(sep="/", folder, output1), width=3.4, height=2)
g3
x = dev.off ()

cat ("Writing", paste(sep="/", folder, output2), "\n")
pdf (paste(sep="/", folder, output2), width=3.4, height=4)
library(gridExtra)
grid.arrange (g2.1, g2.2, nrow=2, ncol=1)
x = dev.off ()
