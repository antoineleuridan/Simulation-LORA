modelname = "HistogrameTempsE2.data"
data = read.table(modelname)
attach(data)
e2 = V1

hist(e2, breaks=50,xlab = "temps d'emision",ylab= "e2", col = "red", main="Histogramme temps d'emision e2")