#pragma once

// ============================================================
//  Grafo.h
//  Modelo del grafo: nodos (nombre numérico y posición) y
//  aristas (dirigidas con un peso, o no dirigidas con peso de
//  ida y de vuelta). C++ puro, no depende de Qt.
//  El nombre de un nodo es un número entero desde 1 (1, 7,
//  23443...). Ese número decide si el nodo es par o impar.
// ============================================================

#include <string>
#include <vector>

namespace grafos {

struct Nodo {
    std::string nombre;       // número desde 1, sin ceros a la izquierda
    bool bloqueado = false;   // el camino no puede pasar por acá
    bool obligatorio = false; // el camino tiene que pasar por acá
                              // (bloqueado y obligatorio nunca están los dos)
    double x = 0.0;           // posición en el dibujo
    double y = 0.0;
};

struct Arista {
    int desde = -1;
    int hasta = -1;
    int pesoIda = 1;          // desde -> hasta (entero)
    int pesoVuelta = 1;       // hasta -> desde (solo si NO es dirigida)
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

    // Busca un nodo por nombre. -1 si no está.
    int indiceDe(const std::string& nombre) const;

    // Número del nodo (su nombre como entero). Lo usan los filtros par/impar.
    long numeroDeNodo(int indice) const;

    // ---- nodos ----
    // Devuelve "" si el nombre es válido o el mensaje de error.
    std::string validarNombre(const std::string& nombre) const;
    // Menor número libre desde 1.
    std::string siguienteNombreLibre() const;
    int agregarNodo(const std::string& nombre, double x, double y);

    // Bloqueado y obligatorio se excluyen: activar uno apaga el otro.
    void setBloqueado(int indice, bool valor);
    void setObligatorio(int indice, bool valor);
    void eliminarNodo(int indice);

    // ---- aristas ----
    std::string validarArista(int desde, int hasta, bool dirigida) const;
    int agregarArista(int desde, int hasta, int pesoIda, int pesoVuelta, bool dirigida);
    void eliminarArista(int indice);
    // Texto para mostrar: "A — B" o "A → B"
    std::string describirArista(int indice) const;

    // ---- general ----
    void limpiar();
    // Grafo precargado de ejemplo. Devuelve origen y destino sugeridos.
    void cargarEjemplo(int& origenSugerido, int& destinoSugerido);

    static const int LARGO_MAXIMO_NOMBRE = 6; // hasta 999999

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

// Nombre automático número k: 0->"1", 1->"2", ...
std::string nombreAutomatico(int k);

} // namespace grafos
