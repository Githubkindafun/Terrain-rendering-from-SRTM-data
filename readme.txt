READ ME

Opis:
    Widok 2D posiada mały + wskazujący gdzie w 3D znajduje się kamera.
    W widoku 3D ten + pokazuje na środek ekranu poporstu.


Odpalanie z konsoli :
    1) najpierw robimy "make"
    2) teraz już możemy normalnie : "./project data/"nazwaKatalogu" 

Dane :
    zamieszczam w katalogu data 2 przykładowe zestawy danych :
    -> H44 (fragment himalajów)
    -> poland (polska)

W programie mamy 3 opcjonalne argumenty :
    -> "-lat" x x
    -> "-lon" x x
    -> "-start" lon lat h (startowe lon lat oraz wysokosc)

Obsługa :
    -> 2D :
        -> WSAD : przód / tył / prawo / lewo
        -> +/- : przybliżanie / oddalanie
    -> 3D :
        -> WSAD : przód / tył / prawo / lewo
        -> +/- : zwiększanie / zmniejszanie prędkości
        -> strzałki UP/DOWN : przemieszczanie się do góry / w dół
        -> strzałki LEFT/RIGHT : obracanie się w prawo w lewo
    -> 2D i 3D :
        -> 1/2/3/4 : kolejne poziomy LoD (1 największa dokładność - 4 najmniejsza dokładność)
        -> 0 : LoD automatyczny oparty na FPS
        -> SPACE : zmiana widoku z 2D na 3D i vice versa

Komentarz :
    -> ./project data/poland -start 21.017532 52.237049 10 (warszawa)
    -> usunołem obsługę myszy
