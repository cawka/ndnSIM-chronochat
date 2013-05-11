library(scales)
library(grid)
suppressMessages (library(ggplot2))
suppressMessages (library(plyr))
source ("graph-style.R")

output1 = 'seqnos-4.pdf'

numClients = 4

input <- paste(sep='','../run-output/seqnos-numClients-', numClients, '.txt')

data <- read.table (input,
                    col.names = c("Run", "Type", "Loss", "Loss2", "Time", "Node1", "Node2", "SeqNo"))
data$Node1 = factor(data$Node1)
data$Node2 = factor(data$Node2)
levels(data$Type)[levels(data$Type)=="IP"] = "IRC"
levels(data$Type)[levels(data$Type)=="NDN"] = "Chronos"

text.labels = data.frame (
  x = c(200,400,800,1000),
  y = c(60, 60, 60, 0),
  halign = c(1,1,1,0),
  labels = c("Link between\n0 and 1 failed",     "Link between\n2 and 3 failed",     "Link between\n0 and 1 healed",     "Link between\n2 and 3 healed"))

g <-
  ggplot(subset(data, Node1==0 & Node2!=0)) +
  geom_step (aes(x=Time, y=SeqNo, colour=Node2, linetype=Node2), size=0.7) +
  ## facet_grid (Type ~ .) +
  geom_vline (xintercept = text.labels$x, linetype='dashed', size=0.2) +
  geom_text (aes(x=x, y=y, label=labels, hjust=halign), family="serif", lineheight=0.8, data=text.labels, angle=90, vjust=-0.2, size=3) +
  scale_colour_brewer("0's knowledge about ", type="div", palette="Set1") +
  scale_linetype_manual ("0's knowledge about ", values=c("solid", "dotted", "dashed")) +
  scale_x_continuous ("Time, seconds") +
  scale_y_continuous ("Message sequence number") +
  theme_custom () +
  opts (legend.title.align = 0.5,
        ## legend.position = "bottom",
        legend.position = c(0.5, -0.18), 
        legend.direction = "horizontal",
        ## legend.justification = "center",
        legend.box = "vertical"
        , plot.margin = unit(c(0.1, 0.1, 1.1, 0), "lines")
        , guide.margin = unit(c(0,0,0,0), "lines")
        , legend.key.width = unit(1.5, "lines")
        ## ,
        ## legend.title = theme_blank ()
        ## ,
        ## legend.title.align = 0
        )



pdf (output1, width=3.4, height=3.4)
g
x = dev.off ()

