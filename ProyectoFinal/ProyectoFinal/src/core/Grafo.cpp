#include "Grafo.h"

#include <cctype>

namespace grafos {

// ------------------------------------------------------------
//  Utilidades de texto (todas recursivas)
// ------------------------------------------------------------
namespace {

char aMinuscula(char c) {
    if (c >= 'A' && c <= 'Z') return static_cast<char>(c - 'A' + 'a');
    return c;
}

// Compara sin distinguir mayúsculas, carácter por carácter.
bool igualesSinMayusculas(const std::string& a, const std::string& b, size_t i) {
    if (a.size() != b.size()) return false;
    if (i == a.size()) return true;
    if (aMinuscula(a[i]) != aMinuscula(b[i])) return false;
    return igualesSinMayusculas(a, b, i + 1);
}

bool tieneDigito(const std::string& s, size_t i) {
    if (i == s.size()) return false;
    if (s[i] >= '0' && s[i] <= '9') return true;
    return tieneDigito(s, i + 1);
}

// Letras ASCII o bytes >= 128 (letras con tilde, ñ, en UTF-8).
bool soloLetras(const std::string& s, size_t i) {
    if (i == s.size()) return true;
    const unsigned char c = static_cast<unsigned char>(s[i]);
    if (c < 128 && !std::isalpha(c)) return false;
    return soloLetras(s, i + 1);
}

// Cantidad de caracteres en UTF-8 (no cuenta bytes de continuación).
int contarCaracteres(const std::string& s, size_t i) {
    if (i == s.size()) return 0;
    const unsigned char c = static_cast<unsigned char>(s[i]);
    const int esInicio = ((c & 0xC0) != 0x80) ? 1 : 0;
    return esInicio + contarCaracteres(s, i + 1);
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

std::string nombreAutomatico(int k) {
    const char letra = static_cast<char>('A' + (k % 26));
    if (k < 26) return std::string(1, letra);
    return nombreAutomatico(k / 26 - 1) + letra;
}

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
    if (igualesSinMayusculas(nodos_[i].nombre, nombre, 0)) return i;
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
    if (nombre.empty()) return "El nombre del nodo no puede estar vacío.";
    if (tieneDigito(nombre, 0))
        return "El nombre no puede tener números (se confundiría con el peso o con el número del nodo).";
    if (!soloLetras(nombre, 0))
        return "El nombre solo puede tener letras.";
    if (contarCaracteres(nombre, 0) > LARGO_MAXIMO_NOMBRE)
        return "El nombre puede tener como máximo " + std::to_string(LARGO_MAXIMO_NOMBRE) + " letras.";
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

int Grafo::agregarNodo(const std::string& nombre, int peso, double x, double y) {
    Nodo n;
    n.nombre = recortar(nombre);
    n.peso = peso;
    n.x = x;
    n.y = y;
    nodos_.push_back(n);
    return cantidadNodos() - 1;
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
    //            nombre peso    x     y
    agregarNodo("A", 0, 90, 240);   // #1
    agregarNodo("B", 1, 260, 100);  // #2
    agregarNodo("C", 0, 260, 380);  // #3
    agregarNodo("D", 2, 450, 100);  // #4
    agregarNodo("E", 1, 450, 380);  // #5
    agregarNodo("F", 0, 630, 170);  // #6
    agregarNodo("G", 0, 790, 300);  // #7

    //            desde hasta ida vuelta dirigida
    agregarArista(0, 1, 2, 2, false); // A — B
    agregarArista(0, 2, 4, 3, false); // A — C (ida y vuelta distintas)
    agregarArista(1, 2, 1, 1, false); // B — C
    agregarArista(1, 3, 3, 3, true);  // B → D
    agregarArista(2, 4, 6, 5, false); // C — E
    agregarArista(3, 4, 2, 2, false); // D — E
    agregarArista(3, 5, 2, 3, false); // D — F
    agregarArista(4, 5, 4, 4, false); // E — F
    agregarArista(5, 6, 1, 1, false); // F — G
    agregarArista(4, 6, 7, 7, true);  // E → G

    origenSugerido = 0;  // A
    destinoSugerido = 6; // G
}

} // namespace grafos
