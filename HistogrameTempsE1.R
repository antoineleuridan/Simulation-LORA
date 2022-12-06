modelname = "HistogrameTempsE1.data"
data = read.table(modelname)
attach(data)
e1 = V1

hist(e1, breaks=50,xlab = "temps d'emision",ylab= "e1", col = "red", main="Histogramme temps d'emision e1")