server:
	g++ -o server server.cpp

subscriber:
	g++ -o subscriber subscriber.cpp

clean:
	rm -f server subscriber