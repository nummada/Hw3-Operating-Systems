
>**Nume: Nuță Mihaela Mădălina**

>**Grupă: 334CB**

  

# Tema 3

## Explicație pentru structura creată (sau soluția de ansamblu aleasă):
* Implementarea **loaderului** de executabile urmărește următorii pași:
	* înlocuirea handlerului default cu funcția care îl implementează: **segv_handler**, alături de salvarea handlerului default în **old_action**
	* implementarea propriu zisă a handlerului care urmărește cele 3 cazuri: 
		* adresă invalidă -> se va rula handlerul default
		* adresă validă deja mapată -> se va rula handlerul default
		* adresa validă nemapată -> se va mapa fișierul în memorie, pași descriși mai jos în subtitlul ``Implementare``

* Utilitatea temei
Prin această temă am înțeles destul de bine cum se mapeaza fișierele în memorie și cum se implementează în mod corect un handler, mai exact in acest caz un handler care trateaza semnalul SISEGV. Prin **mod corect** mă refer la tratarea tuturor cazurilor care puteau să apară. Prin simplitatea implementării, am putut să înțeleg foarte bine conceptul de mapare în memorie a unui fișier, pagina cu pagina per segment. Greutatea temei a constat mai mult în înțelegerea pașilor care trebuie parcurși pentru a mapa un executabil în memorie, nu implementarea în sine
  

## Implementare
- se calculează din ce segment face parte adresa care a generat page fault-ul
	* *SEGMENT_INDEX_ERROR* -> adresa nu se află în niciun segment al executabilului (adică între **vaddr** și **vaddr + mem_size**)
	* un număr diferit de   *SEGMENT_INDEX_ERROR* care reprezintă indexul segmentului din care face parte adresa
- dacă **s-a returnat** **SEGMENT_INDEX_ERROR**, se rulează handlerul default
- se calculează poziția paginii din care face parte adresa în cadrul segmentului
- se inițializează, în caz de NULL, structura de date care reține dacă paginile din cadrul segmentului au fost mapate sau nu, cu ajutorul unui număr cu rol de variabila booleană
- dacă **pagina a fost mapată deja**, se rulează handlerul default
- **altfel**,  se intră în funcția *legal_page_fault* care urmărește 3 cazuri
	*   dacă *dimensiunea fișierului este egală cu dimensiunea memoriei*, nu conține bss, deci se mapează întreaga pagină cu datele din fișier: la poziția în executabil unde incepe pagina, se mapează o pagină întreagă folosind fișierul reprezentat de file descriptor, citind de la offset-ul în cadrul fișierului al paginii
	* daca dimensiunea fișierului este mai mare decât dimensiunea memoriei, exista 3 cazuri:
		* segmentul conține bss, secțiune cuprinsă în cadrul paginii:
			*  se calculează dimensiunea din cadrul memoriei care trebuie să fie setată pe 0 ca diferența între poziția sfârșitului paginii și dimensiunea fișierului (partea dreaptă a paginii)
			* se mapează în memorie doar restul care a rămas (partea stângă a paginii)
			* modifică protecția paginii în modul WRITE pentru a putea apela memset cu 0 partea din dreapta a paginii
			*  se apelează memset cu 0 pe partea paginii cuprinsă în *file_size**
			* se setează înapoi protecția paginii specificată în cadrul segmentului curent
		* segmentul este cuprins integral în bss, deci se va apela mmap pe întreaga pagină folosind **MAP_ANONYMOUS** care inițializează memoria cu 0 by default. Conform **man**, file descriptorul trebuie specificat a fi -1, iar offset-ul la 0
		* segmentul conține bss, dar nu în cadrul paginii curente: exact la fel ca la primul caz, când mem_size este egal cu file_size. Toată pagina va fi mapată din fișier
	* în toate cele 3 cazuri, se setează pe 1 variabila care reține daca pagina a fost mapată sau nu
---
  
## Alte informații

* Întregul enunț al temei este implementat, nu există funcționalități lipsă. Codul verifică fiecare cod  de eroare și se folosește de define-ul ``DIE`` din laborator pentru a afișa erorile la ``stderr``.

* Dificultăți întâmpinate:
	* apelurile funcției ``mmap`` în funcție de caz
	* apelarea greșită a funcției ``mmap``
	* înțelegerea completă a ceea ce trebuie implementat. Era mai greu de înțeles enunțul decât de realizat implementarea
	* mediu de testare/debug greoi pentru fiecare test

* Lucruri interesante descoperite pe parcurs
	* felul cum se mapează un fișier în memorie
	* faptul că handler-ul sistemului de operare poate fi înlocuit cu un handler propriu
  

* Cum se compilează și cum se rulează?

	* ``make`` crează biblioteca `` libso_loader.so``
	* ``LD_LIBRARY_PATH=. ./so_exec nume_executabil`` ruleaza executabilul

* Bibliografie
	* man
	* ocw - ``DIE`` => folosit la coduri de eroare
	* laboratorul 6, task5, segv_handler
