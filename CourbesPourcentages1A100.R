modelname = "CourbesMoyenneCollision.data"
data = read.table(modelname)
attach(data)
k = V1
pourcentage_globlal = V2

plot(k,pourcentage_globlal, type ="l",xlab = "k",ylab = "pourcentage global de colision",col = "red",main="courbe de 1 a 100")
