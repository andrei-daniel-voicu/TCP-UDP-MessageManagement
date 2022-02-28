Nume: Voicu Andrei-Daniel
Grupa: 322CC

Tema am implementat-o in c++ (server.cpp, subscriber.cpp, helpers.hpp).
In headerul helpers am inclus headerele comune folosite de server.cpp
si de subscriber.cpp, variabilele comune (variabila de ret si un buffer de 1560 bytes)
si functiile de send si receive data:
sendData -> trimite intai dimensiunea folosind fct send, iar intr-un loop
trimite date pana se atinge acea dimensiune (returneaza numarul de bytes
trimisi sau -1 la eroare)
receiveData -> primeste intai dimensiunea folosind fct recv, iar intr-un loop
primeste date pana se atinge acea dimensiune (returneaza numarul de bytes
primiti sau -1 la eroare)
Peste aceste 2 functii am creat 4 macrouri: SEND_DATA_TO_SERVER, RECEIVE_DATA_FROM_SERVER
SEND_DATA_TO_CLIENT, RECEIVE_DATA_FROM_CLIENT care verifica numarul de bytes
primiti/transmisi si inchid socketi sau intorc erori in fct de client/server.

subscriber.cpp -> Deschid conexiunea catre server (creez un socket)
si trimit intai un mesaj de forma "ID: " + id catre server pentru a
ii oferi id-ul clientului. Am folosit multiplexarea I/O pentru a asculta
pe descriptorul activ (server/stdin). Daca se citeste de la tastatura
comanda subscribe (se verifica corectitudinea formatului si in caz ca se
dau mai putine argumente sau mai multe nu se transmite si se continua executia)
atunci trimit catre server informatia, si daca acesta o primeste corect
(verificand ce intoarce functia send in MACRO) atunci printez in client mesaj
corespunzator. Pentru comanda unsubscribe am procedat asemanator. La comanda exit
inchid socketul si programul. Daca primeste date de la server atunci apeleaza
functia de parsare a mesajului. Aceasta desparte bufferul in topic, type si
data si intr-un switch iau cele 4 cazuri la rand.


server.cpp -> Deschid conexiunea pe un socket TCP si unul UDP.
Pentru fiecare client conectat deschid un socket nou si adaug
la descriptori (dezactivez si algoritmul Nagle). Am folosit o
structura care modeleaza clientul si contine un string pentru id, socketul
asignat, un bool pentru starea lui (conectat sau nu) si un vector de stringuri
pe post de queue care retine mesajele ce vor urma sa fie trimise la reconectare.
Clientii i-am salvat intr-un unordered map cu cheia ID-ul acestuia, iar pentru
topicuri am create un unordered_map cu cheia numele topicului, iar valoarea un alt
unordered_map cu cheia id-ul clientului si valoarea fiind valoarea de store-forward.
La primirea unui mesaj udp, adaug id si port si parcurg subscriberii, transmitand
mesajul sau salvandu-l in coada acestuia. La comand exit inchid toate socketurile
si programul. Cand un client se deconecteaza, parcurg mapul de clienti pana cand gasesc
clientul cu socketul respectiv si ii modific starea in false. Cand primesc id, daca
clientul nu exista il creez si adaug in map, altfel daca exista deja, daca era conectat
ii dau close pe socketul acela si il scot din descriptori, altfel ii modific starea si
socketul si trimit mesajele din coada, daca este cazul (tot aici printez si mesajul de conectare).
La subscribe inserez topicul in map daca nu exista, iar daca este subscribed deja atunci ii modific valoarea de store-forward.
La unsubscribe, daca topicul nu exista nu fac nimic, altfel caut id-ul dupa socket si sterg din mapul de topicuri.

