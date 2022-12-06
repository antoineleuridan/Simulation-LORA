modelname = "CourbePourcentageCollision.data"
data = read.table(modelname)
attach(data)
t_emission = V1 
e1 = V2
e2 = V3 
e3 = V4 
e4 = V5 
e5 = V6 
e6 = V7 
e7 = V8 

plot(t_emission,e1, type ="l",xlab = "temps",ylab = "pourcentage collision",col = "red",main="courbe pourcentage")
lines(t_emission,e2, col="blue")
lines(t_emission,e3, col="orange")
lines(t_emission,e4, col="green")
lines(t_emission,e5, col="purple")
lines(t_emission,e6, col="pink")
lines(t_emission,e7, col="yellow")