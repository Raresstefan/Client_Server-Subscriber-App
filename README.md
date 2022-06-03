# Client_Server-Subscriber-App

# 1. Implementarea server-ului:
Server-ul a fost implementat in asa fel incat acesta sa poata gestiona interactiunea
    intre un client UDP si unul TCP(subscriber). Ma folosesc de 2 unordered_map, unul in care cheia
    reprezinta id-ul unui client, iar valoarea reprezinta datele despre client, iar in cel de-al
    doilea unordered_map cheia reprezinta socket-ul unui client, iar valoarea reprezinta datele despre
    client.
        Dupa ce am creat cate un socket pentru fiecare tip de conexiune(TCP/UDP) urmeaza sa verificam 
    ce fel de informatie primeste server-ul. Primul caz acopera situatia in care server-ul primeste
    o comanda de la tastatura. Singurul caz in care acest lucru se poate face este acela in care comanda
    este "exit", caz in care server-ul trebuie oprit si totodata inchise toate socket-urile.
        In cazul in care nu s-a dat aceasta comanda verificam daca s-a primit ceva
    de pe socket-ul de TCP. In acest caz acceptam conexiunea cu subscriber-ul si il adaugam
    in map-ul de clienti conectati doar in cazul in care nu exista
    deja un client cu acelasi id conectat.
        In cazul in care conexiunea reuseste sa se stabileasca ne asiguram ca clientul proaspat conectat
    primeste toate mesajele pe care ar fi trebuit sa le primeasca in timpul in care
    acesta nu era conectat, lucru ce tine
    de functionalitatea de Store and Forward.
    In cazul in care primim ceva pe socket-ul de UDP server-ul face conversia mesajului primit
    de la clientul de UDP intr-un mesaj pe care il poate primi clientul 
    de TCP, respectand conventiile din enunt. Dupa efectuarea acestui pas 
    server-ul trimite mesajele atat clientilor conectati si abonati la topicul curent, cat si
    celor care nu sunt conectati dar care s-au abonat cu flag-ul de sf setat.
        Ultimul caz acopera situatia in care primim o comanda de la subscriber, existand 2 posibilitati:
            -Subscribe: Server-ul cauta in lista de topic-uri la care clientul cu id-ul specificat vrea sa se aboneze.
            Doar in cazul in care acesta nu este deja abonat, adauga topic-ul nou in lista de topic-uri la care 
            clientul este abonat, altfel nu face nimic.
            -Unsubscribe: Server-ul sterge topic-ul la care clientul vrea sa se dezabanoze din lista de topic-uri
            ale acestuia. In cazul in care clientul a introdus in comanda un topic 
            la care acesta nu este abonat, server-ul nu va face nimic.
      
# 2. Implementarea clientului:
Se creaza un socket de TCP. Daca conexiunea nu esueaza, primul
    pas este sa verificam daca informatia pe care
    clientul trebuie sa o primeasca vine de la stdin sau de la server. 
    Pentru cazul in care primim o comanda de la tastatura acoperim cazul in care 
    aceasta este comanda de "exit", lucru ce anunta ca clientul trebuie deconectat de la server.
    Cel de-al doilea caz se extinde in alte 2:
        2.1: Se da comanda de subscribe. Verific daca au fost introdusi toti parametrii necesari, daca nu
        printez un mesaj de eroare.
        2.2: Se da comanda de unsubscribe. Verificam daca toti parametrii au fost introdusi.
        In final, daca toti parametrii au fost introdusi pentru oricare din cele 2 tipuri de comenzi, 
        inseamna ca fiecare camp din structura mesajului care trebuie trimis catre server de 
        catre subscriber este completat si trimitem informatia catre server.
    In cazul in care primim un mesaj de la server, inseamna ca primim un mesaj
    de la clientul de UDP, care a fost parsat de catre server, astfel incat
    subscriber-ul sa il poata afisa, insa nu ne este garantat ca primim intreaga 
    informatie dintr-o bucata, de aceea apelam functia recv succesiv pana cand suntem siguri
    ca toata informatia a fost transmisa.
