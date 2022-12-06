modelname = "Courbe90Pourcen.data"
data = read.table(modelname)
attach(data)

e2 = V1

IC1 = t.test(e2, conf.level = 0.9)


plot(e2, type ="h",xlab = "n°execution",ylab = "probabilité de colidion",col = "red",main="courbes 90% intervalle de confiance")
abline(h = IC1$conf.int[1], col="black")
abline(h = IC1$conf.int[2], col="black")

