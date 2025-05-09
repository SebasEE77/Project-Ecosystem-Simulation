#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <thread>
#include <conio.h>
#include <windows.h> 
using namespace std;

const int VACIO = 0;
const int CONEJO = 1;
const int ZORRO = 2;
const int ROCA = 3;

struct Mundo {
    int filas;
    int columnas;
    vector<vector<int>> matriz;
};

struct Conejo {
    int x;
    int y;
    int edad_reproduccion;
};

struct Zorro {
    int x;
    int y;
    int edad_reproduccion;
    int hambre;
};

struct Parametros {
    int gen_proc_conejos = 0;  // Generaciones hasta que un conejo puede procrear
    int gen_proc_zorros;       // Generaciones hasta que un zorro puede procrear
    int gen_comida_zorros;     // Generaciones para que un zorro muera de hambre
    int num_generaciones;      // Número de generaciones para la simulación
    int num_objetos;           // Cantidad de elementos del mundo
};

void inicializar_mundo(ifstream &archivo_entrada, Mundo &mundo, vector<Conejo> &conejos, vector<Zorro> &zorros, Parametros &params, int &num_rocas) {
    // Para leer los parametros en consola o en el archivo de entrada
    if (params.gen_proc_conejos == 0) {
        archivo_entrada >> params.gen_proc_conejos >> params.gen_proc_zorros >> params.gen_comida_zorros 
                        >> params.num_generaciones;
    } else {
        int saltar;
        archivo_entrada >> saltar >> saltar >> saltar >> saltar;
    }

    archivo_entrada >> mundo.filas >> mundo.columnas >> params.num_objetos;
    
    mundo.matriz.resize(mundo.filas, vector<int>(mundo.columnas, VACIO));
    
    for (int i = 0; i < params.num_objetos; i++) {
        string tipo_objeto;
        int x, y;
        archivo_entrada >> tipo_objeto >> x >> y;
        
        if (tipo_objeto == "ROCK") {
            mundo.matriz[x][y] = ROCA;
            num_rocas++;
        } 
        else if (tipo_objeto == "RABBIT") {
            mundo.matriz[x][y] = CONEJO;
            
            Conejo nuevo_conejo;
            nuevo_conejo.x = x;
            nuevo_conejo.y = y;
            nuevo_conejo.edad_reproduccion = 0;
            
            conejos.push_back(nuevo_conejo);
        } 
        else if (tipo_objeto == "FOX") {
            mundo.matriz[x][y] = ZORRO;
            
            Zorro nuevo_zorro;
            nuevo_zorro.x = x;
            nuevo_zorro.y = y;
            nuevo_zorro.edad_reproduccion = 0;
            nuevo_zorro.hambre = 0;
            
            zorros.push_back(nuevo_zorro);
        }
    }
}

void inicializar_edad(Mundo &mundo, vector<vector<Conejo>> &conejos_nuevos, vector<vector<Zorro>> &zorros_nuevos){
    for (int i = 0; i < mundo.filas; i++) {
        for (int j = 0; j < mundo.columnas; j++) {
            conejos_nuevos[i][j].edad_reproduccion = -1; // Marca como no válido
            zorros_nuevos[i][j].edad_reproduccion = -1;  // Marca como no válido
        }
    }
}

void imprimir_estado(ofstream &archivo_salida, const Mundo &mundo, vector<Zorro> &zorros, vector<Conejo> &conejos, const Parametros &params, int generacion_actual, int num_rocas) {
    int num_objetos = 0;
    int ultima_generacion = 0;

    num_objetos = conejos.size() + zorros.size() + num_rocas;
    
    archivo_salida << params.gen_proc_conejos << " " << params.gen_proc_zorros << " " 
                   << params.gen_comida_zorros << " " << ultima_generacion << " " 
                   << mundo.filas << " " << mundo.columnas << " " << num_objetos << endl;
    
    for (int i = 0; i < mundo.filas; i++) {
        for (int j = 0; j < mundo.columnas; j++) {
            if (mundo.matriz[i][j] == ROCA) {
                archivo_salida << "ROCK " << i << " " << j << endl;
            } 
            else if (mundo.matriz[i][j] == CONEJO) {
                archivo_salida << "RABBIT " << i << " " << j << endl;
            } 
            else if (mundo.matriz[i][j] == ZORRO) {
                archivo_salida << "FOX " << i << " " << j << endl;
            }
        }
    }
}

