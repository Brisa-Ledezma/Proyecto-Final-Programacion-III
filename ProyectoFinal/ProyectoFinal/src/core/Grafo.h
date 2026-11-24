#pragma once

// ============================================================
//  Grafo.h
//  Modelo del grafo: nodos (nombre numérico y posición) y
//  aristas (dirigidas con un peso, o no dirigidas con peso de
//  ida y de vuelta). C++ puro, no depende de Qt.
//  El nombre de un nodo es un número entero desde 1 (1, 7,
//  23443...).
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
    bool dirigida = false;    // siempre igual al tipo del grafo
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

    // ---- tipo del grafo (vale para TODAS las aristas) ----
    bool dirigido() const { return dirigido_; }
    // Pasar a dirigido: cada arista A — B queda como A → B con el peso de ida; el peso de vuelta se
    // descarta (la interfaz guarda el estado anterior para "Deshacer").
    // Pasar a no dirigido: A → B y B → A cargadas a mano se juntan en A — B (ida y vuelta); una sola
    // queda con el mismo peso en los dos sentidos.
    void setDirigido(bool valor);
    // Cuántas aristas pierden información al pasar al otro tipo: al pasar a dirigido, las que tienen
    // peso de vuelta distinto del de ida; al pasar a no dirigido, los pares opuestos con distinto bloqueo.
    int aristasQuePierdenDatos(bool haciaDirigido) const;

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
    // Límites para que la aplicación siga fluida (cada cambio rearma tablas, dibujo y resultado)
    static const int MAXIMO_NODOS = 150;
    static const int MAXIMO_ARISTAS = 1000;

private:
    std::vector<Nodo> nodos_;
    std::vector<Arista> aristas_;
    bool dirigido_ = false;

    bool cubreArco(const Arista& e, int u, int v) const;
    int buscarArco(int u, int v, int k) const;
    int buscarNombre(const std::string& nombre, int i) const;
    void marcarUsados(int i, std::vector<bool>& usado) const;
    int primerLibre(const std::vector<bool>& usado, int k) const;
    void quitarAristasDe(int nodo, int k);
    void reindexarAristas(int nodoBorrado, int k);
    void separarEnDirigidas(int k, int total);
    void juntarOpuestas(int k);
    int buscarOpuesta(int k, int j) const;
    int contarPerdidasADirigido(int k) const;
    int contarPerdidasANoDirigido(int k) const;
};

// Nombre automático número k: 0->"1", 1->"2", ...
std::string nombreAutomatico(int k);

} // namespace grafos
