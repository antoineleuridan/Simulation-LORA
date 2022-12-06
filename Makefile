lancer_script : 
	./script_Pourcentage_Collision
	./script_Courbe_Rep
	./script_HistogrammeE1
	./script_HistogrammeE2
	./script_Courbe_Pourcentage_100

clean : 
	rm Simulation
	rm *.data
	rm *.Rout
	rm .RData
	cd Courbe_simulation ;rm *.pdf
	
	
script :
	chmod +x script_Courbe_Pourcentage_100
	chmod +x script_Courbe_Rep
	chmod +x script_HistogrammeE1
	chmod +x script_HistogrammeE2
	chmod +x script_Pourcentage_Collision