vector<pair<int, int>> obtener_celdas_adyacentes(int x, int y, const Mundo &mundo, int estado) {
    vector<pair<int, int>> celdas;

    // Verificar para mover arriba
    if (x > 0 && mundo.matriz[x - 1][y] == estado)
        celdas.push_back(make_pair(x - 1, y));

    // Verificar para mover derecha
    if (y < mundo.columnas - 1 && mundo.matriz[x][y + 1] == estado)
        celdas.push_back(make_pair(x, y + 1));

    // Verificar para mover abajo
    if (x < mundo.filas - 1 && mundo.matriz[x + 1][y] == estado)
        celdas.push_back(make_pair(x + 1, y));
        
    // Verificar para mover izquierda
    if (y > 0 && mundo.matriz[x][y - 1] == estado)
        celdas.push_back(make_pair(x, y - 1));

    return celdas;
}

pair<int, int> seleccionar_celda_destino(int x, int y, const vector<pair<int, int>> &celdas_posibles, int generacion_actual) {
    if (celdas_posibles.empty()) {
        return make_pair(-1, -1); // Indica que no hay destino válido
    }
    
    int p = celdas_posibles.size();
    int indice = (generacion_actual + x + y) % p;
    
    return celdas_posibles[indice];
}

void mover_conejos(Mundo &mundo, vector<Conejo> &conejos, const Parametros &params, int generacion_actual, vector<vector<Conejo>> &conejos_nuevos) {
    vector<vector<bool>> hay_conejo_nuevo(mundo.filas, vector<bool>(mundo.columnas, false));
    // Procesar cada conejo
    for (int i = 0; i < conejos.size(); i++) {
        int x_viejo = conejos[i].x;
        int y_viejo = conejos[i].y;
        
        // Obtener celdas adyacentes
        vector<pair<int, int>> celdas_adyacentes = obtener_celdas_adyacentes(x_viejo, y_viejo, mundo, VACIO);
        
        // Si hay celdas vacías alrededor, intentar moverse
        if (!celdas_adyacentes.empty()) {
            pair<int, int> destino = seleccionar_celda_destino(x_viejo, y_viejo, celdas_adyacentes, generacion_actual);
            
            if (destino.first != -1) { // Si hay un destino válido
                int x_nuevo = destino.first;
                int y_nuevo = destino.second;
                
                // Verificar si puede reproducirse
                bool puede_reproducirse = (conejos[i].edad_reproduccion >= params.gen_proc_conejos);
                
                // Si puede reproducirse, dejar un nuevo conejo en la posición anterior
                if (puede_reproducirse) {
                    hay_conejo_nuevo[x_viejo][y_viejo] = true;
                    conejos[i].edad_reproduccion = 0;
                } else {
                    conejos[i].edad_reproduccion++;
                }
                
                // Mover el conejo a la nueva posición
                conejos[i].x = x_nuevo;
                conejos[i].y = y_nuevo;
                
                // Verificar si ya hay un conejo en la nueva posición
                if (conejos_nuevos[x_nuevo][y_nuevo].edad_reproduccion != -1) {
                    // Comparar edades de reproducción
                    if (conejos[i].edad_reproduccion > conejos_nuevos[x_nuevo][y_nuevo].edad_reproduccion) {
                        // Este conejo tiene mayor edad, sobrevive
                        conejos_nuevos[x_nuevo][y_nuevo] = conejos[i];
                    }
                    // Si no, el conejo existente sobrevive
                } else {
                    // No hay conflicto, colocar el conejo
                    conejos_nuevos[x_nuevo][y_nuevo] = conejos[i];
                }
            } else {
                // No se movió, incrementar edad
                conejos[i].edad_reproduccion++;
                
                // Mantener el conejo en la posición actual
                conejos_nuevos[x_viejo][y_viejo] = conejos[i];
            }
        } else {
            // No hay celdas vacías alrededor, incrementar edad
            conejos[i].edad_reproduccion++;
            
            // Mantener el conejo en la posición actual
            conejos_nuevos[x_viejo][y_viejo] = conejos[i];
        }
    }
    // Actualizar la lista de conejos para solo tener los que sobrevivieron y nacieron
    conejos.clear();
    
    for (int i = 0; i < mundo.filas; i++) {
        for (int j = 0; j < mundo.columnas; j++) {
            //Limpiar la matriz del mundo para los conejos para la nueva generacion 
            if (mundo.matriz[i][j] == CONEJO) {
                mundo.matriz[i][j] = VACIO;
            }
            // Añadir los conejos que sobrevivieron a los conflictos
            if (conejos_nuevos[i][j].edad_reproduccion != -1) {
                conejos.push_back(conejos_nuevos[i][j]);
                mundo.matriz[i][j] = CONEJO;
            }
            // Añadir los nuevos conejos por reproducción
            if (hay_conejo_nuevo[i][j] && mundo.matriz[i][j] == VACIO) {
                Conejo nuevo_conejo;
                nuevo_conejo.x = i;
                nuevo_conejo.y = j;
                nuevo_conejo.edad_reproduccion = 0;
                
                conejos.push_back(nuevo_conejo);
                mundo.matriz[i][j] = CONEJO;
            }
        }
    }
}

