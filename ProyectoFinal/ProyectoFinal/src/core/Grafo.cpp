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

// Convierte "23443" en 23443, dígito por dígito.
long aNumero(const std::string& s, size_t i, long acumulado) {
    if (i == s.size()) return acumulado;
    return aNumero(s, i + 1, acumulado * 10 + (s[i] - '0'));
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

long Grafo::numeroDeNodo(int indice) const {
    return aNumero(nodos_[indice].nombre, 0, 0);
}

// ------------------------------------------------------------
//  Nodos
// ------------------------------------------------------------
std::string Grafo::validarNombre(const std::string& nombreCrudo) const {
    const std::string nombre = recortar(nombreCrudo);
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

std::string Grafo::nombreLibreDesde(int k) const {
    const std::string candidato = nombreAutomatico(k);
    if (indiceDe(candidato) == -1) return candidato;
    return nombreLibreDesde(k + 1);
}

std::string Grafo::siguienteNombreLibre() const { return nombreLibreDesde(0); }

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
    const std::string a = nodos_[desde].nombre;
    const std::string b = nodos_[hasta].nombre;
    if (buscarArco(desde, hasta, 0) != -1)
        return "Ya existe una arista de " + a + " a " + b + ".";
    if (!dirigida && buscarArco(hasta, desde, 0) != -1)
        return "Ya existe una arista de " + b + " a " + a + ". Borrala o creá esta como dirigida.";
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
    //          nombre   x     y
    agregarNodo("1", 90, 240);
    agregarNodo("2", 260, 100);
    agregarNodo("3", 260, 380);
    agregarNodo("4", 450, 100);
    agregarNodo("5", 450, 380);
    agregarNodo("6", 630, 170);
    agregarNodo("7", 790, 300);

    //            desde hasta ida vuelta dirigida
    agregarArista(0, 1, 2, 2, false); // 1 — 2
    agregarArista(0, 2, 4, 3, false); // 1 — 3 (ida y vuelta distintas)
    agregarArista(1, 2, 1, 1, false); // 2 — 3
    agregarArista(1, 3, 3, 3, true);  // 2 → 4
    agregarArista(2, 4, 6, 5, false); // 3 — 5
    agregarArista(3, 4, 2, 2, false); // 4 — 5
    agregarArista(3, 5, 2, 3, false); // 4 — 6
    agregarArista(4, 5, 4, 4, false); // 5 — 6
    agregarArista(5, 6, 1, 1, false); // 6 — 7
    agregarArista(4, 6, 7, 7, true);  // 5 → 7

    origenSugerido = 0;  // 1
    destinoSugerido = 6; // 7
}

} // namespace grafos
