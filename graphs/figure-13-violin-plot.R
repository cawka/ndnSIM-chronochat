
suppressMessages (library(ggplot2))
suppressMessages (library(plyr))
library(grid)
source ("graph-style.R")

output1 = 'link-failures-pairs.pdf'

runs = c(0:9)
numClients = 52

file <- paste(sep='', 'link-failures/heatmap-numClients-',numClients,'.txt')
data <- read.table (file,
                    col.names =c("Type", "Run", "Loss", "RandomLoss", "Node1", "Node2", "Count"),
                    colClasses=c("factor", "factor", "numeric", "factor", "factor", "factor", "numeric"))

data$Type = factor (data$Type, levels=c("IP", "NDN"), ordered=FALSE)
levels(data$Type)[levels(data$Type)=="IP"] = "IRC"
levels(data$Type)[levels(data$Type)=="NDN"] = "Chronos"


## data = subset(data, Count >= 100)
data <- subset(data, Node1 != Node2)
data <- subset(data, Loss != 0)

countPairs <- function(d) {
  data.frame (Pairs = length(d$Count[d$Count >= 100])/(52*51))
}
data.sum = ddply (data, .(Type, Run, Loss), countPairs)

calcLoss <- function(d) {
  data.frame (Pairs = mean(d$Pairs),
             Conf95 = qt (0.95, df=length(d$Pairs)-1) * sd(d$Pairs) / sqrt (length(d$Pairs)))
}
data.avg = ddply (data.sum, .(Type, Loss), calcLoss)
## data.avg$Loss = factor(data.avg$Loss, levels=seq(5,95,5), ordered=TRUE)

## data.avg[order(data.avg$Loss),]

percent <- function (x, ...){
  paste (x*100, '%', sep='')
}

percent1 <- function (x, ...){
  paste (x, '%', sep='')
}

data.avg$Loss = factor(data.avg$Loss)

dodge2 <- position_dodge(width=1.1)

data.subset = subset(data.sum, Loss %in% seq(10,50,10))


g1 <- ggplot (data.subset, aes(x=factor(Loss), y=Pairs, fill=Type)) +
  ## geom_bar (aes(fill=Type), position=dodge2, colour="black", stat="identity", size=0.2, width=0.2) +
  ## geom_errorbar (position=dodge2,  colour="gray20", width=0.2, size=0.2) +
  geom_violin (position=dodge2, trim=TRUE, scale="count", adjust=0.8, size=0.2, width=1.1) +
  geom_boxplot (aes(colour=Type),  position=dodge2, size=0.2, width=0.1, outlier.size=0.1) +
  ## , fill="white"
  scale_y_continuous ("Percent of communicating\npairs (2652 pairs total)", labels=percent) +
  scale_x_discrete ("Number of failed links") +
  ## scale_colour_manual (values=c("black", "red")) +
  scale_fill_brewer (palette="Spectral") +
  scale_color_manual (values=c("black", "black")) +
  theme_custom () +
  opts (legend.position =c(1, 1),
        legend.direction="vertical",
        legend.justification=c(0.9,0.9),
        legend.background = theme_rect(fill="white", colour="black", size=0.1),
        plot.margin = unit(c(0, 0.15, 0, 0), "lines")
        , legend.title = theme_blank()
        )

pdf (output1, width=3.4, height=2.5)
g1
x = dev.off ()