void mover_zorros(Mundo &mundo, vector<Zorro> &zorros, vector<Conejo> &conejos, const Parametros &params, int generacion_actual, vector<vector<Zorro>> &zorros_nuevos) {
    vector<vector<bool>> hay_zorro_nuevo(mundo.filas, vector<bool>(mundo.columnas, false));

    for (int i = 0; i < zorros.size(); i++) {
        int x_viejo = zorros[i].x;
        int y_viejo = zorros[i].y;

        // Buscar celdas adyacentes
        vector<pair<int, int>> celdas_con_conejos = obtener_celdas_adyacentes(x_viejo, y_viejo, mundo, CONEJO);
        vector<pair<int, int>> celdas_adyacentes = obtener_celdas_adyacentes(x_viejo, y_viejo, mundo, VACIO);

        bool comio = false;
        bool murio = false;
        int x_nuevo = x_viejo;
        int y_nuevo = y_viejo;

        // Intentar comer conejo
        if (!celdas_con_conejos.empty()) {
            pair<int, int> destino = seleccionar_celda_destino(x_viejo, y_viejo, celdas_con_conejos, generacion_actual);
            x_nuevo = destino.first;
            y_nuevo = destino.second;
            zorros[i].hambre = 0;  // comió
            comio = true;

            // Eliminar conejo comido
            int bandera = 0;
            for (int j = 0; j < conejos.size() && bandera == 0; j++) {
                if (conejos[j].x == x_nuevo && conejos[j].y == y_nuevo) {
                    conejos[j] = conejos.back();
                    conejos.pop_back();
                    bandera = 1;
                }
            }

        } else {
            zorros[i].hambre++;
            // Si no comió, revisar si muere de hambre
            if (zorros[i].hambre >= params.gen_comida_zorros) {
                murio = true;
            } else if (!celdas_adyacentes.empty()) {
                // Moverse a una celda vacía
                pair<int, int> destino = seleccionar_celda_destino(x_viejo, y_viejo, celdas_adyacentes, generacion_actual);
                x_nuevo = destino.first;
                y_nuevo = destino.second;
            }
        }

        if (!murio) {
            // Verificar reproducción
            bool puede_reproducirse = (zorros[i].edad_reproduccion >= params.gen_proc_zorros);

            if (puede_reproducirse && (x_nuevo != x_viejo || y_nuevo != y_viejo)) {
                // Reproduce
                Zorro hijo;
                hijo.x = x_viejo;
                hijo.y = y_viejo;
                hijo.edad_reproduccion = 0;
                hijo.hambre = 0;
                hay_zorro_nuevo[x_viejo][y_viejo] = true;
                zorros[i].edad_reproduccion = 0;
            } else {
                zorros[i].edad_reproduccion++;
            }

            // Mover zorro
            zorros[i].x = x_nuevo;
            zorros[i].y = y_nuevo;

            // Resolver conflictos: mantener el de mayor edad reproducción o el menos hambriento en empate
            if (zorros_nuevos[x_nuevo][y_nuevo].edad_reproduccion != -1) {
                Zorro &otro = zorros_nuevos[x_nuevo][y_nuevo];
                if (zorros[i].edad_reproduccion > otro.edad_reproduccion ||
                   (zorros[i].edad_reproduccion == otro.edad_reproduccion && zorros[i].hambre < otro.hambre)) {
                    zorros_nuevos[x_nuevo][y_nuevo] = zorros[i];
                }
            } else {
                zorros_nuevos[x_nuevo][y_nuevo] = zorros[i];
            }
        } 
    }

    zorros.clear();

    for (int i = 0; i < mundo.filas; i++) {
        for (int j = 0; j < mundo.columnas; j++) {
            // Limpiar zorros del mundo
            if (mundo.matriz[i][j] == ZORRO) {
                mundo.matriz[i][j] = VACIO;
            }
            // Actualizar zorros y matriz con los sobrevivientes
            if (zorros_nuevos[i][j].edad_reproduccion != -1) {
                zorros.push_back(zorros_nuevos[i][j]);
                mundo.matriz[i][j] = ZORRO;
            }
            // Añadir zorros nacidos por reproducción
            if (hay_zorro_nuevo[i][j] && mundo.matriz[i][j] == VACIO) {
                Zorro nuevo;
                nuevo.x = i;
                nuevo.y = j;
                nuevo.edad_reproduccion = 0;
                nuevo.hambre = 0;
                zorros.push_back(nuevo);
                mundo.matriz[i][j] = ZORRO;
            }
            
        }
    }
}

