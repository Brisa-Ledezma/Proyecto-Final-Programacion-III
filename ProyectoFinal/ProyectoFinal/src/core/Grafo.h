#pragma once

// ============================================================
//  Grafo.h
//  Modelo del grafo: nodos (con nombre, peso y posición) y
//  aristas (dirigidas con un peso, o no dirigidas con peso de
//  ida y de vuelta). C++ puro, no depende de Qt.
// ============================================================

#include <string>
#include <vector>

namespace grafos {

struct Nodo {
    std::string nombre;
    double peso = 0.0;        // se suma cada vez que el camino ENTRA al nodo
    bool bloqueado = false;   // el camino no puede pasar por acá
    bool obligatorio = false; // el camino tiene que pasar por acá
    double x = 0.0;           // posición en el dibujo
    double y = 0.0;
};

struct Arista {
    int desde = -1;
    int hasta = -1;
    double pesoIda = 1.0;     // desde -> hasta
    double pesoVuelta = 1.0;  // hasta -> desde (solo si NO es dirigida)
    bool dirigida = false;
    bool bloqueada = false;
};

class Grafo {
public:
    // ---- consulta ----
    int cantidadNodos() const;
    int cantidadAristas() const;
    const Nodo& nodo(int i) const;
    Nodo& nodo(int i);
    const Arista& arista(int i) const;
    Arista& arista(int i);

    // Busca un nodo por nombre (sin distinguir mayúsculas). -1 si no está.
    int indiceDe(const std::string& nombre) const;

    // Número que se usa para los filtros par/impar (#1, #2, ...).
    static int numeroDeNodo(int indice) { return indice + 1; }

    // ---- nodos ----
    // Devuelve "" si el nombre es válido o el mensaje de error.
    std::string validarNombre(const std::string& nombre) const;
    // Primer nombre libre de la serie A, B, ..., Z, AA, AB, ...
    std::string siguienteNombreLibre() const;
    int agregarNodo(const std::string& nombre, double peso, double x, double y);
    void eliminarNodo(int indice);

    // ---- aristas ----
    std::string validarArista(int desde, int hasta, bool dirigida) const;
    int agregarArista(int desde, int hasta, double pesoIda, double pesoVuelta, bool dirigida);
    void eliminarArista(int indice);
    // Texto para mostrar: "A — B" o "A → B"
    std::string describirArista(int indice) const;

    // ---- general ----
    void limpiar();
    // Grafo precargado de ejemplo. Devuelve origen y destino sugeridos.
    void cargarEjemplo(int& origenSugerido, int& destinoSugerido);

    static const int LARGO_MAXIMO_NOMBRE = 8;

private:
    std::vector<Nodo> nodos_;
    std::vector<Arista> aristas_;

    bool cubreArco(const Arista& e, int u, int v) const;
    int buscarArco(int u, int v, int k) const;
    int buscarNombre(const std::string& nombre, int i) const;
    std::string nombreLibreDesde(int k) const;
    void quitarAristasDe(int nodo, int k);
    void reindexarAristas(int nodoBorrado, int k);
};

// Nombre automático número k: 0->A, 25->Z, 26->AA, 27->AB ...
std::string nombreAutomatico(int k);

} // namespace grafos
