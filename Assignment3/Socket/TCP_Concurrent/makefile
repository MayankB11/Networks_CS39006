all : server client
	
server :
	(cd Server; make all)

client :
	(cd Client; make all)
	
clean :
	(cd Server; rm server md5sum.txt)
	(cd Client; rm client md5sum.txt)