void imprimir_mundo(const Mundo &mundo, int generacion) {
    cout << "Generacion " << generacion << endl;
    cout << string(mundo.columnas * 2 + 1, '-') << endl;
    
    for (int i = 0; i < mundo.filas; i++) {
        cout << "|";
        for (int j = 0; j < mundo.columnas; j++) {
            char simbolo;
            switch (mundo.matriz[i][j]) {
                case VACIO: simbolo = '.'; break;
                case CONEJO: simbolo = 'R'; break;
                case ZORRO: simbolo = 'F'; break;
                case ROCA: simbolo = '*'; break;
                default: simbolo = '?'; break;
            }
            cout << simbolo << " ";
        }
        cout << "\b|" << endl; // Quitar último espacio y cerrar la línea
    }
    
    cout << string(mundo.columnas * 2 + 1, '-') << endl << endl;
}

void imprimir_estadisticas(int generacion, const vector<Conejo> &conejos, const vector<Zorro> &zorros) {
    cout << "Generacion " << generacion << ":\n";
    cout << " - Conejos: " << conejos.size() << "\n";
    cout << " - Zorros : " << zorros.size() << "\n";
    cout << "-------------------------" << endl;
}

void mostrar_controles() {
    cout << "\n--- CONTROLES ---\n";
    cout << "p: Pausar/Reanudar\n";
    cout << "+: Aumentar velocidad\n";
    cout << "-: Disminuir velocidad\n";
    cout << "q: Acabar la simulacion\n";
    cout << "----------------\n";
}

char procesar_input() {
    // Permite leer la entrada del teclado sin pausar el programa.
    if (_kbhit()) {
        // Devuelve el carácter presionado sin necesidad de presionar ENTER.
        return _getch();
    }
    return 0;
}

