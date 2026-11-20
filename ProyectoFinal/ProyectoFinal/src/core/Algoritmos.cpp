#include "Algoritmos.h"
#include "Recorrer.h"

namespace grafos {

// ============================================================
//  Tipos internos
// ============================================================
namespace {

using ListaAdyacencia = std::vector<std::vector<Arco>>;
using Matriz = std::vector<std::vector<double>>;
using MatrizInt = std::vector<std::vector<int>>;

struct ArcoPlano {   // para Bellman-Ford: lista plana de arcos
    int desde;
    int hacia;
    double costo;
};

// Árbol de caminos mínimos desde una fuente.
struct Arbol {
    std::vector<double> dist; // costo mínimo desde la fuente
    std::vector<int> prev;    // nodo anterior en el camino (-1 = ninguno)
};

Arbol arbolVacio(int n, int fuente) {
    Arbol t;
    t.dist = std::vector<double>(n, INFINITO);
    t.prev = std::vector<int>(n, -1);
    t.dist[fuente] = 0.0;
    return t;
}

std::string comillas(const std::string& s) { return "\"" + s + "\""; }

// ============================================================
//  Construcción de los arcos (aplicando bloqueos y filtros)
//  Costo de ir u -> v = peso de la arista en ese sentido.
//  Cada sentido se descarta por separado si su peso está
//  bloqueado.
// ============================================================
void agregarArco(ListaAdyacencia& ady, std::vector<ArcoPlano>& plano,
                 int u, int v, double costo, int arista) {
    Arco a;
    a.hacia = v;
    a.costo = costo;
    a.arista = arista;
    ady[u].push_back(a);
    plano.push_back({u, v, costo});
}

void armarArcos(const Grafo& g, const std::vector<bool>& excl, const Filtros& f, int k,
                ListaAdyacencia& ady, std::vector<ArcoPlano>& plano, int& cortadosPorPeso) {
    if (k >= g.cantidadAristas()) return;
    const Arista& e = g.arista(k);
    if (!e.bloqueada && !excl[e.desde] && !excl[e.hasta]) {
        if (!pesoBloqueado(f, e.pesoIda))
            agregarArco(ady, plano, e.desde, e.hasta, static_cast<double>(e.pesoIda), k);
        else
            cortadosPorPeso++;
        if (!e.dirigida) {
            if (!pesoBloqueado(f, e.pesoVuelta))
                agregarArco(ady, plano, e.hasta, e.desde, static_cast<double>(e.pesoVuelta), k);
            else
                cortadosPorPeso++;
        }
    }
    armarArcos(g, excl, f, k + 1, ady, plano, cortadosPorPeso);
}

bool pesoEnLista(const std::vector<int>& lista, int peso, size_t k) {
    if (k >= lista.size()) return false;
    if (lista[k] == peso) return true;
    return pesoEnLista(lista, peso, k + 1);
}

bool hayCostoNegativo(const std::vector<ArcoPlano>& plano, size_t k) {
    if (k >= plano.size()) return false;
    if (plano[k].costo < 0) return true;
    return hayCostoNegativo(plano, k + 1);
}

// ============================================================
//  DIJKSTRA
//  1) elegir el nodo no visitado con menor distancia
//  2) marcarlo como visitado
//  3) relajar sus vecinos
//  4) repetir (recursión) hasta que no queden alcanzables
// ============================================================
int menorNoVisitado(const std::vector<double>& dist, const std::vector<bool>& visitado,
                    int i, int mejor) {
    if (i >= static_cast<int>(dist.size())) return mejor;
    if (!visitado[i] && dist[i] < INFINITO && (mejor == -1 || dist[i] < dist[mejor]))
        mejor = i;
    return menorNoVisitado(dist, visitado, i + 1, mejor);
}

void relajarVecinos(const std::vector<Arco>& arcos, size_t k, int u, Arbol& t) {
    if (k >= arcos.size()) return;
    const Arco& a = arcos[k];
    if (t.dist[u] + a.costo < t.dist[a.hacia]) {
        t.dist[a.hacia] = t.dist[u] + a.costo;
        t.prev[a.hacia] = u;
    }
    relajarVecinos(arcos, k + 1, u, t);
}

void pasoDijkstra(const ListaAdyacencia& ady, std::vector<bool>& visitado, Arbol& t) {
    const int u = menorNoVisitado(t.dist, visitado, 0, -1);
    if (u == -1) return; // no quedan nodos alcanzables sin visitar
    visitado[u] = true;
    relajarVecinos(ady[u], 0, u, t);
    pasoDijkstra(ady, visitado, t);
}

Arbol dijkstra(const ListaAdyacencia& ady, int fuente) {
    const int n = static_cast<int>(ady.size());
    Arbol t = arbolVacio(n, fuente);
    std::vector<bool> visitado(n, false);
    pasoDijkstra(ady, visitado, t);
    return t;
}

// ============================================================
//  BELLMAN-FORD
//  Relaja TODOS los arcos n-1 veces (corta antes si en una
//  pasada no cambió nada). Si después todavía se puede mejorar
//  algo, hay un ciclo de peso negativo.
// ============================================================
bool pasadaBellman(const std::vector<ArcoPlano>& arcos, size_t k, Arbol& t, bool huboCambio) {
    if (k >= arcos.size()) return huboCambio;
    const ArcoPlano& a = arcos[k];
    if (t.dist[a.desde] < INFINITO && t.dist[a.desde] + a.costo < t.dist[a.hacia]) {
        t.dist[a.hacia] = t.dist[a.desde] + a.costo;
        t.prev[a.hacia] = a.desde;
        huboCambio = true;
    }
    return pasadaBellman(arcos, k + 1, t, huboCambio);
}

void iterarBellman(const std::vector<ArcoPlano>& arcos, int iteracion, int n, Arbol& t) {
    if (iteracion >= n - 1) return;
    if (!pasadaBellman(arcos, 0, t, false)) return; // ya no hay cambios
    iterarBellman(arcos, iteracion + 1, n, t);
}

bool quedaMejora(const std::vector<ArcoPlano>& arcos, size_t k, const Arbol& t) {
    if (k >= arcos.size()) return false;
    const ArcoPlano& a = arcos[k];
    if (t.dist[a.desde] < INFINITO && t.dist[a.desde] + a.costo < t.dist[a.hacia]) return true;
    return quedaMejora(arcos, k + 1, t);
}

Arbol bellmanFord(const std::vector<ArcoPlano>& arcos, int n, int fuente, bool& cicloNegativo) {
    Arbol t = arbolVacio(n, fuente);
    iterarBellman(arcos, 0, n, t);
    cicloNegativo = quedaMejora(arcos, 0, t);
    return t;
}

// ============================================================
//  FLOYD-WARSHALL sobre la MATRIZ DE ADYACENCIA
//  A[i][j] = costo del arco directo i -> j (∞ si no hay)
//  Para cada k, i, j: si pasar por k mejora i -> j, se actualiza.
//  P[i][j] = nodo anterior a j en el mejor camino desde i.
// ============================================================
void cargarArcoEnMatriz(const std::vector<ArcoPlano>& arcos, size_t k, Matriz& A, MatrizInt& P) {
    if (k >= arcos.size()) return;
    const ArcoPlano& a = arcos[k];
    if (a.costo < A[a.desde][a.hacia]) {
        A[a.desde][a.hacia] = a.costo;
        P[a.desde][a.hacia] = a.desde;
    }
    cargarArcoEnMatriz(arcos, k + 1, A, P);
}

void floydJ(int k, int i, int j, int n, Matriz& D, MatrizInt& P) {
    if (j >= n) return;
    if (D[k][j] < INFINITO && D[i][k] + D[k][j] < D[i][j]) {
        D[i][j] = D[i][k] + D[k][j];
        P[i][j] = P[k][j];
    }
    floydJ(k, i, j + 1, n, D, P);
}

void floydI(int k, int i, int n, Matriz& D, MatrizInt& P) {
    if (i >= n) return;
    if (D[i][k] < INFINITO) floydJ(k, i, 0, n, D, P);
    floydI(k, i + 1, n, D, P);
}

void floydK(int k, int n, Matriz& D, MatrizInt& P) {
    if (k >= n) return;
    floydI(k, 0, n, D, P);
    floydK(k + 1, n, D, P);
}

bool diagonalNegativa(const Matriz& D, int i) {
    if (i >= static_cast<int>(D.size())) return false;
    if (D[i][i] < 0) return true;
    return diagonalNegativa(D, i + 1);
}

Arbol arbolDesdeFloyd(const Matriz& D, const MatrizInt& P, int fuente) {
    Arbol t;
    t.dist = D[fuente];
    t.prev = P[fuente];
    t.prev[fuente] = -1;
    return t;
}

// ============================================================
//  Reconstrucción del camino a partir de "prev"
// ============================================================
bool armarCamino(const Arbol& t, int fuente, int v, int pasosRestantes, std::vector<int>& salida) {
    if (v == fuente) {
        salida.push_back(fuente);
        return true;
    }
    if (v < 0 || pasosRestantes <= 0) return false;
    if (!armarCamino(t, fuente, t.prev[v], pasosRestantes - 1, salida)) return false;
    salida.push_back(v);
    return true;
}

// La arista más barata que va de u a v (para saber cuál pintar).
int mejorArista(const std::vector<Arco>& arcos, int v, size_t k, int mejor, double costoMejor) {
    if (k >= arcos.size()) return mejor;
    if (arcos[k].hacia == v && (mejor == -1 || arcos[k].costo < costoMejor))
        return mejorArista(arcos, v, k + 1, arcos[k].arista, arcos[k].costo);
    return mejorArista(arcos, v, k + 1, mejor, costoMejor);
}

// ============================================================
//  Nodos obligatorios: se prueban todos los órdenes posibles
//  (permutaciones por backtracking) y se queda el más barato.
// ============================================================
struct BusquedaOrden {
    const std::vector<int>* obligatorios = nullptr;
    const std::vector<Arbol>* arboles = nullptr;
    const std::vector<int>* arbolDe = nullptr; // nodo -> índice de árbol
    int destino = -1;
    bool podar = true; // solo es válido podar si no hay costos negativos
    double mejor = INFINITO;
    std::vector<int> mejorOrden;
    std::vector<int> actual;
    std::vector<bool> usado;
};

double distanciaEntre(const BusquedaOrden& b, int desde, int hasta) {
    return (*b.arboles)[(*b.arbolDe)[desde]].dist[hasta];
}

void explorarOrden(BusquedaOrden& b, int ultimo, double acumulado);

void probarCandidato(BusquedaOrden& b, int ultimo, double acumulado, size_t i) {
    if (i >= b.obligatorios->size()) return;
    if (!b.usado[i]) {
        const int candidato = (*b.obligatorios)[i];
        const double c = distanciaEntre(b, ultimo, candidato);
        if (c < INFINITO && (!b.podar || acumulado + c < b.mejor)) {
            b.usado[i] = true;
            b.actual.push_back(candidato);
            explorarOrden(b, candidato, acumulado + c);
            b.actual.pop_back();
            b.usado[i] = false;
        }
    }
    probarCandidato(b, ultimo, acumulado, i + 1);
}

void explorarOrden(BusquedaOrden& b, int ultimo, double acumulado) {
    if (b.actual.size() == b.obligatorios->size()) {
        const double total = acumulado + distanciaEntre(b, ultimo, b.destino);
        if (total < b.mejor) {
            b.mejor = total;
            b.mejorOrden = b.actual;
        }
        return;
    }
    probarCandidato(b, ultimo, acumulado, 0);
}

Resultado conError(Resultado r, const std::string& mensaje) {
    r.ok = false;
    r.error = mensaje;
    return r;
}

} // namespace

// ============================================================
//  API pública
// ============================================================
std::string nombreAlgoritmo(Algoritmo alg) {
    if (alg == Algoritmo::Dijkstra) return "Dijkstra";
    if (alg == Algoritmo::BellmanFord) return "Bellman-Ford";
    return "Matriz de adyacencia (Floyd-Warshall)";
}

bool pesoBloqueado(const Filtros& f, int peso) { return pesoEnLista(f.pesosBloqueados, peso, 0); }

bool nodoExcluido(const Grafo& g, int indice, const Filtros& f) {
    return !motivoExclusion(g, indice, f, "").empty();
}

std::string motivoExclusion(const Grafo& g, int indice, const Filtros& f, const std::string& quien) {
    const Nodo& n = g.nodo(indice);
    const long numero = g.numeroDeNodo(indice);
    const std::string nombre = quien + (quien.empty() ? "" : " ") + comillas(n.nombre);
    if (n.bloqueado) return nombre + " está bloqueado.";
    if (f.evitarPares && numero % 2 == 0)
        return nombre + " es par y está activo el filtro \"Evitar nodos pares\".";
    if (f.evitarImpares && numero % 2 != 0)
        return nombre + " es impar y está activo el filtro \"Evitar nodos impares\".";
    return "";
}

Resultado resolver(const Grafo& g, int origen, int destino, Algoritmo alg, const Filtros& f) {
    Resultado r;
    r.algoritmo = alg;
    r.origen = origen;
    r.destino = destino;
    const int n = g.cantidadNodos();

    // ---------- validaciones ----------
    if (n == 0) return conError(r, "El grafo está vacío. Agregá nodos o cargá el ejemplo.");
    if (origen < 0 || destino < 0 || origen >= n || destino >= n)
        return conError(r, "Elegí un origen y un destino.");

    r.excluido = std::vector<bool>(n, false);
    recorrer(0, n, [&](int i) { r.excluido[i] = nodoExcluido(g, i, f); });

    const std::string motivoOrigen = motivoExclusion(g, origen, f, "El origen");
    if (!motivoOrigen.empty()) return conError(r, motivoOrigen);
    const std::string motivoDestino = motivoExclusion(g, destino, f, "El destino");
    if (!motivoDestino.empty()) return conError(r, motivoDestino);

    std::vector<int> obligatorios;
    std::string motivoObligatorio;
    recorrer(0, n, [&](int i) {
        if (!g.nodo(i).obligatorio || i == origen || i == destino) return;
        if (motivoObligatorio.empty())
            motivoObligatorio = motivoExclusion(g, i, f, "El nodo obligatorio");
        obligatorios.push_back(i);
    });
    if (!motivoObligatorio.empty()) return conError(r, motivoObligatorio);
    if (static_cast<int>(obligatorios.size()) > MAXIMO_OBLIGATORIOS)
        return conError(r, "Hay más de " + std::to_string(MAXIMO_OBLIGATORIOS) +
                               " nodos obligatorios; probar todos los órdenes sería demasiado lento.");

    // ---------- arcos ----------
    ListaAdyacencia ady(n);
    std::vector<ArcoPlano> plano;
    armarArcos(g, r.excluido, f, 0, ady, plano, r.sentidosPorPeso);
    const bool negativos = hayCostoNegativo(plano, 0);

    if (alg == Algoritmo::Dijkstra && negativos)
        return conError(r, "Dijkstra no admite pesos negativos (hay una arista con peso negativo). "
                           "Probá con Bellman-Ford o con la matriz de adyacencia.");

    // ---------- Floyd: matriz completa una sola vez ----------
    Matriz D;
    MatrizInt P;
    if (alg == Algoritmo::Floyd) {
        D = Matriz(n, std::vector<double>(n, INFINITO));
        P = MatrizInt(n, std::vector<int>(n, -1));
        recorrer(0, n, [&](int i) { D[i][i] = 0.0; });
        cargarArcoEnMatriz(plano, 0, D, P);
        r.matrizAdyacencia = D; // la matriz de adyacencia antes de iterar
        floydK(0, n, D, P);
        if (diagonalNegativa(D, 0))
            return conError(r, "Hay un ciclo de peso negativo: no existe camino mínimo.");
        r.matrizDistancias = D;
    }

    // ---------- un árbol por cada fuente (origen y obligatorios) ----------
    std::vector<int> fuentes;
    fuentes.push_back(origen);
    recorrer(0, static_cast<int>(obligatorios.size()), [&](int i) { fuentes.push_back(obligatorios[i]); });

    std::vector<Arbol> arboles;
    std::vector<int> arbolDe(n, -1);
    std::string errorCiclo;
    recorrer(0, static_cast<int>(fuentes.size()), [&](int i) {
        if (!errorCiclo.empty()) return;
        const int s = fuentes[i];
        arbolDe[s] = static_cast<int>(arboles.size());
        if (alg == Algoritmo::Dijkstra) {
            arboles.push_back(dijkstra(ady, s));
        } else if (alg == Algoritmo::BellmanFord) {
            bool ciclo = false;
            arboles.push_back(bellmanFord(plano, n, s, ciclo));
            if (ciclo)
                errorCiclo = "Hay un ciclo de peso negativo alcanzable desde " + comillas(g.nodo(s).nombre) +
                             ": no existe camino mínimo.";
        } else {
            arboles.push_back(arbolDesdeFloyd(D, P, s));
        }
    });
    if (!errorCiclo.empty()) return conError(r, errorCiclo);

    // ---------- mejor orden de obligatorios ----------
    BusquedaOrden b;
    b.obligatorios = &obligatorios;
    b.arboles = &arboles;
    b.arbolDe = &arbolDe;
    b.destino = destino;
    b.podar = !negativos;
    b.usado = std::vector<bool>(obligatorios.size(), false);
    explorarOrden(b, origen, 0.0);

    if (!(b.mejor < INFINITO)) {
        if (obligatorios.empty())
            return conError(r, "No hay camino de " + comillas(g.nodo(origen).nombre) + " a " +
                                   comillas(g.nodo(destino).nombre) + " con los bloqueos y filtros actuales.");
        return conError(r, "No hay un camino que pase por todos los nodos obligatorios.");
    }

    // ---------- armar el camino completo ----------
    std::vector<int> secuencia;
    secuencia.push_back(origen);
    recorrer(0, static_cast<int>(b.mejorOrden.size()), [&](int i) { secuencia.push_back(b.mejorOrden[i]); });
    secuencia.push_back(destino);

    bool caminoOk = true;
    r.camino.push_back(origen);
    recorrer(0, static_cast<int>(secuencia.size()) - 1, [&](int i) {
        if (!caminoOk) return;
        std::vector<int> tramo;
        const Arbol& t = arboles[arbolDe[secuencia[i]]];
        caminoOk = armarCamino(t, secuencia[i], secuencia[i + 1], n + 1, tramo);
        recorrer(1, static_cast<int>(tramo.size()), [&](int j) { r.camino.push_back(tramo[j]); });
    });
    if (!caminoOk) return conError(r, "No se pudo reconstruir el camino.");

    recorrer(0, static_cast<int>(r.camino.size()) - 1, [&](int i) {
        r.aristasCamino.push_back(mejorArista(ady[r.camino[i]], r.camino[i + 1], 0, -1, 0.0));
    });

    r.costo = b.mejor;
    r.ordenObligatorios = b.mejorOrden;
    r.distDesdeOrigen = arboles[0].dist;
    r.ok = true;
    return r;
}

} // namespace grafos
