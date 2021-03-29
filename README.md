Sistemul de conducere efectuează următoarele funcții:
- incălzește un bec până la o temperatură "Tset" într-un timp "tIncal"
- menținerea temperaturii pentru o perioadă de timp "tMen"
- răcirea sistemului la temperatura inițială într-un timp "tRac"

Interfața cu utilizatorul:
- sistemul va dispune de un LCD 16x2 pe care va fi afişat meniul, iar în timpul rulării se vor afişa 
temperatura setată, temperatura curentă şi timpul rămas din etapa actuală (tIncal, tMen, tRac).
- meniul sistemului va permite modificarea următorilor parametri: Tset, tIncal, tMen, tRac, kp, ki, kd.
- parametrii vor fi salvaţi în memoria nevolatilă. Repornirea sistemului nu va afecta parametrii salvaţi.
- Navigarea prin meniul echipamentului se va face prin patru butoane: "OK", "Cancel", "+", "-".

Reglarea temperaturii:
- controlul temperaturii va fi asigurat de un regulator de tip PID
- senzorul de temperatură folosit este unul de tip DHT11.
- elementul de execuţie al sistemului ce va asigura încălzirea senzorului va fi un bec de 12V.
- se folosește un releu pentru a comanda pornirea/oprirea alimentării becului de către microcontroller.