int main(int argc, char* argv[]) {
    ifstream archivo_entrada(argv[1]);
    if (!archivo_entrada.is_open()) {
        cout << "Error: No se pudo abrir el archivo de entrada: " << argv[1] << endl;
        return 1;
    }
    
    ofstream archivo_salida(argv[2]);
    if (!archivo_salida.is_open()) {
        cout << "Error: No se pudo abrir el archivo de salida: " << argv[2] << endl;
        archivo_entrada.close();
        return 1;
    }
    
    Mundo mundo;
    vector<Conejo> conejos;
    vector<Zorro> zorros;
    Parametros params;
    int num_rocas = 0;

    cout << "Deseas ajustar parametros? (s/n): ";
    char opcion;
    cin >> opcion;
    if (opcion == 's' || opcion == 'S') {
        cout << "Generaciones hasta que un conejo se reproduce: ";
        cin >> params.gen_proc_conejos;
        cout << "Generaciones hasta que un zorro se reproduce: ";
        cin >> params.gen_proc_zorros;
        cout << "Generaciones sin comer para que un zorro muera: ";
        cin >> params.gen_comida_zorros;
        cout << "Numero total de generaciones: ";
        cin >> params.num_generaciones;
    }

    inicializar_mundo(archivo_entrada, mundo, conejos, zorros, params, num_rocas);
    
    // Medir tiempo para evaluar mejora de rendimiento
    
    // Matrices temporales para construir el nuevo estado del mundo después de procesar todo lo demas
    vector<vector<Conejo>> conejos_nuevos(mundo.filas, vector<Conejo>(mundo.columnas));
    vector<vector<Zorro>> zorros_nuevos(mundo.filas, vector<Zorro>(mundo.columnas));
    
    cout << "¿Desea ver la simulacion poco a poco usando controles de tiempo o ver de una vez la respuesta?: \n";
    cout << "1. Simulacion con controles de tiempo\n";
    cout << "2. Simulacion con respuesta inmediata\n";
    cout << "Escriba el numero de la opcion: ";
    int opcion2;
    cin >> opcion2;
    
    auto inicio = chrono::high_resolution_clock::now();
    
    if (opcion2 == 1){
        bool pausado = false;
        int velocidad_ms = 600;
        int gen = 0;
        bool continuar = true;
        while (gen < params.num_generaciones && continuar) {
            // Procesar la generación actual
            inicializar_edad(mundo, conejos_nuevos, zorros_nuevos);
            mover_conejos(mundo, conejos, params, gen, conejos_nuevos);
            mover_zorros(mundo, zorros, conejos, params, gen, zorros_nuevos);
            
            // Mostrar el estado actual
            system("cls"); // Limpiar pantalla (Windows)
            imprimir_mundo(mundo, gen);
            imprimir_estadisticas(gen, conejos, zorros);
            
            cout << "Velocidad: " << velocidad_ms << "ms | ";
            if (pausado) {
                cout << "PAUSADO (presiona 'p' para continuar)\n";
            } else {
                cout << "EN EJECUCION (presiona 'p' para pausar)\n";
            }
            
            // Entrada del usuario
            do {
                char tecla = procesar_input();
                if (tecla == 'p' || tecla == 'P') {
                    pausado = !pausado;
                    if (pausado) {
                        cout << "Simulacion pausada. Presiona 'p' para continuar.\n";
                    } else {
                        cout << "Simulacion reanudada.\n";
                        break; // Salir del bucle de pausa
                    }
                } else if (tecla == '+') {
                    velocidad_ms = max(50, velocidad_ms - 50);
                    cout << "Velocidad aumentada: " << velocidad_ms << "ms\n";
                } else if (tecla == '-') {
                    velocidad_ms += 50;
                    cout << "Velocidad disminuida: " << velocidad_ms << "ms\n";
                } else if (tecla == 'q' || tecla == 'Q') {
                    cout << "Saliendo de la simulación...\n";
                    continuar = false;
                    break;
                } else if (tecla == 'h' || tecla == 'H') {
                    mostrar_controles();
                }
                
                if (!pausado && tecla != 0) {
                    break;
                }
                
                Sleep(velocidad_ms); // Pequeña pausa
            } while (pausado);
            
            // Esperar según la velocidad configurada (solo si no está pausado)
            if (!pausado && continuar) {
                Sleep(velocidad_ms);        
            }
            gen++;
        }
        system("cls"); // Limpiar pantalla (Windows)
    } else{

        for (int gen = 0; gen < params.num_generaciones; gen++) {
            inicializar_edad(mundo, conejos_nuevos, zorros_nuevos);
            mover_conejos(mundo, conejos, params, gen, conejos_nuevos);
            mover_zorros(mundo, zorros, conejos, params, gen, zorros_nuevos);    
            //imprimir_mundo(mundo, gen);
            //imprimir_estadisticas(gen, conejos, zorros);
        }
        
    }
    
    //imprimir_mundo(mundo, params.num_generaciones);
    imprimir_estadisticas(params.num_generaciones, conejos, zorros);
    imprimir_estado(archivo_salida, mundo, zorros, conejos, params, params.num_generaciones, num_rocas);
    
    auto fin = chrono::high_resolution_clock::now();
    chrono::duration<double> duracion = fin - inicio;
    cout << "Tiempo de ejecucion: " << duracion.count() << " segundos" << endl;
    
    archivo_entrada.close();
    archivo_salida.close();    
    return 0;
}