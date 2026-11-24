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

// Posición del primer arco de costo negativo, o -1.
int primerCostoNegativo(const std::vector<ArcoPlano>& plano, size_t k) {
    if (k >= plano.size()) return -1;
    if (plano[k].costo < 0) return static_cast<int>(k);
    return primerCostoNegativo(plano, k + 1);
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

// Marca los nodos con distancia "-infinito": los que se alcanzan desde un ciclo negativo.
// Un ciclo negativo solo impide el camino mínimo si alguno de los nodos que se buscan queda marcado.
bool propagarIlimitados(const std::vector<ArcoPlano>& arcos, size_t k, const Arbol& t,
                        std::vector<bool>& ilimitado, bool huboCambio) {
    if (k >= arcos.size()) return huboCambio;
    const ArcoPlano& a = arcos[k];
    if (t.dist[a.desde] < INFINITO && !ilimitado[a.hacia] &&
        (ilimitado[a.desde] || t.dist[a.desde] + a.costo < t.dist[a.hacia])) {
        ilimitado[a.hacia] = true;
        huboCambio = true;
    }
    return propagarIlimitados(arcos, k + 1, t, ilimitado, huboCambio);
}

void repetirPropagacion(const std::vector<ArcoPlano>& arcos, int iteracion, int n, const Arbol& t,
                        std::vector<bool>& ilimitado) {
    if (iteracion >= n) return;
    if (!propagarIlimitados(arcos, 0, t, ilimitado, false)) return;
    repetirPropagacion(arcos, iteracion + 1, n, t, ilimitado);
}

Arbol bellmanFord(const std::vector<ArcoPlano>& arcos, int n, int fuente, std::vector<bool>& ilimitado) {
    Arbol t = arbolVacio(n, fuente);
    iterarBellman(arcos, 0, n, t);
    ilimitado = std::vector<bool>(n, false);
    repetirPropagacion(arcos, 0, n, t, ilimitado);
    return t;
}

// ============================================================
//  MATRIZ DE ADYACENCIA (se itera sobre la propia matriz)
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

void matrizJ(int k, int i, int j, int n, Matriz& D, MatrizInt& P) {
    if (j >= n) return;
    if (D[k][j] < INFINITO && D[i][k] + D[k][j] < D[i][j]) {
        D[i][j] = D[i][k] + D[k][j];
        P[i][j] = P[k][j];
    }
    matrizJ(k, i, j + 1, n, D, P);
}

void matrizI(int k, int i, int n, Matriz& D, MatrizInt& P) {
    if (i >= n) return;
    if (D[i][k] < INFINITO) matrizJ(k, i, 0, n, D, P);
    matrizI(k, i + 1, n, D, P);
}

void matrizK(int k, int n, Matriz& D, MatrizInt& P) {
    if (k >= n) return;
    matrizI(k, 0, n, D, P);
    matrizK(k + 1, n, D, P);
}

// ¿Hay un ciclo negativo por el que se pueda pasar yendo de s a t?
bool cicloAfecta(const Matriz& D, int s, int t, int k) {
    if (k >= static_cast<int>(D.size())) return false;
    if (D[k][k] < 0 && D[s][k] < INFINITO && D[k][t] < INFINITO) return true;
    return cicloAfecta(D, s, t, k + 1);
}

Arbol arbolDesdeMatriz(const Matriz& D, const MatrizInt& P, int fuente) {
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

bool pesoBloqueado(const Filtros& f, int peso) {
    if (f.evitarAristasPares && peso % 2 == 0) return true;
    if (f.evitarAristasImpares && peso % 2 != 0) return true;
    return false;
}

// Un nodo queda afuera solo si está bloqueado.
bool nodoExcluido(const Grafo& g, int indice, const Filtros&) {
    return g.nodo(indice).bloqueado;
}

std::string motivoExclusion(const Grafo& g, int indice, const Filtros&, const std::string& quien) {
    const Nodo& n = g.nodo(indice);
    if (!n.bloqueado) return "";
    return quien + (quien.empty() ? "" : " ") + comillas(n.nombre) + " está bloqueado.";
}

Resultado resolver(const Grafo& g, int origen, int destino, Algoritmo alg, const Filtros& f) {
    Resultado r;
    r.algoritmo = alg;
    r.origen = origen;
    r.destino = destino;
    const int n = g.cantidadNodos();

    // ---------- validaciones ----------
    if (n == 0) return conError(r, "El grafo está vacío. Agregá nodos o cargá el ejemplo.");
    if (f.evitarAristasPares && f.evitarAristasImpares)
        return conError(r, "Evitar aristas pares e impares a la vez corta todas las aristas. Dejá tildada solo una.");
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
    if (origen == destino && obligatorios.empty())
        return conError(r, "El origen y el destino son el mismo nodo (el costo es 0). Elegí dos nodos distintos.");
    if (g.nodo(origen).obligatorio || g.nodo(destino).obligatorio)
        r.aviso = "El origen y el destino ya forman parte del camino: marcarlos como obligatorios no cambia nada.";
    if (static_cast<int>(obligatorios.size()) > MAXIMO_OBLIGATORIOS)
        return conError(r, "Hay más de " + std::to_string(MAXIMO_OBLIGATORIOS) +
                               " nodos obligatorios; probar todos los órdenes sería demasiado lento.");

    // ---------- arcos ----------
    ListaAdyacencia ady(n);
    std::vector<ArcoPlano> plano;
    armarArcos(g, r.excluido, f, 0, ady, plano, r.sentidosPorPeso);
    const int negativo = primerCostoNegativo(plano, 0);
    const bool negativos = negativo >= 0;

    if (alg == Algoritmo::Dijkstra && negativos)
        return conError(r, "Dijkstra no admite pesos negativos: el camino " + comillas(g.nodo(plano[negativo].desde).nombre) +
                               " → " + comillas(g.nodo(plano[negativo].hacia).nombre) + " pesa " +
                               std::to_string(static_cast<long long>(plano[negativo].costo)) +
                               ". Probá con Bellman-Ford o con la matriz de adyacencia.");

    // ---------- matriz de adyacencia: solo si se muestra o si es el algoritmo elegido ----------
    const bool esMatriz = (alg == Algoritmo::MatrizAdyacencia);
    if (esMatriz && n > MAXIMO_NODOS_ALGORITMO_MATRIZ)
        return conError(r, "La matriz de adyacencia como algoritmo tiene un costo cúbico: con " + std::to_string(n) +
                               " nodos tardaría demasiado. Usá Dijkstra o Bellman-Ford (hasta " +
                               std::to_string(MAXIMO_NODOS_ALGORITMO_MATRIZ) + " nodos con esta opción).");
    Matriz D;
    MatrizInt P;
    if (esMatriz || n <= MAXIMO_NODOS_EN_MATRIZ) {
        D = Matriz(n, std::vector<double>(n, INFINITO));
        P = MatrizInt(n, std::vector<int>(n, -1));
        recorrer(0, n, [&](int i) { D[i][i] = 0.0; });
        cargarArcoEnMatriz(plano, 0, D, P);
        if (n <= MAXIMO_NODOS_EN_MATRIZ) r.matrizAdyacencia = D;
        if (esMatriz) matrizK(0, n, D, P); // se itera la misma matriz una sola vez
    }

    // ---------- un árbol por cada fuente (origen y obligatorios) ----------
    std::vector<int> fuentes;
    fuentes.push_back(origen);
    recorrer(0, static_cast<int>(obligatorios.size()), [&](int i) { fuentes.push_back(obligatorios[i]); });

    std::vector<Arbol> arboles;
    std::vector<int> arbolDe(n, -1);
    // Nodos que se buscan desde cada fuente: el destino y los obligatorios
    std::vector<int> metas = obligatorios;
    metas.push_back(destino);
    std::string errorCiclo;
    recorrer(0, static_cast<int>(fuentes.size()), [&](int i) {
        if (!errorCiclo.empty()) return;
        const int s = fuentes[i];
        arbolDe[s] = static_cast<int>(arboles.size());
        const std::string textoCiclo = "Hay un ciclo de peso negativo que afecta al camino desde " +
                                       comillas(g.nodo(s).nombre) + ": no existe camino mínimo." +
                                       (g.dirigido() ? std::string()
                                                     : std::string(" En un grafo no dirigido, una arista con ida y vuelta "
                                                                   "negativas forma un ciclo negativo (A → B → A)."));
        if (alg == Algoritmo::Dijkstra) {
            arboles.push_back(dijkstra(ady, s));
        } else if (alg == Algoritmo::BellmanFord) {
            std::vector<bool> ilimitado;
            arboles.push_back(bellmanFord(plano, n, s, ilimitado));
            // Un ciclo negativo que no lleva a ninguno de los nodos buscados no impide el camino mínimo
            recorrer(0, static_cast<int>(metas.size()), [&](int j) {
                if (ilimitado[metas[j]]) errorCiclo = textoCiclo;
            });
        } else {
            arboles.push_back(arbolDesdeMatriz(D, P, s));
            recorrer(0, static_cast<int>(metas.size()), [&](int j) {
                if (cicloAfecta(D, s, metas[j], 0)) errorCiclo = textoCiclo;
            });
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
