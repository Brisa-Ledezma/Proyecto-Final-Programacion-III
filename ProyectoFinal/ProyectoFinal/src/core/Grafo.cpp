#include "Grafo.h"


namespace grafos {

// ------------------------------------------------------------
//  Utilidades de texto (todas recursivas)
// ------------------------------------------------------------
namespace {

bool esDigito(char c) { return c >= '0' && c <= '9'; }

bool soloDigitos(const std::string& s, size_t i) {
    if (i == s.size()) return true;
    if (!esDigito(s[i])) return false;
    return soloDigitos(s, i + 1);
}

size_t primerNoEspacio(const std::string& s, size_t i) {
    if (i == s.size()) return i;
    if (s[i] != ' ' && s[i] != '\t') return i;
    return primerNoEspacio(s, i + 1);
}

size_t ultimoNoEspacio(const std::string& s, size_t fin) {
    if (fin == 0) return 0;
    if (s[fin - 1] != ' ' && s[fin - 1] != '\t') return fin;
    return ultimoNoEspacio(s, fin - 1);
}

std::string recortar(const std::string& s) {
    const size_t ini = primerNoEspacio(s, 0);
    const size_t fin = ultimoNoEspacio(s, s.size());
    if (ini >= fin) return "";
    return s.substr(ini, fin - ini);
}

} // namespace

std::string nombreAutomatico(int k) { return std::to_string(k + 1); }

// ------------------------------------------------------------
//  Consulta
// ------------------------------------------------------------
int Grafo::cantidadNodos() const { return static_cast<int>(nodos_.size()); }
int Grafo::cantidadAristas() const { return static_cast<int>(aristas_.size()); }
const Nodo& Grafo::nodo(int i) const { return nodos_[i]; }
Nodo& Grafo::nodo(int i) { return nodos_[i]; }
const Arista& Grafo::arista(int i) const { return aristas_[i]; }
Arista& Grafo::arista(int i) { return aristas_[i]; }

int Grafo::buscarNombre(const std::string& nombre, int i) const {
    if (i >= cantidadNodos()) return -1;
    if (nodos_[i].nombre == nombre) return i;
    return buscarNombre(nombre, i + 1);
}

int Grafo::indiceDe(const std::string& nombre) const {
    return buscarNombre(recortar(nombre), 0);
}

// ------------------------------------------------------------
//  Nodos
// ------------------------------------------------------------
std::string Grafo::validarNombre(const std::string& nombreCrudo) const {
    const std::string nombre = recortar(nombreCrudo);
    if (cantidadNodos() >= MAXIMO_NODOS)
        return "Se alcanzó el máximo de " + std::to_string(MAXIMO_NODOS) + " nodos. Borrá alguno para agregar otro.";
    if (nombre.empty()) return "El nombre del nodo no puede estar vacío.";
    if (!soloDigitos(nombre, 0))
        return "El nombre del nodo tiene que ser un número (por ejemplo 1, 7 o 23443).";
    if (nombre[0] == '0')
        return "El número del nodo empieza en 1 (no se admite el 0 ni ceros a la izquierda).";
    if (static_cast<int>(nombre.size()) > LARGO_MAXIMO_NOMBRE)
        return "El número del nodo puede tener como máximo " + std::to_string(LARGO_MAXIMO_NOMBRE) + " cifras.";
    if (indiceDe(nombre) != -1)
        return "Ya existe un nodo llamado \"" + nombre + "\".";
    return "";
}

// Un solo recorrido de los nodos marca qué números del 1 al n+1 están usados; antes se buscaba cada
// candidato nombre por nombre, y agregar nodos costaba cada vez más (n² por nodo nuevo).
void Grafo::marcarUsados(int i, std::vector<bool>& usado) const {
    if (i >= cantidadNodos()) return;
    const std::string& nombre = nodos_[i].nombre;
    if (!nombre.empty() && nombre.size() <= 9 && soloDigitos(nombre, 0)) { // los nombres tienen como mucho 6 cifras
        const long valor = std::stol(nombre);
        if (valor >= 1 && valor <= static_cast<long>(usado.size())) usado[valor - 1] = true;
    }
    marcarUsados(i + 1, usado);
}

int Grafo::primerLibre(const std::vector<bool>& usado, int k) const {
    if (k >= static_cast<int>(usado.size()) || !usado[k]) return k;
    return primerLibre(usado, k + 1);
}

std::string Grafo::siguienteNombreLibre() const {
    std::vector<bool> usado(static_cast<size_t>(cantidadNodos()) + 1, false);
    marcarUsados(0, usado);
    return nombreAutomatico(primerLibre(usado, 0));
}

int Grafo::agregarNodo(const std::string& nombre, double x, double y) {
    Nodo n;
    n.nombre = recortar(nombre);
    n.x = x;
    n.y = y;
    nodos_.push_back(n);
    return cantidadNodos() - 1;
}

void Grafo::setBloqueado(int indice, bool valor) {
    nodos_[indice].bloqueado = valor;
    if (valor) nodos_[indice].obligatorio = false;
}

void Grafo::setObligatorio(int indice, bool valor) {
    nodos_[indice].obligatorio = valor;
    if (valor) nodos_[indice].bloqueado = false;
}

// Recorre las aristas de atrás para adelante borrando las que tocan al nodo.
void Grafo::quitarAristasDe(int nodo, int k) {
    if (k < 0) return;
    if (aristas_[k].desde == nodo || aristas_[k].hasta == nodo)
        aristas_.erase(aristas_.begin() + k);
    quitarAristasDe(nodo, k - 1);
}

// Los nodos que estaban después del borrado bajan un lugar.
void Grafo::reindexarAristas(int nodoBorrado, int k) {
    if (k >= cantidadAristas()) return;
    if (aristas_[k].desde > nodoBorrado) aristas_[k].desde--;
    if (aristas_[k].hasta > nodoBorrado) aristas_[k].hasta--;
    reindexarAristas(nodoBorrado, k + 1);
}

void Grafo::eliminarNodo(int indice) {
    if (indice < 0 || indice >= cantidadNodos()) return;
    quitarAristasDe(indice, cantidadAristas() - 1);
    nodos_.erase(nodos_.begin() + indice);
    reindexarAristas(indice, 0);
}

// ------------------------------------------------------------
//  Aristas
// ------------------------------------------------------------
bool Grafo::cubreArco(const Arista& e, int u, int v) const {
    if (e.desde == u && e.hasta == v) return true;
    if (!e.dirigida && e.desde == v && e.hasta == u) return true;
    return false;
}

int Grafo::buscarArco(int u, int v, int k) const {
    if (k >= cantidadAristas()) return -1;
    if (cubreArco(aristas_[k], u, v)) return k;
    return buscarArco(u, v, k + 1);
}

std::string Grafo::validarArista(int desde, int hasta, bool dirigida) const {
    if (desde < 0 || hasta < 0 || desde >= cantidadNodos() || hasta >= cantidadNodos())
        return "Elegí los dos nodos de la arista.";
    if (desde == hasta) return "Elegí dos nodos distintos.";
    if (cantidadAristas() >= MAXIMO_ARISTAS)
        return "Se alcanzó el máximo de " + std::to_string(MAXIMO_ARISTAS) + " aristas. Borrá alguna para agregar otra.";
    const std::string a = nodos_[desde].nombre;
    const std::string b = nodos_[hasta].nombre;
    if (buscarArco(desde, hasta, 0) != -1)
        return "Ya existe una arista de " + a + " a " + b + ".";
    if (!dirigida && buscarArco(hasta, desde, 0) != -1)
        return "Ya existe una arista de " + b + " a " + a + ". En un grafo no dirigido esa arista ya sirve para los dos sentidos.";
    return "";
}

int Grafo::agregarArista(int desde, int hasta, int pesoIda, int pesoVuelta, bool dirigida) {
    Arista e;
    e.desde = desde;
    e.hasta = hasta;
    e.pesoIda = pesoIda;
    e.pesoVuelta = dirigida ? pesoIda : pesoVuelta;
    e.dirigida = dirigida;
    aristas_.push_back(e);
    return cantidadAristas() - 1;
}

// ---- cambio de tipo del grafo ----
void Grafo::separarEnDirigidas(int k, int total) {
    if (k >= total) return;
    // Cada arista queda con un solo sentido (desde -> hasta): se descarta el peso de vuelta.
    // Para recuperarlo, "Deshacer" vuelve al grafo no dirigido anterior.
    aristas_[k].dirigida = true;
    aristas_[k].pesoVuelta = aristas_[k].pesoIda;
    separarEnDirigidas(k + 1, total);
}

int Grafo::buscarOpuesta(int k, int j) const {
    if (j >= cantidadAristas()) return -1;
    if (aristas_[j].desde == aristas_[k].hasta && aristas_[j].hasta == aristas_[k].desde) return j;
    return buscarOpuesta(k, j + 1);
}

void Grafo::juntarOpuestas(int k) {
    if (k >= cantidadAristas()) return;
    Arista& e = aristas_[k];
    e.dirigida = false;
    const int j = buscarOpuesta(k, k + 1);
    if (j == -1) {
        e.pesoVuelta = e.pesoIda; // sin opuesta: mismo peso en los dos sentidos
    } else {
        e.pesoVuelta = aristas_[j].pesoIda;
        e.bloqueada = e.bloqueada || aristas_[j].bloqueada;
        aristas_.erase(aristas_.begin() + j);
    }
    juntarOpuestas(k + 1);
}

int Grafo::contarPerdidasADirigido(int k) const {
    if (k >= cantidadAristas()) return 0;
    const int pierde = (!aristas_[k].dirigida && aristas_[k].pesoVuelta != aristas_[k].pesoIda) ? 1 : 0;
    return pierde + contarPerdidasADirigido(k + 1);
}

// Cada par A→B / B→A se junta en una sola arista. Los pesos se conservan (ida y vuelta); lo que se
// pierde es el bloqueo si solo una de las dos estaba bloqueada.
int Grafo::contarPerdidasANoDirigido(int k) const {
    if (k >= cantidadAristas()) return 0;
    const int j = buscarOpuesta(k, k + 1);
    const int pierde = (j != -1 && aristas_[k].bloqueada != aristas_[j].bloqueada) ? 1 : 0;
    return pierde + contarPerdidasANoDirigido(k + 1);
}

int Grafo::aristasQuePierdenDatos(bool haciaDirigido) const {
    if (haciaDirigido == dirigido_) return 0;
    return haciaDirigido ? contarPerdidasADirigido(0) : contarPerdidasANoDirigido(0);
}

void Grafo::setDirigido(bool valor) {
    if (valor == dirigido_) return;
    dirigido_ = valor;
    if (valor) separarEnDirigidas(0, cantidadAristas());
    else juntarOpuestas(0);
}

void Grafo::eliminarArista(int indice) {
    if (indice < 0 || indice >= cantidadAristas()) return;
    aristas_.erase(aristas_.begin() + indice);
}

std::string Grafo::describirArista(int indice) const {
    const Arista& e = aristas_[indice];
    const std::string flecha = e.dirigida ? " \xE2\x86\x92 " : " \xE2\x80\x94 "; // → o —
    return nodos_[e.desde].nombre + flecha + nodos_[e.hasta].nombre;
}

// ------------------------------------------------------------
//  General
// ------------------------------------------------------------
void Grafo::limpiar() {
    nodos_.clear();
    aristas_.clear();
}

void Grafo::cargarEjemplo(int& origenSugerido, int& destinoSugerido) {
    limpiar();
    dirigido_ = false;
    //          nombre   x     y
    agregarNodo("1", 90, 240);
    agregarNodo("2", 260, 100);
    agregarNodo("3", 260, 380);
    agregarNodo("4", 450, 100);
    agregarNodo("5", 450, 380);
    agregarNodo("6", 630, 170);
    agregarNodo("7", 790, 300);

    // Pesos fijos: el mismo ejemplo da siempre el mismo resultado (se puede repetir y comparar)
    //            desde hasta ida vuelta dirigida (el ejemplo es no dirigido)
    agregarArista(0, 1, 4, 4, false); // 1 — 2
    agregarArista(0, 2, 2, 3, false); // 1 — 3
    agregarArista(1, 2, 1, 1, false); // 2 — 3
    agregarArista(1, 3, 5, 6, false); // 2 — 4
    agregarArista(2, 4, 7, 7, false); // 3 — 5
    agregarArista(3, 4, 2, 2, false); // 4 — 5
    agregarArista(3, 5, 3, 4, false); // 4 — 6
    agregarArista(4, 5, 6, 5, false); // 5 — 6
    agregarArista(5, 6, 2, 2, false); // 6 — 7
    agregarArista(4, 6, 9, 8, false); // 5 — 7

    origenSugerido = 0;  // 1
    destinoSugerido = 6; // 7
}

} // namespace grafos
